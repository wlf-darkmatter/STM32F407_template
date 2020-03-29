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

#include "piclib.h"

//如果SD卡错误，是否自动格式化，1=是，0=否
#define FORMAT_IF_ERROR 0


char* lcd_string;//指向需要打印的字符串的指针
DIR picdir;
u16* picindextbl; //用于存放图片索引
int mytemp;//只局限于一个函数向外部传递信息

void STM32_init(void);
u16 pic_get_tnum(u8* path);
u8 PictureFile_Init(void);

/*不要使用PA13和PA14，它们分别对应着【SW-DIO】和【SW-CLK】，且本身一直处于AF复用模式*/
/*配置外设的时候要记得先使能时钟，然后在配置，因为需要一段时间等待时钟正常运行*/
void STM32_init(void) {


	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	lcd_string = mymalloc(SRAMIN, 32);

	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	Key_Init();/*******************************************************************/
	usart1_init(115200);/*******************************************************************/
	usmart_init(168);/*******************************************************************/
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
		POINT_COLOR = BLACK;
	}

	res=PictureFile_Init();/*******************************************************************/
	sprintf(lcd_string, "图片读取完毕，共%d个", mytemp);
	if(res==0) LCD_ShowString(20, 230, 200, 16, 16, lcd_string);
	








	//myfree(SRAMIN, lcd_string);
}


u8 PictureFile_Init(void) {
	FILINFO picfileinfo;//记录图片的文件信息
	u16 totpicnum;
	u8* pname;//带路径的文件名
	u8 res;
	u16 curindex;//文件当前索引
	u16 temp;
	u8* fn;//长文件名
	u8 time = 0;
	while (f_opendir(&picdir, "0:/PICTURE"))//打开图片文件夹
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
	res = f_opendir(&picdir, "0:/PICTURE"); //打开目录
	if (res == FR_OK)
	{
		curindex = 0;//当前索引为0
		while (1)//全部查询一遍
		{
			temp = picdir.index;								//记录当前index
			res = f_readdir(&picdir, &picfileinfo);       		//读取目录下的一个文件
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
	mytemp = curindex;
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



int main(void) {





	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	
	
	STM32_init();




//读取SD卡示例
/*
	
	buf = mymalloc(0, 512);		//申请内存
	if (SD_ReadDisk(buf, 0, 1) == 0)	//读取0扇区的内容
	{
		POINT_COLOR = BROWN;
		LCD_ShowString(20, 230, 200, 16, 16, "USART1 Sending Data...");
		printf("SECTOR 0 DATA:\r\n");
		for (sd_size = 0; sd_size < 512; sd_size++)printf("%x ", buf[sd_size]);//打印0扇区数据    	   
		printf("\r\nDATA ENDED\r\n");
		LCD_ShowString(20, 230, 200, 16, 16, "USART1 Send Data Over!");
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




