#include <Beep.h>
#include <LED_STM32F407ZET6.h>
#include <delay.h>

//初始化蜂鸣器端口函数，需要明确该蜂鸣器是有源还是无源的
void Beep_Init(void) {

	GPIO_InitTypeDef Beep_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOF

	Beep_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	Beep_InitStructure.GPIO_Pin = GPIO_Pin_0;
	Beep_InitStructure.GPIO_OType = GPIO_OType_PP;
	Beep_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	Beep_InitStructure.GPIO_Speed = GPIO_High_Speed;//100MHz
	GPIO_Init(GPIOF, &Beep_InitStructure);

	GPIO_ResetBits(GPIOF,GPIO_Pin_0);//初始化的时候把PF(0)置低电平

}

//无源,			#之后需要按照自己的要求改为PWM输出
/*
void Beep_Ring(uint16_t frequence) {
	
}
*/

//有源			#还没写，嘿嘿嘿
void Beep_Ring(void) {

}

//蜂鸣器秒跳动
void Beep_Sec(control _state) {//该函数用【TIM3】时钟实现
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


