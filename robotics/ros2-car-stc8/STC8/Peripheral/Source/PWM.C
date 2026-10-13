/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.STCMCU.com --------------------------------------------*/
/* --- Web: www.STCMCUDATA.com  ---------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了STC的资料及程序            */
/*---------------------------------------------------------------------*/

#include "PWM.h"

u8 PWM_Configuration(u8 PWM, PWMx_InitDefine *PWMx)
{
	if(PWM == PWMA)
	{
		EAXSFR();		/* MOVX A,@DPTR/MOVX @DPTR,A指令的操作对象为扩展SFR(XSFR) */
		
		if(PWMx->PWM1_SetPolity > Polity_3)	return 2;	//错误
		PWM1_Polity(PWMx->PWM1_SetPolity);	//指定中断优先级(低到高) Polity_0,Polity_1,Polity_2,Polity_3
		if(PWMx->PWM2_SetPolity > Polity_3)	return 2;	//错误
		PWM2_Polity(PWMx->PWM2_SetPolity);	//指定中断优先级(低到高) Polity_0,Polity_1,Polity_2,Polity_3
		if(PWMx->PWM3_SetPolity > Polity_3)	return 2;	//错误
		PWM3_Polity(PWMx->PWM3_SetPolity);	//指定中断优先级(低到高) Polity_0,Polity_1,Polity_2,Polity_3
		if(PWMx->PWM4_SetPolity > Polity_3)	return 2;	//错误
		PWM4_Polity(PWMx->PWM4_SetPolity);	//指定中断优先级(低到高) Polity_0,Polity_1,Polity_2,Polity_3
		
		PWMA_CCER1_Disable();		//关闭所有输入捕获/比较输出
		PWMA_CCER2_Disable();		//关闭所有输入捕获/比较输出
		PWMA_OC1ModeSet(PWMx->PWM1_Mode);		//设置输出比较模式
		PWMA_OC2ModeSet(PWMx->PWM2_Mode);		//设置输出比较模式
		PWMA_OC3ModeSet(PWMx->PWM3_Mode);		//设置输出比较模式
		PWMA_OC4ModeSet(PWMx->PWM4_Mode);		//设置输出比较模式
		if(PWMx->PWM_Reload == ENABLE)	PWMA_OC1_ReloadEnable();	//输出比较的预装载使能
		else		PWMA_OC1_RelosdDisable();	//禁止输出比较的预装载
		if(PWMx->PWM_Fast == ENABLE)	PWMA_OC1_FastEnable();		//输出比较快速功能使能
		else		PWMA_OC1_FastDisable();	//禁止输出比较快速功能
		
		if(PWMx->PWM_CC1Enable == ENABLE)	PWMA_CC1E_Enable();			//开启输入捕获/比较输出
		else		PWMA_CC1E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC1NEnable == ENABLE)	PWMA_CC1NE_Enable();	//开启输入捕获/比较输出
		else		PWMA_CC1NE_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC2Enable == ENABLE)	PWMA_CC2E_Enable();			//开启输入捕获/比较输出
		else		PWMA_CC2E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC2NEnable == ENABLE)	PWMA_CC2NE_Enable();	//开启输入捕获/比较输出
		else		PWMA_CC2NE_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC3Enable == ENABLE)	PWMA_CC3E_Enable();			//开启输入捕获/比较输出
		else		PWMA_CC3E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC3NEnable == ENABLE)	PWMA_CC3NE_Enable();	//开启输入捕获/比较输出
		else		PWMA_CC3NE_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC4Enable == ENABLE)	PWMA_CC4E_Enable();			//开启输入捕获/比较输出
		else		PWMA_CC4E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC4NEnable == ENABLE)	PWMA_CC4NE_Enable();	//开启输入捕获/比较输出
		else		PWMA_CC4NE_Disable();	//关闭输入捕获/比较输出
		
		PWMA_AutoReload(PWMx->PWM_Period);
		PWMA_Duty1(PWMx->PWM1_Duty);
		PWMA_Duty2(PWMx->PWM2_Duty);
		PWMA_Duty3(PWMx->PWM3_Duty);
		PWMA_Duty4(PWMx->PWM4_Duty);
		
		PWMA_CCPCAPreloaded(PWMx->PWM_PreLoad);	//捕获/比较预装载控制位(该位只对具有互补输出的通道起作用)
		PWMA_PS = PWMx->PWM_PS_SW;			//切换IO
		PWMA_ENO = PWMx->PWM_EnoSelect;	//输出通道选择
		PWMA_DeadTime(PWMx->PWM_DeadTime);	//死区发生器设置
		
		if(PWMx->PWM_BrakeEnable == ENABLE)	PWMA_BrakeEnable();	//开启刹车输入
		else		PWMA_BrakeDisable();		//禁止刹车输入
		if(PWMx->PWM_MainOutEnable == ENABLE)	PWMA_BrakeOutputEnable();	//主输出使能
		else		PWMA_BrakeOutputDisable();		//主输出禁止
		if(PWMx->PWM_CEN_Enable == ENABLE)	PWMA_CEN_Enable();	//使能计数器
		else		PWMA_CEN_Disable();		//禁止计数器
		
		EAXRAM();		/* MOVX A,@DPTR/MOVX @DPTR,A指令的操作对象为扩展RAM(XRAM) */
		return	0;
	}

#ifndef STC_MOTOR_APP
	if(PWM == PWMB)
	{
		EAXSFR();		/* MOVX A,@DPTR/MOVX @DPTR,A指令的操作对象为扩展SFR(XSFR) */
		
		if(PWMx->PWM5_SetPolity > Polity_3)	return 2;	//错误
		PWM5_Polity(PWMx->PWM5_SetPolity);	//指定中断优先级(低到高) Polity_0,Polity_1,Polity_2,Polity_3
		
		PWMB_CCER1_Disable();		//关闭所有输入捕获/比较输出
		PWMB_CCER2_Disable();		//关闭所有输入捕获/比较输出
		PWMB_OC5ModeSet(PWMx->PWM5_Mode);		//设置输出比较模式
		PWMB_OC6ModeSet(PWMx->PWM6_Mode);		//设置输出比较模式
		PWMB_OC7ModeSet(PWMx->PWM7_Mode);		//设置输出比较模式
		PWMB_OC8ModeSet(PWMx->PWM8_Mode);		//设置输出比较模式
		if(PWMx->PWM_Reload == ENABLE)	PWMB_OC5_ReloadEnable();	//输出比较的预装载使能
		else		PWMB_OC5_RelosdDisable();	//禁止输出比较的预装载
		if(PWMx->PWM_Fast == ENABLE)	PWMB_OC5_FastEnable();		//输出比较快速功能使能
		else		PWMB_OC5_FastDisable();	//禁止输出比较快速功能
		
		if(PWMx->PWM_CC5Enable == ENABLE)	PWMB_CC5E_Enable();			//开启输入捕获/比较输出
		else		PWMB_CC5E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC6Enable == ENABLE)	PWMB_CC6E_Enable();			//开启输入捕获/比较输出
		else		PWMB_CC6E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC7Enable == ENABLE)	PWMB_CC7E_Enable();			//开启输入捕获/比较输出
		else		PWMB_CC7E_Disable();	//关闭输入捕获/比较输出
		if(PWMx->PWM_CC8Enable == ENABLE)	PWMB_CC8E_Enable();			//开启输入捕获/比较输出
		else		PWMB_CC8E_Disable();	//关闭输入捕获/比较输出
		
		PWMB_AutoReload(PWMx->PWM_Period);
		PWMB_Duty5(PWMx->PWM5_Duty);
		PWMB_Duty6(PWMx->PWM6_Duty);
		PWMB_Duty7(PWMx->PWM7_Duty);
		PWMB_Duty8(PWMx->PWM8_Duty);
		
		PWMB_CCPCBPreloaded(PWMx->PWM_PreLoad);	//捕获/比较预装载控制位(该位只对具有互补输出的通道起作用)
		PWMB_PS = PWMx->PWM_PS_SW;			//切换IO
		PWMB_ENO = PWMx->PWM_EnoSelect;	//输出通道选择
		PWMB_DeadTime(PWMx->PWM_DeadTime);	//死区发生器设置
		
		if(PWMx->PWM_BrakeEnable == ENABLE)	PWMB_BrakeEnable();	//开启刹车输入
		else		PWMB_BrakeDisable();		//禁止刹车输入
		if(PWMx->PWM_MainOutEnable == ENABLE)	PWMB_BrakeOutputEnable();	//主输出使能
		else		PWMB_BrakeOutputDisable();		//主输出禁止
		if(PWMx->PWM_CEN_Enable == ENABLE)	PWMB_CEN_Enable();	//使能计数器
		else		PWMB_CEN_Disable();		//禁止计数器
		
		EAXRAM();		/* MOVX A,@DPTR/MOVX @DPTR,A指令的操作对象为扩展RAM(XRAM) */
		return	0;
	}
#endif

	return	2;	//错误
}

/*********************************************************/

/******************* PWM 占空比设置函数 *******************/
#ifndef STC_MOTOR_APP
void UpdatePwm(u8 PWM, PWMx_Duty *PWMx)
{
	EAXSFR();		/* MOVX A,@DPTR/MOVX @DPTR,A指令的操作对象为扩展SFR(XSFR) */

	if(PWM == PWMA)
	{
		PWMA_Duty1(PWMx->PWM1_Duty);
		PWMA_Duty2(PWMx->PWM2_Duty);
		PWMA_Duty3(PWMx->PWM3_Duty);
		PWMA_Duty4(PWMx->PWM4_Duty);
	}
	else
	{
		PWMB_Duty5(PWMx->PWM5_Duty);
		PWMB_Duty6(PWMx->PWM6_Duty);
		PWMB_Duty7(PWMx->PWM7_Duty);
		PWMB_Duty8(PWMx->PWM8_Duty);
	}
	
	EAXRAM();		/* MOVX A,@DPTR/MOVX @DPTR,A指令的操作对象为扩展RAM(XRAM) */
}

/*********************************************************/

#endif
