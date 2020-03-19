#include <TFT_LCD.h>	
#include "stdlib.h"

#include <delay.h>
#include "usart.h"


//LCD的画笔颜色和背景色	   
u16 POINT_COLOR = 0x0000;	//画笔颜色
u16 BACK_COLOR = 0xFFFF;  //背景色 

//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;

//写寄存器函数
//regval:寄存器值
void LCD_WR_REG(vu16 regval)
{
	regval = regval;		//使用-O2优化的时候,必须插入的延时
	LCD->LCD_REG = regval;//写入要写的寄存器序号	 
}
//写LCD数据
//data:要写入的值
void LCD_WR_DATA(vu16 data)
{
	data = data;			//使用-O2优化的时候,必须插入的延时
	LCD->LCD_RAM = data;
}
//读LCD数据
//返回值:读到的值
u16 LCD_RD_DATA(void)
{
	vu16 ram;			//防止被优化
	ram = LCD->LCD_RAM;
	return ram;
}
//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{
	LCD->LCD_REG = LCD_Reg;		//写入要写的寄存器序号	 
	LCD->LCD_RAM = LCD_RegValue;//写入数据	    		 
}
//读寄存器
//LCD_Reg:寄存器地址
//返回值:读到的数据
u16 LCD_ReadReg(u16 LCD_Reg)
{
	LCD_WR_REG(LCD_Reg);		//写入要读的寄存器序号
	delay_us(5);
	return LCD_RD_DATA();		//返回读到的值
}

void LCD_Init(void){
	vu32 i=0;
//	LCD_TFTPin_Init();
//	LCD_FSMC_Init();

	/*******************/
	LCD_TFTPin_Init();
	LCD_FSMC_Init();
	/*****************/
	delay_ms(30);
	LCD_WriteReg(0x0000, 0x0001);
	delay_ms(50); // delay 50 ms
	printf("%d", LCD_ReadReg(0x0000));
	delay_ms(50);

	lcddev.id = LCD_ReadReg(0x0000);


	if (lcddev.id < 0XFF || lcddev.id == 0XFFFF || lcddev.id == 0X9300)//读到ID不正确,新增lcddev.id==0X9300判断，因为9341在未被复位的情况下会被读成9300
	{
		//尝试9341 ID的读取		
		LCD_WR_REG(0XD3);
		lcddev.id = LCD_RD_DATA();	//dummy read 	
		lcddev.id = LCD_RD_DATA();	//读到0X00
		lcddev.id = LCD_RD_DATA();   	//读取93								   
		lcddev.id <<= 8;
		lcddev.id |= LCD_RD_DATA();  	//读取41 	   			   
		if (lcddev.id != 0X9341)		//非9341,尝试是不是6804
		{
			LCD_WR_REG(0XBF);
			lcddev.id = LCD_RD_DATA(); 	//dummy read 	 
			lcddev.id = LCD_RD_DATA();   	//读回0X01			   
			lcddev.id = LCD_RD_DATA(); 	//读回0XD0 			  	
			lcddev.id = LCD_RD_DATA();	//这里读回0X68 
			lcddev.id <<= 8;
			lcddev.id |= LCD_RD_DATA();	//这里读回0X04	  
			if (lcddev.id != 0X6804)		//也不是6804,尝试看看是不是NT35310
			{
				LCD_WR_REG(0XD4);
				lcddev.id = LCD_RD_DATA();//dummy read  
				lcddev.id = LCD_RD_DATA();//读回0X01	 
				lcddev.id = LCD_RD_DATA();//读回0X53	
				lcddev.id <<= 8;
				lcddev.id |= LCD_RD_DATA();	//这里读回0X10	 
				if (lcddev.id != 0X5310)		//也不是NT35310,尝试看看是不是NT35510
				{
					LCD_WR_REG(0XDA00);
					lcddev.id = LCD_RD_DATA();		//读回0X00	 
					LCD_WR_REG(0XDB00);
					lcddev.id = LCD_RD_DATA();		//读回0X80
					lcddev.id <<= 8;
					LCD_WR_REG(0XDC00);
					lcddev.id |= LCD_RD_DATA();		//读回0X00		
					if (lcddev.id == 0x8000)lcddev.id = 0x5510;//NT35510读回的ID是8000H,为方便区分,我们强制设置为5510
					if (lcddev.id != 0X5510)			//也不是NT5510,尝试看看是不是SSD1963
					{
						LCD_WR_REG(0XA1);
						lcddev.id = LCD_RD_DATA();
						lcddev.id = LCD_RD_DATA();	//读回0X57
						lcddev.id <<= 8;
						lcddev.id |= LCD_RD_DATA();	//读回0X61	
						if (lcddev.id == 0X5761)lcddev.id = 0X1963;//SSD1963读回的ID是5761H,为方便区分,我们强制设置为1963
					}
				}
			}
		}
	}
	if (lcddev.id == 0X9341 || lcddev.id == 0X5310 || lcddev.id == 0X5510 || lcddev.id == 0X1963)//如果是这几个IC,则设置WR时序为最快
	{
		//重新配置写时序控制寄存器的时序   	 							    
		FSMC_Bank1E->BWTR[6] &= ~(0XF << 0);//地址建立时间(ADDSET)清零 	 
		FSMC_Bank1E->BWTR[6] &= ~(0XF << 8);//数据保存时间清零
		FSMC_Bank1E->BWTR[6] |= 3 << 0;		//地址建立时间(ADDSET)为3个HCLK =18ns  	 
		FSMC_Bank1E->BWTR[6] |= 2 << 8; 	//数据保存时间(DATAST)为6ns*3个HCLK=18ns
	}
	else if (lcddev.id == 0X6804 || lcddev.id == 0XC505)	//6804/C505速度上不去,得降低
	{
		//重新配置写时序控制寄存器的时序   	 							    
		FSMC_Bank1E->BWTR[6] &= ~(0XF << 0);//地址建立时间(ADDSET)清零 	 
		FSMC_Bank1E->BWTR[6] &= ~(0XF << 8);//数据保存时间清零
		FSMC_Bank1E->BWTR[6] |= 10 << 0;	//地址建立时间(ADDSET)为10个HCLK =60ns  	 
		FSMC_Bank1E->BWTR[6] |= 12 << 8; 	//数据保存时间(DATAST)为6ns*13个HCLK=78ns
	}
	printf(" LCD ID:%x\r\n", lcddev.id); //打印LCD ID   

	if (lcddev.id == 0X9341)	//9341初始化
	{
		LCD_WR_REG(0xCF);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0xC1);
		LCD_WR_DATA(0X30);
		LCD_WR_REG(0xED);
		LCD_WR_DATA(0x64);
		LCD_WR_DATA(0x03);
		LCD_WR_DATA(0X12);
		LCD_WR_DATA(0X81);
		LCD_WR_REG(0xE8);
		LCD_WR_DATA(0x85);
		LCD_WR_DATA(0x10);
		LCD_WR_DATA(0x7A);
		LCD_WR_REG(0xCB);
		LCD_WR_DATA(0x39);
		LCD_WR_DATA(0x2C);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x34);
		LCD_WR_DATA(0x02);
		LCD_WR_REG(0xF7);
		LCD_WR_DATA(0x20);
		LCD_WR_REG(0xEA);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_REG(0xC0);    //Power control 
		LCD_WR_DATA(0x1B);   //VRH[5:0] 
		LCD_WR_REG(0xC1);    //Power control 
		LCD_WR_DATA(0x01);   //SAP[2:0];BT[3:0] 
		LCD_WR_REG(0xC5);    //VCM control 
		LCD_WR_DATA(0x30); 	 //3F
		LCD_WR_DATA(0x30); 	 //3C
		LCD_WR_REG(0xC7);    //VCM control2 
		LCD_WR_DATA(0XB7);
		LCD_WR_REG(0x36);    // Memory Access Control 
		LCD_WR_DATA(0x48);
		LCD_WR_REG(0x3A);
		LCD_WR_DATA(0x55);
		LCD_WR_REG(0xB1);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x1A);
		LCD_WR_REG(0xB6);    // Display Function Control 
		LCD_WR_DATA(0x0A);
		LCD_WR_DATA(0xA2);
		LCD_WR_REG(0xF2);    // 3Gamma Function Disable 
		LCD_WR_DATA(0x00);
		LCD_WR_REG(0x26);    //Gamma curve selected 
		LCD_WR_DATA(0x01);
		LCD_WR_REG(0xE0);    //Set Gamma 
		LCD_WR_DATA(0x0F);
		LCD_WR_DATA(0x2A);
		LCD_WR_DATA(0x28);
		LCD_WR_DATA(0x08);
		LCD_WR_DATA(0x0E);
		LCD_WR_DATA(0x08);
		LCD_WR_DATA(0x54);
		LCD_WR_DATA(0XA9);
		LCD_WR_DATA(0x43);
		LCD_WR_DATA(0x0A);
		LCD_WR_DATA(0x0F);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_REG(0XE1);    //Set Gamma 
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x15);
		LCD_WR_DATA(0x17);
		LCD_WR_DATA(0x07);
		LCD_WR_DATA(0x11);
		LCD_WR_DATA(0x06);
		LCD_WR_DATA(0x2B);
		LCD_WR_DATA(0x56);
		LCD_WR_DATA(0x3C);
		LCD_WR_DATA(0x05);
		LCD_WR_DATA(0x10);
		LCD_WR_DATA(0x0F);
		LCD_WR_DATA(0x3F);
		LCD_WR_DATA(0x3F);
		LCD_WR_DATA(0x0F);
		LCD_WR_REG(0x2B);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x01);
		LCD_WR_DATA(0x3f);
		LCD_WR_REG(0x2A);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0xef);
		LCD_WR_REG(0x11); //Exit Sleep
		delay_ms(120);
		LCD_WR_REG(0x29); //display on	
	}

	LCD_Display_Dir(0);		//默认为竖屏
	LCD_LED = 1;				//点亮背光
	LCD_Clear(WHITE);
}

void LCD_FSMC_Init(void) {


	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);//使能FSMC时钟

	FSMC_NORSRAMInitTypeDef LCD_FSMC_InitStructure;

	LCD_FSMC_InitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;//使用Bank1的储存块4
	LCD_FSMC_InitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;//储存模式为SRAM
	LCD_FSMC_InitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;//使用拓展功能,读写使用不同的时序
	LCD_FSMC_InitStructure.FSMC_MemoryDataWidth= FSMC_MemoryDataWidth_16b;//使用的是16位
	LCD_FSMC_InitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;//使能【写】
	LCD_FSMC_InitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;//地址位和数据位复用吗?【不复用】
	LCD_FSMC_InitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
	LCD_FSMC_InitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;//等待信号极性=【低】
	LCD_FSMC_InitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;//异步等待=【不可用】
	LCD_FSMC_InitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	LCD_FSMC_InitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	LCD_FSMC_InitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	LCD_FSMC_InitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;

	
	//配置读写模式相关的结构体【FSMC_NORSRAMTimingInitTypeDef】【FSMC_NORSRAMTimingInitTypeDef】
	FSMC_NORSRAMTimingInitTypeDef  LCD_FSMC_RWT_Structure;//Read & Write Timing——【FSMC_BTRx】
	FSMC_NORSRAMTimingInitTypeDef  LCD_FSMC_WT_Structure;//Write Timing——【FSMC_BWTRx】
	
		LCD_FSMC_RWT_Structure.FSMC_AddressSetupTime = 0XF;		//地址建立时间（ADDSET）为16个HCLK 1/168M=6ns*16=96ns
		LCD_FSMC_RWT_Structure.FSMC_AddressHoldTime = 0x00;		//地址保持时间（ADDHLD）模式A未用到	
		LCD_FSMC_RWT_Structure.FSMC_DataSetupTime = 60;			//数据保存时间为60个HCLK	=6*60=360ns
		LCD_FSMC_RWT_Structure.FSMC_BusTurnAroundDuration = 0x00;
		LCD_FSMC_RWT_Structure.FSMC_CLKDivision = 0x00;
		LCD_FSMC_RWT_Structure.FSMC_DataLatency = 0x00;
		LCD_FSMC_RWT_Structure.FSMC_AccessMode = FSMC_AccessMode_A;	//模式A 

		LCD_FSMC_WT_Structure.FSMC_AddressSetupTime = 9;		//地址建立时间（ADDSET）为9个HCLK =54ns 
		LCD_FSMC_WT_Structure.FSMC_AddressHoldTime = 0x00;	//地址保持时间（A		
		LCD_FSMC_WT_Structure.FSMC_DataSetupTime = 8;			//数据保存时间为6ns*9个HCLK=54ns
		LCD_FSMC_WT_Structure.FSMC_BusTurnAroundDuration = 0x00;
		LCD_FSMC_WT_Structure.FSMC_CLKDivision = 0x00;
		LCD_FSMC_WT_Structure.FSMC_DataLatency = 0x00;
		LCD_FSMC_WT_Structure.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 
	

	LCD_FSMC_InitStructure.FSMC_ReadWriteTimingStruct = &LCD_FSMC_RWT_Structure;	//读写时序
	LCD_FSMC_InitStructure.FSMC_WriteTimingStruct = &LCD_FSMC_WT_Structure;			//写时序

	FSMC_NORSRAMInit(&LCD_FSMC_InitStructure);//初始化FSMC配置

	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  // 使能BANK1 

}
void LCD_TFTPin_Init(void) {

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);//使能PD,PE,PF,PG时钟  

	GPIO_InitTypeDef LCD_Pin_InitStructure;

	/******************************************/
	//【地址线】A0-A15
	/******************************************/
	//PF0、PF1、PF2、PF3、PF4、PF5、  PF12、PF13、PF14、PF15
	//A0、 A1、 A2、 A3、 A4、 A5、   A6、  A7、  A8、  A9
/*	
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOF, &LCD_Pin_InitStructure);//初始化 
	//PG0、PG1、PG2、PG3、PG4、PG5
	//A10、A11、A12、A13、A14、A15
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//初始化 
*/
	//PF12
	//A6 = RS
	LCD_Pin_InitStructure.GPIO_Pin =  GPIO_Pin_12 ;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOF, &LCD_Pin_InitStructure);//初始化 

	/************************************/
	//【数据线】D0-D15
	/************************************/

	//###【D】
	//PD14 PD15 PD0 PD1 
	//D0   D1   D2  D3
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//初始化

	//###【E】
	//PE7  PE8  PE9  PE10 PE11 PE12 PF13 PF14 PF15
	//D4   D5   D6   D7   D8   D9   D10  D11  D12
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOE, &LCD_Pin_InitStructure);//初始化

	//###【D】
	//PD8 PD9 PD10 
	//D13 D14 D15  
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//初始化

	/*******【指令控制引脚部分】****************/
	//PD4 PD5 PD6	
	//NOE NWE NWAIT
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 /*| GPIO_Pin_6*/;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//初始化
	//NE4
	//PG12
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_12;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//初始化


	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_15;//PB15 推挽输出,控制背光
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &LCD_Pin_InitStructure);//初始化 //PB15 推挽输出,控制背光


//复用部分
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FSMC);//PD14——	D0
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FSMC);//PD15——	D1
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_FSMC);//PD0——	D2
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_FSMC);//PD1——	D3
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_FSMC);//PE7——	D4
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_FSMC);//PE8——	D5
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_FSMC);//PE9——	D6
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_FSMC);//PE10——	D7
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_FSMC);//PE11——	D8
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_FSMC);//PE12——	D9
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_FSMC);//PE13——	D10
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_FSMC);//PE14——	D11
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_FSMC);//PE15——	D12
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_FSMC);//PD8——	D13
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_FSMC);//PD9——	D14
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FSMC);//PD10——	D15

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_FSMC);//PD4——	NOE
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_FSMC);//PD5——	NWE

	GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_FSMC);//PF12——	A6 = RS
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource12, GPIO_AF_FSMC);//PG12——	NE4 = CS

	


}


void TFT_PinDetect(FunctionalState NewState) {
	if (NewState == DISABLE) {
		return;
	}
	else if (NewState == ENABLE) {
		GPIO_InitTypeDef LCD_Pin_InitStructure;
		//检测【地址线】A0-A15
		//A0、 A1、 A2、 A3、 A4、 A5、   A6、  A7、  A8、  A9
		//PF0、PF1、PF2、PF3、PF4、PF5、  PF12、PF13、PF14、PF15
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5 |GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOF, &LCD_Pin_InitStructure);//初始化 
		//A10、A11、A12、A13、A14、A15
		//PG0、PG1、PG2、PG3、PG4、PG5
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1| GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 ;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//初始化 

		//FSMC NE4
		//PG12
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_12;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//初始化 


		//检测【数据线】D0-D7
		//PD14 PD15 PD0 PD1
		//D0   D1   D2  D3
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//初始化
		//PE7  PE8  PE9  PE10
		//D4   D5   D6   D7
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOE, &LCD_Pin_InitStructure);//初始化


		//PB0
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOB, &LCD_Pin_InitStructure);//初始化 
		//PD4 PD5
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_4| GPIO_Pin_5;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//初始化 

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOG, ENABLE);

	}
	while (1) {
		//PB0——TFT_PinSTA[0]
		//PD4——TFT_PinSTA[1]
		//PD5——TFT_PinSTA[2]
		//PF12——TFT_PinSTA[3]
		//PG12——TFT_PinSTA[4]
		delay_ms(500);
		if(PBin(0) == 1) printf("PB0");
		if(PDin(4) == 1) printf("PD4");
		if(PDin(5) == 1) printf("PD5");
		if(PFin(12) == 1) printf("PF12");
		if(PGin(12) == 1) printf("PG12");

		//地址
		if(PFin(0) == 1) printf("PF0");
		if(PFin(1) == 1) printf("PF1");
		if(PFin(2) == 1) printf("PF2");
		if(PFin(3) == 1) printf("PF3");
		if(PFin(4) == 1) printf("PF4");
		if(PFin(12) == 1) printf("PF12");
		if(PFin(13) == 1) printf("PF13");
		if(PFin(14) == 1) printf("PF14");
		if(PFin(15) == 1) printf("PF15");
		if(PGin(0) == 1) printf("PG0");
		if(PGin(1) == 1) printf("PG1");
		if(PGin(2) == 1) printf("PG2");
		if(PGin(3) == 1) printf("PG3");
		if(PGin(4) == 1) printf("PG4");
		if(PGin(5) == 1) printf("PG5");

		//数据
		if(PDin(14) == 1) printf("PD14");
		if(PDin(15) == 1) printf("PD15");
		if(PDin(0) == 1) printf("PD0");
		if(PDin(1) == 1) printf("PD1");
		if(PEin(7) == 1) printf("PE7");
		if(PEin(8) == 1) printf("PE8");
		if(PEin(9) == 1) printf("PE9");
		if(PEin(10) == 1) printf("PE10");

	}

}



//开始写GRAM
void LCD_WriteRAM_Prepare(void)
{
	LCD->LCD_REG = lcddev.wramcmd;
}
//LCD写GRAM
//RGB_Code:颜色值
void LCD_WriteRAM(u16 RGB_Code)
{
	LCD->LCD_RAM = RGB_Code;//写十六位GRAM
}
//从ILI93xx读出的数据为GBR格式，而我们写入的时候为RGB格式。
//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
u16 LCD_BGR2RGB(u16 c)
{
	u16  r, g, b, rgb;
	b = (c >> 0) & 0x1f;
	g = (c >> 5) & 0x3f;
	r = (c >> 11) & 0x1f;
	rgb = (b << 11) + (g << 5) + (r << 0);
	return(rgb);
}
//当mdk -O1时间优化时需要设置
//延时i
void opt_delay(u8 i)
{
	while (i--);
}
//读取个某点的颜色值	 
//x,y:坐标
//返回值:此点的颜色
u16 LCD_ReadPoint(u16 x, u16 y)
{
	u16 r = 0, g = 0, b = 0;
	if (x >= lcddev.width || y >= lcddev.height)return 0;	//超过了范围,直接返回		   
	LCD_SetCursor(x, y);
	if (lcddev.id == 0X9341 || lcddev.id == 0X6804 || lcddev.id == 0X5310 || lcddev.id == 0X1963)LCD_WR_REG(0X2E);//9341/6804/3510/1963 发送读GRAM指令
	else if (lcddev.id == 0X5510)LCD_WR_REG(0X2E00);	//5510 发送读GRAM指令
	else LCD_WR_REG(0X22);      		 			//其他IC发送读GRAM指令
	if (lcddev.id == 0X9320)opt_delay(2);				//FOR 9320,延时2us	    
	r = LCD_RD_DATA();								//dummy Read	   
	if (lcddev.id == 0X1963)return r;					//1963直接读就可以 
	opt_delay(2);
	r = LCD_RD_DATA();  		  						//实际坐标颜色
	if (lcddev.id == 0X9341 || lcddev.id == 0X5310 || lcddev.id == 0X5510)		//9341/NT35310/NT35510要分2次读出
	{
		opt_delay(2);
		b = LCD_RD_DATA();
		g = r & 0XFF;		//对于9341/5310/5510,第一次读取的是RG的值,R在前,G在后,各占8位
		g <<= 8;
	}
	if (lcddev.id == 0X9325 || lcddev.id == 0X4535 || lcddev.id == 0X4531 || lcddev.id == 0XB505 || lcddev.id == 0XC505)return r;	//这几种IC直接返回颜色值
	else if (lcddev.id == 0X9341 || lcddev.id == 0X5310 || lcddev.id == 0X5510)return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));//ILI9341/NT35310/NT35510需要公式转换一下
	else return LCD_BGR2RGB(r);						//其他IC
}
//LCD开启显示
void LCD_DisplayOn(void) {
		LCD_WR_REG(0X29);	//开启显示
}
//LCD关闭显示
void LCD_DisplayOff(void) {
	LCD_WR_REG(0X28);	//关闭显示
}

//设置光标位置
//Xpos:横坐标
//Ypos:纵坐标
void LCD_SetCursor(u16 Xpos, u16 Ypos) {
	/*if (lcddev.id == 0X9341 || lcddev.id == 0X5310)
	{*/
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(Xpos >> 8); LCD_WR_DATA(Xpos & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(Ypos >> 8); LCD_WR_DATA(Ypos & 0XFF);
//	}
	/*else if (lcddev.id == 0X6804)
	{
		if (lcddev.dir == 1)Xpos = lcddev.width - 1 - Xpos;//横屏时处理
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(Xpos >> 8); LCD_WR_DATA(Xpos & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(Ypos >> 8); LCD_WR_DATA(Ypos & 0XFF);
	}
	else if (lcddev.id == 0X1963)
	{
		if (lcddev.dir == 0)//x坐标需要变换
		{
			Xpos = lcddev.width - 1 - Xpos;
			LCD_WR_REG(lcddev.setxcmd);
			LCD_WR_DATA(0); LCD_WR_DATA(0);
			LCD_WR_DATA(Xpos >> 8); LCD_WR_DATA(Xpos & 0XFF);
		}
		else
		{
			LCD_WR_REG(lcddev.setxcmd);
			LCD_WR_DATA(Xpos >> 8); LCD_WR_DATA(Xpos & 0XFF);
			LCD_WR_DATA((lcddev.width - 1) >> 8); LCD_WR_DATA((lcddev.width - 1) & 0XFF);
		}
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(Ypos >> 8); LCD_WR_DATA(Ypos & 0XFF);
		LCD_WR_DATA((lcddev.height - 1) >> 8); LCD_WR_DATA((lcddev.height - 1) & 0XFF);

	}
	else if (lcddev.id == 0X5510)
	{
		LCD_WR_REG(lcddev.setxcmd); LCD_WR_DATA(Xpos >> 8);
		LCD_WR_REG(lcddev.setxcmd + 1); LCD_WR_DATA(Xpos & 0XFF);
		LCD_WR_REG(lcddev.setycmd); LCD_WR_DATA(Ypos >> 8);
		LCD_WR_REG(lcddev.setycmd + 1); LCD_WR_DATA(Ypos & 0XFF);
	}
	else
	{
		if (lcddev.dir == 1)Xpos = lcddev.width - 1 - Xpos;//横屏其实就是调转x,y坐标
		LCD_WriteReg(lcddev.setxcmd, Xpos);
		LCD_WriteReg(lcddev.setycmd, Ypos);
	}*/
}
//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/5408/9341/5310/5510/1963等IC已经实际测试	   	   
void LCD_Scan_Dir(u8 dir)
{
	u16 regval = 0;
	u16 dirreg = 0;
	u16 temp;
	if ((lcddev.dir == 1 && lcddev.id != 0X6804 && lcddev.id != 0X1963) || (lcddev.dir == 0 && lcddev.id == 0X1963))//横屏时，对6804和1963不改变扫描方向！竖屏时1963改变方向
	{
		switch (dir)//方向转换
		{
		case 0:dir = 6; break;
		case 1:dir = 7; break;
		case 2:dir = 4; break;
		case 3:dir = 5; break;
		case 4:dir = 1; break;
		case 5:dir = 0; break;
		case 6:dir = 3; break;
		case 7:dir = 2; break;
		}
	}
	if (lcddev.id == 0x9341 || lcddev.id == 0X6804 || lcddev.id == 0X5310 || lcddev.id == 0X5510 || lcddev.id == 0X1963)//9341/6804/5310/5510/1963,特殊处理
	{
		switch (dir)
		{
		case L2R_U2D://从左到右,从上到下
			regval |= (0 << 7) | (0 << 6) | (0 << 5);
			break;
		case L2R_D2U://从左到右,从下到上
			regval |= (1 << 7) | (0 << 6) | (0 << 5);
			break;
		case R2L_U2D://从右到左,从上到下
			regval |= (0 << 7) | (1 << 6) | (0 << 5);
			break;
		case R2L_D2U://从右到左,从下到上
			regval |= (1 << 7) | (1 << 6) | (0 << 5);
			break;
		case U2D_L2R://从上到下,从左到右
			regval |= (0 << 7) | (0 << 6) | (1 << 5);
			break;
		case U2D_R2L://从上到下,从右到左
			regval |= (0 << 7) | (1 << 6) | (1 << 5);
			break;
		case D2U_L2R://从下到上,从左到右
			regval |= (1 << 7) | (0 << 6) | (1 << 5);
			break;
		case D2U_R2L://从下到上,从右到左
			regval |= (1 << 7) | (1 << 6) | (1 << 5);
			break;
		}
		if (lcddev.id == 0X5510)dirreg = 0X3600;
		else dirreg = 0X36;
		if ((lcddev.id != 0X5310) && (lcddev.id != 0X5510) && (lcddev.id != 0X1963))regval |= 0X08;//5310/5510/1963不需要BGR   
		if (lcddev.id == 0X6804)regval |= 0x02;//6804的BIT6和9341的反了	   
		LCD_WriteReg(dirreg, regval);
		if (lcddev.id != 0X1963)//1963不做坐标处理
		{
			if (regval & 0X20)
			{
				if (lcddev.width < lcddev.height)//交换X,Y
				{
					temp = lcddev.width;
					lcddev.width = lcddev.height;
					lcddev.height = temp;
				}
			}
			else
			{
				if (lcddev.width > lcddev.height)//交换X,Y
				{
					temp = lcddev.width;
					lcddev.width = lcddev.height;
					lcddev.height = temp;
				}
			}
		}
		if (lcddev.id == 0X5510)
		{
			LCD_WR_REG(lcddev.setxcmd); LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setxcmd + 1); LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setxcmd + 2); LCD_WR_DATA((lcddev.width - 1) >> 8);
			LCD_WR_REG(lcddev.setxcmd + 3); LCD_WR_DATA((lcddev.width - 1) & 0XFF);
			LCD_WR_REG(lcddev.setycmd); LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setycmd + 1); LCD_WR_DATA(0);
			LCD_WR_REG(lcddev.setycmd + 2); LCD_WR_DATA((lcddev.height - 1) >> 8);
			LCD_WR_REG(lcddev.setycmd + 3); LCD_WR_DATA((lcddev.height - 1) & 0XFF);
		}
		else
		{
			LCD_WR_REG(lcddev.setxcmd);
			LCD_WR_DATA(0); LCD_WR_DATA(0);
			LCD_WR_DATA((lcddev.width - 1) >> 8); LCD_WR_DATA((lcddev.width - 1) & 0XFF);
			LCD_WR_REG(lcddev.setycmd);
			LCD_WR_DATA(0); LCD_WR_DATA(0);
			LCD_WR_DATA((lcddev.height - 1) >> 8); LCD_WR_DATA((lcddev.height - 1) & 0XFF);
		}
	}
	else
	{
		switch (dir)
		{
		case L2R_U2D://从左到右,从上到下
			regval |= (1 << 5) | (1 << 4) | (0 << 3);
			break;
		case L2R_D2U://从左到右,从下到上
			regval |= (0 << 5) | (1 << 4) | (0 << 3);
			break;
		case R2L_U2D://从右到左,从上到下
			regval |= (1 << 5) | (0 << 4) | (0 << 3);
			break;
		case R2L_D2U://从右到左,从下到上
			regval |= (0 << 5) | (0 << 4) | (0 << 3);
			break;
		case U2D_L2R://从上到下,从左到右
			regval |= (1 << 5) | (1 << 4) | (1 << 3);
			break;
		case U2D_R2L://从上到下,从右到左
			regval |= (1 << 5) | (0 << 4) | (1 << 3);
			break;
		case D2U_L2R://从下到上,从左到右
			regval |= (0 << 5) | (1 << 4) | (1 << 3);
			break;
		case D2U_R2L://从下到上,从右到左
			regval |= (0 << 5) | (0 << 4) | (1 << 3);
			break;
		}
		dirreg = 0X03;
		regval |= 1 << 12;
		LCD_WriteReg(dirreg, regval);
	}
}
//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x, u16 y)
{
	LCD_SetCursor(x, y);		//设置光标位置 
	LCD_WriteRAM_Prepare();	//开始写入GRAM
	LCD->LCD_RAM = POINT_COLOR;
}
//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color)
{
	if (lcddev.id == 0X9341 || lcddev.id == 0X5310)
	{
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0XFF);
	}
	else if (lcddev.id == 0X5510)
	{
		LCD_WR_REG(lcddev.setxcmd); LCD_WR_DATA(x >> 8);
		LCD_WR_REG(lcddev.setxcmd + 1); LCD_WR_DATA(x & 0XFF);
		LCD_WR_REG(lcddev.setycmd); LCD_WR_DATA(y >> 8);
		LCD_WR_REG(lcddev.setycmd + 1); LCD_WR_DATA(y & 0XFF);
	}
	else if (lcddev.id == 0X1963)
	{
		if (lcddev.dir == 0)x = lcddev.width - 1 - x;
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0XFF);
		LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0XFF);
		LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0XFF);
	}
	else if (lcddev.id == 0X6804)
	{
		if (lcddev.dir == 1)x = lcddev.width - 1 - x;//横屏时处理
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0XFF);
	}
	else
	{
		if (lcddev.dir == 1)x = lcddev.width - 1 - x;//横屏其实就是调转x,y坐标
		LCD_WriteReg(lcddev.setxcmd, x);
		LCD_WriteReg(lcddev.setycmd, y);
	}
	LCD->LCD_REG = lcddev.wramcmd;
	LCD->LCD_RAM = color;
}
//SSD1963 背光设置
//pwm:背光等级,0~100.越大越亮.
void LCD_SSD_BackLightSet(u8 pwm)
{
	LCD_WR_REG(0xBE);	//配置PWM输出
	LCD_WR_DATA(0x05);	//1设置PWM频率
	LCD_WR_DATA(pwm * 2.55);//2设置PWM占空比
	LCD_WR_DATA(0x01);	//3设置C
	LCD_WR_DATA(0xFF);	//4设置D
	LCD_WR_DATA(0x00);	//5设置E
	LCD_WR_DATA(0x00);	//6设置F
}

//设置LCD显示方向
//dir:0,竖屏；1,横屏
void LCD_Display_Dir(u8 dir)
{
	if (dir == 0)			//竖屏
	{
		lcddev.dir = 0;	//竖屏
		lcddev.width = 240;
		lcddev.height = 320;
		if (lcddev.id == 0X9341 || lcddev.id == 0X6804 || lcddev.id == 0X5310)
		{
			lcddev.wramcmd = 0X2C;
			lcddev.setxcmd = 0X2A;
			lcddev.setycmd = 0X2B;
			if (lcddev.id == 0X6804 || lcddev.id == 0X5310)
			{
				lcddev.width = 320;
				lcddev.height = 480;
			}
		}
		else if (lcddev.id == 0x5510)
		{
			lcddev.wramcmd = 0X2C00;
			lcddev.setxcmd = 0X2A00;
			lcddev.setycmd = 0X2B00;
			lcddev.width = 480;
			lcddev.height = 800;
		}
		else if (lcddev.id == 0X1963)
		{
			lcddev.wramcmd = 0X2C;	//设置写入GRAM的指令 
			lcddev.setxcmd = 0X2B;	//设置写X坐标指令
			lcddev.setycmd = 0X2A;	//设置写Y坐标指令
			lcddev.width = 480;		//设置宽度480
			lcddev.height = 800;		//设置高度800  
		}
		else
		{
			lcddev.wramcmd = 0X22;
			lcddev.setxcmd = 0X20;
			lcddev.setycmd = 0X21;
		}
	}
	else 				//横屏
	{
		lcddev.dir = 1;	//横屏
		lcddev.width = 320;
		lcddev.height = 240;
		if (lcddev.id == 0X9341 || lcddev.id == 0X5310)
		{
			lcddev.wramcmd = 0X2C;
			lcddev.setxcmd = 0X2A;
			lcddev.setycmd = 0X2B;
		}
		else if (lcddev.id == 0X6804)
		{
			lcddev.wramcmd = 0X2C;
			lcddev.setxcmd = 0X2B;
			lcddev.setycmd = 0X2A;
		}
		else if (lcddev.id == 0x5510)
		{
			lcddev.wramcmd = 0X2C00;
			lcddev.setxcmd = 0X2A00;
			lcddev.setycmd = 0X2B00;
			lcddev.width = 800;
			lcddev.height = 480;
		}
		else if (lcddev.id == 0X1963)
		{
			lcddev.wramcmd = 0X2C;	//设置写入GRAM的指令 
			lcddev.setxcmd = 0X2A;	//设置写X坐标指令
			lcddev.setycmd = 0X2B;	//设置写Y坐标指令
			lcddev.width = 800;		//设置宽度800
			lcddev.height = 480;		//设置高度480  
		}
		else
		{
			lcddev.wramcmd = 0X22;
			lcddev.setxcmd = 0X21;
			lcddev.setycmd = 0X20;
		}
		if (lcddev.id == 0X6804 || lcddev.id == 0X5310)
		{
			lcddev.width = 480;
			lcddev.height = 320;
		}
	}
	LCD_Scan_Dir(DFT_SCAN_DIR);	//默认扫描方向
}
//设置窗口,并自动设置画点坐标到窗口左上角(sx,sy).
//sx,sy:窗口起始坐标(左上角)
//width,height:窗口宽度和高度,必须大于0!!
//窗体大小:width*height. 
void LCD_Set_Window(u16 sx, u16 sy, u16 width, u16 height)
{
	u8 hsareg, heareg, vsareg, veareg;
	u16 hsaval, heaval, vsaval, veaval;
	u16 twidth, theight;
	twidth = sx + width - 1;
	theight = sy + height - 1;
	if (lcddev.id == 0X9341 || lcddev.id == 0X5310 || lcddev.id == 0X6804 || (lcddev.dir == 1 && lcddev.id == 0X1963))
	{
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(sx >> 8);
		LCD_WR_DATA(sx & 0XFF);
		LCD_WR_DATA(twidth >> 8);
		LCD_WR_DATA(twidth & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(sy >> 8);
		LCD_WR_DATA(sy & 0XFF);
		LCD_WR_DATA(theight >> 8);
		LCD_WR_DATA(theight & 0XFF);
	}
	else if (lcddev.id == 0X1963)//1963竖屏特殊处理
	{
		sx = lcddev.width - width - sx;
		height = sy + height - 1;
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(sx >> 8);
		LCD_WR_DATA(sx & 0XFF);
		LCD_WR_DATA((sx + width - 1) >> 8);
		LCD_WR_DATA((sx + width - 1) & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(sy >> 8);
		LCD_WR_DATA(sy & 0XFF);
		LCD_WR_DATA(height >> 8);
		LCD_WR_DATA(height & 0XFF);
	}
	else if (lcddev.id == 0X5510)
	{
		LCD_WR_REG(lcddev.setxcmd); LCD_WR_DATA(sx >> 8);
		LCD_WR_REG(lcddev.setxcmd + 1); LCD_WR_DATA(sx & 0XFF);
		LCD_WR_REG(lcddev.setxcmd + 2); LCD_WR_DATA(twidth >> 8);
		LCD_WR_REG(lcddev.setxcmd + 3); LCD_WR_DATA(twidth & 0XFF);
		LCD_WR_REG(lcddev.setycmd); LCD_WR_DATA(sy >> 8);
		LCD_WR_REG(lcddev.setycmd + 1); LCD_WR_DATA(sy & 0XFF);
		LCD_WR_REG(lcddev.setycmd + 2); LCD_WR_DATA(theight >> 8);
		LCD_WR_REG(lcddev.setycmd + 3); LCD_WR_DATA(theight & 0XFF);
	}
	else	//其他驱动IC
	{
		if (lcddev.dir == 1)//横屏
		{
			//窗口值
			hsaval = sy;
			heaval = theight;
			vsaval = lcddev.width - twidth - 1;
			veaval = lcddev.width - sx - 1;
		}
		else
		{
			hsaval = sx;
			heaval = twidth;
			vsaval = sy;
			veaval = theight;
		}
		hsareg = 0X50; heareg = 0X51;//水平方向窗口寄存器
		vsareg = 0X52; veareg = 0X53;//垂直方向窗口寄存器	   							  
		//设置寄存器值
		LCD_WriteReg(hsareg, hsaval);
		LCD_WriteReg(heareg, heaval);
		LCD_WriteReg(vsareg, vsaval);
		LCD_WriteReg(veareg, veaval);
		LCD_SetCursor(sx, sy);	//设置光标位置
	}
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(u16 color)
{
	u32 index = 0;
	u32 totalpoint = lcddev.width;
	totalpoint *= lcddev.height; 			//得到总点数
	if ((lcddev.id == 0X6804) && (lcddev.dir == 1))//6804横屏的时候特殊处理  
	{
		lcddev.dir = 0;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;
		LCD_SetCursor(0x00, 0x0000);		//设置光标位置  
		lcddev.dir = 1;
		lcddev.setxcmd = 0X2B;
		lcddev.setycmd = 0X2A;
	}
	else LCD_SetCursor(0x00, 0x0000);	//设置光标位置 
	LCD_WriteRAM_Prepare();     		//开始写入GRAM	 	  
	for (index = 0; index < totalpoint; index++)
	{
		LCD->LCD_RAM = color;
	}
}
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
	u16 i, j;
	u16 xlen = 0;
	u16 temp;
	if ((lcddev.id == 0X6804) && (lcddev.dir == 1))	//6804横屏的时候特殊处理  
	{
		temp = sx;
		sx = sy;
		sy = lcddev.width - ex - 1;
		ex = ey;
		ey = lcddev.width - temp - 1;
		lcddev.dir = 0;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;
		LCD_Fill(sx, sy, ex, ey, color);
		lcddev.dir = 1;
		lcddev.setxcmd = 0X2B;
		lcddev.setycmd = 0X2A;
	}
	else
	{
		xlen = ex - sx + 1;
		for (i = sy; i <= ey; i++)
		{
			LCD_SetCursor(sx, i);      				//设置光标位置 
			LCD_WriteRAM_Prepare();     			//开始写入GRAM	  
			for (j = 0; j < xlen; j++)LCD->LCD_RAM = color;	//显示颜色 	    
		}
	}
}
//在指定区域内填充指定颜色块			 
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16* color)
{
	u16 height, width;
	u16 i, j;
	width = ex - sx + 1; 			//得到填充的宽度
	height = ey - sy + 1;			//高度
	for (i = 0; i < height; i++)
	{
		LCD_SetCursor(sx, sy + i);   	//设置光标位置 
		LCD_WriteRAM_Prepare();     //开始写入GRAM
		for (j = 0; j < width; j++)LCD->LCD_RAM = color[i * width + j];//写入数据 
	}
}
//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1; //计算坐标增量 
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)incx = 1; //设置单步方向 
	else if (delta_x == 0)incx = 0;//垂直线 
	else { incx = -1; delta_x = -delta_x; }
	if (delta_y > 0)incy = 1;
	else if (delta_y == 0)incy = 0;//水平线 
	else { incy = -1; delta_y = -delta_y; }
	if (delta_x > delta_y)distance = delta_x; //选取基本增量坐标轴 
	else distance = delta_y;
	for (t = 0; t <= distance + 1; t++)//画线输出 
	{
		LCD_DrawPoint(uRow, uCol);//画点 
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}
//画矩形	  
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}
//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0; b = r;
	di = 3 - (r << 1);             //判断下个点位置的标志
	while (a <= b)
	{
		LCD_DrawPoint(x0 + a, y0 - b);             //5
		LCD_DrawPoint(x0 + b, y0 - a);             //0           
		LCD_DrawPoint(x0 + b, y0 + a);             //4               
		LCD_DrawPoint(x0 + a, y0 + b);             //6 
		LCD_DrawPoint(x0 - a, y0 + b);             //1       
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b);             //2             
		LCD_DrawPoint(x0 - b, y0 - a);             //7     	         
		a++;
		//使用Bresenham算法画圆     
		if (di < 0)di += 4 * a + 6;
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(u16 x, u16 y, u8 chr, u8 size, u8 mode) {
	const unsigned char* font;
	u8 temp, t1, t;
	u16 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);		//得到字体一个字符对应点阵集所占的字节数	
	chr -= ' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
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
				LCD_Fast_DrawPoint(x, y, POINT_COLOR);
			else
				LCD_Fast_DrawPoint(x, y, BACK_COLOR);
			temp >>= 1;
			y++;
			//判断是否画到了最底部 - 如1206,到达底部后,当前字节未画完,换新列继续画
			if (y >= lcddev.height) return;//超【底部】区域了
			if ((y - y0) == size) {
				y = y0;
				x++;
				if (x >= lcddev.width)return;	//超【侧边】区域了
				break;
			}
		}
	}
	/*
	for (t = 0; t < csize; t++)	{
		for (t1 = 0; t1 < 8; t1++) {//读取一个字节中的信息
			if (temp & 0x80)LCD_Fast_DrawPoint(x, y, POINT_COLOR);
			else if (mode == 0)LCD_Fast_DrawPoint(x, y, BACK_COLOR);
			temp <<= 1;
			y++;
			if (y >= lcddev.height)return;		//超区域了
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				if (x >= lcddev.width)return;	//超区域了
				break;
			}
		}
	}
	*/
}
//m^n函数
//返回值:m^n次方.
u32 LCD_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)result *= m;
	return result;
}
//显示数字,高位为0,则不显示
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//color:颜色 
//num:数值(0~4294967295);	 
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
	u8 t, temp;
	u8 enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / LCD_Pow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				LCD_ShowChar(x + (size / 2) * t, y, ' ', size, 0);
				continue;
			}
			else enshow = 1;

		}
		LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, 0);
	}
}
//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);	 
//len:长度(即要显示的位数)
//size:字体大小
//mode:
//[7]:0,不填充;1,填充0.
//[6:1]:保留
//[0]:0,非叠加显示;1,叠加显示.
void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode)
{
	u8 t, temp;
	u8 enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / LCD_Pow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				if (mode & 0X80)LCD_ShowChar(x + (size / 2) * t, y, '0', size, mode & 0X01);
				else LCD_ShowChar(x + (size / 2) * t, y, ' ', size, mode & 0X01);
				continue;
			}
			else enshow = 1;

		}
		LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, mode & 0X01);
	}
}
//显示字符串
//x,y:起点坐标
//width,height:区域大小  
//size:字体大小
//*p:字符串起始地址		  
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8* p)
{
	u8 x0 = x;
	width += x;
	height += y;
	while ((*p <= '~') && (*p >= ' '))//判断是不是非法字符!
	{
		if (x >= width) { x = x0; y += size; }
		if (y >= height)break;//退出
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}



























