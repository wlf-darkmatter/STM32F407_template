#include <Tim.h>
#include <delay.h>
#include <usart.h>

//TIM3��ʱ���жϵĳ�ʼ������
//��Period����������
//��Prescaler���Ƿ�Ƶϵ��
//��Ft��=��ʱ������Ƶ��,��λ:(MHz)
//��ʱ�����ʱ����㷽��:Tout=((Period+1)*(Prescaler+1))/Ft ��us��,�������ʱ�䣨us��
void TIM3_INT_Init(u32 Period,u16 Prescaler) {

//��ʹ�ܡ�TIM3����ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
//�ڳ�ʼ��TIM3
	TIM_TimeBaseInitTypeDef Timer_InitStructure;
	//����������
	Timer_InitStructure.TIM_Period = Period;
	//���÷�Ƶϵ��
	Timer_InitStructure.TIM_Prescaler = Prescaler;
	//����ʱ�ӷ�Ƶ����
	Timer_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	//���ü�ʱģʽ
	Timer_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
//д���ʼ������
	TIM_TimeBaseInit(TIM3,&Timer_InitStructure);

//�����á�TIM3�����������ж�
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

//������TIM3�������ж����ȼ�������NVIC��
	NVIC_InitTypeDef NVIC_TIM3;
	NVIC_TIM3.NVIC_IRQChannel = TIM3_IRQn; //��ʱ��3�ж�
	NVIC_TIM3.NVIC_IRQChannelPreemptionPriority = 0x03; //��ռ���ȼ�3
	NVIC_TIM3.NVIC_IRQChannelSubPriority = 0x03; //�����ȼ�3
	NVIC_TIM3.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_TIM3);

//��ʹ��TIM3
	TIM_Cmd(TIM3, ENABLE);
}



//����APB1�µĶ�ʱ�������ʱ�䣨�������̫���ˣ�
//��Period����������
//��Prescaler���Ƿ�Ƶϵ��
//��Ft��=��ʱ������Ƶ��,��λ:(MHz)
//��ʱ�����ʱ����㷽��:Tout=((Period+1)*(Prescaler+1))/Ft ��us��,
//�������ʱ�䣨us��
u32 Get_TIMx_OutputTime(TIM_TypeDef* TIMx) {
	u32 Ft = 0; //����λ:MHz��
	uint32_t SysClk = 0/*168000000*/, HClk = 0/*168000000*/,PClk1 = 0/*168000000 / 4*/, PClk2 = 0 /*168000000 / 2*/;
	uint32_t pll_vco = 0, pll_p = 2, pll_m = 8, pll_n = 336, AHB_Presc = 1, APB1_Presc = 4, APB2_Presc = 2;
	uint32_t PLL_source = 8000000;
	uint32_t temp = 0;
	uint32_t TIMx_Period = 0;
	uint16_t TIMx_Prescaler = 0;
	//��֪ʹ�õ�ϵͳʱ�ӡ�SysClk������λHz��
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
			
			//PLLCFGR[22]λ����֪��PLL������Դ����0����HSI����Ϊ������Դ��	1����HSE����Ϊ������Դ
			if (((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22) != 0) {
				PLL_source = HSE_VALUE; }/* HSE used */
			else{
				PLL_source = HSI_VALUE; }/* HSI used */

			//PLLCFGR[5:0]λΪ��PLL_M����PLLCFGR[14:6]λΪ��PLL_N����PLLCFGR[17:16]λΪ��PLL_P��
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
	//��֪��AHB_Presc��
	//ȡCFGR[7:4]��AHBԤ��Ƶ�����Ĵ���,����ֱ�Ӿ�������λ�ã�û����λ
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

	//��֪��APB1_Presc����APB����Ԥ��Ƶ����
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
	//��֪��APB2_Presc����APB����Ԥ��Ƶ����
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
		Ft = PClk1 / 1000000;// ����λ:MHz��
	else
		Ft = 2 * PClk1 / 1000000;// ����λ:MHz��

	//��ȡ��֪����TIMx�ġ����������͡���Ƶϵ����
	/* 
	TIMx->ARR = TIM_TimeBaseInitStruct->TIM_Period;	//Set the Autoreload value 
	TIMx->PSC = TIM_TimeBaseInitStruct->TIM_Prescaler; //Set the Prescaler value 
	*/
	//ץȡ��Ӧʱ�ӵ�����
	TIMx_Period=TIMx->ARR;//u32_t TIM auto-reload register
	TIMx_Prescaler = TIMx->PSC;//u16_t TIM prescaler

	temp = ((TIMx_Period + 1) * (TIMx_Prescaler + 1)) / Ft;
	return temp;
}


int Tout = 0;
int wait = 0;
//�ޱ�д�жϷ�����
//��ʱ��3�жϷ�����
void TIM3_IRQHandler(void){
	

	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) //����ж�
	{
		PFout(0) = !PFout(0);	/*����ִ�е�����*/
		if (wait == 5) {
			printf("����������\r\n");	//��ӡOK��ʾ��������
//			delay_ms(5000);	  //ÿ��1s��ӡһ��
			Tout = (int)Get_TIMx_OutputTime(TIM3);
			printf("%d\n", Tout);//�����Ĵ�ӡ�ַ�����ʽ��֪��Ϊʲô���ر�ĵ磬���µ�ѹ��һ�����ݵ�ʱ����ͻȻ����
			wait = 0;
		}
		else wait++;
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //�����ʱ�Ӹ����жϡ���־λ
	}
	
}
