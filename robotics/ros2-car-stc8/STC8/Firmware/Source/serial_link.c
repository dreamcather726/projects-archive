#include "board_config.h"
#include "serial_link.h"
#include "app.h"
#include "motor_hal.h"
#define RX_RING_SIZE 64
static volatile u8 xdata rx_ring[RX_RING_SIZE];
static volatile u8 rx_read,rx_write,rx_overflow;
void Serial_RxISR(u8 value)
{
    u8 next=(rx_write+1)&63;
#if BOARD_COMM_DIAGNOSTICS
    ++app_debug_rx_irq;
#endif
    if(next==rx_read) {
        MOTOR_STBY=0; app_fault |= FAULT_RX_OVERFLOW; rx_overflow=1;
    } else { rx_ring[rx_write]=value; rx_write=next; }
}
u8 Serial_Read(u8 APP_DATA *value)
{
    if(rx_read==rx_write) return 0;
    *value=rx_ring[rx_read]; rx_read=(rx_read+1)&63;
    return 1;
}
u8 Serial_Recover(void)
{
    u8 old_es;
    if(!rx_overflow) return 0;
    old_es=ES; ES=0; rx_read=rx_write; rx_overflow=0; ES=old_es;
    return 1;
}
void Serial_Send(const u8 APP_XDATA *bytes,u8 length) { Hal_Send(bytes,length); }
