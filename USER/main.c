#include "sys.h"
#include "delay.h"
#include "usart.h"
#include <LED_STM32F407ZET6.h>
#include <Key_STM32F407ZET6.h>
#include <lcd.h>
#include <Beep.h>
#include "usmart.h"
#include <Tim.h>
#include <PWM.h>
#include <wlf_I2C.h>
#include <OLED.h>
#include <WLF_font.h>

#include <WIFI_ESP8266.h>
#include "SDIO_SDCard.h"
#include "malloc.h"

//如果SD卡错误，是否自动格式化，1=是，0=否
#define FORMAT_IF_ERROR 0

/*不要使用PA13和PA14，它们分别对应着【SW-DIO】和【SW-CLK】，且本身一直处于AF复用模式*/
/*配置外设的时候要记得先使能时钟，然后在配置，因为需要一段时间等待时钟正常运行*/
void STM32_init(void) {
	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	usart1_init(115200);/*******************************************************************/
	usmart_init(168);/*******************************************************************/
	Key_Init();/*******************************************************************/
	LCD_Init();/*******************************************************************/
	POINT_COLOR = BLACK;	//设置字体为黑色 									    
	LCD_ShowString(30, 20, 200, 16, 16, "Welcome [LiuQian].");
	LCD_ShowString(30, 40, 200, 16, 16, "LCD  Initialized.");

	OLED_Init();/*******************************************************************/
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);
	LCD_ShowString(30, 60, 200, 16, 16, "OLED Initialized.");



	SD_Init();/*******************************************************************/
	show_sdcard_info();	//打印SD卡相关信息
	LCD_ShowString(30, 150, 200, 16, 16, "SD Card OK    ");
	POINT_COLOR = RED;
	LCD_ShowNum(30 + 13 * 8, 170, SDCardInfo.CardCapacity >> 20, 5, 16);//显示SD卡容量
	
	//为fatfs相关变量申请内存
	exfuns_init();/*******************************************************************/				 
	res = f_mount(fs[0], "0:", 1);//挂载SD卡 

	if (res == 0X0D)//SD盘,FAT文件系统错误,
	{
		printf("SD卡错误！\r");
		POINT_COLOR = RED;
		LCD_ShowString(30, 80, 200, 20, 20, "SD card !ERROR!.");
		//重新格式化SD
#if FORMAT_IF_ERROR==1
		LCD_ShowString(30, 150, 200, 16, 16, "Flash Disk Formatting...");	//格式化SD
		res = f_mkfs("0:", 1, 4096);//格式化SD,0,盘符;0,不需要引导区,8个扇区为1个簇
		if (res == 0)
		{
			f_setlabel((const TCHAR*)"0:WLF_SD");	//设置SD磁盘的名字为：WLF_SD
			LCD_ShowString(30, 150, 200, 16, 16, "Flash Disk Format Finish");	//格式化完成
		}
		else LCD_ShowString(30, 150, 200, 16, 16, "Flash Disk Format Error ");	//格式化失败
		delay_ms(1000);
#endif
	}

	t = 0;
	while ((t++>100)||exf_getfree("0", &total, &free))	//得到SD卡的总容量和剩余容量
	{
		LCD_ShowString(30, 150, 200, 16, 16, "SD Card FATFS Error!");
		delay_ms(200);
		LED1 != LED1;
	}
	t = 0;
	POINT_COLOR = BLACK;//设置字体   
	LCD_ShowString(30, 150, 200, 16, 16, "FATFS OK!   ");
	LCD_ShowString(30, 170, 200, 16, 16, "SD Total Size:     MB");
	LCD_ShowString(30, 190, 200, 16, 16, "SD  Free Size:     MB");
	LCD_ShowNum(30 + 8 * 14, 170, total >> 10, 5, 16);//显示SD卡总容量 MB
	POINT_COLOR = (free < total / 5) ? RED : BLUE;//设置字体
	LCD_ShowNum(30 + 8 * 14, 190, free >> 10, 5, 16);//显示SD卡剩余容量 MB	


	//初始化字库
	font_init();/*******************************************************************/	





	
}

int main(void) {

	u32 sd_size;

	u8* buf;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	
	
	STM32_init();




//读取SD卡示例
/*
	
	buf = mymalloc(0, 512);		//申请内存
	if (SD_ReadDisk(buf, 0, 1) == 0)	//读取0扇区的内容
	{
		POINT_COLOR = BROWN;
		LCD_ShowString(30, 230, 200, 16, 16, "USART1 Sending Data...");
		printf("SECTOR 0 DATA:\r\n");
		for (sd_size = 0; sd_size < 512; sd_size++)printf("%x ", buf[sd_size]);//打印0扇区数据    	   
		printf("\r\nDATA ENDED\r\n");
		LCD_ShowString(30, 230, 200, 16, 16, "USART1 Send Data Over!");
	}
	myfree(0, buf);//释放内存	   
	*/

	



	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//降低系统中断优先级分组4
	while (1) {
		
	}

}


/*【PWM】部分

//PWM_TIM14_Init(500 - 1, 84 - 1);//周期时间为500us,第一个参数是【ARR】，也叫【period】周期值
*/

/*【TIM3】计数及【TIM3】中断

TIM3_INT_Init(5000 - 1, 8400 - 1);//一般情况下，Tout=(I+1)*(II+1)/84 (单位us)
*/


//通过串口打印SD卡相关信息




