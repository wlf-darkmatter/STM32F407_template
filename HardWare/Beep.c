#include <Beep.h>
#include <LED_STM32F407ZET6.h>
#include <delay.h>

//��ʼ���������˿ں�������Ҫ��ȷ�÷���������Դ������Դ��
void Beep_Init(void) {

	GPIO_InitTypeDef Beep_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//ʹ��GPIOF

	Beep_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	Beep_InitStructure.GPIO_Pin = GPIO_Pin_0;
	Beep_InitStructure.GPIO_OType = GPIO_OType_PP;
	Beep_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	Beep_InitStructure.GPIO_Speed = GPIO_High_Speed;//100MHz
	GPIO_Init(GPIOF, &Beep_InitStructure);

	GPIO_ResetBits(GPIOF,GPIO_Pin_0);//��ʼ����ʱ���PF(0)�õ͵�ƽ

}

//��Դ,			#֮����Ҫ�����Լ���Ҫ���ΪPWM���
/*
void Beep_Ring(uint16_t frequence) {
	
}
*/

//��Դ			#��ûд���ٺٺ�
void Beep_Ring(void) {

}

//������������
void Beep_Sec(control _state) {//�ú����á�TIM3��ʱ��ʵ��
	Beep_Init();
	while (1) {
		LED1 = 0;
		LED2 = 1;

		BEEP = 0;

		delay_ms(1);
		BEEP = 1;
		delay_ms(999);

		LED1 = 1;
		LED2 = 0;

		BEEP = 0;
		delay_ms(1);
		BEEP = 1;
		delay_ms(999);
	}

}


