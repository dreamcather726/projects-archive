#include "board_config.h"
#include "motor_driver.h"
#include "app.h"
#include "encoder.h"
#include "parameters.h"
#include "control_logic.h"
#include "motor_hal.h"

s16 APP_XDATA motor_applied[MOTOR_COUNT];
static signed char last_sign[MOTOR_COUNT];
static u8 reverse_wait[MOTOR_COUNT];
static u16 APP_XDATA compare_values[MOTOR_COUNT];

void Motor_Reconfigure(u8 scope)
{
    u8 i;
    for(i=0;i<MOTOR_COUNT;++i) if((!scope || scope==i+1) && last_sign[i])
        reverse_wait[i]=(u8)((settings.motor[i].value[PAR_REVERSE_MS]+CONTROL_PERIOD_MS-1)/CONTROL_PERIOD_MS);
}

void Motor_Disable(u8 mask)
{
    u8 i,active=0;
    for(i=0;i<MOTOR_COUNT;++i) {
        if(mask&(1U<<i)) { motor_applied[i]=0; compare_values[i]=0; }
        if(mask&128) { last_sign[i]=0; reverse_wait[i]=0; }
        if(motor_applied[i]) active=1;
    }
    if(!active) MOTOR_STBY=0;
    Hal_PWM(compare_values);
}

void Motor_Apply(const s16 APP_XDATA *requested)
{
    u8 i,directions=0,active=0,old_ea;
    s16 value,wanted;
    signed char sign;
    u16 step;
    const MotorParameters APP_XDATA *p;
    MOTOR_STBY=0;
    for(i=0;i<MOTOR_COUNT;++i) {
        p=&settings.motor[i]; wanted=requested[i];
        if(motor_modes[i]==MODE_STOP) {
            value=0;
            if(reverse_wait[i]) --reverse_wait[i];
        } else if(reverse_wait[i]) { --reverse_wait[i]; value=0; }
        else {
            sign=wanted>0 ? 1 : (wanted<0 ? -1 : 0);
            if(p->value[PAR_POLARITY]) sign=-sign;
            if(sign && last_sign[i] && sign!=last_sign[i]) {
                /* Direction changes only after zero PWM and the configured pause. */
                value=0; last_sign[i]=sign;
                reverse_wait[i]=(u8)((p->value[PAR_REVERSE_MS]+CONTROL_PERIOD_MS-1)/CONTROL_PERIOD_MS-1);
            } else {
                if(sign) last_sign[i]=sign;
                step=((wanted>=0 ? wanted : -wanted) >
                      (motor_applied[i]>=0 ? motor_applied[i] : -motor_applied[i]))
                     ? p->value[PAR_PWM_ACCEL] : p->value[PAR_PWM_DECEL];
                value=Control_Ramp(motor_applied[i],wanted,step);
            }
        }
        motor_applied[i]=value;
        if(value) {
            encoder_direction[i]=value>0 ? 1 : -1;
            active=1;
        }
        if(p->value[PAR_POLARITY]) value=-value;
        if(value>0) directions |= (1U<<i);
        compare_values[i]=(u16)(((u32)(value<0 ? -value : value)*PWM_COUNTS)/1000UL);
    }
    MOTOR_DIR1=(directions&1)!=0; MOTOR_DIR2=(directions&2)!=0;
    MOTOR_DIR3=(directions&4)!=0; MOTOR_DIR4=(directions&8)!=0;
    Hal_PWM(compare_values);
    old_ea=EA; EA=0;
    if(!app_fault && active) MOTOR_STBY=1;
    EA=old_ea;
}
