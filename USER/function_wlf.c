#include "function_wlf.h"

u8 USART1_Busy;//1代表独占
//重定义fputc函数 
//加入串口独占判断指令
#define WIFI_DEBUG_TASK_PRIO				2
#define WIFI_DEBUG_STK_SIZE					128
OS_STK WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE];
/******************************* WiFi 部分 ********************************/
u8 WiFi_State;//[7]位是1，那就是处于debug状态
void WiFi_Debug(void) {
	OSTaskCreate(WiFi_Debug_task,(void*)0,(OS_STK*)&WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE-1],WIFI_DEBUG_TASK_PRIO);
	printf("\n/*********进入ESP8266【Debug】状态，退出请输入：-q \n");
}
void WiFi_Debug_task(void* pdata) {
	pdata=pdata;
	OS_CPU_SR cpu_sr = 0;
	WiFi_State |= 0x80;//设置Debug状态
	u8 len;
	u16 RX1=USART1_RX_STA;
	u16 RX2=USART2_RX_STA;
	u8* push_usart1 = mymalloc(SRAMIN, MAX_FNAME_LEN);
	memcpy(push_usart1, USART1_RX_BUF, MAX_FNAME_LEN);
	//关闭USMART
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE); //取消定时器4更新中断
	TIM_Cmd(TIM4, DISABLE); //关闭定时器4
//	printf("\n/*********进入ESP8266【Debug】状态，退出请输入：-q \n");
	USART1_Busy = 1;//独占串口，不打印其他数据
	USART1_RX_STA = 0;
	USART2_RX_STA = 0;
	//扫描输入
	while (1) {
		delay_ms(300);
		OS_ENTER_CRITICAL();
		if (USART1_RX_STA & 0x8000)//串口1接收完成？
		{
			if (USART1_RX_BUF[0] == '-' && USART1_RX_BUF[1] == 'q') {
				USART2_RX_STA=RX2;
				USART1_RX_STA=RX1;
				memcpy(USART1_RX_BUF, push_usart1, MAX_FNAME_LEN);
				myfree(SRAMIN, push_usart1);
				/*********************退出程序代码*********************/
				WiFi_State &= ~(0x80);//取消Debug状态
				TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //定时器4更新中断
				TIM_Cmd(TIM4, ENABLE); //打开定时器4
				OS_EXIT_CRITICAL();
				USART1_Busy = 0;//释放串口独占权
				OSTaskDel(OS_PRIO_SELF);
				return;
				/*****************************************************/
			}
			len = USART1_RX_STA & 0x3fff;	//得到此次接收到的数据长度[13:0]
			USART1_RX_BUF[len] = '\0';	//在末尾加入结束符. 
			/********退出条件*******/
			/*******不是退出命令,那就是发送的命令,******/
			OS_EXIT_CRITICAL(); ;//向ESP8266打印这个需要中断
			USART1_Busy = 0;
			printf("<----%s\n", USART1_RX_BUF);
			USART1_Busy = 1;
			USART1_RX_STA = 0;//状态寄存器清空	    
			usart2_printf("%s\r\n", USART1_RX_BUF);	//发送给esp8266
		}
		if (USART2_RX_STA & 0x8000) {
			len = USART2_RX_STA & 0x7fff;
			USART2_RX_BUF[len] = '\0';	//在末尾加入结束符.
			USART1_Busy = 0;
			printf("%s", USART2_RX_BUF);
			USART1_Busy = 1;
			USART2_RX_STA = 0;
		}
		OS_EXIT_CRITICAL();

	}
	
}


/*******************************图片部分********************************/
struct _app_LCD App_LCD;
DIR PictureDir;//之后每次打开一个文件，都需要这个，所以就保存为实体了

u8 PictureFile_Init(void) {
	FILINFO picfileinfo;
	u16 totpicnum;
	u8* pname;//带路径的文件名
	u8 res;
	u16 curindex;//文件当前索引
	u16 temp;
	u8* fn;//长文件名
	u8 time = 0;
	u16* picindextbl; //用于存放图片索引

	while (f_opendir(&PictureDir, "0:/PICTURE"))//打开图片文件夹
	{
		LCD_ShowString(20, 230, 200, 16, 16, "PICTURE文件夹错误!");
		delay_ms(20);
		if (time++ >= 0xff) return 1;
	}
	totpicnum = pic_get_tnum("0:/PICTURE"); //得到总有效文件数
	time = 0;
	while (totpicnum == NULL)//图片文件为0		
	{
		LCD_ShowString(20, 230, 200, 16, 16, "没有图片文件!");
		delay_ms(20);
		if (time++ >= 0xff) return 2;
	}
	picfileinfo.lfsize = _MAX_LFN * 2 + 1;						//长文件名最大长度
	picfileinfo.lfname = mymalloc(SRAMIN, picfileinfo.lfsize);	//为长文件缓存区分配内存
	pname = mymalloc(SRAMIN, picfileinfo.lfsize);				//为带路径的文件名分配内存
	picindextbl = mymalloc(SRAMIN, 2 * totpicnum);				//申请2*totpicnum个字节的内存,用于存放图片索引
	time = 0;
	while (picfileinfo.lfname == NULL || pname == NULL || picindextbl == NULL)//内存分配出错
	{
		LCD_ShowString(20, 230, 200, 16, 16, "内存分配失败!");
		delay_ms(20);
		if (time++ >= 0xff) return 3;
	}
	//记录索引
	res = f_opendir(&PictureDir, "0:/PICTURE"); //打开目录
	if (res == FR_OK)
	{
		curindex = 0;//当前索引为0
		while (1)//全部查询一遍
		{
			temp = PictureDir.index;								//记录当前index
			res = f_readdir(&PictureDir, &picfileinfo);       		//读取目录下的一个文件
			if (res != FR_OK || picfileinfo.fname[0] == 0) break;	//错误了/到末尾了,退出		  
			fn = (u8*)(*picfileinfo.lfname ? picfileinfo.lfname : picfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//取高四位,看看是不是图片文件	
			{
				picindextbl[curindex] = temp;//记录索引
				curindex++;
			}
		}
	}
	App_LCD.Picture_num = curindex;
	myfree(SRAMIN, picfileinfo.lfname);	//释放内存			    
	myfree(SRAMIN, pname);				//释放内存			    
	myfree(SRAMIN, picindextbl);		//释放内存		
	return 0;
}

u16 pic_get_tnum(u8* path)
{
	u8 res;
	u16 rval = 0;
	DIR tdir;	 		//临时目录
	FILINFO tfileinfo;	//临时文件信息	
	u8* fn;
	res = f_opendir(&tdir, (const TCHAR*)path); 	//打开目录
	tfileinfo.lfsize = _MAX_LFN * 2 + 1;				//长文件名最大长度
	tfileinfo.lfname = mymalloc(SRAMIN, tfileinfo.lfsize);//为长文件缓存区分配内存
	if (res == FR_OK && tfileinfo.lfname != NULL)
	{
		while (1)//查询总的有效文件数
		{
			res = f_readdir(&tdir, &tfileinfo);       		//读取目录下的一个文件
			if (res != FR_OK || tfileinfo.fname[0] == 0) break;	//错误了/到末尾了,退出		  
			fn = (u8*)(*tfileinfo.lfname ? tfileinfo.lfname : tfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//取高四位,看看是不是图片文件	
			{
				rval++;//有效文件数增加1
			}
		}
	}
	return rval;
}

/*******************************OLED GUI部分********************************/
void OLED_GUIGRAM_Init(void) {
	//安放整体布局
	OLED_Clear();
	OLED_DrawStr_manual(0, 0, "CPU: 00 %", 16, 1);//利用率
	OLED_DrawStr_manual(80, 0, "|18:58", 16, 1);//时间
	OLED_DrawStr_manual(80, 16, "|06/14", 16, 1);//时期
	OLED_DrawStr_manual(80, 32, "|20_19", 16, 1);//年份
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
		//**************  CPU利用率部分  *************
		sprintf(strtemp, "%02d", OSCPUUsage);
		OLED_DrawStr_manual(40, 0, strtemp, 16, 1);//利用率
		/********************  秒  ******************/
		if(Second%2) OLED_DrawChar(104, 0, ':', 16, 1);	//23:59
		else OLED_DrawChar(104, 0, ' ', 16, 1);			//23:59
		/********************  分  ******************/
		if (Minute == RTC_TimeStruct.RTC_Minutes) OLED_Refresh();//分没变,直接刷新
		else//变了
		{
			Minute = RTC_TimeStruct.RTC_Minutes;
			sprintf(strtemp, "%02d", Minute);
			OLED_DrawStr_manual(112, 0, strtemp, 16, 1);//23:00

			if (Hour == RTC_TimeStruct.RTC_Hours) OLED_Refresh();//时没变,直接刷新
			else//变了
			{
				Hour = RTC_TimeStruct.RTC_Hours;
				sprintf(strtemp, "%02d", Hour);
				OLED_DrawStr_manual(88, 0, strtemp, 16, 1);//00:00

				if (Date == RTC_DateStruct.RTC_Date) OLED_Refresh();//日没变,直接刷新
				else//变了
				{
					Date = RTC_DateStruct.RTC_Date;
					sprintf(strtemp, "%02d", Date);
					OLED_DrawStr_manual(112, 16, strtemp, 16, 1);//12/01

					if (Month == RTC_DateStruct.RTC_Month) OLED_Refresh();//月没变,直接刷新
					else//变了
					{
						Month = RTC_DateStruct.RTC_Month;
						sprintf(strtemp, "%02d", Month);
						OLED_DrawStr_manual(88, 16, strtemp, 16, 1);//01/01

						if (Year == RTC_DateStruct.RTC_Year) OLED_Refresh();//年没变,直接刷新
						else//变了
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

