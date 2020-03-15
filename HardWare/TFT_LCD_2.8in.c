#include <TFT_LCD_2.8in.h>	
#include "stm32f4xx_fsmc.h"//因为要使用到SRAM的外设来快速控制LCD（其实已经被包含）
#include <delay.h>
#include "usart.h"
void LCD_Init(void){

	



//	FSMC_NORSRAMInit();
	}

void LCD_FSMC_Init(void) {
	
	FSMC_NORSRAMInitTypeDef LCD_FSMC_InitStructure;

	LCD_FSMC_InitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;//使用Bank1的储存块4
	LCD_FSMC_InitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;//储存模式为SRAM
	LCD_FSMC_InitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;//使用拓展功能
	LCD_FSMC_InitStructure.FSMC_MemoryDataWidth= FSMC_MemoryDataWidth_16b;//使用的是16位
	LCD_FSMC_InitStructure.FSMC_WriteOperation=FSMC_WriteOperation_Enable;//使能【写】
	LCD_FSMC_InitStructure.FSMC_DataAddressMux=FSMC_DataAddressMux_Disable;//地址位和数据位复用吗?【不复用】
	FSMC_NORSRAMInit(&LCD_FSMC_InitStructure);


	FSMC_NORSRAMTimingInitTypeDef LCD_FSMC_RWT_Structure;//Read & Write Timing——【FSMC_BTRx】
	FSMC_NORSRAMTimingInitTypeDef LCD_FSMC_WT_Structure;//Write Timing——【FSMC_BWTRx】

	



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


