/*
 * dianji_uart.c
 *
 *  Created on: Oct 26, 2024
 *      Author: GAO
 */
#include "dianji_uart.h"

//__IO bool rxFrameFlag = false;
//__IO uint8_t rxCmd[FIFO_SIZE] = {0};
//__IO uint8_t rxCount = 0;

/**
	* @brief   USART1中断函数
	* @param   无
	* @retval  无
	*/
//void USART1_IRQHandler(void)
//{
//	__IO uint8_t i;
//	HAL_UART_IRQHandler(&huart1);
//	/**********************************************************
//	***	串口接收中断
//	**********************************************************/
////		if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
//		if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_RXNE) != RESET)
//		{
//			// 未完成一帧数据接收，数据进入缓冲队列
//			fifo_enQueue((uint8_t)USART1->DR);
//
//			// 清除串口接收中断
//	//		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//			__HAL_UART_CLEAR_FLAG(&huart1,UART_FLAG_RXNE);
//		}
//	/**********************************************************
//	***	串口空闲中断
//	**********************************************************/
//	//	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
//		else if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_RXNE) != RESET)
//		{
//			// 先读SR再读DR，清除IDLE中断
//			USART1->SR; USART1->DR;
//
//			// 提取一帧数据命令
//			rxCount = fifo_queueLength();
//			for(i=0; i < rxCount; i++)
//			{ rxCmd[i] = fifo_deQueue(); }
//
//			// 一帧数据接收完成，置位帧标志位
//			rxFrameFlag = true;
//		}
//}

/**
	* @brief   USART发送多个字节
	* @param   无
	* @retval  无
	*/


















