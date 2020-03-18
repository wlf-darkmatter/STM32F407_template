#include <TFT_LCD_2.8in.h>	
#include "stm32f4xx_fsmc.h"//因为要使用到SRAM的外设来快速控制LCD（其实已经被包含）
#include <delay.h>
#include "usart.h"

void LCD_Init(void){
	LCD_TFTPin_Init();
	LCD_FSMC_Init();

	/*******************/

	/*****************/
	delay_ms(30);
	LCD_WriteReg(0x0000, 0x0001);
	delay_ms(50); // delay 50 ms
	printf("%d", LCD_ReadReg(0x0000));
}

//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue) {
	LCD->LCD_REG = LCD_Reg;		//写入要写的寄存器序号	 
	LCD->LCD_RAM = LCD_RegValue;//写入数据	    		 
}

//读寄存器
//LCD_Reg:寄存器地址
//返回值:读到的数据
u16 LCD_ReadReg(u16 LCD_Reg) {
	LCD->LCD_REG = LCD_Reg;	//写入要读的寄存器序号
	delay_us(5);
	return LCD->LCD_RAM;	//返回读到的值
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
	//PB0
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &LCD_Pin_InitStructure);//初始化

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


