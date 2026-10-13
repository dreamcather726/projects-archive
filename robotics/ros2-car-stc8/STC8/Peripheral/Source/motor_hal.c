/* STC8H1K08 motor adapter, derived from STC MCU Limited GPIO/PWM/UART/
 * timer/Exti SDK examples. Uses the STC register definitions.
 * The general SDK modules are retained separately for other applications.
 */
#include "board_config.h"
#include "motor_hal.h"
#include "serial_link.h"
#include "encoder.h"
#include "app.h"

#define TX_SIZE 128
static u8 xdata tx_buffer[TX_SIZE];
static volatile u8 tx_read,tx_write,tx_busy;

void Hal_Init(void)
{
    EA=0; IE=0; IE2=0; INT_CLKO=0;
    EAXRAM(); InternalXdata_Enable();
    PCON &= 0x7F; P3 |= 0x03;
    MOTOR_STBY=0; P1=0;
    P1M1=(u8)~(BOARD_PWM_P1_MASK | DIR_MASK(1));
    P1M0=BOARD_PWM_P1_MASK | DIR_MASK(1);
    P5M1 &= ~0x10; P5M0 |= 0x10;
    /* Match the board-verified UART probe: RX quasi-bidirectional, TX push-pull. */
    P3M1=(P3M1 | BOARD_ENCODER_MASK) & ~(0x03 | BOARD_PWM_P3_MASK | DIR_MASK(3));
    P3M0=(P3M0 & ~(BOARD_ENCODER_MASK | 1)) | 0x02 | BOARD_PWM_P3_MASK | DIR_MASK(3);
    EAXSFR();
    P3PU=(P3PU | BOARD_ENCODER_MASK) & ~0x01;
    PWMA_CR1=0; PWMA_CR2=0; PWMA_IER=0;
    PWMA_PSCR=0; PWMA_ARR=(u16)(PWM_COUNTS-1);
    PWMA_CCR1=PWMA_CCR2=PWMA_CCR3=PWMA_CCR4=0;
    PWMA_CCMR1=PWMA_CCMR2=PWMA_CCMR3=PWMA_CCMR4=0x68;
    PWMA_CCER1=PWMA_CCER2=0x11;
    PWMA_PS=BOARD_PWM_ROUTE; PWMA_ENO=0x55; PWMA_DTR=0;
    PWMA_BRK=0x80; PWMA_EGR=1; PWMA_SR1=0; PWMA_SR2=0;
    PWMA_CR1=0x81;
    EAXRAM();
    TR1=0; TMOD &= ~0xF0; AUXR=(AUXR & ~0x01) | 0x40;
    TH1=(u8)((65536UL-((FOSC_HZ/4+UART_BAUD/2)/UART_BAUD))>>8);
    TL1=(u8)(65536UL-((FOSC_HZ/4+UART_BAUD/2)/UART_BAUD));
    ET1=0; INT_CLKO &= ~0x03; TR1=1;
    P_SW1 &= 0x3F; SCON=0x50; UART1_Polity(1); ES=1;
    tx_read=tx_write=tx_busy=0;
    IE0=IE1=0; IT0=IT1=1; EX0=EX1=1;
    INT0_Polity(3); INT1_Polity(3); INT_CLKO |= 0x30;
    TR0=0; TMOD &= ~0x0F; AUXR |= 0x80;
    TH0=(u8)((65536UL-FOSC_HZ/1000)>>8);
    TL0=(u8)(65536UL-FOSC_HZ/1000);
    Timer0_Polity(2); TF0=0; ET0=1; TR0=1;
}

void Hal_PWM(const u16 xdata *compare)
{
    u8 old_ea=EA,old_switch=P_SW2;
    EA=0; EAXSFR();
    PWMA_CCR1=compare[0]; PWMA_CCR2=compare[1];
    PWMA_CCR3=compare[2]; PWMA_CCR4=compare[3];
    PWMA_EGR=1; P_SW2=old_switch; EA=old_ea;
}

u8 Hal_Send(const u8 xdata *bytes,u8 length)
{
    u8 end=tx_write,old_es;
    if(length>127-((tx_write-tx_read)&127)) return 0;
    while(length--) { tx_buffer[end]=*bytes++; end=(end+1)&127; }
    old_es=ES; ES=0; tx_write=end;
    if(!tx_busy && tx_read!=tx_write) {
        tx_busy=1; SBUF=tx_buffer[tx_read]; tx_read=(tx_read+1)&127;
    }
    ES=old_es;
    return 1;
}

void Hal_UART_ISR(void) interrupt UART1_VECTOR
{
    u8 received;
    if(RI) { received=SBUF; RI=0; Serial_RxISR(received); }
    if(TI) {
        TI=0;
        if(tx_read!=tx_write) { SBUF=tx_buffer[tx_read]; tx_read=(tx_read+1)&127; }
        else tx_busy=0;
    }
}
#if BOARD_COMM_DIAGNOSTICS
static void debug_byte(u8 value)
{ SBUF=value; while(!TI) { } TI=0; }
static void debug_hex(u8 value)
{
    static const char code digits[]="0123456789ABCDEF";
    debug_byte(digits[value>>4]); debug_byte(digits[value&15]);
}
static void debug_field(u8 key,u8 value)
{ debug_byte(' '); debug_byte(key); debug_byte('='); debug_hex(value); }
static void debug_dump(void)
{
    u8 old_ea=EA,uart=SCON,interrupts=IE;
    /* Diagnostic build only, stopped motors and an idle TX queue. RX may lose
     * bytes during this polled message; this build is not for motor operation. */
    if(app_mode || tx_busy) return;
    EA=0;
    debug_byte('\r'); debug_byte('\n'); debug_byte('D'); debug_byte('B'); debug_byte('G');
    debug_field('S',app_debug_stage); debug_field('I',app_debug_rx_irq);
    debug_field('R',app_debug_rx_main); debug_field('C',app_debug_frames);
    debug_field('U',uart); debug_field('E',interrupts);
    debug_byte('\r'); debug_byte('\n'); EA=old_ea;
}
#endif
void Hal_Timer_ISR(void) interrupt TIMER0_VECTOR
{
#if BOARD_COMM_DIAGNOSTICS
    static u16 debug_ticks;
#endif
    App_TickISR();
#if BOARD_COMM_DIAGNOSTICS
    if(++debug_ticks>=1000) { debug_ticks=0; debug_dump(); }
#endif
}
