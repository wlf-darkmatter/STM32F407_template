#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//5,增加了【WiFi】的串口通讯
////////////////////////////////////////////////////////////////////////////////// 	
/*要注意，用Arduino当串口通讯器时，要把Arduino的通讯波特率调为与STM32相应串口一致*/

//**定义优先级
#define NVIC_USART1_PreemptionPriority 1
#define NVIC_USART1_SubPriority 1

#define USART1_REC_LEN  			200  	//定义最大接收字节数 200


#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收
#define EN_USART1_TX			DISABLE		//串口1【发送完成中断TCIE】是否打开

extern u8  USART1_RX_BUF[USART1_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART1_RX_STA;         		//接收状态标记
extern u8  USART1_TX_STA;				//自己写的一个关于发送状态的标记变量



//如果想串口中断接收，请不要注释以下宏定义
void usart1_init(u32 bound);




#endif


