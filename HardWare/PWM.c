#include <PWM.h>
//ͨ��ʹ�á�TIM14����ʵ�֡�PWM�������
//����Ϊ��PA7��

void PWM_TIM14_Init(u32 Period, u16 Prescaler) {
	//�ٿ�����TIM14����ʱ�Ӻ͡�GPIO�������Ÿ����ض���
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//ʹ��PORTAʱ��	!!!һ��Ҫ�ǵã���
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_TIM14);

	//������GPIO���ƶ�GPIOA7�����Ÿ��ù���
	GPIO_InitTypeDef PWM_PA7_InitStructure;
	PWM_PA7_InitStructure.GPIO_Pin = GPIO_Pin_7;
	PWM_PA7_InitStructure.GPIO_Mode = GPIO_Mode_AF;			//���ù���
	PWM_PA7_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
	PWM_PA7_InitStructure.GPIO_OType = GPIO_OType_PP;		//���츴�����
	PWM_PA7_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;			//����
	GPIO_Init(GPIOA, &PWM_PA7_InitStructure);				//��ʼ��PA7

	//������TIM14��һЩ��������
	TIM_TimeBaseInitTypeDef  PWM_TIM14_InitStructure;
	PWM_TIM14_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	PWM_TIM14_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;//���ϼ���
	PWM_TIM14_InitStructure.TIM_Period = Period;
	PWM_TIM14_InitStructure.TIM_Prescaler = Prescaler;
	TIM_TimeBaseInit(TIM14, &PWM_TIM14_InitStructure); //��ʼ����ʱ��14


	//������TIM14��ͨ��һ�еġ�PWM������
	TIM_OCInitTypeDef PWM_Mode_InitStructure;
//	PWM_Mode_InitStructure.TIM_OCIdleState;
	PWM_Mode_InitStructure.TIM_OCMode = TIM_OCMode_PWM1;		//ѡ��PWMģʽ1
//	PWM_Mode_InitStructure.TIM_OCNIdleState;
//	PWM_Mode_InitStructure.TIM_OCNPolarity;
	PWM_Mode_InitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
//	PWM_Mode_InitStructure.TIM_OutputNState;
	PWM_Mode_InitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	PWM_Mode_InitStructure.TIM_Pulse;
	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);	//ʹ��TIM14��CCR1�ϵ�Ԥװ�ؼĴ���
	TIM_OC1Init(TIM14, &PWM_Mode_InitStructure);

	//��ʹ�ܡ�PWM������TIM14��

	TIM_ARRPreloadConfig(TIM14, ENABLE);				//ARPEʹ��
	TIM_Cmd(TIM14, ENABLE);								//����һ�̿�ʼ��TIM14�Ѿ���ʼ�����

}
	//���޸ġ�TIM14_CCR1������PWM��ռ�ձ�
//ʹ�ú���	TIM_SetCompare1(TIM14,VAL)


