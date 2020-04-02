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


#define FLOLAT_TASK_PRIO				5
#define FLOAT_STK_SIZE					128//设置任务堆栈大小
#if __FPU_USED//如果任务中使用printf函数打印浮点数的话一定要8字节对齐
__align(8) OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];
#else
OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];
#endif
//任务函数
//浮点测试任务
void float_task(void* pdata);

OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];

//见【function.c】
//#define WIFI_DEBUG_TASK_PRIO				2
//#define WIFI_DEBUG_STK_SIZE				128
/***************************************************************************************/

char lcd_string[64];//指向需要打印的字符串的指针



void STM32_init(void);
void STM32_init(void) {

	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	Key_Init();/*******************************************************************/
	My_RTC_Init();/*******************************************************************/
	RTC_Set_WakeUp(RTC_WakeUpClock_CK_SPRE_16bits, 0);
	usart1_init(115200);/*******************************************************************/
	usmart_init(84);/*******************************************************************/
	delay_ms(200);//LCD复位比较慢
	LCD_Init();/*******************************************************************/
	POINT_COLOR = BLACK;	//设置字体为黑色 									    
	LCD_ShowString(20, 20, 200, 16, 16, "Welcome [LiuQian].");
	LCD_ShowString(20, 40, 200, 16, 16, "LCD  Initialized.");
	sprintf(lcd_string, "LCD_H=[%d]  LCD_W=[%d]",lcddev.height,lcddev.width);
	LCD_ShowString(20, 60, 200, 16, 16, lcd_string);

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
	LCD_ShowString(20, 150, 200, 16, 16, "FATFS OK!      ");
	LCD_ShowString(20, 170, 200, 16, 16, "SD Total Size:     MB");
	LCD_ShowString(20, 190, 200, 16, 16, "SD  Free Size:     MB");
	LCD_ShowNum(20 + 8 * 14, 170, total >> 10, 5, 16);//显示SD卡总容量 MB
	POINT_COLOR = (free < total / 5) ? RED : BLUE;//设置字体
	LCD_ShowNum(20 + 8 * 14, 190, free >> 10, 5, 16);//显示SD卡剩余容量 MB	

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
	POINT_COLOR = BLACK;
	res=PictureFile_Init();/*******************************************************************/
	sprintf(lcd_string, "图片读取完毕，共%d个", App_LCD.Picture_num);
	printf("%s\n",lcd_string);
	if(res==0) LCD_ShowString(20, 230, 200, 16, 16, lcd_string);
	piclib_init();/*******************************************************************/
	
	OLED_GUIGRAM_Init();/*******************************************************************/
	OLED_Refresh();

	LCD_ShowString(20, 246, 200, 24, 24, "即将显示桌面."); 
	delay_ms(800);
	LCD_ShowString(20, 246, 200, 24, 24, "即将显示桌面.."); 
	delay_ms(800);

	LCD_Clear(GREEN);
	show_picture("0:/PICTURE/头像.bmp", 1);//显示图片 

	ESP8266_init();/*******************************************************************/




	//myfree(SRAMIN, lcd_string);
}



//开始任务
void start_task(void* pdata)
{
	OS_CPU_SR cpu_sr = 0;
	message_SD = OSMboxCreate((void*)0);
	OSStatInit();					//初始化统计任务.这里会延时1秒钟左右
	pdata = pdata;
	OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
	OSTaskCreate(USMART_APP, (void*)0, (OS_STK*)&USMART_APP_TASK_STK[USMART_APP_STK_SIZE - 1], USMART_APP_TASK_PRIO);
	OSTaskCreate(OLED_GUI_update, (void*)0, (OS_STK*)&OLED_TASK_STK[OLED_STK_SIZE - 1], OLED_TASK_PRIO);
	OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
}

/*【PWM】部分

//PWM_TIM14_Init(500 - 1, 84 - 1);//周期时间为500us,第一个参数是【ARR】，也叫【period】周期值
*/

/*【TIM3】计数及【TIM3】中断

TIM3_INT_Init(5000 - 1, 8400 - 1);//一般情况下，Tout=(I+1)*(II+1)/84 (单位us)
*/


//通过串口打印SD卡相关信息


/**********************************浮点测试任务****************************************/
void float_task(void* pdata) {
/*	OS_CPU_SR cpu_sr = 0;
	static float float_num = 0.01f;
	while (1) {
		float_num += 0.01f;
		OS_ENTER_CRITICAL();			//进入临界区(无法被中断打断)    
		printf("float_num的值为：%.4f\n", float_num);
		OS_EXIT_CRITICAL();				//退出临界区(可以被中断打断)
		delay_ms(500);
	}
*/
}
/**************************************************************************************/




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

//在OLED.c中实现
/*u8 OLED_GUI_Init(void) {
	OLED_Clear();
	OLED_DrawStr(0, 0, "CPU: 00 %", 16, 1);//利用率
	OLED_DrawStr(72, 0, " |12:13", 16, 1);//时间
	OLED_DrawStr(0, 22, "User: QianQian", 12, 1);
	OLED_DrawStr(80, 16, "|03/31", 16, 1);
}*/


