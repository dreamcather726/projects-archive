#include "control_logic.h"
void Speed_Reset(SpeedController APP_XDATA *controller) { controller->integral=0; }

s16 Control_Ramp(s16 current,s16 target,u16 step)
{
    /* Both speed and duty targets are bounded to +/-10000, so delta fits s16. */
    s16 difference=target-current;
    if(difference>(s16)step) return current+(s16)step;
    if(difference<-(s16)step) return current-(s16)step;
    return target;
}

s16 Speed_Step(SpeedController APP_XDATA *controller,s16 target,s16 measured,
               const MotorParameters APP_XDATA *parameters)
{
    s32 error=(s32)target-measured;
    s32 limit=(s32)parameters->value[PAR_I_LIMIT]*64L;
    s32 candidate=controller->integral+error*parameters->value[PAR_KI];
    s32 proportional=error*parameters->value[PAR_KP];
    s32 effort;
    s16 maximum=(s16)parameters->value[PAR_MAX_PWM];
    if(!target) { Speed_Reset(controller); return 0; }
    if(candidate>limit) candidate=limit;
    if(candidate<-limit) candidate=-limit;
    effort=(proportional+candidate)/64L;
    if(!((effort>maximum && error>0) || (effort<-maximum && error<0)))
        controller->integral=candidate;
    effort=(proportional+controller->integral)/64L;
    if(effort>maximum) effort=maximum;
    if(effort<-maximum) effort=-maximum;
    if(target>0 && effort<0) effort=0;
    if(target<0 && effort>0) effort=0;
    return (s16)effort;
}
