#include <key_STM32F407ZET6.h>
#include <LED_STM32F407ZET6.h>
#include <delay.h>
#include <OLED.h>
#include <lcd.h>
#include "ucos_ii.h"
u8 KEY_STA = 0;


//先初始化
void Key_Init(void) {
	KEY_STA = 0;
	//永远记住，先使能时钟
	//KEY0、KEY1分布在GPIOE，WK_UP分布在GPIOA
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE);
/*①初始化GPIO*/

	GPIO_InitTypeDef Key_InitStructure;
	//定义PE4（KEY0)、PE3(KEY1)
	Key_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3;
	Key_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	Key_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//默认上拉（高电平），这样按下按钮后相应的引脚就接地
	Key_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOE,&Key_InitStructure);
	
	//定义PA0(WK_UP)
	Key_InitStructure.GPIO_Pin = GPIO_Pin_0;
	Key_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	Key_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	Key_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOA,&Key_InitStructure);

#ifndef OS_CFG_H
/*②用SYSCFG_EXTILineConfig()函数来初始化配置【输入源（引脚）】与【EXTI线】。
	#记得使能【SYSCFG】（在【APB2】里）*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//使能SYSCFG系统配置寄存器
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource4);//GPIOE与EXTI4连接，也就是PE4对应中断线4
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource3);//同【PE3】
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);//同【PA0】

/*③用EXTI_Init()函数初始化外部中断线，并设置触发条件。*/
	EXTI_InitTypeDef Key_EXTI;
	/*配置【PE3-KEY1与PE4-KEY0】*/
	Key_EXTI.EXTI_Line = EXTI_Line3 | EXTI_Line4;
	Key_EXTI.EXTI_Mode = EXTI_Mode_Interrupt;
	Key_EXTI.EXTI_Trigger = EXTI_Trigger_Falling;//本来就是上拉的，落下说明接地了（按钮按下）
	Key_EXTI.EXTI_LineCmd = ENABLE;
	EXTI_Init(&Key_EXTI);

	/*配置【PA0-WK_UP】*/
	Key_EXTI.EXTI_Line = EXTI_Line0;
	Key_EXTI.EXTI_Mode = EXTI_Mode_Interrupt;
	Key_EXTI.EXTI_Trigger = EXTI_Trigger_Rising;//本来是下拉的，按钮被按下就会接电
	Key_EXTI.EXTI_LineCmd = ENABLE;
	EXTI_Init(&Key_EXTI);

/*④用NVIC_Init()函数配置NVIC IRQ通道及其中断分组，并使能。*/
	NVIC_InitTypeDef Key_NVIC;
	/* 配置EXTI_Line0-【WK_UP】*/
	Key_NVIC.NVIC_IRQChannel = EXTI0_IRQn;//外部中断0
	Key_NVIC.NVIC_IRQChannelPreemptionPriority = 0x01;//抢占优先级1
	Key_NVIC.NVIC_IRQChannelSubPriority = 0x02;//子优先级2
	Key_NVIC.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
	NVIC_Init(&Key_NVIC);//配置

	/*【KEY0】*/
	Key_NVIC.NVIC_IRQChannel = EXTI4_IRQn;//外部中断4
	Key_NVIC.NVIC_IRQChannelPreemptionPriority = 0x01;//抢占优先级1
	Key_NVIC.NVIC_IRQChannelSubPriority = 0x02;//子优先级2
	Key_NVIC.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
	NVIC_Init(&Key_NVIC);//配置

	/*【KEY1】*/
	Key_NVIC.NVIC_IRQChannel = EXTI3_IRQn;//外部中断3
	Key_NVIC.NVIC_IRQChannelPreemptionPriority = 0x01;//抢占优先级1
	Key_NVIC.NVIC_IRQChannelSubPriority = 0x02;//子优先级2
	Key_NVIC.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
	NVIC_Init(&Key_NVIC);//配置
#endif
}

/*⑤定义外部中断服务函数*/
/*要记得清除中断标志位！！！！！*/
/*
void EXTI3_IRQHandler(void) {
	Key_IRQHandler();
	EXTI_ClearITPendingBit(EXTI_Line3);
}
void EXTI4_IRQHandler(void) {
	Key_IRQHandler();
	EXTI_ClearITPendingBit(EXTI_Line4);
}
void EXTI0_IRQHandler(void) {
	Key_IRQHandler();
	EXTI_ClearITPendingBit(EXTI_Line0);
}
*/
/*中断函数全部都进入这个函数统一判断*/
/*void Key_IRQHandler(void) {
	
	//这一部分仅仅是实现通过按按钮K0和K1来控制STM32自带的LED灯的功能
	Key_Scan();
	D1 = (((KEY_STA & 0x01) != 0) ? (!D1) : D1);
	D2 = (((KEY_STA & 0x02) != 0) ? (!D2) : D2);
	


}*/

u8 Key_Scan(void) {
	KEY_STA = 0;//每次查询前先清零
	//查询前消抖
	delay_ms(10);
//返回值0x0000 0000
//[0]位为KEY0 = PE4，[1]位为KEY1 = PE3，[7]位为WK_UP = PA0,相应位被按下，就会显示为1
	KEY_STA = (u8)(((KEY0 == 0) << 0) | ((KEY1 == 0) << 1) | ((WK_UP != 0) << 7));
	if (KEY_STA & 0x01) {//KEY0按下，OLED刷新
		OLED_Refresh();
	}
	return KEY_STA;
}

