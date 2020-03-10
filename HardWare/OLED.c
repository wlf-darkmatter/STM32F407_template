#include <OLED.h>
#include <wlf_I2C.h>
#include <LED_STM32F407ZET6.h>
#include "usart.h"
#include "delay.h"

/*字库添加*/
#include "oledfont.h"

/*OLED（SSD1306）【地址】通讯规则*/
/*以上用枚举来定义*/
enum OLED_Address{
/*********************************************************************/
//(bit) b7		b6		b5		b4		b3		b2		b1		b0
//		0		1		1		1		1		0		D/#C	R/#W
//——————————————————————————————————
//		【D/#C】为【数据】（高）、【指令】（低）
//		【R/#W】为【读】（高）、【写】（低）
/*********************************************************************/
/*********************************************************************/
//(bit) b7		b6		b5		b4		b3		b2		b1		b0
//		C0		D/#C	0		0		0		0		0		0
//——————————————————————————————————
//		【D/#C】为【数据】（1）、【指令】（0），决定了下个字节是【数据字节】还是【命令字节】
//		【Co】为【连续】（1）、【单次】（0）
/*********************************************************************/
	OLED_ADD_CW = 0x78,	//【控制】、【写】0111 1000‬B
	OLED_ADD_CR = 0x79,	//【控制】、【读】0111 1001B
	OLED_ADD_DW = 0x7A,	//【数据】、【写】0111 1010B
	OLED_ADD_DR = 0x7B,	//【数据】、【读】0111 1011B
	OLED_ADD_CMD = 0x00,				//00 000000		【控制字说明】
	OLED_ADD_DATA = 0x40,				//01 000000		【数据字说明】
//	OLED_ADD_CODATA = 0xC0,				//11 000000		【连续数据字说明】
}OLED_ADD;
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
u8 OLED_GRAM[128][8];

//写【命令函数】//128列，8*8=64行
void OLED_Cmd(u8 command) {
	IIC_Start();//发送【开始信号】
	IIC_Send_Byte(OLED_ADD_CW);//OLED地址
	IIC_Send_Byte(OLED_ADD_CMD);//寄存器地址
	IIC_Send_Byte(command);
	IIC_Stop();
}

//写【数据函数】
void OLED_Data(u8 data) {
	IIC_Start();
	IIC_Send_Byte(OLED_ADD_CW);//OLED地址
	IIC_Send_Byte(OLED_ADD_DATA);//寄存器地址
	IIC_Send_Byte(data);
	IIC_Stop();
}

/*//【连续写数据函数】
//【data】连续写的数据
//【num】连续写的次数
void OLED_CoData(u8 data,u8 num) {
	IIC_Start();
	IIC_Send_Byte(OLED_ADD_CW);//OLED地址
	IIC_Send_Byte(OLED_ADD_CODATA);//寄存器地址
	for (u8 i = 0; i < num; i++) {
		IIC_Send_Byte(data);
	}
	IIC_Stop();
}好像没用，不知道为什么*/

/*OLED的【控制字】说明*/
enum OLED_Control {//枚举定义部分控制字

	OLED_CTR_ContrastSetting = 0x81,	//10 000001 #双字节命令【对比度设置】+对比度的值（8位）A[7:0]

	OLED_CTR_DisplayOn = 0xAF,			//10 10111 1 【显示设置】开启显示
	OLED_CTR_DisplayOff = 0xAE,			//10 10111 0 【显示设置】关闭显示
	OLED_CTR_DisplayEntireOn = 0xA5,	//10 10010 1 【整体显示】开启-会忽略RAM
	OLED_CTR_DisplayEntireOff = 0xA4,	//10 10010 0 【整体显示】关闭-（默认）
	OLED_CTR_RCCSetting = 0xD5,			//11010101‬ #双字节命令 设置时钟【分频因子】,【震荡频率】 + A[3:0],分频因子;A[7:4],震荡频率 ，默认0x80
	OLED_CTR_ReverseOn = 0xA7,			//10 10011 1 【负片显示】-开启反相
	OLED_CTR_ReverseOff = 0xA6,			//10 10011 0 【正常显示】-关闭反相(进行重映射)
	OLED_CTR_ReverseUDOn = 0xC0,		//1100 0 00‬0 【上下反置】-启用
	OLED_CTR_ReverseUDOff = 0xC8,		//1100 1 000‬ 【上下反置】-正常(进行重映射)

//用于改变屏幕数据列地址和段驱动器间的映射关系,此命令只影响其后的数据输入, 已存储在GDDRAM中的数据将保持不变。
	OLED_CTR_SegRemapSet = 0xA1,		//1010 000 1 【段重映射】-【列地址127】 定向到 【段0】,即左右反置
	OLED_CTR_SegRemapReset = 0xA0,		//1010 000 0‬ 【段重映射】-（Reset）【列地址0】 定向到 【段0】
	OLED_CTR_ReverseLROn = 0xA0,		//【左右正常】
	OLED_CTR_ReverseLROff = 0xA1,		//【左右反置】
	
	OLED_CTR_ChargePumpSetting = 0x8D,	//10 001101 【电荷泵设置】+电荷泵开关命令
	OLED_CTR_ChargePumpOn = 0x14,		//** 010 1 00 【↑开】
	OLED_CTR_ChargePumpOff = 0x10, 		//** 010 0 00 【↑关】

	OLED_CTR_RollHorizontal_L = 0x27,	//0010 011 1 #五字节命令【水平左滚】+空字节A[7:0]+开始页地址B[2:0]+时间间隔C[2:0]+结束页地址D[2:0]
	OLED_CTR_RollHorizontal_R = 0x26,	//0010 011 0 #五字节命令【水平右滚】+空字节A[7:0]+开始页地址B[2:0]+时间间隔C[2:0]+结束页地址D[2:0]
	OLED_CTR_RollVH_L = 0x2A,			//0010 10 10 #六字节命令【垂直+水平左滚】+空字节A[7:0]+开始页地址B[2:0]+时间间隔C[2:0]+结束页地址D[2:0]+垂直滚动的位移E[5:0]
	OLED_CTR_RollVH_R = 0x29,			//0010 10 01 #六字节命令【垂直+水平右滚】+空字节A[7:0]+开始页地址B[2:0]+时间间隔C[2:0]+结束页地址D[2:0]+垂直滚动的位移E[5:0]
	OLED_CTR_RollDisable = 0x2E,		//0010 111 0 【关闭滚屏】，关闭滚屏命令后，滚屏命令需要重写
	OLED_CTR_RollEnable = 0x2F,		//0010 111 1 【激活滚屏】，按命令顺序0x26、0x27、0x29、0x2A +【0x2F】，或单个激活
	/*OLED_CTR_RollVerticalAera=0xA3//太复杂，不记了*/


	OLED_CTR_AddColBeginLow = 0x00,	//0000 XXXX【设置作为页地址的开始地址列的低地址】+X[3:0](复位后为0x0)
	OLED_CTR_AddColBeginHigh = 0x10,//0001 XXXX【设置作为页地址的开始地址列的高地址】+X[3:0](复位后为0x0)

/*	00100000 #双字节命令【设置内存地址模式】+模式A[1:0]	（0x00=Horizontal; 0x01=Vertical; 0x03=Page）
	00,【Horizontal Addressing Mode】;从左到右，然后是从上到下，#会换行
	01,【Vertical Addressing Mode】;从上到下，然后是从左到右，#会换列
	10,【Page Addressing Mode】(RESET);从左到右，不自动切换页，需要自己切换（OLED_CTR_ADDPageBase）
		Users have to set the new page and column addresses in order to access the next page RAM content
	11,Invalid
*/	
	OLED_CTR_AddRAM_ModeSetting = 0x20,

	OLED_CTR_ADDRowSetting = 0x21,		//00100001#三字节命令【设置列地址范围】+列开始地址A[2:0]+列结束地址B[2:0]
	OLED_CTR_ADDPageSetting = 0x22,		//00100010#三字节命令【设置页地址范围】+页开始地址A[2:0]+页结束地址B[2:0]
	
	OLED_CTR_ADDPageBase = 0xB0			//10 110 XXX	【转到GDDRAM页地址】+(直接加)页数X[2:0]，即0~8

}OLED_CTR;


//【初始化函数】
//初始化函数里，上下是颠倒的，但是每个单元格的位的上下顺序是保持不变的，所以为了可以正常显示，需要使用【逆向字库】（其实左右也是颠倒的）
void OLED_Init(void) {
	IIC_Init();//开启GPIO

	OLED_Cmd(OLED_CTR_DisplayOff);		//【显示设置】关闭显示

	OLED_Cmd(OLED_CTR_AddRAM_ModeSetting); //【设置内存地址模式】+ 模式A[1:0]
	OLED_Cmd(0x00); 
	
	OLED_Cmd(OLED_CTR_ADDPageBase + 0); //Set Page Start Address for Page Addressing Mode,0-7
//	OLED_Cmd(0xC8); //1100 1000 Set COM Output Scan Direction
	OLED_Cmd(OLED_CTR_AddColBeginLow + 0); //【设置作为页地址中的列的开始低地址】+X[3:0]
	OLED_Cmd(OLED_CTR_AddColBeginHigh + 0); //【设置作为页地址中的列的开始高地址】+X[3:0]
	OLED_Cmd(0x40); //--set start line address
	OLED_Cmd(OLED_CTR_ContrastSetting); //【对比度设置】+对比度的值（8位）A[7:0]
	OLED_Cmd(0xFF);						//亮度调节 0x00~0xff
   
	OLED_Cmd(OLED_CTR_RCCSetting); //设置时钟【分频因子】, 【震荡频率】
	OLED_Cmd(0xF0);//A[3:0],分频因子;A[7:4],震荡频率 ，默认0x80

	OLED_Cmd(OLED_CTR_ReverseUDOff); //0xC0上下反置 0xC8正常
	OLED_Cmd(OLED_CTR_ReverseLROff); //0xA0左右反置 0xA1正常
	OLED_Cmd(OLED_CTR_ReverseOff); //【正常显示】-关闭反相

	OLED_Cmd(0xA8); //--set multiplex ratio(1 to 64)
	OLED_Cmd(0x3F); //设置为--1/64 duty

	OLED_Cmd(OLED_CTR_DisplayEntireOff); // Disable 整体显示（忽略RAM） On (0xa4/0xa5)

	OLED_Cmd(0xD3); //-set display offset
	OLED_Cmd(0x00); //-not offset

	OLED_Cmd(0xD9); //--set 预充电周期
	OLED_Cmd(0xF1); //官方推荐值

	OLED_Cmd(0xda); //--set com pins hardware configuration
	OLED_Cmd(0x12);
	OLED_Cmd(0xdb); //--set vcomh
	OLED_Cmd(0x20); //0x20,0.77xVcc
	OLED_Cmd(OLED_CTR_ChargePumpSetting);	//--电荷泵设置
	OLED_Cmd(OLED_CTR_ChargePumpOn);		//↑开
	OLED_Cmd(OLED_CTR_DisplayOn);			//--开启显示


	for(int a=0;a<=3;a++){
		LED2 = 0;
		delay_ms(50);
		LED2 = 1;
		delay_ms(60);
	}
	printf("<OLED> initialized success!\r");
}

//OLED_ON，将OLED从休眠中【唤醒】
void OLED_On(void) {
	OLED_Cmd(0X8D);  //设置电荷泵
	OLED_Cmd(0X14);  //开启电荷泵
	OLED_Cmd(0XAF);  //OLED唤醒
}
//【填充函数】OLED_Fill，填充整个屏幕，fill_Data:要填充的数据
void OLED_Fill(unsigned char fill_Data) {//全屏填充
	unsigned char m, n;
	for (m = 0; m < 8; m++) {	
		OLED_Cmd(OLED_CTR_ADDPageBase + m);       //page0-page1
		OLED_Cmd(OLED_CTR_AddColBeginLow + 0);		//low column start address
		OLED_Cmd(OLED_CTR_AddColBeginHigh + 0);		//high column start address
		for (n = 0; n < 128; n++) {//写完页[m]
			OLED_Data(fill_Data);
		}
	}
}


//【设置光标】 光标x【列位置】，光标p【页位置】
void OLED_SetPos(unsigned char x, /*页*/unsigned char p) {//设置起始点坐标
	OLED_Cmd(OLED_CTR_ADDPageBase + p); //【设置GDDRAM页地址】 + (直接加)页数X[2:0]，即0~8
	//OLED_CTR_AddRowBeginHigh = 0x10,	//0001 XXXX【设置列的高地址作为页地址的开始地址】+X[3:0](复位后为0x0)
	OLED_Cmd(((x & 0xF0) >> 4) | 0x10);	//取高字节，= 0 0 0 1 X7 X6 X5 X4
	//OLED_CTR_AddRowBeginLow = 0x00,	//0000 XXXX【设置列的低地址作为页地址的开始地址】+X[3:0](复位后为0x0)
	OLED_Cmd((x & 0x0F) /* | 0x01*/);	//取低字节，= 0 0 0 0 X3 X2 X1 X0
}

//【刷新显存】刷新整个屏幕
void OLED_Refresh(void) {
	for (int i = 0; i < 8; i++) {
		OLED_Cmd(OLED_CTR_ADDPageBase + i);
		OLED_Cmd(OLED_CTR_AddColBeginLow + 0);
		OLED_Cmd(OLED_CTR_AddColBeginHigh + 0);
		for (int j = 0; j < 128; j++) {
			OLED_Data(OLED_GRAM[j][i]);
		}
	}
	printf("OLED refreshed!\r");
}
//【打点函数】（在显存中打点）
void OLED_DrawPoint(u8 x, u8 y, unsigned char mode) {
		u8 bit, page, temp = 0;
		if (x > 127 || y > 63) return;//超出范围了.

		page = y / 8;			//计算出页数
		bit =  y % 8;		//计算出位数，一页的一个列中，高位在上面，低位在下面，但y是从上到下的
		temp = 1 << bit;
		if (mode)
			OLED_GRAM[x][page] |= temp;
		else
			OLED_GRAM[x][page] &= ~temp;
}
/**
 * 【打印字符函数】
 * 在指定位置显示字符
 * x:0~127
 * y:0~63
 * mode: 0:反白显示 1:正常显示
 * size: 字号 12/16/24
**/
void OLED_DrawChar(u8 x, u8 y, u8 chr, u8 size, u8 mode) {
	const unsigned char* font;
	u8 temp = 0;
	u8 t, t1;
	u8 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);//字符对应点阵字节数
	chr -= ' ';//得到在字库码表中的位置索引
	//匹配到字库
		switch (size) {
		case 12:
			font = asc2_1206[chr];// 1206字体
			break;
		case 16:
			font = asc2_1608[chr];// 1608字体
			break;
		case 24:
			font = asc2_2412[chr];// 2412字体
			break;
		default:
			printf("Worning: No such font!\n");
			return;					 // 未匹配到字库-放弃
		}
	for (t = 0; t < csize; t++) {//t表示字体的第几个字节
		//画点
		temp = font[t];
		for (t1 = 0; t1 < 8; t1++) {//t1表示该字节的第几个位
			if (temp & 0x01)
				OLED_DrawPoint(x, y, mode);
			else
				OLED_DrawPoint(x, y, !mode);
			temp >>= 1;
			y++;
			//判断是否画到了最底部 - 如1206,到达底部后,当前字节未画完,换新列继续画
			if ((y - y0) == size) {
				y = y0;
				x++;
				break;
			}
			
		}
	}
//	OLED_Refresh();
}

//写字符串函数
void OLED_DrawStr(u8 x, u8 y,  char *str, u8 size, u8 mode) {
	while (*str != '\0') {
		OLED_DrawChar(x, y, *str, size, mode);
		x += size/2;
		str++;
	}
	OLED_Refresh();
}
//写中文字符函数
void OLED_ShowGBK(u8 x, u8 y, u8 num, u8 size, u8 mode) {
	const unsigned char* font;
	u8 temp, t, t1;
	u8 y0 = y;
	//u8 size = 16;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * size;     //得到字体一个字符对应点阵集所占的字节数
	switch (size) {
	case 12:
		font = gbk_1212[num];//调用1212字体
		break;
	case 16:
		font = gbk_1616[num];//调用1616字体
		break;
	case 24:
		font = gbk_2424[num];//调用2424字体
		break;
	default:
		printf("警告：没有该中文字！\n");
		return;					 // 未匹配到字库-放弃
	}
	for (t = 0; t < csize; t++)	{
		temp = font[t];
		for (t1 = 0; t1 < 8; t1++) {
			if (temp & 0x80)
				OLED_DrawPoint(x, y, mode);
			else 
				OLED_DrawPoint(x, y, !mode);
			temp <<= 1;
			y++;
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				break;
			}
		}
	}
	OLED_Refresh();
}
//写中文字符串【未完善......】
/*void OLED_DrawString(u8 x, u8 y, char* str, u8 size, u8 mode) {
	while (*str != '\0') {
		OLED_DrawChar(x, y, *str, size, mode);
		x += size ;	//中文字库
		str++;
	}
}
*/

//【清屏】
void OLED_Clear(void) {
	OLED_Fill(0x00);
}



