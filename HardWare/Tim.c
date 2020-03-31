#include <Tim.h>
#include <delay.h>
#include <usart.h>

//TIM3定时器中断的初始化函数
//【Period】是周期数
//【Prescaler】是分频系数
//【Ft】=定时器工作频率,单位:(MHz)
//定时器溢出时间计算方法:Tout=((Period+1)*(Prescaler+1))/Ft （us）,返回溢出时间（us）
void TIM3_INT_Init(u32 Period,u16 Prescaler) {

//①使能【TIM3】的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
//②初始化TIM3
	TIM_TimeBaseInitTypeDef Timer_InitStructure;
	//设置周期数
	Timer_InitStructure.TIM_Period = Period;
	//设置分频系数
	Timer_InitStructure.TIM_Prescaler = Prescaler;
	//设置时钟分频因子
	Timer_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	//设置计时模式
	Timer_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
//写入初始化配置
	TIM_TimeBaseInit(TIM3,&Timer_InitStructure);

//③设置【TIM3】产生更新中断
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

//④设置TIM3产生的中断优先级——【NVIC】
	NVIC_InitTypeDef NVIC_TIM3;
	NVIC_TIM3.NVIC_IRQChannel = TIM3_IRQn; //定时器3中断
	NVIC_TIM3.NVIC_IRQChannelPreemptionPriority = 0x03; //抢占优先级3
	NVIC_TIM3.NVIC_IRQChannelSubPriority = 0x03; //子优先级3
	NVIC_TIM3.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_TIM3);

//⑤使能TIM3
	TIM_Cmd(TIM3, ENABLE);
}



//计算APB1下的定时器的溢出时间（我真的是太闲了）
//【Period】是周期数
//【Prescaler】是分频系数
//【Ft】=定时器工作频率,单位:(MHz)
//定时器溢出时间计算方法:Tout=((Period+1)*(Prescaler+1))/Ft （us）,
//返回溢出时间（us）
u32 Get_TIMx_OutputTime(TIM_TypeDef* TIMx) {
	u32 Ft = 0; //（单位:MHz）
	uint32_t SysClk = 0/*168000000*/, HClk = 0/*168000000*/,PClk1 = 0/*168000000 / 4*/, PClk2 = 0 /*168000000 / 2*/;
	uint32_t pll_vco = 0, pll_p = 2, pll_m = 8, pll_n = 336, AHB_Presc = 1, APB1_Presc = 4, APB2_Presc = 2;
	uint32_t PLL_source = 8000000;
	uint32_t temp = 0;
	uint32_t TIMx_Period = 0;
	uint16_t TIMx_Prescaler = 0;
	//获知使用的系统时钟【SysClk】（单位Hz）
	switch (RCC->CFGR & RCC_CFGR_SWS) {
		case 0x00:  /* HSI used as system clock source */
			SysClk = HSI_VALUE;
			break;
		case 0x04:  /* HSE used as system clock source */
			SysClk = HSE_VALUE;
			break;
		case 0x08:  /* PLL used as system clock source */
		/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
		SYSCLK = PLL_VCO / PLL_P*/	 
			
			//PLLCFGR[22]位，获知【PLL】输入源——0：【HSI】作为其输入源；	1：【HSE】作为其输入源
			if (((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22) != 0) {
				PLL_source = HSE_VALUE; }/* HSE used */
			else{
				PLL_source = HSI_VALUE; }/* HSI used */

			//PLLCFGR[5:0]位为【PLL_M】，PLLCFGR[14:6]位为【PLL_N】，PLLCFGR[17:16]位为【PLL_P】
			pll_m = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
			pll_p = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1) * 2;
			pll_n = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6;
			pll_vco= (PLL_source / pll_m) * pll_n;
	
			SysClk = pll_vco / pll_p;
			break;

		default:
			SysClk = HSI_VALUE;
			break;
	}
	//获知【AHB_Presc】
	//取CFGR[7:4]【AHB预分频器】寄存器,这里直接就是掩码位置，没有移位
	switch (RCC->CFGR & RCC_CFGR_HPRE)
	{
	case RCC_CFGR_HPRE_DIV2:
		AHB_Presc = 2;
		break;
	case RCC_CFGR_HPRE_DIV4:
		AHB_Presc = 4;
		break;
	case RCC_CFGR_HPRE_DIV8:
		AHB_Presc = 8;
		break;
	case RCC_CFGR_HPRE_DIV16:
		AHB_Presc = 16;
		break;
	case RCC_CFGR_HPRE_DIV64:
		AHB_Presc = 64;
		break;
	case RCC_CFGR_HPRE_DIV128:
		AHB_Presc = 128;
		break;
	case RCC_CFGR_HPRE_DIV256:
		AHB_Presc = 256;
		break;
	case RCC_CFGR_HPRE_DIV512:
		AHB_Presc = 512;
		break;

	default:
		AHB_Presc = 1;
		break;
	}
	HClk = SysClk / AHB_Presc;

	//获知【APB1_Presc】（APB低速预分频器）
	switch (RCC->CFGR & RCC_CFGR_PPRE1)
	{
	case RCC_CFGR_PPRE1_DIV2:
		APB1_Presc = 2;
		break;
	case RCC_CFGR_PPRE1_DIV4:
		APB1_Presc = 4;
		break;
	case RCC_CFGR_PPRE1_DIV8:
		APB1_Presc = 8;
		break;
	case RCC_CFGR_PPRE1_DIV16:
		APB1_Presc = 16;
		break;
	default:
		APB1_Presc = 1;
		break;
	}
	//获知【APB2_Presc】（APB高速预分频器）
	switch (RCC->CFGR & RCC_CFGR_PPRE2)
	{
	case RCC_CFGR_PPRE2_DIV2:
		APB2_Presc = 2;
		break;
	case RCC_CFGR_PPRE2_DIV4:
		APB2_Presc = 4;
		break;
	case RCC_CFGR_PPRE2_DIV8:
		APB2_Presc = 8;
		break;
	case RCC_CFGR_PPRE2_DIV16:
		APB2_Presc = 16;
		break;
	default:
		APB2_Presc = 1;
		break;
	}
	PClk1 = HClk / APB1_Presc;
	PClk2 = HClk / APB2_Presc;

	if (APB1_Presc == 1)
		Ft = PClk1 / 1000000;// （单位:MHz）
	else
		Ft = 2 * PClk1 / 1000000;// （单位:MHz）

	//获取想知道的TIMx的【周期数】和【分频系数】
	/* 
	TIMx->ARR = TIM_TimeBaseInitStruct->TIM_Period;	//Set the Autoreload value 
	TIMx->PSC = TIM_TimeBaseInitStruct->TIM_Prescaler; //Set the Prescaler value 
	*/
	//抓取对应时钟的数据
	TIMx_Period=TIMx->ARR;//u32_t TIM auto-reload register
	TIMx_Prescaler = TIMx->PSC;//u16_t TIM prescaler

	temp = ((TIMx_Period + 1) * (TIMx_Prescaler + 1)) / Ft;
	return temp;
}


int Tout = 0;
int wait = 0;
//⑥编写中断服务函数
//定时器3中断服务函数
void TIM3_IRQHandler(void){
	

	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) //溢出中断
	{
		PFout(0) = !PFout(0);	/*程序执行的内容*/
		if (wait == 5) {
			printf("程序在运行\r\n");	//打印OK提示程序运行
//			delay_ms(5000);	  //每隔1s打印一次
			Tout = (int)Get_TIMx_OutputTime(TIM3);
			printf("%d\n", Tout);//这样的打印字符的形式不知道为什么会特别耗电，导致电压在一个短暂的时间里突然降低
			wait = 0;
		}
		else wait++;
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除【时钟更新中断】标志位
	}
	
}
