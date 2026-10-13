#include "board_config.h"
#include "encoder.h"
static volatile u32 APP_XDATA raw_counts[MOTOR_COUNT];
volatile signed char encoder_direction[MOTOR_COUNT];

void Encoder_ReadAll(u32 APP_XDATA *counts)
{
    u8 i,old_ea=EA;
    EA=0;
    for(i=0;i<MOTOR_COUNT;++i) counts[i]=raw_counts[i];
    EA=old_ea;
}
void Encoder_Zero(u8 mask)
{
    u8 i,old_ea=EA;
    EA=0;
    for(i=0;i<MOTOR_COUNT;++i) if(mask&(1U<<i)) raw_counts[i]=0;
    EA=old_ea;
}
/* One A falling edge = one count; sign is inferred from commanded direction. */
#ifdef MOTOR_HOST_TEST
#define ENCODER_ISR(vector)
#else
#define ENCODER_ISR(vector) interrupt vector
#endif
void Encoder_M1ISR(void) ENCODER_ISR(INT0_VECTOR) { if(encoder_direction[0]>0) ++raw_counts[0]; else --raw_counts[0]; }
void Encoder_M2ISR(void) ENCODER_ISR(INT1_VECTOR) { if(encoder_direction[1]>0) ++raw_counts[1]; else --raw_counts[1]; }
void Encoder_M3ISR(void) ENCODER_ISR(INT2_VECTOR) { if(encoder_direction[2]>0) ++raw_counts[2]; else --raw_counts[2]; }
void Encoder_M4ISR(void) ENCODER_ISR(INT3_VECTOR) { if(encoder_direction[3]>0) ++raw_counts[3]; else --raw_counts[3]; }
