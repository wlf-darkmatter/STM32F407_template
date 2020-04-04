#include "function_wlf.h"

//STM32F407ZET6_info.USART1_Busy
//[0]����1����ռ�ã����Ǳ���λ������ִ��printf
//[7]����1�����ڽ������ݣ�0������
struct _STM32_INFO STM32F407ZET6_info;

/********************************  �������  ***********************************/


OS_EVENT* Message_Input;

_RMT_CMD Remote_CmdStr[22] = {
	0,		"    ",
	162,	"CH -",			//01
	98,		"CH  ",			//02
	226,	"CH +",			//03
	34,		"|<< ",			//04
	2,		">>| ",			//05
	194,	"Play",			//06
	224,	" -  ",			//07
	168,	" +  ",			//08
	144,	" EQ ",			//09
	104,	" 0  ",			//10
	152,	"100+",			//11
	176,	"200+",			//12
	48,		" 1  ",			//13
	24,		" 2  ",			//14
	122,	" 3  ",			//15
	16,		" 4  ",			//16
	56,		" 5  ",			//17
	90,		" 6  ",			//18
	66,		" 7  ",			//19
	74,		" 8  ",			//20
	82,		" 9  ",			//21
};




//�ض���fputc���� 
//���봮�ڶ�ռ�ж�ָ��
/******************************* WiFi ���� ********************************/

OS_STK WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE];
u8 WiFi_State;//[7]λ��1���Ǿ��Ǵ���debug״̬
void WiFi_Debug(void) {
	OSTaskCreate(WiFi_Debug_task,(void*)0,(OS_STK*)&WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE-1],WIFI_DEBUG_TASK_PRIO);
	printf("\n/*********����ESP8266��Debug��״̬���˳������룺-q \n");
}
void WiFi_Debug_task(void* pdata) {
	pdata=pdata;
	OS_CPU_SR cpu_sr ;
	WiFi_State |= 0x80;//����Debug״̬
	u8 len;
	u16 RX1=USART1_RX_STA;
	u16 RX2=USART2_RX_STA;
	u8* push_usart1 = mymalloc(SRAMIN, MAX_FNAME_LEN);
	memcpy(push_usart1, USART1_RX_BUF, MAX_FNAME_LEN);
	//�ر�USMART
#if USE_SMART_APP==0
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE); //ȡ����ʱ��4�����ж�
	TIM_Cmd(TIM4, DISABLE); //�رն�ʱ��4
#elif USE_SMART_APP==1
	OSTaskSuspend(USMART_APP_TASK_PRIO);
#endif // USE_SMART_APP==0
	STM32F407ZET6_info.USART1_Busy |= 0x01;//��ռ���ڣ�����ӡ��������
	USART1_RX_STA = 0;
	USART2_RX_STA = 0;
	//ɨ������
	while (1) {
		delay_ms(100);
		OS_ENTER_CRITICAL();
		
		if (USART1_RX_STA & 0x8000)//����1������ɣ�
		{
			if (USART1_RX_BUF[0] == '-' && USART1_RX_BUF[1] == 'q') {
				USART2_RX_STA=RX2;
				USART1_RX_STA=RX1;
				memcpy(USART1_RX_BUF, push_usart1, MAX_FNAME_LEN);
				myfree(SRAMIN, push_usart1);
				/*********************�˳��������*********************/
				WiFi_State &= ~(0x80);//ȡ��Debug״̬
#if USE_SMART_APP==0
				TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //��ʱ��4�����ж�
				TIM_Cmd(TIM4, ENABLE); //�򿪶�ʱ��4
#elif USE_SMART_APP==1
				OSTaskResume(USMART_APP_TASK_PRIO);
#endif
				STM32F407ZET6_info.USART1_Busy |= ~(0x01);//�ͷŴ��ڶ�ռȨ
				OSTaskDel(OS_PRIO_SELF);
				OS_EXIT_CRITICAL();
				return;
				/*****************************************************/
			}
			len = USART1_RX_STA & 0x3fff;	//�õ��˴ν��յ������ݳ���[13:0]
			USART1_RX_BUF[len] = '\0';	//��ĩβ���������. 
			/********�˳�����*******/
			/*******�����˳�����,�Ǿ��Ƿ��͵�����,******/
			OS_EXIT_CRITICAL();//��ESP8266��ӡ�����Ҫ�ж�
			STM32F407ZET6_info.USART1_Busy &= ~(0x01);
			printf("<----%s\n", USART1_RX_BUF);
			STM32F407ZET6_info.USART1_Busy |= 0x01;
			USART1_RX_STA = 0;//״̬�Ĵ������	    
			usart2_printf("%s\r\n", USART1_RX_BUF);	//���͸�esp8266
		}
		if (USART2_RX_STA & 0x8000) {
			len = USART2_RX_STA & 0x7fff;
			USART2_RX_BUF[len] = '\0';	//��ĩβ���������.
			STM32F407ZET6_info.USART1_Busy &= ~(0x01);
			printf("%s", USART2_RX_BUF);
			STM32F407ZET6_info.USART1_Busy |= 0x01;
			USART2_RX_STA = 0;
		}
		OS_EXIT_CRITICAL();

	}
	
}
/*******************************       SD       ****************************************/
OS_EVENT* message_SD;			//SD����д�����¼���ָ��

/*******************************ͼƬ����********************************/
struct _app_LCD App_LCD;
DIR PictureDir;//֮��ÿ�δ�һ���ļ�������Ҫ��������Ծͱ���Ϊʵ����

u8 PictureFile_Init(void) {
	FILINFO picfileinfo;
	u16 totpicnum;
	u8* pname;//��·�����ļ���
	u8 res;
	u16 curindex;//�ļ���ǰ����
	u16 temp;
	u8* fn;//���ļ���
	u8 time = 0;
	u16* picindextbl; //���ڴ��ͼƬ����

	while (f_opendir(&PictureDir, "0:/PICTURE"))//��ͼƬ�ļ���
	{
		LCD_ShowString(20, 230, 200, 16, 16, "PICTURE�ļ��д���!");
		delay_ms(20);
		if (time++ >= 0xff) return 1;
	}
	totpicnum = pic_get_tnum("0:/PICTURE"); //�õ�����Ч�ļ���
	time = 0;
	while (totpicnum == NULL)//ͼƬ�ļ�Ϊ0		
	{
		LCD_ShowString(20, 230, 200, 16, 16, "û��ͼƬ�ļ�!");
		delay_ms(20);
		if (time++ >= 0xff) return 2;
	}
	picfileinfo.lfsize = _MAX_LFN * 2 + 1;						//���ļ�����󳤶�
	picfileinfo.lfname = mymalloc(SRAMIN, picfileinfo.lfsize);	//Ϊ���ļ������������ڴ�
	pname = mymalloc(SRAMIN, picfileinfo.lfsize);				//Ϊ��·�����ļ��������ڴ�
	picindextbl = mymalloc(SRAMIN, 2 * totpicnum);				//����2*totpicnum���ֽڵ��ڴ�,���ڴ��ͼƬ����
	time = 0;
	while (picfileinfo.lfname == NULL || pname == NULL || picindextbl == NULL)//�ڴ�������
	{
		LCD_ShowString(20, 230, 200, 16, 16, "�ڴ����ʧ��!");
		delay_ms(20);
		if (time++ >= 0xff) return 3;
	}
	//��¼����
	res = f_opendir(&PictureDir, "0:/PICTURE"); //��Ŀ¼
	if (res == FR_OK)
	{
		curindex = 0;//��ǰ����Ϊ0
		while (1)//ȫ����ѯһ��
		{
			temp = PictureDir.index;								//��¼��ǰindex
			res = f_readdir(&PictureDir, &picfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
			if (res != FR_OK || picfileinfo.fname[0] == 0) break;	//������/��ĩβ��,�˳�		  
			fn = (u8*)(*picfileinfo.lfname ? picfileinfo.lfname : picfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				picindextbl[curindex] = temp;//��¼����
				curindex++;
			}
		}
	}
	App_LCD.Picture_num = curindex;
	myfree(SRAMIN, picfileinfo.lfname);	//�ͷ��ڴ�			    
	myfree(SRAMIN, pname);				//�ͷ��ڴ�			    
	myfree(SRAMIN, picindextbl);		//�ͷ��ڴ�		
	return 0;
}

u16 pic_get_tnum(u8* path)
{
	u8 res;
	u16 rval = 0;
	DIR tdir;	 		//��ʱĿ¼
	FILINFO tfileinfo;	//��ʱ�ļ���Ϣ	
	u8* fn;
	res = f_opendir(&tdir, (const TCHAR*)path); 	//��Ŀ¼
	tfileinfo.lfsize = _MAX_LFN * 2 + 1;				//���ļ�����󳤶�
	tfileinfo.lfname = mymalloc(SRAMIN, tfileinfo.lfsize);//Ϊ���ļ������������ڴ�
	if (res == FR_OK && tfileinfo.lfname != NULL)
	{
		while (1)//��ѯ�ܵ���Ч�ļ���
		{
			res = f_readdir(&tdir, &tfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
			if (res != FR_OK || tfileinfo.fname[0] == 0) break;	//������/��ĩβ��,�˳�		  
			fn = (u8*)(*tfileinfo.lfname ? tfileinfo.lfname : tfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				rval++;//��Ч�ļ�������1
			}
		}
	}
	return rval;
}

/*******************************OLED GUI����********************************/
OS_EVENT* message_OLED;
OS_STK OLED_TASK_STK[OLED_STK_SIZE];
OS_EVENT* message_OLED;
void OLED_GUIGRAM_Init(void) {
	//�������岼��
	OLED_Clear();
	OLED_DrawStr_manual(0, 0, "CPU: 00 %", 16, 1);//������
	OLED_DrawStr_manual(80, 0,  "|18:58", 16, 1);//ʱ��
	OLED_DrawStr_manual(80, 16, "|06/14", 16, 1);//ʱ��
	OLED_DrawStr_manual(80, 32, "|@2019", 16, 1);//���
	OLED_DrawStr_manual(0, 22, "User QianQian", 12, 1);
}

void OLED_GUI_update(void* pdata) {
	OS_CPU_SR cpu_sr;
	char strtemp[8];
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
	u8 Year = RTC_DateStruct.RTC_Year, Month = RTC_DateStruct.RTC_Month, Date = RTC_DateStruct.RTC_Date;
	u8 Hour=RTC_TimeStruct.RTC_Hours, Minute= RTC_TimeStruct.RTC_Minutes, Second= RTC_TimeStruct.RTC_Seconds;
	sprintf(strtemp, "%02d", Minute);
	OLED_DrawStr_manual(112, 0, strtemp, 16, 1);//23:00
	sprintf(strtemp, "%02d", Hour);
	OLED_DrawStr_manual(88, 0, strtemp, 16, 1);//00:00
	sprintf(strtemp, "%02d", Date);
	OLED_DrawStr_manual(112, 16, strtemp, 16, 1);//12/01
	sprintf(strtemp, "%02d", Month);
	OLED_DrawStr_manual(88, 16, strtemp, 16, 1);//01/01
	if (Date == 25 && Month == 5) {
		OLED_DrawStr_manual(88, 16, "05/25", 16, 0);//01/01
	}
	if (Date == 13 && Month == 12) {
		OLED_DrawStr_manual(88, 16, "12/13", 16, 0);//01/01
	}
	sprintf(strtemp, "%02d", Year);
	OLED_DrawStr_manual(112, 32, strtemp, 16, 1);//2020
	OLED_Refresh();
	//OLED_GUI_Init();
	while (1) {
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
		RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
		Second = RTC_TimeStruct.RTC_Seconds;
		OS_ENTER_CRITICAL();
		//**************  CPU�����ʲ���  *************
		sprintf(strtemp, "%02d", OSCPUUsage);
		OLED_DrawStr_manual(40, 0, strtemp, 16, 1);//������
		/********************  ��  ******************/
		if(Second%2) OLED_DrawChar(104, 0, ':', 16, 1);	//23:59
		else OLED_DrawChar(104, 0, ' ', 16, 1);			//23:59
		/********************  ��  ******************/
		if (Minute == RTC_TimeStruct.RTC_Minutes) OLED_Refresh();//��û��,ֱ��ˢ��
		else//����
		{
			Minute = RTC_TimeStruct.RTC_Minutes;
			sprintf(strtemp, "%02d", Minute);
			OLED_DrawStr_manual(112, 0, strtemp, 16, 1);//23:00

			if (Hour == RTC_TimeStruct.RTC_Hours) OLED_Refresh();//ʱû��,ֱ��ˢ��
			else//����
			{
				Hour = RTC_TimeStruct.RTC_Hours;
				sprintf(strtemp, "%02d", Hour);
				OLED_DrawStr_manual(88, 0, strtemp, 16, 1);//00:00

				if (Date == RTC_DateStruct.RTC_Date) OLED_Refresh();//��û��,ֱ��ˢ��
				else//����
				{
					Date = RTC_DateStruct.RTC_Date;
					sprintf(strtemp, "%02d", Date);
					OLED_DrawStr_manual(112, 16, strtemp, 16, 1);//12/01

					if (Month == RTC_DateStruct.RTC_Month) OLED_Refresh();//��û��,ֱ��ˢ��
					else//����
					{
						Month = RTC_DateStruct.RTC_Month;
						sprintf(strtemp, "%02d", Month);
						OLED_DrawStr_manual(88, 16, strtemp, 16, 1);//01/01

						if (Year == RTC_DateStruct.RTC_Year) OLED_Refresh();//��û��,ֱ��ˢ��
						else//����
						{
							Year = RTC_DateStruct.RTC_Year;
							sprintf(strtemp, "%02d", Year);
							OLED_DrawStr_manual(112, 32, strtemp, 16, 1);//2020
							OLED_Refresh();
						}
					}
				}
			}
		}

		OS_EXIT_CRITICAL();
		delay_ms(333);
	}
}

/*******************************RTC**********************************/
RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;
void Show_RTC(void) {
	u8 Year = RTC_DateStruct.RTC_Year, Month = RTC_DateStruct.RTC_Month, Date = RTC_DateStruct.RTC_Date;
	u8 Hour = RTC_TimeStruct.RTC_Hours, Minute = RTC_TimeStruct.RTC_Minutes, Second = RTC_TimeStruct.RTC_Seconds;
	printf("20%02d-%02d-%02d\n%02d:%02d:%02d", Year, Month, Date, Hour, Minute, Second);
}

/*******************************  USMART  **********************************/
#if USE_SMART_APP==1
OS_STK USMART_APP_TASK_STK[USMART_APP_STK_SIZE];
void USMART_APP(void* pdata) {
	OS_CPU_SR cpu_sr ;
	while (1) {
		delay_ms(150);
		OS_ENTER_CRITICAL();
		usmart_scan();
		OS_EXIT_CRITICAL();
	}
}

void Debug(void) {
	OS_CPU_SR cpu_sr;
	OS_ENTER_CRITICAL();
	OSTaskSuspend(4);
	OSTaskSuspend(OLED_TASK_PRIO);
	OS_EXIT_CRITICAL();
}

void Exit(void) {
	OSTaskResume(4);
	OSTaskResume(OLED_TASK_PRIO);
}
#endif

/****************  KEY  *****************/

u8 Key_detect(void) {
	
	




	return 0;
}

/**************   REMOTE    ***************/

//����ң�س�ʼ��
//����IO�Լ�TIM2_CH1�����벶��
void Remote_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef  TIM1_ICInitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);//TIM1ʱ��ʹ�� 

	//GPIOA8  ���ù���,����
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1); //GPIOA8����ΪTIM1

	TIM_TimeBaseStructure.TIM_Prescaler = 167;  ////Ԥ��Ƶ��,1M�ļ���Ƶ��,1us��1.	
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period = 10000;   //�趨�������Զ���װֵ ���10ms���  
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	//��ʼ��TIM2���벶�����
	TIM1_ICInitStructure.TIM_Channel = TIM_Channel_1; //CC1S=01 	ѡ������� IC1ӳ�䵽TI1��
	TIM1_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	//�����ز���
	TIM1_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //ӳ�䵽TI1��
	TIM1_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //���������Ƶ,����Ƶ 
	TIM1_ICInitStructure.TIM_ICFilter = 0x03;//IC1F=0003 8����ʱ��ʱ�������˲�
	TIM_ICInit(TIM1, &TIM1_ICInitStructure);//��ʼ����ʱ��2���벶��ͨ��

	TIM_ITConfig(TIM1, TIM_IT_Update | TIM_IT_CC1, ENABLE);//��������ж� ,����CC1IE�����ж�	
	TIM_Cmd(TIM1, ENABLE); 	 	//ʹ�ܶ�ʱ��1

	NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//��ʼ��NVIC�Ĵ���

	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//��ʼ��NVIC�Ĵ���
}

//ң��������״̬
//[7]:�յ����������־
//[6]:�õ���һ��������������Ϣ
//[5]:����	
//[4]:����������Ƿ��Ѿ�������								   
//[3:0]:�����ʱ��
u8 	RmtSta = 0;
u16 Dval;		//�½���ʱ��������ֵ
u32 RmtRec = 0;	//������յ�������	   		    
u8  RmtCnt = 0;	//�������µĴ���	 
//��ʱ��1����ж�
void TIM1_UP_TIM10_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET) //����ж�
	{
		if (RmtSta & 0x80)//�ϴ������ݱ����յ���
		{
			
			RmtSta &= ~0X10;						//ȡ���������Ѿ���������
			if ((RmtSta & 0X0F) == 0X00)
				RmtSta |= 1 << 6;//����Ѿ����һ�ΰ����ļ�ֵ��Ϣ�ɼ�
				OSMboxPost(Message_Input, (void*)1);//�����ź�
			if ((RmtSta & 0X0F) < 7)
				RmtSta++;
			else
			{
				RmtSta &= ~(1 << 7);//���������ʶ
				RmtSta &= 0XF0;	//��ռ�����	
			}
		}
	}
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);  //����жϱ�־λ 
}	

//��ʱ��1���벶���жϷ������	 
void TIM1_CC_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_CC1) == SET) //������(CC1IE)�ж�
	{
		LED2 = 0;
		if (RDATA)//�����ز���
		{
			TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Falling);		//CC1P=1 ����Ϊ�½��ز���
			TIM_SetCounter(TIM1, 0);	   	//��ն�ʱ��ֵ
			RmtSta |= 0X10;					//����������Ѿ�������
		}
		else //�½��ز���
		{
			Dval = TIM_GetCapture1(TIM1);//��ȡCCR1Ҳ������CC1IF��־λ
			TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Rising); //CC1P=0	����Ϊ�����ز���
			if (RmtSta & 0X10)					//���һ�θߵ�ƽ���� 
			{
				if (RmtSta & 0X80)//���յ���������
				{
					if (Dval > 300 && Dval < 800)			//560Ϊ��׼ֵ,560us
					{
						RmtRec <<= 1;	//����һλ.
						RmtRec |= 0;	//���յ�0	   
					}
					else if (Dval > 1400 && Dval < 1800)	//1680Ϊ��׼ֵ,1680us
					{
						RmtRec <<= 1;	//����һλ.
						RmtRec |= 1;	//���յ�1
					}
					else if (Dval > 2200 && Dval < 2600)	//�õ�������ֵ���ӵ���Ϣ 2500Ϊ��׼ֵ2.5ms
					{
						RmtCnt++; 		//������������1��
						RmtSta &= 0XF0;	//��ռ�ʱ��
						
					}
				}
				else if (Dval > 4200 && Dval < 4700)		//4500Ϊ��׼ֵ4.5ms
				{
					RmtSta |= 1 << 7;	//��ǳɹ����յ���������
					RmtCnt = 0;		//�����������������
					
				}
			}
			RmtSta &= ~(1 << 4);
		}
	}
	TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);  //����жϱ�־λ 
}

//����������
//����ֵ:
//	 0,û���κΰ�������
//����,���µİ�����ֵ.
u8 Remote_Scan(void)
{
	u8 sta = 0;
	u8 t1, t2;
	if (RmtSta & (1 << 6))//�õ�һ��������������Ϣ��
	{
		t1 = RmtRec >> 24;			//�õ���ַ��
		t2 = (RmtRec >> 16) & 0xff;	//�õ���ַ���� 
		if ((t1 == (u8)~t2) && t1 == REMOTE_ID)//����ң��ʶ����(ID)����ַ 
		{
			t1 = RmtRec >> 8;
			t2 = RmtRec;
			if (t1 == (u8)~t2)
			{
				printf("%d,%d,%d,%d\n", RmtRec >> 24, (RmtRec >> 16) & 0xFF, (RmtRec >> 8) & 0xFF, (RmtRec) & 0xFF);
				sta = t1;//��ֵ��ȷ	 
				RmtRec = 0;
			}
		}
		if ((sta == 0) || ((RmtSta & 0X80) == 0))//�������ݴ���/ң���Ѿ�û�а�����
		{
			RmtSta &= ~(1 << 6);//������յ���Ч������ʶ
			RmtCnt = 0;		//�����������������
		}
	}
	LED2 = 1;
	return sta;
}


/**********************           APP ������   *************************/
void APP_task(void* pdata) {
	return;
}


/**********************           APP �ⲿ����   *************************/
OS_STK APP_TASK_STK[APP_STK_SIZE];

//��ʾϵͳ��Ϣ����
void lcd_ShowSystemInfo(void) {
	/*******************�������********************/
	LCD_Clear(WHITE);
	POINT_COLOR = BLACK;
	LCD_ShowString(0, 0 , 240, 16, 16, "(>��<)����ϵͳ״̬��(��������)");
	POINT_COLOR = BLUE;
	/****************************************************************************/
	LCD_DrawLine(0, 17, 240, 16); LCD_DrawLine(0, 18, 240, 16); LCD_DrawLine(0, 19, 240, 16);
	POINT_COLOR = BLACK;
	LCD_ShowString(8, 20 , 230, 16, 16, "<ʱ��> 19:10:00 <������>�ٺ�");//x:5~53
	LCD_Draw_setting(BLACK, WHITE, 128);
	LCD_ShowString(8, 40 , 230, 16, 16, "<����>2020��04��04��������");
	/********************************Ӳ��ģ��************************************/
	LCD_DrawLine(0, 58, 240, 16); LCD_DrawLine(0, 59, 240, 16);
	LCD_ShowString(8, 60 , 230, 16, 16, "<����> WiFiģ�� [�쳣]��");
	LCD_ShowString(8, 80 , 230, 16, 16, "<����> ң��ģ�� [�쳣]��");
	LCD_ShowString(8, 100, 230, 16, 16, "<��ʾ> OLEDģ�� [�쳣]��");
	LCD_ShowString(8, 120, 230, 16, 16, "<��ʾ> LCD ģ�� [�쳣]��");
	LCD_ShowString(8, 140, 230, 16, 16, "<����> SD��ģ�� [�쳣]��");
	LCD_ShowString(8, 160, 230, 16, 16, "����: 0000 MB ����: 0000 MB");
	/********************************���ģ��************************************/
	LCD_DrawLine(0, 168, 240, 16); LCD_DrawLine(0, 169, 240, 16);
	LCD_ShowString(8, 180, 230, 16, 16, "<����> ��溺�� 21,003��");
	LCD_ShowString(8, 200, 230, 16, 16, "<ͼƬ> ���ͼƬ    23 ��");

	LCD_ShowString(8, 220, 230, 16, 16, "<��ֵ> ");
	LCD_ShowString(8, 240, 230, 16, 16, "�����������޷�ʶ��");


	LCD_ShowString(8, 260, 230, 16, 16, "<�ϴβ鿴ʱ��>��");
	LCD_ShowString(8, 280, 230, 16, 16, "2020�� 4�� 4�� 19:48");
	LCD_Draw_setting(BLACK, WHITE, 128);
	LCD_ShowString(8, 300, 230, 16, 16, "�㶼������������ �i�n�i...");

	

//	LCD_Draw_setting(0x0000, 0xFFFF, 64);

}

























