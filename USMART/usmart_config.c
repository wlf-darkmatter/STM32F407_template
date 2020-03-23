#include "usmart.h"
#include "usmart_str.h"
////////////////////////////用户配置区///////////////////////////////////////////////
//这下面要包含所用到的函数所申明的头文件(用户自己添加) 
#include "delay.h"		
#include "sys.h"
#include "lcd.h"
#include <OLED.h>
#include <usart.h>
#include "SDIO_SDCard.h"

//extern void led_set(u8 sta);
//extern void test_fun(void(*ledset)(u8),u8 sta);
//函数名列表初始化(用户自己添加)
//用户直接在这里输入要执行的函数名及其查找串
struct _m_usmart_nametab usmart_nametab[]=
{
#if USMART_USE_WRFUNS==1 	//如果使能了读写操作
	(void*)read_addr,"u32 read_addr(u32 addr)",
	(void*)write_addr,"void write_addr(u32 addr,u32 val)",	 
#endif		   
	(void*)delay_ms,"void delay_ms(u16 nms)",
 	(void*)delay_us,"void delay_us(u32 nus)",	

	(void*)LCD_Clear,"void LCD_Clear(u16 color)",
	(void*)LCD_Fill,"void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)",
	(void*)LCD_DrawLine,"void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)",
	(void*)LCD_DrawRectangle,"void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)",
	(void*)LCD_Draw_Circle,"void Draw_Circle(u16 x0,u16 y0,u8 r)",
	(void*)LCD_ShowNum,"void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)",
	(void*)LCD_ShowString,"void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)",
	(void*)LCD_Fast_DrawPoint,"void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)",
	(void*)LCD_ReadPoint,"u16 LCD_ReadPoint(u16 x,u16 y)",							 
	(void*)LCD_Display_Dir,"void LCD_Display_Dir(u8 dir)",
	(void*)LCD_ShowxNum,"void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode)",
//	(void*)led_set,"void led_set(u8 sta)",
//	(void*)test_fun,"void test_fun(void(*ledset)(u8),u8 sta)",
	(void*)OLED_Cmd,"void OLED_Cmd(u8 command)",
	(void*)OLED_Data,"void OLED_Data(u8 data)",
	(void*)OLED_Init,"void OLED_Init(void)",
	(void*)OLED_On,"void OLED_On(void)",
	(void*)OLED_Fill,"void OLED_Fill(unsigned char fill_Data)",
	(void*)OLED_SetPos,"void OLED_SetPos(unsigned char x, unsigned char y)",
	(void*)OLED_Refresh,"void OLED_Refresh(void)",
	(void*)OLED_DrawPoint,"void OLED_DrawPoint(u8 x, u8 y, unsigned char mode)",
	(void*)OLED_DrawChar,"void OLED_DrawChar(u8 x, u8 y, u8 chr, u8 size, u8 mode)",
	(void*)OLED_DrawStr,"void OLED_DrawStr(u8 x, u8 y, char* str, u8 size, u8 mode)",
	(void*)OLED_ShowGBK,"void OLED_ShowGBK(u8 x, u8 y, u8 num, u8 size, u8 mode)",
	(void*)OLED_Clear,"void OLED_Clear(void)",






};						  
///////////////////////////////////END///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//函数控制管理器初始化
//得到各个受控函数的名字
//得到函数总数量
struct _m_usmart_dev usmart_dev= {
	usmart_nametab,//函数名指针
	usmart_init,//void (*init)(u8);		初始化
	usmart_cmd_rec,//u8 (*cmd_rec)(u8*str);	识别函数名及参数——返回值类型 ( * 指针变量名) ([形参列表]);
	usmart_exe,//void (*exe)(void);执行
	usmart_scan,//void (*scan)(void);扫描
	sizeof(usmart_nametab)/sizeof(struct _m_usmart_nametab),//u8 fnum;函数数量
	0,	  	//u8 pnum;参数数量
	0,	 	//u8 id;函数ID
	1,		//u8 sptype;参数显示类型,0,10进制;1,16进制
	0,		//u16 parmtype;参数类型.bitx:,0,数字;1,字符串	    
	0,	  	//u8  plentbl[MAX_PARM];每个参数的长度暂存表,需要MAX_PARM个0初始化
	0,		//u8  parm[PARM_LEN];函数的参数,需要PARM_LEN个0初始化
	//u8 runtimeflag;——0,不统计函数执行时间;1,统计函数执行时间,注意:此功能必须在USMART_ENTIMX_SCAN使能的时候,才有用
	//u32 runtime;运行时间,单位:0.1ms,最大延时时间为定时器CNT值的2倍*0.1ms
};   



