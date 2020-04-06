#include "function_wlf.h"

//STM32F407ZET6_info.USART1_Busy
//[0]����1����ռ�ã����Ǳ���λ������ִ��printf
//[7]����1�����ڽ������ݣ�0������
struct _STM32_INFO STM32F407ZET6_info;

/********************************  �������  ***********************************/


OS_EVENT* Message_Input;

_RMT_CMD Remote_CmdStr[22] = {
0,	0,		"    ",
1,	162,	"CH -",			//01
2,	98,		"CH  ",			//02
3,	226,	"CH +",			//03
4,	34,		"|<< ",			//04
5,	2,		">>| ",			//05
6,	194,	"Play",			//06
7,	224,	" -  ",			//07
8,	168,	" +  ",			//08
9,	144,	" EQ ",			//09
10,	104,	" 0  ",			//10
11,	152,	"100+",			//11
12,	176,	"200+",			//12
13,	48,		" 1  ",			//13
14,	24,		" 2  ",			//14
15,	122,	" 3  ",			//15
16,	16,		" 4  ",			//16
17,	56,		" 5  ",			//17
18,	90,		" 6  ",			//18
19,	66,		" 7  ",			//19
20,	74,		" 8  ",			//20
21,	82,		" 9  ",			//21
};
char string_buff[64];//ָ����Ҫ��ӡ���ַ�����ָ��




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

//SD����д����ȡ��д�뵽�ļ���Picture_reference.wlf���ļ���
//pic_fil ָ���Խ��������ļ�ָ��
//write_Structure д������һ���ṹ��ָ�롾_structure_picture_name��
//ȷ���ļ��Ѿ���
void SD_picinfo_write(FIL* pic_fil, u8 index, struct _structure_picture_name* write_Structure) {
	//���ļ���С���Կ�����¼�˼���ͼƬ�ļ�
	u16 offset = index * 64;
	UINT* ByteRead;
	u8 len = 0;
	f_lseek(pic_fil, offset);
	f_write(pic_fil, &(write_Structure->picture_index), 2, ByteRead);
	len = strlen(write_Structure->picture_name);
	if (index != 0)	f_write(pic_fil, &(write_Structure->picture_name), len+1, ByteRead);
	//֮����Ҫ������һȦ������Ϊ�ز���������,
}

//��ȡ�Լ�����ͼƬ����
//pic_fil ָ���Խ��������ļ�ָ��
//index Ҫ��ȡ��������
//ȷ���ļ��Ѿ���
void SD_picinfo_read(FIL* pic_fil, u8 index, struct _structure_picture_name* read_Structure) {
	//���ļ��ĵ�һ��64B����д��ǰ���ڼ�¼�ڼ���ͼƬ��ÿ�ζ�Ҫ��
	u16 offset = index * 64;
	UINT* ByteRead;
	f_lseek(pic_fil, offset);
	f_read(pic_fil, &(read_Structure->picture_index),2 ,ByteRead);
	if (index != 0) f_read(pic_fil, &(read_Structure->picture_name), 62, ByteRead);
}
/*******************************ͼƬ����********************************/

//����д��Ͷ�ȡͼƬ��Ϣ��һ���ṹ��
//��ʼ����ʱ����������һ��
struct _structure_picture_name* pic_reference;
DIR PictureDir;//֮��ÿ�δ�һ���ļ�������Ҫ��������Ծͱ���Ϊʵ����

u8 PictureFile_Init(void) {
	FILINFO picfileinfo;
	u16 totpicnum;
	u8* pname;//��·�����ļ���
	u8 res;
	u16 curindex;//�ļ���ǰ����
	u16 wlf_picnum;//SD���м�¼��ͼƬ�ļ�����
	u8* fn;//���ļ���
	u8 time = 0;
//	u16* picindextbl; //���ڴ��ͼƬ����
	FIL* pic_fil_reference;//�����ָ��SD���б�Ӧ�ñ���ͼƬ��Ϣ��һ���ļ���ָ��
	pic_fil_reference = mymalloc(SRAMIN, sizeof(FIL));
	pic_reference = mymalloc(SRAMIN, 64);//picture_nam�ĵ���ʵ��ռ��64B

	res = f_open(pic_fil_reference, "0:/Picture_reference.wlf", FA_OPEN_EXISTING | FA_WRITE);
	if (res == FR_NO_FILE) {
		printf("û���ҵ��ļ���������һ����������!!!\n");
		POINT_COLOR = RED;
		LCD_ShowString(20, 150, 220, 16, 16, "û���ҵ�ͼƬ�����ļ�,�����С�");
		POINT_COLOR = BLACK;
		f_open(pic_fil_reference, "0:/Picture_reference.wlf", FA_CREATE_NEW | FA_WRITE);
	}
	else {
		printf("�ҵ�ͼƬ�����ļ���\n");
		POINT_COLOR = BLUE;
		LCD_ShowString(20, 150, 220, 16, 16, "�ҵ�ͼƬ�����ļ�");
		POINT_COLOR = BLACK;
	}
	SD_picinfo_read(pic_fil_reference, 0, pic_reference);//��ȡ��0�����У�ֵд��index���ʵ��ͼƬ������
	wlf_picnum = pic_reference->picture_index;
	//��ͼƬ�ļ���
	while (f_opendir(&PictureDir, "0:/PICTURE"))
	{
		LCD_ShowString(20, 230, 200, 16, 16, "PICTURE�ļ��д���!");
		delay_ms(20);
		if (time++ >= 0xFF) return 1;
	}
	totpicnum = pic_get_tnum("0:/PICTURE"); //�õ�����Ч�ļ���
	time = 0;
	//ͼƬ�ļ�Ϊ0
	while (totpicnum == NULL)
	{
		LCD_ShowString(20, 230, 200, 16, 16, "û��ͼƬ�ļ�!");
		delay_ms(20);
		if (time++ >= 0xff) return 2;
	}
	

	picfileinfo.lfsize = _MAX_LFN * 2 + 1;						//���ļ�����󳤶�
	picfileinfo.lfname = mymalloc(SRAMIN, picfileinfo.lfsize);	//Ϊ���ļ������������ڴ�
	pname = mymalloc(SRAMIN, picfileinfo.lfsize);				//Ϊ��·�����ļ��������ڴ�
//	picindextbl = mymalloc(SRAMIN, 2 * totpicnum);				//����2*totpicnum���ֽڵ��ڴ�,���ڴ��ͼƬ����
	time = 0;
	//�ڴ�������
	while (picfileinfo.lfname == NULL || pname == NULL )
	{
		LCD_ShowString(20, 230, 200, 16, 16, "�ڴ����ʧ��!");
		delay_ms(20);
		if (time++ >= 0xff) return 3;
	}
	res = f_opendir(&PictureDir, "0:/PICTURE"); //��Ŀ¼
	if (res == FR_OK)
	{
		curindex = 0;//��ǰ����Ϊ0
	/*************************************************************************/
	//��¼��������һ�μ���SD���е�ͼƬ,��¼ͼƬ����
		while (1)//ȫ����ѯһ��
		{
			res = f_readdir(&PictureDir, &picfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
			if (res != FR_OK || picfileinfo.fname[0] == 0) break;	//������/��ĩβ��,�˳�
			fn = (u8*)(*picfileinfo.lfname ? picfileinfo.lfname : picfileinfo.fname);
			res = f_typetell(fn);//����λ��ʾ��������,����λ��ʾ����С��
			if ((res & 0xF0) == 0x50)//ȡ����λ,�����ǲ���ͼƬ�ļ�
				curindex++;
		}
		/*************************************************************************/
		//�������ļ��м�¼�����������ϣ����±�׫����
		if (curindex != wlf_picnum) 
		{
			printf("��Ҫ����д�������ļ���\n");
			f_closedir(&PictureDir);//�ȹرգ����´�һ��
			wlf_picnum = curindex;
			////////////////////////////////////////////
			curindex = 0;//��ǰ����Ϊ1
			f_opendir(&PictureDir, "0:/PICTURE"); //��Ŀ¼
			while (1)//ȫ����ѯһ��
			{
				res = f_readdir(&PictureDir, &picfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
				if (res != FR_OK || picfileinfo.fname[0] == 0) break;	//������/��ĩβ��,�˳�		  
				fn = (u8*)(*picfileinfo.lfname ? picfileinfo.lfname : picfileinfo.fname);
				res = f_typetell(fn);//����λ��ʾ��������,����λ��ʾ����С��
				if ((res & 0XF0) == 0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
				{
					curindex++;
			/*************************    д��ͼƬ��Ӧ������������       ************************/
					memcpy(pic_reference->picture_name, fn, strlen((const char*)fn) + 1);
					pic_reference->picture_index = PictureDir.index;			//��¼��ǰindex
					SD_picinfo_write(pic_fil_reference, curindex, pic_reference);//������д����Ӧ�������ļ���
				}
			}
			/*******************************************************************/
			pic_reference->picture_index = wlf_picnum;
			SD_picinfo_write(pic_fil_reference, 0, pic_reference);
			myfree(SRAMIN, pic_reference);		//�ͷ��ڴ�
		}
		f_close(pic_fil_reference);
	}
	STM32F407ZET6_info.Picture_totalnum = wlf_picnum;

	myfree(SRAMIN, picfileinfo.lfname);	//�ͷ��ڴ�			    
	myfree(SRAMIN, pname);				//�ͷ��ڴ�			    
//	myfree(SRAMIN, picindextbl);		//�ͷ��ڴ�		
	myfree(SRAMIN, pic_fil_reference);		//�ͷ��ڴ�		
	myfree(SRAMIN, pic_reference);		//�ͷ��ڴ�		
			
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
	CLOCK_pic_change(Hour,Minute);/******************************************************************/
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
		OS_EXIT_CRITICAL();
		if(Second%2) OLED_DrawChar(104, 0, ':', 16, 1);	//23:59
		else OLED_DrawChar(104, 0, ' ', 16, 1);			//23:59
		OS_ENTER_CRITICAL();
		/********************  ��  ******************/
		if (Minute == RTC_TimeStruct.RTC_Minutes) OLED_Refresh();//��û��,ֱ��ˢ��
		else//����
		{
			Minute = RTC_TimeStruct.RTC_Minutes;
			CLOCK_pic_change(Hour,Minute);/******************************************************************/
			sprintf(strtemp, "%02d", Minute);
			OLED_DrawStr_manual(112, 0, strtemp, 16, 1);//23:00
			OSMboxPost(Message_LQ_clock, &Minute);
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
					LQ_period_write(STM32F407ZET6_info.LQ_period++);//д��������
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
		OSTimeDly(65);
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
		LED2 = 1;
		if (RmtSta & 0x80)//�ϴ������ݱ����յ���
		{
			RmtSta &= ~0X10;						//ȡ���������Ѿ���������
			if ((RmtSta & 0X0F) == 0X00)
			{
				RmtSta |= 1 << 6;//����Ѿ����һ�ΰ����ļ�ֵ��Ϣ�ɼ�
				OSMboxPost(Message_Input, (void*)1);//�����ź�
			}
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
		if (RDATA)//�����ز���
		{
			TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Falling);		//CC1P=1 ����Ϊ�½��ز���
			TIM_SetCounter(TIM1, 0);	   	//��ն�ʱ��ֵ
			RmtSta |= 0X10;					//����������Ѿ�������
			LED2 = 0;
		}
		else //�½��ز���
		{
			LED2 = 1;
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
//				printf("%d,%d,%d,%d\n", RmtRec >> 24, (RmtRec >> 16) & 0xFF, (RmtRec >> 8) & 0xFF, (RmtRec) & 0xFF);
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
OS_STK APP_TASK_STK[APP_STK_SIZE];
OS_EVENT* Message_APP_cmd;
//0��ʾδ���ã�1��ʾ����
u8 LQ_clock_state = 1;

void APP_task(void* pdata) {
	pdata = pdata;
	OS_CPU_SR cpu_sr;
//	message_APP_cmd = OSMboxCreate((void*)0); ��start_task�ж�����
	u8 app_cmd_index = 0;
	u8 err;
	_RMT_CMD* cmd;
	//3�����������
	//1����ʱ�ӽ���
	//2�������棬Ҳ����ͼƬ
	u8 Dialog_state;
	while (1) {
		app_cmd_index = *(u8*)OSMboxPend(Message_APP_cmd, 0, &err);
		OS_ENTER_CRITICAL();
		cmd = &Remote_CmdStr[app_cmd_index];
		
		
/*************************************************************************/
		//��CH�� �źţ��޸�ϵͳ������

		switch (app_cmd_index)
		{
		default:
			break;
		case 1:
			Dialog_state = 1;
			break;
		case 2:
			Dialog_state = 2;
			break;
		case 3:
			Dialog_state = 3;
			break;
		case 6:
			//���²��ż�
			if (Dialog_state == 1) //�������LQ_clock����Ļ������л� ʱ�ӵ���ʾ״̬
			{
				LQ_clock_state = !LQ_clock_state;
				Dialog_set(Dialog_state);
			}
			break;
		case 8://+
//			OS_ENTER_CRITICAL();
			STM32F407ZET6_info.LQ_period++;
			OS_EXIT_CRITICAL();
			LQ_period_write(STM32F407ZET6_info.LQ_period);
			break;
		case 7://-
//			OS_ENTER_CRITICAL();
			if (STM32F407ZET6_info.LQ_period != 0) {
				STM32F407ZET6_info.LQ_period--;
				OS_EXIT_CRITICAL();
				LQ_period_write(STM32F407ZET6_info.LQ_period);
			}
			break;
		case 9:
			LCD_Init();
			break;
		case 10:
			STM32F407ZET6_info.LQ_period = 0;
			LQ_period_write(0);
			OS_EXIT_CRITICAL();
			break;

		}
		OS_EXIT_CRITICAL();

		OLED_DrawStr(0, 34, (char*)cmd->name, 24, 1);
//		printf("\nIndex:%d", cmd->index);

//		OS_ENTER_CRITICAL();
		Dialog_set(Dialog_state);
//		OS_EXIT_CRITICAL();
	}
}

void Dialog_set(u8 dialog_state) {
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);

	if (dialog_state == 1) {
		show_picture((u8*)CLOCK_picname,1);//��ʾ��ǰӦ����ʾ��ͼ��
		//���ͬʱ������ʾʱ��״̬���Ϳ�������
		if (LQ_clock_state == 1) {
			OSTaskResume(LQ_CLOCK_TASK_PRIO);
			OSMboxPost(Message_LQ_clock, &LQ_clock_state);
			return;
		}
	}
	else {
		//������Ǵ���ʱ�ӽ��棬���Զ��ر�����
		OSTaskSuspend(LQ_CLOCK_TASK_PRIO);
	}


	if (dialog_state == 2) {
		
		if (RTC_DateStruct.RTC_Month == 5 && (RTC_DateStruct.RTC_Date ==25 || RTC_DateStruct.RTC_Date == 26)) {
			show_picture("0:/PICTURE/���տ���.jpg", 1);//���յ���͵ڶ�����ʾ��ͷ��

		}
		else 
			show_picture("0:/PICTURE/ͷ��.bmp", 1);
	}


	if (dialog_state == 3) {
		lcd_ShowSystemInfo();
	}
}




/**********************           APP �ⲿ����   *************************/

u8 CLOCK_index;
void CLOCK_pic_change(u8 hour,u8 minute) 
{
	u8 n;
	n = hour * 2 + minute / 30;
	//Ĭ����ɫ��λ��
	CLOCK_height = 100;
	CLOCK_color = RGB2u16(153, 0, 144);
	
		CLOCK_index = n;
		sprintf(CLOCK_picname, "0:/SYSTEM_WLF/%02d.jpg", CLOCK_index);

		if (n < 12) {//0-11��0:00-5:59
			CLOCK_height = 30;
			CLOCK_color = BLACK;
		}
		if (n ==12|| n == 13) {
			CLOCK_height = 30;
			CLOCK_color = WHITE;
		}
		if (n == 18 || n == 19 || n == 33|| n == 34)
		{//����Ů����ɢ��������ͼ
			CLOCK_height = 10;
			CLOCK_color = BLACK;
		}
		if (n == 20|| n== 21 || n== 22) {
			CLOCK_height = 15;
			CLOCK_color = WHITE;
		}



		if (n == 37||n==38) {
			CLOCK_height = 100;
			CLOCK_color = RGB2u16(153, 0, 144);
		}
		if (n == 39 ) {//ӵ��������
			CLOCK_height = 20;
			CLOCK_color = YELLOW;
		}
		if (n >=40) {
			CLOCK_height = 20;
			CLOCK_color = RGB2u16(111, 111, 255);
		}
	
	


}

/*************��ʾ LQ_CLOCK ������1�� ��APP��********************/

OS_EVENT* Message_LQ_clock;
OS_STK LQ_CLOCK_TASK_STK[LQ_CLOCK_STK_SIZE];
char CLOCK_picname[30];
u8 CLOCK_height;

u16 CLOCK_color;
void Show_LQ_CLOCK(void* pdata) {
	pdata = pdata;
	OS_CPU_SR cpu_sr;
	INT8U err;
	u8 hour_10, hour_1, minute_10, minute_1;
	u8 res;
	res=res;
	while (1) {
		res=*(u8*)OSMboxPend(Message_LQ_clock, 4000, &err);
		if (LQ_clock_state == 1) {
			//��¼ʱ�䣬��������
			hour_10 = RTC_TimeStruct.RTC_Hours / 10;
			hour_1 = RTC_TimeStruct.RTC_Hours % 10;
			minute_10 = RTC_TimeStruct.RTC_Minutes / 10;
			minute_1 = RTC_TimeStruct.RTC_Minutes % 10;



			show_picture((u8*)CLOCK_picname, 1);
			OS_ENTER_CRITICAL();
			LCD_ShowLQ_CLOCK(105, CLOCK_height, ':', 60);//��ʾð��
			LCD_ShowLQ_CLOCK(45, CLOCK_height, hour_10, 60);
			LCD_ShowLQ_CLOCK(75, CLOCK_height, hour_1, 60);
			LCD_ShowLQ_CLOCK(135, CLOCK_height, minute_10, 60);
			LCD_ShowLQ_CLOCK(165, CLOCK_height, minute_1, 60);
			OS_EXIT_CRITICAL();

		}
		

	}
	
}



/*************��ʾϵͳ��Ϣ������3��********************/
void lcd_ShowSystemInfo(void) {
	/*******************�������********************/
	LCD_Clear(WHITE);
	POINT_COLOR = BLACK;
	LCD_ShowString(0, 2 , 240, 16, 16, "(>��<)����ϵͳ״̬��(��������)");
	POINT_COLOR = BLUE;
	/****************************************************************************/
	LCD_DrawLine(8, 17, 232, 17); LCD_DrawLine(8, 18, 232, 18); //����
	LCD_DrawLine(8, 17, 8, 300); LCD_DrawLine(232, 17, 232, 300);//��������
	POINT_COLOR = BLACK;
	sprintf(string_buff, "<ʱ��> %02d:%02d:%02d <������> %02d",\
				RTC_TimeStruct.RTC_Hours, RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds,\
				STM32F407ZET6_info.LQ_period);
	LCD_ShowString(8, 20 , 230, 16, 16, string_buff);//x:5~53
	
	LCD_Draw_setting(BLACK, WHITE, 128);
	sprintf(string_buff, "<����> 20%02d��%02d��%02d�� ����",\
				RTC_DateStruct.RTC_Year, RTC_DateStruct.RTC_Month, RTC_DateStruct.RTC_Date);
	switch (RTC_DateStruct.RTC_WeekDay)
	{
	case 1:
		strcat(string_buff, "һ");
		break;
	case 2:
		strcat(string_buff, "��");
		break;
	case 3:
		strcat(string_buff, "��");
		break;
	case 4:
		strcat(string_buff, "��");
		break;
	case 5:
		strcat(string_buff, "��");
		break;
	case 6:
		strcat(string_buff, "��");
		break;
	case 7:
		strcat(string_buff, "��");
		break;
	default:
		break;
	}
	LCD_ShowString(8, 40 , 230, 16, 16, string_buff);//д����

	/********************************Ӳ��ģ��************************************/
	LCD_DrawLine(8, 57, 232, 57); LCD_DrawLine(8, 58, 232, 58); 
	LCD_ShowString(8, 60 , 230, 16, 16, "<����> WiFiģ��    [����]��");
	LCD_ShowString(8, 80 , 230, 16, 16, "<����> ң��ģ��    [����]��");
	LCD_ShowString(8, 100, 230, 16, 16, "<��ʾ> OLEDģ��    [����]��");
	LCD_ShowString(8, 120, 230, 16, 16, "<��ʾ> LCD ģ��    [����]��");
	LCD_ShowString(8, 140, 230, 16, 16, "<����> SD��ģ��    [����]��");
	sprintf(string_buff, "����: %04d MB", STM32F407ZET6_info.SD_total/1024);
	LCD_ShowString(8, 160, 230, 16, 16, string_buff);
	sprintf(string_buff, "����: %04d MB", STM32F407ZET6_info.SD_free/1024);
	LCD_ShowString(8, 180, 230, 16, 16, string_buff);
	/********************************���ģ��************************************/
	LCD_DrawLine(8, 197, 232, 197);LCD_DrawLine(8, 198, 232, 198); 
	LCD_ShowString(8, 200, 230, 16, 16, "<����> ��溺��    21,003��");
	LCD_ShowString(8, 220, 230, 16, 16, "<ͼƬ> ���ͼƬ WLF�� 48 ��");
	LCD_Draw_setting(PURPLE, WHITE, 0);
	LCD_ShowString(8, 240, 230, 32, 16, "<����Ա�绰��13671145174> ");
	LCD_Draw_setting(GREEN, WHITE, 0);
	LCD_DrawLine(8, 257, 232, 257); LCD_DrawLine(8, 258, 232, 258);
	LCD_Draw_setting(BLACK, WHITE, 128);
	LCD_ShowString(8, 260, 230, 32, 16, "<����Ȩ������>�� @����� ");
	

//	LCD_Draw_setting(0x0000, 0xFFFF, 64);

}

//��ȡ������ʱ��
//ͬʱͬ����ϵͳ��Ϣ��
u8 LQ_period_read(void) {
	FIL* LQ_f_period;//�����ָ��SD���б�Ӧ�ñ�����ٻ������ʱ����ļ�
	u8 res;
	UINT* br;
	LQ_f_period = mymalloc(SRAMIN, sizeof(FIL));
	res = f_open(LQ_f_period, "0:/LQ_period.wlf", FA_OPEN_EXISTING | FA_READ);
	if (res == FR_NO_FILE) {
		f_open(LQ_f_period, "0:/LQ_period.wlf", FA_CREATE_NEW | FA_READ);
	}
	f_lseek(LQ_f_period, 1);
	f_read(LQ_f_period, &STM32F407ZET6_info.LQ_period, 1,br );

	myfree(SRAMIN,LQ_f_period);
	f_close(LQ_f_period);
	return STM32F407ZET6_info.LQ_period;
}

void LQ_period_write(u8 period) {
	FIL* LQ_f_period;//�����ָ��SD���б�Ӧ�ñ�����ٻ������ʱ����ļ�
	UINT* br;
	LQ_f_period = mymalloc(SRAMIN, sizeof(FIL));
	f_open(LQ_f_period, "0:/LQ_period.wlf", FA_OPEN_EXISTING | FA_WRITE);
	f_lseek(LQ_f_period, 1);
	f_write(LQ_f_period, &period, 1, br);

	myfree(SRAMIN, LQ_f_period);
	f_close(LQ_f_period);
	return ;
}





















