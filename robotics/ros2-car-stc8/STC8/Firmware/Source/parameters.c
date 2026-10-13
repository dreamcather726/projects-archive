#include "parameters.h"
#include "board_config.h"

Configuration APP_XDATA settings;
u8 config_state;
static const u16 code defaults[MOTOR_PARAM_COUNT] = {
    0,1000,10000,1,4,1,1000,100,100,100,300,0,500,1000,1000
};
static const u16 code minimum[MOTOR_PARAM_COUNT] = {
    0,1,1,1,0,0,0,1,1,50,0,0,50,1,1
};
static const u16 code maximum[MOTOR_PARAM_COUNT] = {
    1,1000,10000,60000,1024,1024,1000,1000,1000,5000,1000,10000,10000,10000,10000
};
void Config_Default(void)
{
    u8 i,j;
    for(i=0;i<MOTOR_COUNT;++i)
        for(j=0;j<MOTOR_PARAM_COUNT;++j) settings.motor[i].value[j]=defaults[j];
    for(i=0;i<MOTOR_COUNT;++i) settings.motor[i].value[PAR_POLARITY]=(BOARD_INVERT_DEFAULT_MASK>>i)&1;
    settings.timeout_ms=1000;
    config_state=0;
}

u8 Config_Access(u8 write,u8 scope,u8 parameter,u16 APP_XDATA *value)
{
    u8 i,first,last;
    if(scope>4) return 2;
    if(parameter==PAR_TIMEOUT) {
        if(scope || (write && (*value<200 || *value>10000))) return 2;
        if(write) { settings.timeout_ms=*value; config_state=2; }
        else *value=settings.timeout_ms;
        return 0;
    }
    if(parameter>=MOTOR_PARAM_COUNT || (!write && !scope)) return 2;
    if(write && (*value<minimum[parameter] || *value>maximum[parameter])) return 2;
    if(!write) { *value=settings.motor[scope-1].value[parameter]; return 0; }
    first=scope ? scope-1 : 0;
    last=scope ? scope : MOTOR_COUNT;
    for(i=first;i<last;++i) settings.motor[i].value[parameter]=*value;
    config_state=2;
    return 0;
}
