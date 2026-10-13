#include "board_config.h"
#include "app.h"
#include "motor_driver.h"
#include "serial_link.h"
#include "protocol.h"
#include "control_logic.h"
#include "encoder.h"
#include "parameters.h"
#include "motor_hal.h"

volatile u8 app_fault,app_mode;
volatile u16 app_ticks,app_command_age,app_control_age;
u8 motor_modes[MOTOR_COUNT],fault_channels;
u32 APP_XDATA encoder_counts[MOTOR_COUNT];
static u32 APP_XDATA previous_counts[MOTOR_COUNT];
static s16 APP_XDATA targets[MOTOR_COUNT],setpoints[MOTOR_COUNT],speeds[MOTOR_COUNT],outputs[MOTOR_COUNT];
static u16 APP_XDATA stall_time[MOTOR_COUNT];
static SpeedController xdata controllers[MOTOR_COUNT];
static PacketParser xdata parser;
static MotorCommand xdata command;
static u8 xdata status_packet[STATUS_SIZE];
static u16 xdata parameter_value;
static u16 last_control;
static u8 proposed_modes[MOTOR_COUNT];
static s16 APP_XDATA proposed_targets[MOTOR_COUNT];
static u8 ping_index;
static u16 last_rx_tick;
static const u8 code ping_request[]="PING";
static const u8 code ping_reply[]="PONG\r\n";
#if BOARD_COMM_DIAGNOSTICS
volatile u8 app_debug_stage,app_debug_rx_irq,app_debug_rx_main,app_debug_frames;
#define DEBUG_STAGE(value) app_debug_stage=(value)
#define DEBUG_BOOT(value) do { SBUF=(value); while(!TI) { } TI=0; } while(0)
#else
#define DEBUG_STAGE(value)
#define DEBUG_BOOT(value)
#endif

static void ping_feed(u8 value)
{
    u8 i;
    /* Text is recognized only outside a binary frame. No watchdog renewal. */
    if(parser.used) { ping_index=0; return; }
    value &= 0xDF;
    if(value==ping_request[ping_index]) ++ping_index;
    else ping_index=value=='P' ? 1 : 0;
    if(ping_index==4) {
        ping_index=0;
        for(i=0;i<6;++i) status_packet[i]=ping_reply[i];
        Serial_Send(status_packet,6);
    }
}

void App_TickISR(void)
{
    ++app_ticks;
    if(app_command_age<65535U) ++app_command_age;
    if(app_control_age<65535U) ++app_control_age;
    if(app_mode) {
        if(app_command_age>=settings.timeout_ms) app_fault |= FAULT_TIMEOUT;
        if(app_control_age>CONTROL_MAX_DELAY_MS) app_fault |= FAULT_SCHEDULER;
        if(app_fault) MOTOR_STBY=0;
    }
}

static void update_active(void)
{
    u8 i;
    app_mode=0;
    for(i=0;i<MOTOR_COUNT;++i) if(motor_modes[i] || motor_applied[i]) app_mode=1;
}

static void stop_mask(u8 mask)
{
    u8 i;
    for(i=0;i<MOTOR_COUNT;++i) if(mask&(1U<<i)) {
        motor_modes[i]=MODE_STOP; targets[i]=setpoints[i]=outputs[i]=0;
        stall_time[i]=0; Speed_Reset(&controllers[i]);
    }
    Motor_Disable(mask);
    update_active();
}

static void send_status(u8 type,u8 sequence,u8 error)
{
    u8 i,offset,old_ea;
    status_packet[0]=0xAA; status_packet[1]=0x55;
    status_packet[2]=type; status_packet[3]=sequence;
    status_packet[4]=error; status_packet[5]=app_fault; status_packet[6]=fault_channels;
    status_packet[7]=1 | (CONFIG_SCHEMA_VERSION<<4); /* Inferred direction + parameter schema. */
    old_ea=EA; EA=0;
    Protocol_Put16(status_packet+8,app_command_age); EA=old_ea;
    Encoder_ReadAll(encoder_counts);
    for(i=0;i<MOTOR_COUNT;++i) {
        offset=10+11*i;
        status_packet[offset]=motor_modes[i];
        Protocol_Put16(status_packet+offset+1,(u16)targets[i]);
        Protocol_Put16(status_packet+offset+3,(u16)motor_applied[i]);
        Protocol_Put16(status_packet+offset+5,(u16)speeds[i]);
        Protocol_Put32(status_packet+offset+7,encoder_counts[i]);
    }
    Protocol_Put16(status_packet+54,parameter_value);
    status_packet[56]=(u8)command.values[0]; status_packet[57]=command.flags;
    Protocol_Put16(status_packet+58,FW_VERSION);
    Protocol_Put16(status_packet+60,FW_CAPABILITIES);
    status_packet[62]=config_state;
    status_packet[63]=Protocol_Crc(status_packet+2,61);
    Serial_Send(status_packet,STATUS_SIZE);
}

static u8 motion_command(void)
{
    u8 i,mode,old_ea;
    s16 limit;
    if(command.type==CMD_SINGLE) {
        if(command.flags>4 || command.values[0]<0 || command.values[0]>3 ||
           command.values[2] || command.values[3]) return 2;
    }
    for(i=0;i<MOTOR_COUNT;++i) {
        proposed_modes[i]=motor_modes[i]; proposed_targets[i]=targets[i];
        if(command.type==CMD_SINGLE) {
            if(command.flags && command.flags!=i+1) continue;
            mode=(u8)command.values[0]; proposed_targets[i]=command.values[1];
        } else {
            mode=(command.flags>>(2*i))&3;
            proposed_targets[i]=command.values[i];
        }
        proposed_modes[i]=mode;
        limit=mode==MODE_PWM ? (s16)settings.motor[i].value[PAR_MAX_PWM] :
              (mode==MODE_SPEED ? (s16)settings.motor[i].value[PAR_MAX_SPEED] : 0);
        if(proposed_targets[i]<-limit || proposed_targets[i]>limit) return 2;
    }
    if(app_fault) return 3;
    /* Validate the complete request before changing any motor. */
    old_ea=EA; EA=0;
    if(app_fault) { EA=old_ea; return 3; }
    app_command_age=app_control_age=0;
    for(i=0;i<MOTOR_COUNT;++i) {
        if(motor_modes[i]!=proposed_modes[i] ||
           (targets[i]>0 && proposed_targets[i]<0) || (targets[i]<0 && proposed_targets[i]>0)) {
            Speed_Reset(&controllers[i]); setpoints[i]=0; stall_time[i]=0;
        }
        motor_modes[i]=proposed_modes[i]; targets[i]=proposed_targets[i];
    }
    EA=old_ea;
    if(command.type==CMD_SINGLE)
        for(i=0;i<MOTOR_COUNT;++i)
            if(motor_modes[i]==MODE_STOP && (!command.flags || command.flags==i+1)) stop_mask(1U<<i);
    update_active();
    return 0;
}

static void process_command(void)
{
    u8 error=0,i,mask,old_ea;
    parameter_value=0;
    if(command.type==CMD_SINGLE || command.type==CMD_BATCH) error=motion_command();
    else if(command.type==CMD_PARAM_GET || command.type==CMD_PARAM_SET) {
        if(command.values[0]<0 || command.values[0]>128 || command.values[2] || command.values[3] ||
           (command.type==CMD_PARAM_GET && command.values[1])) error=2;
        else if(command.type==CMD_PARAM_SET && app_mode) error=3;
        else {
            parameter_value=(u16)command.values[1];
            error=Config_Access(command.type==CMD_PARAM_SET,command.flags,
                                (u8)command.values[0],&parameter_value);
            if(!error && command.type==CMD_PARAM_SET &&
               (command.values[0]==PAR_POLARITY || command.values[0]==PAR_REVERSE_MS))
                Motor_Reconfigure(command.flags);
        }
    } else {
        for(i=0;i<MOTOR_COUNT;++i) if(command.values[i]) error=2;
        if(command.flags && command.type!=CMD_ZERO) error=1;
        if(!error) switch(command.type) {
        case CMD_STOP: stop_mask(15); break;
        case CMD_QUERY: break;
        case CMD_HEARTBEAT:
            if(app_fault) error=3;
            else { old_ea=EA; EA=0; app_command_age=0; EA=old_ea; }
            break;
        case CMD_CLEAR:
            if(app_mode) error=3;
            else { old_ea=EA; EA=0; app_fault=0; fault_channels=0; EA=old_ea; }
            break;
        case CMD_ZERO:
            if(command.flags>4) { error=2; break; }
            mask=command.flags ? (1U<<(command.flags-1)) : 15;
            for(i=0;i<MOTOR_COUNT;++i) if((mask&(1U<<i)) && (motor_modes[i] || motor_applied[i])) error=3;
            if(!error) {
                Encoder_Zero(mask);
                for(i=0;i<MOTOR_COUNT;++i) if(mask&(1U<<i)) {
                    encoder_counts[i]=previous_counts[i]=0; speeds[i]=0;
                }
            }
            break;
        case CMD_DEFAULT:
            if(app_mode) error=3;
            else { Config_Default(); Motor_Reconfigure(0); }
            break;
        default: error=1;
        }
    }
#if BOARD_COMM_DIAGNOSTICS
    /* This target exercises the complete idle main loop; never drive motors. */
    if(command.type==CMD_SINGLE || command.type==CMD_BATCH) { stop_mask(15); error=3; }
#endif
    send_status(RESPONSE_TYPE,command.sequence,error);
}

static s16 speed_from_delta(u32 current,u32 previous,u16 elapsed)
{
    s32 delta=(s32)(current-previous),result;
    if(delta>32767L) return 32767;
    if(delta<-32767L) return -32767;
    result=delta*1000L/elapsed;
    if(result>32767L) return 32767;
    if(result<-32767L) return -32767;
    return (s16)result;
}

static void control_step(u16 elapsed)
{
    u8 i,old_ea=EA;
    s16 absolute_pwm,absolute_speed;
    u16 step;
    const MotorParameters APP_XDATA *p;
    EA=0; app_control_age=0; EA=old_ea;
    Encoder_ReadAll(encoder_counts);
    for(i=0;i<MOTOR_COUNT;++i) {
        speeds[i]=speed_from_delta(encoder_counts[i],previous_counts[i],elapsed);
        previous_counts[i]=encoder_counts[i];
        p=&settings.motor[i];
        outputs[i]=targets[i];
        if(motor_modes[i]==MODE_SPEED) {
            step=((targets[i]>=0 ? targets[i] : -targets[i]) >
                  (setpoints[i]>=0 ? setpoints[i] : -setpoints[i])) ?
                   p->value[PAR_SPEED_ACCEL] : p->value[PAR_SPEED_DECEL];
            setpoints[i]=Control_Ramp(setpoints[i],targets[i],step);
            outputs[i]=Speed_Step(&controllers[i],setpoints[i],speeds[i],p);
        }
        absolute_pwm=motor_applied[i]>=0 ? motor_applied[i] : -motor_applied[i];
        absolute_speed=speeds[i]>=0 ? speeds[i] : -speeds[i];
        if(motor_modes[i]!=MODE_STOP && motor_modes[i]!=MODE_DECEL && targets[i] &&
           p->value[PAR_STALL_PWM] && absolute_pwm>=p->value[PAR_STALL_PWM] &&
           absolute_speed<=p->value[PAR_STALL_SPEED]) {
            if(stall_time[i]<p->value[PAR_STALL_MS]) stall_time[i]+=elapsed;
            if(stall_time[i]>=p->value[PAR_STALL_MS]) {
                app_fault |= FAULT_ENCODER; fault_channels |= (1U<<i);
            }
        } else stall_time[i]=0;
    }
    if(app_fault) stop_mask(15);
    else {
        Motor_Apply(outputs);
        for(i=0;i<MOTOR_COUNT;++i)
            if(motor_modes[i]==MODE_DECEL && !motor_applied[i]) stop_mask(1U<<i);
        update_active();
    }
}

void App_Init(void)
{
    u8 i;
    Hal_Init();
    DEBUG_STAGE(1); DEBUG_BOOT('H');
    Config_Default();
    DEBUG_STAGE(2); DEBUG_BOOT('D');
    app_fault=app_mode=fault_channels=0;
    app_ticks=app_command_age=app_control_age=last_control=0;
    ping_index=0; last_rx_tick=0;
    Protocol_Reset(&parser);
    for(i=0;i<MOTOR_COUNT;++i) {
        encoder_direction[i]=1; motor_modes[i]=0; speeds[i]=0; previous_counts[i]=0;
    }
    Encoder_Zero(15); stop_mask(255);
    DEBUG_STAGE(3); DEBUG_BOOT('M');
    EA=1;
}

void App_Poll(void)
{
    u8 value,budget,old_ea;
    u16 now,elapsed;
    DEBUG_STAGE(4);
    old_ea=EA; EA=0; now=app_ticks; EA=old_ea;
    if(app_fault && app_mode) stop_mask(15);
    if(Serial_Recover() || (u16)(now-last_rx_tick)>100U) {
        Protocol_Reset(&parser); ping_index=0;
    }
    for(budget=0;budget<32 && Serial_Read(&value);++budget) {
#if BOARD_COMM_DIAGNOSTICS
        ++app_debug_rx_main;
#endif
        ping_feed(value);
        if(Protocol_Feed(&parser,value,&command)) {
#if BOARD_COMM_DIAGNOSTICS
            ++app_debug_frames;
#endif
            process_command();
        }
        last_rx_tick=now;
    }
    elapsed=(u16)(now-last_control);
    if(elapsed>=CONTROL_PERIOD_MS) { last_control=now; DEBUG_STAGE(5); control_step(elapsed); }
    DEBUG_STAGE(6);
}
#ifndef MOTOR_HOST_TEST
void main(void) { App_Init(); while(1) App_Poll(); }
#endif
