#include <key_STM32F407ZET6.h>
#include <LED_STM32F407ZET6.h>
#include <delay.h>
#include <OLED.h>
#include <lcd.h>
#include "ucos_ii.h"
u8 KEY_STA = 0;


//�ȳ�ʼ��
void Key_Init(void) {
	KEY_STA = 0;
	//��Զ��ס����ʹ��ʱ��
	//KEY0��KEY1�ֲ���GPIOE��WK_UP�ֲ���GPIOA
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE);
/*�ٳ�ʼ��GPIO*/

	GPIO_InitTypeDef Key_InitStructure;
	//����PE4��KEY0)��PE3(KEY1)
	Key_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3;
	Key_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	Key_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//Ĭ���������ߵ�ƽ�����������°�ť����Ӧ�����žͽӵ�
	Key_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOE,&Key_InitStructure);
	
	//����PA0(WK_UP)
	Key_InitStructure.GPIO_Pin = GPIO_Pin_0;
	Key_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	Key_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	Key_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOA,&Key_InitStructure);

#ifndef OS_CFG_H
/*����SYSCFG_EXTILineConfig()��������ʼ�����á�����Դ�����ţ����롾EXTI�ߡ���
	#�ǵ�ʹ�ܡ�SYSCFG�����ڡ�APB2���*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//ʹ��SYSCFGϵͳ���üĴ���
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource4);//GPIOE��EXTI4���ӣ�Ҳ����PE4��Ӧ�ж���4
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource3);//ͬ��PE3��
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);//ͬ��PA0��

/*����EXTI_Init()������ʼ���ⲿ�ж��ߣ������ô���������*/
	EXTI_InitTypeDef Key_EXTI;
	/*���á�PE3-KEY1��PE4-KEY0��*/
	Key_EXTI.EXTI_Line = EXTI_Line3 | EXTI_Line4;
	Key_EXTI.EXTI_Mode = EXTI_Mode_Interrupt;
	Key_EXTI.EXTI_Trigger = EXTI_Trigger_Falling;//�������������ģ�����˵���ӵ��ˣ���ť���£�
	Key_EXTI.EXTI_LineCmd = ENABLE;
	EXTI_Init(&Key_EXTI);

	/*���á�PA0-WK_UP��*/
	Key_EXTI.EXTI_Line = EXTI_Line0;
	Key_EXTI.EXTI_Mode = EXTI_Mode_Interrupt;
	Key_EXTI.EXTI_Trigger = EXTI_Trigger_Rising;//�����������ģ���ť�����¾ͻ�ӵ�
	Key_EXTI.EXTI_LineCmd = ENABLE;
	EXTI_Init(&Key_EXTI);

/*����NVIC_Init()��������NVIC IRQͨ�������жϷ��飬��ʹ�ܡ�*/
	NVIC_InitTypeDef Key_NVIC;
	/* ����EXTI_Line0-��WK_UP��*/
	Key_NVIC.NVIC_IRQChannel = EXTI0_IRQn;//�ⲿ�ж�0
	Key_NVIC.NVIC_IRQChannelPreemptionPriority = 0x01;//��ռ���ȼ�1
	Key_NVIC.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
	Key_NVIC.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
	NVIC_Init(&Key_NVIC);//����

	/*��KEY0��*/
	Key_NVIC.NVIC_IRQChannel = EXTI4_IRQn;//�ⲿ�ж�4
	Key_NVIC.NVIC_IRQChannelPreemptionPriority = 0x01;//��ռ���ȼ�1
	Key_NVIC.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
	Key_NVIC.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
	NVIC_Init(&Key_NVIC);//����

	/*��KEY1��*/
	Key_NVIC.NVIC_IRQChannel = EXTI3_IRQn;//�ⲿ�ж�3
	Key_NVIC.NVIC_IRQChannelPreemptionPriority = 0x01;//��ռ���ȼ�1
	Key_NVIC.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
	Key_NVIC.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
	NVIC_Init(&Key_NVIC);//����
#endif
}

/*�ݶ����ⲿ�жϷ�����*/
/*Ҫ�ǵ�����жϱ�־λ����������*/
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
/*�жϺ���ȫ���������������ͳһ�ж�*/
/*void Key_IRQHandler(void) {
	
	//��һ���ֽ�����ʵ��ͨ������ťK0��K1������STM32�Դ���LED�ƵĹ���
	Key_Scan();
	D1 = (((KEY_STA & 0x01) != 0) ? (!D1) : D1);
	D2 = (((KEY_STA & 0x02) != 0) ? (!D2) : D2);
	


}*/

u8 Key_Scan(void) {
	KEY_STA = 0;//ÿ�β�ѯǰ������
	//��ѯǰ����
	delay_ms(10);
//����ֵ0x0000 0000
//[0]λΪKEY0 = PE4��[1]λΪKEY1 = PE3��[7]λΪWK_UP = PA0,��Ӧλ�����£��ͻ���ʾΪ1
	KEY_STA = (u8)(((KEY0 == 0) << 0) | ((KEY1 == 0) << 1) | ((WK_UP != 0) << 7));
	if (KEY_STA & 0x01) {//KEY0���£�OLEDˢ��
		OLED_Refresh();
	}
	return KEY_STA;
}

