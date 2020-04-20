#include "function_wlf.h"



/*不要使用PA13和PA14，它们分别对应着【SW-DIO】和【SW-CLK】，且本身一直处于AF复用模式*/
/*配置外设的时候要记得先使能时钟，然后在配置，因为需要一段时间等待时钟正常运行*/


/*******************************定时器****************************/
//【定时器4】是给【USMART】的
//【定时器7】是给【USART2】的，用于WiFi串口设置



/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			10 //开始任务的优先级设置为最低
#define START_STK_SIZE  				128//设置任务堆栈大小
OS_STK START_TASK_STK[START_STK_SIZE];//任务堆栈	
void start_task(void* pdata);//任务函数

#define MAIN_TASK_PRIO					4
#define MAIN_STK_SIZE					64
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
void main_task(void* padta);



void STM32_init(void);
void STM32_init(void) {
	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	Key_Init();/*******************************************************************/
	Remote_Init();/*******************************************************************/
	My_RTC_Init();/*******************************************************************/
	RTC_Set_WakeUp(RTC_WakeUpClock_CK_SPRE_16bits, 0);
	usart1_init(115200);/*******************************************************************/
	delay_ms(500);//LCD复位比较慢
	usmart_init(84);/*******************************************************************/
	LCD_Init();/*******************************************************************/
	LCD_Init();/*******************************************************************/
	POINT_COLOR = BLACK;	//设置字体为黑色 									    
	LCD_ShowString(20, 20, 200, 16, 16, "Welcome [LiuQian].");
	LCD_ShowString(20, 40, 200, 16, 16, "LCD  Initialized.");
	sprintf(string_buff, "LCD_H=[%d]  LCD_W=[%d]",lcddev.height,lcddev.width);
	LCD_ShowString(20, 60, 200, 16, 16, string_buff);

	OLED_Init();/*******************************************************************/
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);
	LCD_ShowString(20, 80, 200, 16, 16, "OLED Initialized.");


	SD_Init();/*******************************************************************/
	show_sdcard_info();	//打印SD卡相关信息
	LCD_ShowString(20, 150, 200, 16, 16, "SD Card OK    ");
	POINT_COLOR = RED;
	LCD_ShowNum(20 + 13 * 8, 170, SDCardInfo.CardCapacity >> 20, 5, 16);//显示SD卡容量
	//初始化内部内存池
	my_mem_init(SRAMIN);/*******************************************************************/
	//为fatfs相关变量申请内存
	exfuns_init();/*******************************************************************/				 
	res = f_mount(fs[0], "0:", 1);//挂载SD卡 
	if (res == 0X0D)//SD盘,FAT文件系统错误,
	{
		printf("SD卡错误！\r");
		POINT_COLOR = RED;
		LCD_ShowString(20, 80, 200, 20, 20, "SD card !ERROR!.");
		//重新格式化SD
#if FORMAT_IF_ERROR==1
		LCD_ShowString(20, 150, 200, 16, 16, "Flash Disk Formatting...");	//格式化SD
		res = f_mkfs("0:", 1, 4096);//格式化SD,0,盘符;0,不需要引导区,8个扇区为1个簇
		if (res == 0)
		{
			f_setlabel((const TCHAR*)"0:WLF_SD");	//设置SD磁盘的名字为：WLF_SD
			LCD_ShowString(20, 150, 200, 16, 16, "Flash Disk Format Finish");	//格式化完成
		}
		else LCD_ShowString(20, 150, 200, 16, 16, "Flash Disk Format Error ");	//格式化失败
		delay_ms(1000);
#endif
	}

	t = 0;//得到SD卡的总容量和剩余容量
	while ((t++>100)||exf_getfree("0", &total, &free))	
	{
		LCD_ShowString(20, 150, 200, 16, 16, "SD Card FATFS Error!");
		delay_ms(200);
		LED1 != LED1;
	}
	t = 0;
	POINT_COLOR = BLACK;//设置字体   
	OLED_GUIGRAM_Init();
	LCD_ShowString(20, 150, 200, 16, 16, "FATFS OK!      ");
	LCD_ShowString(20, 170, 200, 16, 16, "SD Total Size:     MB");
	LCD_ShowString(20, 190, 200, 16, 16, "SD  Free Size:     MB");
	LCD_ShowNum(20 + 8 * 14, 170, total >> 10, 5, 16);//显示SD卡总容量 MB
	POINT_COLOR = (free < total / 5) ? RED : BLUE;//设置字体
	LCD_ShowNum(20 + 8 * 14, 190, free >> 10, 5, 16);//显示SD卡剩余容量 MB	
	STM32F407ZET6_info.SD_total = total;
	STM32F407ZET6_info.SD_free = free;
	STM32F407ZET6_info.SD_free = free;


	//初始化cc936
	cc936_Init();/*******************************************************************/

	//初始化字库
	POINT_COLOR = BLACK;
	res=font_init();/*******************************************************************/	
	if(res==0) LCD_ShowString(20, 210, 200, 16, 16, "字库初始化完毕！");
	else {
		POINT_COLOR = RED;
		LCD_ShowString(20, 210, 200, 16, 16, "Chinese font failed!"); 
		
	}
	LCD_ShowString(20, 100, 200, 24, 24, "刘倩,欢迎使用~");
	POINT_COLOR = BLACK;
	res=PictureFile_Init();/*******************************************************************/
	sprintf(string_buff, "图片读取完毕，共%d个", STM32F407ZET6_info.Picture_totalnum);

	printf("%s\r\n",string_buff);
	if(res==0) LCD_ShowString(20, 230, 200, 16, 16, string_buff);
	piclib_init();/*******************************************************************/
	
	OLED_GUIGRAM_Init();/*******************************************************************/
	OLED_Refresh();

	LCD_ShowString(20, 246, 200, 24, 24, "即将显示桌面."); 
	delay_ms(200);
	LCD_ShowString(20, 246, 200, 24, 24, "即将显示桌面.."); 
	delay_ms(200);

	
	LCD_Clear(PURPLE);
	show_picture("0:/PICTURE/头像.bmp", 1);//显示图片 

//	ESP8266_init();/*******************************************************************/


	STM32F407ZET6_info.point_COLOR=&POINT_COLOR;
	STM32F407ZET6_info.back_COLOR=&BACK_COLOR;
	STM32F407ZET6_info.font_BOLD=&FontBold;

	
	LQ_period_read();//读取生理期时间
}



//开始任务
void start_task(void* pdata)
{
	OS_CPU_SR cpu_sr = 0;
	pdata = pdata;
	message_SD = OSMboxCreate((void*)0);
	Message_Input = OSMboxCreate((void*)0);//传递【红外】的和【按钮】的输入
	Message_APP_cmd = OSMboxCreate((void*)0);//传递给【主函数】的命令输入
	Message_LQ_clock = OSMboxCreate((void*)0);//传递给【LQ_clock】的时间输入
	OSStatInit();					//初始化统计任务.这里会延时1秒钟左右
	OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
	OSTaskCreate(USMART_APP,(void*)0,(OS_STK*)&USMART_APP_TASK_STK[USMART_APP_STK_SIZE-1],USMART_APP_TASK_PRIO);//3级
	OSTaskCreate(main_task, (void*)0, (OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1], MAIN_TASK_PRIO);//4级
	OSTaskCreate(APP_task, (void*)0, (OS_STK*)&APP_TASK_STK[APP_STK_SIZE-1], APP_TASK_PRIO);//5级
	OSTaskCreate(OLED_GUI_update, (void*)0, (OS_STK*)&OLED_TASK_STK[OLED_STK_SIZE - 1], OLED_TASK_PRIO);//6级
	OSTaskCreate(Show_LQ_CLOCK, (void*)0, (OS_STK*)&LQ_CLOCK_TASK_STK[LQ_CLOCK_STK_SIZE - 1], LQ_CLOCK_TASK_PRIO);//7级
	OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
}



int main(void) {
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	//基本程序初始化
	STM32_init();
	//系统初始化
	OSInit();
	//创建开始任务
	OSTaskCreate(start_task, (void*)0, (OS_STK*)&START_TASK_STK[START_STK_SIZE - 1], START_TASK_PRIO);
	//开始任务
	OSStart();
}

void main_task(void* padta) {
	OS_CPU_SR cpu_sr;
	u8 err;
	u32 res;
	_RMT_CMD* cmd;
	u8 cmd_index;
	char* cmd_str;
	
	cmd_str=cmd_str;
	while (1) {
		
		while (1) {

			if ((u32)OSMboxPend(Message_Input, 800, &err) != 0) break;
			else OLED_DrawStr(0, 34, "    ", 24, 1);//800个系统滴答都没有收到消息后，自动清零
		}
		res = Remote_Scan();//等待500个系统滴答，在此期间会启用系统调度
		
		OS_ENTER_CRITICAL();

		if (res != 0) {
			switch (res)
		{
		default:	cmd = &Remote_CmdStr[0];
			break;
		case 162:	cmd = &Remote_CmdStr[1];
			break;
		case 98:	cmd = &Remote_CmdStr[2];
			break;
		case 226:	cmd = &Remote_CmdStr[3];
			break;
		case 34:	cmd = &Remote_CmdStr[4];
			break;
		case 2:		cmd = &Remote_CmdStr[5];
			break;
		case 194:	cmd = &Remote_CmdStr[6];
			break;
		case 224:	cmd = &Remote_CmdStr[7];
			break;
		case 168:	cmd = &Remote_CmdStr[8];
			break;
		case 144:	cmd = &Remote_CmdStr[9];
			break;
		case 104:	cmd = &Remote_CmdStr[10];
			break;
		case 152:	cmd = &Remote_CmdStr[11];
			break;
		case 176:	cmd = &Remote_CmdStr[12];
			break;
		case 48:	cmd = &Remote_CmdStr[13];
			break;
		case 24:	cmd = &Remote_CmdStr[14];
			break;
		case 122:	cmd = &Remote_CmdStr[15];
			break;
		case 16:	cmd = &Remote_CmdStr[16];
			break;
		case 56:	cmd = &Remote_CmdStr[17];
			break;
		case 90:	cmd = &Remote_CmdStr[18];
			break;
		case 66:	cmd = &Remote_CmdStr[19];
			break;
		case 74:	cmd = &Remote_CmdStr[20];
			break;
		case 82:	cmd = &Remote_CmdStr[21];
			break;
		}
			cmd_index = cmd->index;
//			cmd_str = (char*)(cmd->name);
			/*********************给APP发送消息***********************/

//			printf("%d\n",cmd_num);
			OSMboxPost(Message_APP_cmd, &cmd_index);//发送信号
			OS_EXIT_CRITICAL();
			/*********************************************************/
			
		}

		OS_EXIT_CRITICAL();

		
	}
}
