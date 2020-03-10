/*���ļ��ǵ�Ƭ���ϵ�����LED*/
/*�������ļ���ζ��������GPIOF��ʱ��*/
#include "LED_STM32F407ZET6.h"
//#include "stm32f4xx_gpio.h"


void LED_Init(void){
	GPIO_InitTypeDef InnerLED_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF,ENABLE);//ʹ��GPIOFʱ��
	
	//����GPIO9��GPIO10
	InnerLED_InitStructure.GPIO_Pin 	= GPIO_Pin_9|GPIO_Pin_10;
	InnerLED_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	InnerLED_InitStructure.GPIO_OType	= GPIO_OType_PP;
	InnerLED_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	InnerLED_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	
	//����
	GPIO_Init(GPIOF,&InnerLED_InitStructure);
	
	//������ʹ����(��λ=�𣬸�λ=��)
	GPIO_SetBits(GPIOF,GPIO_Pin_9|GPIO_Pin_10);
	
}



