#include "function_wlf.h"

//STM32F407ZET6_info.USART1_Busy
//[0]——1，被占用，除非被复位，否则不执行printf
//[7]——1，正在接受数据；0，空闲
struct _STM32_INFO STM32F407ZET6_info;

/********************************  输入控制  ***********************************/


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




//重定义fputc函数 
//加入串口独占判断指令
/******************************* WiFi 部分 ********************************/

OS_STK WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE];
u8 WiFi_State;//[7]位是1，那就是处于debug状态
void WiFi_Debug(void) {
	OSTaskCreate(WiFi_Debug_task,(void*)0,(OS_STK*)&WIFI_DEBUG_TASK_STK[WIFI_DEBUG_STK_SIZE-1],WIFI_DEBUG_TASK_PRIO);
	printf("\n/*********进入ESP8266【Debug】状态，退出请输入：-q \n");
}
void WiFi_Debug_task(void* pdata) {
	pdata=pdata;
	OS_CPU_SR cpu_sr ;
	WiFi_State |= 0x80;//设置Debug状态
	u8 len;
	u16 RX1=USART1_RX_STA;
	u16 RX2=USART2_RX_STA;
	u8* push_usart1 = mymalloc(SRAMIN, MAX_FNAME_LEN);
	memcpy(push_usart1, USART1_RX_BUF, MAX_FNAME_LEN);
	//关闭USMART
#if USE_SMART_APP==0
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE); //取消定时器4更新中断
	TIM_Cmd(TIM4, DISABLE); //关闭定时器4
#elif USE_SMART_APP==1
	OSTaskSuspend(USMART_APP_TASK_PRIO);
#endif // USE_SMART_APP==0
	STM32F407ZET6_info.USART1_Busy |= 0x01;//独占串口，不打印其他数据
	USART1_RX_STA = 0;
	USART2_RX_STA = 0;
	//扫描输入
	while (1) {
		delay_ms(100);
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
#if USE_SMART_APP==0
				TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //定时器4更新中断
				TIM_Cmd(TIM4, ENABLE); //打开定时器4
#elif USE_SMART_APP==1
				OSTaskResume(USMART_APP_TASK_PRIO);
#endif
				STM32F407ZET6_info.USART1_Busy |= ~(0x01);//释放串口独占权
				OSTaskDel(OS_PRIO_SELF);
				OS_EXIT_CRITICAL();
				return;
				/*****************************************************/
			}
			len = USART1_RX_STA & 0x3fff;	//得到此次接收到的数据长度[13:0]
			USART1_RX_BUF[len] = '\0';	//在末尾加入结束符. 
			/********退出条件*******/
			/*******不是退出命令,那就是发送的命令,******/
			OS_EXIT_CRITICAL();//向ESP8266打印这个需要中断
			STM32F407ZET6_info.USART1_Busy &= ~(0x01);
			printf("<----%s\n", USART1_RX_BUF);
			STM32F407ZET6_info.USART1_Busy |= 0x01;
			USART1_RX_STA = 0;//状态寄存器清空	    
			usart2_printf("%s\r\n", USART1_RX_BUF);	//发送给esp8266
		}
		if (USART2_RX_STA & 0x8000) {
			len = USART2_RX_STA & 0x7fff;
			USART2_RX_BUF[len] = '\0';	//在末尾加入结束符.
			STM32F407ZET6_info.USART1_Busy &= ~(0x01);
			printf("%s", USART2_RX_BUF);
			STM32F407ZET6_info.USART1_Busy |= 0x01;
			USART2_RX_STA = 0;
		}
		OS_EXIT_CRITICAL();

	}
	
}
/*******************************       SD       ****************************************/
OS_EVENT* message_SD;			//SD卡读写邮箱事件块指针

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
OS_EVENT* message_OLED;
OS_STK OLED_TASK_STK[OLED_STK_SIZE];
OS_EVENT* message_OLED;
void OLED_GUIGRAM_Init(void) {
	//安放整体布局
	OLED_Clear();
	OLED_DrawStr_manual(0, 0, "CPU: 00 %", 16, 1);//利用率
	OLED_DrawStr_manual(80, 0,  "|18:58", 16, 1);//时间
	OLED_DrawStr_manual(80, 16, "|06/14", 16, 1);//时期
	OLED_DrawStr_manual(80, 32, "|@2019", 16, 1);//年份
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

//红外遥控初始化
//设置IO以及TIM2_CH1的输入捕获
void Remote_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef  TIM1_ICInitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);//TIM1时钟使能 

	//GPIOA8  复用功能,上拉
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1); //GPIOA8复用为TIM1

	TIM_TimeBaseStructure.TIM_Prescaler = 167;  ////预分频器,1M的计数频率,1us加1.	
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseStructure.TIM_Period = 10000;   //设定计数器自动重装值 最大10ms溢出  
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	//初始化TIM2输入捕获参数
	TIM1_ICInitStructure.TIM_Channel = TIM_Channel_1; //CC1S=01 	选择输入端 IC1映射到TI1上
	TIM1_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	//上升沿捕获
	TIM1_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //映射到TI1上
	TIM1_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //配置输入分频,不分频 
	TIM1_ICInitStructure.TIM_ICFilter = 0x03;//IC1F=0003 8个定时器时钟周期滤波
	TIM_ICInit(TIM1, &TIM1_ICInitStructure);//初始化定时器2输入捕获通道

	TIM_ITConfig(TIM1, TIM_IT_Update | TIM_IT_CC1, ENABLE);//允许更新中断 ,允许CC1IE捕获中断	
	TIM_Cmd(TIM1, ENABLE); 	 	//使能定时器1

	NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//初始化NVIC寄存器

	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//初始化NVIC寄存器
}

//遥控器接收状态
//[7]:收到了引导码标志
//[6]:得到了一个按键的所有信息
//[5]:保留	
//[4]:标记上升沿是否已经被捕获								   
//[3:0]:溢出计时器
u8 	RmtSta = 0;
u16 Dval;		//下降沿时计数器的值
u32 RmtRec = 0;	//红外接收到的数据	   		    
u8  RmtCnt = 0;	//按键按下的次数	 
//定时器1溢出中断
void TIM1_UP_TIM10_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET) //溢出中断
	{
		if (RmtSta & 0x80)//上次有数据被接收到了
		{
			
			RmtSta &= ~0X10;						//取消上升沿已经被捕获标记
			if ((RmtSta & 0X0F) == 0X00)
				RmtSta |= 1 << 6;//标记已经完成一次按键的键值信息采集
				OSMboxPost(Message_Input, (void*)1);//发送信号
			if ((RmtSta & 0X0F) < 7)
				RmtSta++;
			else
			{
				RmtSta &= ~(1 << 7);//清空引导标识
				RmtSta &= 0XF0;	//清空计数器	
			}
		}
	}
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);  //清除中断标志位 
}	

//定时器1输入捕获中断服务程序	 
void TIM1_CC_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_CC1) == SET) //处理捕获(CC1IE)中断
	{
		LED2 = 0;
		if (RDATA)//上升沿捕获
		{
			TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Falling);		//CC1P=1 设置为下降沿捕获
			TIM_SetCounter(TIM1, 0);	   	//清空定时器值
			RmtSta |= 0X10;					//标记上升沿已经被捕获
		}
		else //下降沿捕获
		{
			Dval = TIM_GetCapture1(TIM1);//读取CCR1也可以清CC1IF标志位
			TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Rising); //CC1P=0	设置为上升沿捕获
			if (RmtSta & 0X10)					//完成一次高电平捕获 
			{
				if (RmtSta & 0X80)//接收到了引导码
				{
					if (Dval > 300 && Dval < 800)			//560为标准值,560us
					{
						RmtRec <<= 1;	//左移一位.
						RmtRec |= 0;	//接收到0	   
					}
					else if (Dval > 1400 && Dval < 1800)	//1680为标准值,1680us
					{
						RmtRec <<= 1;	//左移一位.
						RmtRec |= 1;	//接收到1
					}
					else if (Dval > 2200 && Dval < 2600)	//得到按键键值增加的信息 2500为标准值2.5ms
					{
						RmtCnt++; 		//按键次数增加1次
						RmtSta &= 0XF0;	//清空计时器
						
					}
				}
				else if (Dval > 4200 && Dval < 4700)		//4500为标准值4.5ms
				{
					RmtSta |= 1 << 7;	//标记成功接收到了引导码
					RmtCnt = 0;		//清除按键次数计数器
					
				}
			}
			RmtSta &= ~(1 << 4);
		}
	}
	TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);  //清除中断标志位 
}

//处理红外键盘
//返回值:
//	 0,没有任何按键按下
//其他,按下的按键键值.
u8 Remote_Scan(void)
{
	u8 sta = 0;
	u8 t1, t2;
	if (RmtSta & (1 << 6))//得到一个按键的所有信息了
	{
		t1 = RmtRec >> 24;			//得到地址码
		t2 = (RmtRec >> 16) & 0xff;	//得到地址反码 
		if ((t1 == (u8)~t2) && t1 == REMOTE_ID)//检验遥控识别码(ID)及地址 
		{
			t1 = RmtRec >> 8;
			t2 = RmtRec;
			if (t1 == (u8)~t2)
			{
				printf("%d,%d,%d,%d\n", RmtRec >> 24, (RmtRec >> 16) & 0xFF, (RmtRec >> 8) & 0xFF, (RmtRec) & 0xFF);
				sta = t1;//键值正确	 
				RmtRec = 0;
			}
		}
		if ((sta == 0) || ((RmtSta & 0X80) == 0))//按键数据错误/遥控已经没有按下了
		{
			RmtSta &= ~(1 << 6);//清除接收到有效按键标识
			RmtCnt = 0;		//清除按键次数计数器
		}
	}
	LED2 = 1;
	return sta;
}


/**********************           APP 主函数   *************************/
void APP_task(void* pdata) {
	return;
}


/**********************           APP 外部函数   *************************/
OS_STK APP_TASK_STK[APP_STK_SIZE];

//显示系统信息函数
void lcd_ShowSystemInfo(void) {
	/*******************基本格局********************/
	LCD_Clear(WHITE);
	POINT_COLOR = BLACK;
	LCD_ShowString(0, 0 , 240, 16, 16, "(>▽<)～【系统状态】(￣▽￣～)");
	POINT_COLOR = BLUE;
	/****************************************************************************/
	LCD_DrawLine(0, 17, 240, 16); LCD_DrawLine(0, 18, 240, 16); LCD_DrawLine(0, 19, 240, 16);
	POINT_COLOR = BLACK;
	LCD_ShowString(8, 20 , 230, 16, 16, "<时间> 19:10:00 <周期数>嘿嘿");//x:5~53
	LCD_Draw_setting(BLACK, WHITE, 128);
	LCD_ShowString(8, 40 , 230, 16, 16, "<日期>2020年04月04日星期六");
	/********************************硬件模块************************************/
	LCD_DrawLine(0, 58, 240, 16); LCD_DrawLine(0, 59, 240, 16);
	LCD_ShowString(8, 60 , 230, 16, 16, "<网络> WiFi模块 [异常]×");
	LCD_ShowString(8, 80 , 230, 16, 16, "<控制> 遥控模块 [异常]×");
	LCD_ShowString(8, 100, 230, 16, 16, "<显示> OLED模块 [异常]×");
	LCD_ShowString(8, 120, 230, 16, 16, "<显示> LCD 模块 [异常]×");
	LCD_ShowString(8, 140, 230, 16, 16, "<储存> SD卡模块 [异常]×");
	LCD_ShowString(8, 160, 230, 16, 16, "总量: 0000 MB 空闲: 0000 MB");
	/********************************软件模块************************************/
	LCD_DrawLine(0, 168, 240, 16); LCD_DrawLine(0, 169, 240, 16);
	LCD_ShowString(8, 180, 230, 16, 16, "<汉字> 库存汉字 21,003个");
	LCD_ShowString(8, 200, 230, 16, 16, "<图片> 库存图片    23 张");

	LCD_ShowString(8, 220, 230, 16, 16, "<颜值> ");
	LCD_ShowString(8, 240, 230, 16, 16, "过于美丽，无法识别");


	LCD_ShowString(8, 260, 230, 16, 16, "<上次查看时间>：");
	LCD_ShowString(8, 280, 230, 16, 16, "2020年 4月 4日 19:48");
	LCD_Draw_setting(BLACK, WHITE, 128);
	LCD_ShowString(8, 300, 230, 16, 16, "你都不来看看我惹 ╥﹏╥...");

	

//	LCD_Draw_setting(0x0000, 0xFFFF, 64);

}

























