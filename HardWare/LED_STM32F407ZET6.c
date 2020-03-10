/*该文件是单片机上的内置LED*/
/*启动该文件意味着启动了GPIOF的时钟*/
#include "LED_STM32F407ZET6.h"
//#include "stm32f4xx_gpio.h"


void LED_Init(void){
	GPIO_InitTypeDef InnerLED_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF,ENABLE);//使能GPIOF时钟
	
	//设置GPIO9与GPIO10
	InnerLED_InitStructure.GPIO_Pin 	= GPIO_Pin_9|GPIO_Pin_10;
	InnerLED_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	InnerLED_InitStructure.GPIO_OType	= GPIO_OType_PP;
	InnerLED_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	InnerLED_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	
	//启动
	GPIO_Init(GPIOF,&InnerLED_InitStructure);
	
	//启动后，使灯灭(置位=灭，复位=亮)
	GPIO_SetBits(GPIOF,GPIO_Pin_9|GPIO_Pin_10);
	
}



