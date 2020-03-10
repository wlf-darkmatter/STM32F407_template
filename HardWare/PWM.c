#include <PWM.h>
//通过使用【TIM14】来实现【PWM】的输出
//引脚为【PA7】

void PWM_TIM14_Init(u32 Period, u16 Prescaler) {
	//①开启【TIM14】的时钟和【GPIO】的引脚复用重定向
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 	//使能PORTA时钟	!!!一定要记得！！
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_TIM14);

	//②配置GPIO，制定GPIOA7的引脚复用功能
	GPIO_InitTypeDef PWM_PA7_InitStructure;
	PWM_PA7_InitStructure.GPIO_Pin = GPIO_Pin_7;
	PWM_PA7_InitStructure.GPIO_Mode = GPIO_Mode_AF;			//复用功能
	PWM_PA7_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	PWM_PA7_InitStructure.GPIO_OType = GPIO_OType_PP;		//推挽复用输出
	PWM_PA7_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;			//上拉
	GPIO_Init(GPIOA, &PWM_PA7_InitStructure);				//初始化PA7

	//③配置TIM14的一些基本配置
	TIM_TimeBaseInitTypeDef  PWM_TIM14_InitStructure;
	PWM_TIM14_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	PWM_TIM14_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数
	PWM_TIM14_InitStructure.TIM_Period = Period;
	PWM_TIM14_InitStructure.TIM_Prescaler = Prescaler;
	TIM_TimeBaseInit(TIM14, &PWM_TIM14_InitStructure); //初始化定时器14


	//④配置TIM14的通道一中的【PWM】设置
	TIM_OCInitTypeDef PWM_Mode_InitStructure;
//	PWM_Mode_InitStructure.TIM_OCIdleState;
	PWM_Mode_InitStructure.TIM_OCMode = TIM_OCMode_PWM1;		//选择PWM模式1
//	PWM_Mode_InitStructure.TIM_OCNIdleState;
//	PWM_Mode_InitStructure.TIM_OCNPolarity;
	PWM_Mode_InitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
//	PWM_Mode_InitStructure.TIM_OutputNState;
	PWM_Mode_InitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	PWM_Mode_InitStructure.TIM_Pulse;
	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);	//使能TIM14在CCR1上的预装载寄存器
	TIM_OC1Init(TIM14, &PWM_Mode_InitStructure);

	//⑤使能【PWM】及【TIM14】

	TIM_ARRPreloadConfig(TIM14, ENABLE);				//ARPE使能
	TIM_Cmd(TIM14, ENABLE);								//从这一刻开始，TIM14已经开始输出了

}
	//⑥修改【TIM14_CCR1】控制PWM的占空比
//使用函数	TIM_SetCompare1(TIM14,VAL)


