#include "function_wlf.h"

u8 USART1_Busy;//1�����ռ
//�ض���fputc���� 
//���봮�ڶ�ռ�ж�ָ��
#define WIFI_DEBUG_TASK_PRIO				2
#define WIFI_DEBUG_STK_SIZE					128
OS_STK WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE];
/******************************* WiFi ���� ********************************/
u8 WiFi_State;//[7]λ��1���Ǿ��Ǵ���debug״̬
void WiFi_Debug(void) {
	OSTaskCreate(WiFi_Debug_task,(void*)0,(OS_STK*)&WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE-1],WIFI_DEBUG_TASK_PRIO);
	printf("\n/*********����ESP8266��Debug��״̬���˳������룺-q \n");
}
void WiFi_Debug_task(void* pdata) {
	pdata=pdata;
	OS_CPU_SR cpu_sr = 0;
	WiFi_State |= 0x80;//����Debug״̬
	u8 len;
	u16 RX1=USART1_RX_STA;
	u16 RX2=USART2_RX_STA;
	u8* push_usart1 = mymalloc(SRAMIN, MAX_FNAME_LEN);
	memcpy(push_usart1, USART1_RX_BUF, MAX_FNAME_LEN);
	//�ر�USMART
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE); //ȡ����ʱ��4�����ж�
	TIM_Cmd(TIM4, DISABLE); //�رն�ʱ��4
//	printf("\n/*********����ESP8266��Debug��״̬���˳������룺-q \n");
	USART1_Busy = 1;//��ռ���ڣ�����ӡ��������
	USART1_RX_STA = 0;
	USART2_RX_STA = 0;
	//ɨ������
	while (1) {
		delay_ms(300);
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
				TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //��ʱ��4�����ж�
				TIM_Cmd(TIM4, ENABLE); //�򿪶�ʱ��4
				OS_EXIT_CRITICAL();
				USART1_Busy = 0;//�ͷŴ��ڶ�ռȨ
				OSTaskDel(OS_PRIO_SELF);
				return;
				/*****************************************************/
			}
			len = USART1_RX_STA & 0x3fff;	//�õ��˴ν��յ������ݳ���[13:0]
			USART1_RX_BUF[len] = '\0';	//��ĩβ���������. 
			/********�˳�����*******/
			/*******�����˳�����,�Ǿ��Ƿ��͵�����,******/
			OS_EXIT_CRITICAL(); ;//��ESP8266��ӡ�����Ҫ�ж�
			USART1_Busy = 0;
			printf("<----%s\n", USART1_RX_BUF);
			USART1_Busy = 1;
			USART1_RX_STA = 0;//״̬�Ĵ������	    
			usart2_printf("%s\r\n", USART1_RX_BUF);	//���͸�esp8266
		}
		if (USART2_RX_STA & 0x8000) {
			len = USART2_RX_STA & 0x7fff;
			USART2_RX_BUF[len] = '\0';	//��ĩβ���������.
			USART1_Busy = 0;
			printf("%s", USART2_RX_BUF);
			USART1_Busy = 1;
			USART2_RX_STA = 0;
		}
		OS_EXIT_CRITICAL();

	}
	
}


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
void OLED_GUIGRAM_Init(void) {
	//�������岼��
	OLED_Clear();
	OLED_DrawStr_manual(0, 0, "CPU: 00 %", 16, 1);//������
	OLED_DrawStr_manual(80, 0, "|18:58", 16, 1);//ʱ��
	OLED_DrawStr_manual(80, 16, "|06/14", 16, 1);//ʱ��
	OLED_DrawStr_manual(80, 32, "|20_19", 16, 1);//���
	OLED_DrawStr_manual(0, 22, "User QianQian", 12, 1);
}

void OLED_GUI_update(void* pdata) {
	OS_CPU_SR cpu_sr = 0;
	char strtemp[8];
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
	sprintf(strtemp, "%02d", Year);
	OLED_DrawStr_manual(112, 32, strtemp, 16, 1);//2020

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

