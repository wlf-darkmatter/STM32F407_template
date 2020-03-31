#ifndef __OLED_H
#define __OLED_H

#include "sys.h"
/*************************/
//OLED的通讯类型
//0x00是【IIC】
//0x01是【SPI】
//0x02是【8080】
/************************/


#define OLED_TX_TYPE 0x00



#if OLED_TX_TYPE==0x00//OLED的通讯方式是【IIC】

//定义OLED的【IIC】通讯端口

//SCL = PB8
#define OLED_SLC IIC_SCL
//SDA（默认为输出）= PB9
#define OLED_SDA IIC_SDA
//SDA（输入）= PB9
#define OLED_SDA_INPUT IIC_SDA_Input

#define Max_Column 128


					/*************************/
					//===OLED的显存
					//===存放格式如下.
					//———    列 列 列 列
					//---页[0]	0  1  2  3 ... 127
					//---页[1]	0  1  2  3 ... 127
					//---页[2]	0  1  2  3 ... 127
					//---页[3]	0  1  2  3 ... 127
					//---页[4]	0  1  2  3 ... 127
					//---页[5]	0  1  2  3 ... 127
					//---页[6]	0  1  2  3 ... 127
					//---页[7]	0  1  2  3 ... 127
//*其中：页[x]，有 128 列和 8 行；
//8行 以字节的8个位的形式表达
					/*************************/
//定义【显存】
extern u8 OLED_GRAM[128][8];//128列，8*8=64行

/*1、开始信号：处理器让SCL时钟保持高电平，然后让SDA数据信号由高变低就表示一个开始信号。同时IIC总线上的设备检测到这个开始信号它就知道处理器要发送数据了。
2、停止信号：处理器让SCL时钟保持高电平，然后让SDA数据信号由低变高就表示一个停止信号。同时IIC总线上的设备检测到这个停止信号它就知道处理器已经结束了数据传输，我们就可以各忙各个的了，如休眠等。 */

void OLED_Cmd(u8 command);
void OLED_Data(u8 data);

//OLED初始化函数
void OLED_Init(void);

void OLED_On(void);


//全屏填充
void OLED_Fill(unsigned char fill_Data);


//【设置光标】x,光标x【列位置】y,光标y【页位置】
void OLED_SetPos(unsigned char x, unsigned char y);



//【刷新显存】刷新整个屏幕
void OLED_Refresh(void);
//【打点函数】（在显存中打点）
void OLED_DrawPoint(u8 x, u8 y, unsigned char mode);

/**
 * 【打印字符函数】
 * 在指定位置显示字符
 * x:0~127
 * y:0~63
 * mode: 0:反白显示 1:正常显示
 * size: 字号 12/16/24
**/
void OLED_DrawChar(u8 x, u8 y, u8 chr, u8 size, u8 mode);
/*
 * x:0~127
 * y:0~63*/
void OLED_DrawStr(u8 x, u8 y, char* str, u8 size, u8 mode);

//写中文字符函数
void OLED_ShowGBK(u8 x, u8 y, u8 num, u8 size, u8 mode);
/*//写中文字符串,str是数字【未完善......】
void OLED_DrawString(u8 x, u8 y, char* str, u8 size, u8 mode);
*/
//【清屏】
void OLED_Clear(void);
void OLED_GUI_Init(void);
void OLED_CPUstate(void);//输出CPU信息
#endif // OLED_TX_TYPE != 0x00

#endif // !__OLED_H
