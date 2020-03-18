#include <TFT_LCD_2.8in.h>	
#include "stm32f4xx_fsmc.h"//��ΪҪʹ�õ�SRAM�����������ٿ���LCD����ʵ�Ѿ���������
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

//д�Ĵ���
//LCD_Reg:�Ĵ�����ַ
//LCD_RegValue:Ҫд�������
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue) {
	LCD->LCD_REG = LCD_Reg;		//д��Ҫд�ļĴ������	 
	LCD->LCD_RAM = LCD_RegValue;//д������	    		 
}

//���Ĵ���
//LCD_Reg:�Ĵ�����ַ
//����ֵ:����������
u16 LCD_ReadReg(u16 LCD_Reg) {
	LCD->LCD_REG = LCD_Reg;	//д��Ҫ���ļĴ������
	delay_us(5);
	return LCD->LCD_RAM;	//���ض�����ֵ
}


void LCD_FSMC_Init(void) {


	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);//ʹ��FSMCʱ��

	FSMC_NORSRAMInitTypeDef LCD_FSMC_InitStructure;

	LCD_FSMC_InitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;//ʹ��Bank1�Ĵ����4
	LCD_FSMC_InitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;//����ģʽΪSRAM
	LCD_FSMC_InitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;//ʹ����չ����,��дʹ�ò�ͬ��ʱ��
	LCD_FSMC_InitStructure.FSMC_MemoryDataWidth= FSMC_MemoryDataWidth_16b;//ʹ�õ���16λ
	LCD_FSMC_InitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;//ʹ�ܡ�д��
	LCD_FSMC_InitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;//��ַλ������λ������?�������á�
	LCD_FSMC_InitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
	LCD_FSMC_InitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;//�ȴ��źż���=���͡�
	LCD_FSMC_InitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;//�첽�ȴ�=�������á�
	LCD_FSMC_InitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	LCD_FSMC_InitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	LCD_FSMC_InitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	LCD_FSMC_InitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;

	
	//���ö�дģʽ��صĽṹ�塾FSMC_NORSRAMTimingInitTypeDef����FSMC_NORSRAMTimingInitTypeDef��
	FSMC_NORSRAMTimingInitTypeDef  LCD_FSMC_RWT_Structure;//Read & Write Timing������FSMC_BTRx��
	FSMC_NORSRAMTimingInitTypeDef  LCD_FSMC_WT_Structure;//Write Timing������FSMC_BWTRx��
	
		LCD_FSMC_RWT_Structure.FSMC_AddressSetupTime = 0XF;		//��ַ����ʱ�䣨ADDSET��Ϊ16��HCLK 1/168M=6ns*16=96ns
		LCD_FSMC_RWT_Structure.FSMC_AddressHoldTime = 0x00;		//��ַ����ʱ�䣨ADDHLD��ģʽAδ�õ�	
		LCD_FSMC_RWT_Structure.FSMC_DataSetupTime = 60;			//���ݱ���ʱ��Ϊ60��HCLK	=6*60=360ns
		LCD_FSMC_RWT_Structure.FSMC_BusTurnAroundDuration = 0x00;
		LCD_FSMC_RWT_Structure.FSMC_CLKDivision = 0x00;
		LCD_FSMC_RWT_Structure.FSMC_DataLatency = 0x00;
		LCD_FSMC_RWT_Structure.FSMC_AccessMode = FSMC_AccessMode_A;	//ģʽA 

		LCD_FSMC_WT_Structure.FSMC_AddressSetupTime = 9;		//��ַ����ʱ�䣨ADDSET��Ϊ9��HCLK =54ns 
		LCD_FSMC_WT_Structure.FSMC_AddressHoldTime = 0x00;	//��ַ����ʱ�䣨A		
		LCD_FSMC_WT_Structure.FSMC_DataSetupTime = 8;			//���ݱ���ʱ��Ϊ6ns*9��HCLK=54ns
		LCD_FSMC_WT_Structure.FSMC_BusTurnAroundDuration = 0x00;
		LCD_FSMC_WT_Structure.FSMC_CLKDivision = 0x00;
		LCD_FSMC_WT_Structure.FSMC_DataLatency = 0x00;
		LCD_FSMC_WT_Structure.FSMC_AccessMode = FSMC_AccessMode_A;	 //ģʽA 
	

	LCD_FSMC_InitStructure.FSMC_ReadWriteTimingStruct = &LCD_FSMC_RWT_Structure;	//��дʱ��
	LCD_FSMC_InitStructure.FSMC_WriteTimingStruct = &LCD_FSMC_WT_Structure;			//дʱ��

	FSMC_NORSRAMInit(&LCD_FSMC_InitStructure);//��ʼ��FSMC����

	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  // ʹ��BANK1 

}
void LCD_TFTPin_Init(void) {

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG, ENABLE);//ʹ��PD,PE,PF,PGʱ��  

	GPIO_InitTypeDef LCD_Pin_InitStructure;

	/******************************************/
	//����ַ�ߡ�A0-A15
	/******************************************/
	//PF0��PF1��PF2��PF3��PF4��PF5��  PF12��PF13��PF14��PF15
	//A0�� A1�� A2�� A3�� A4�� A5��   A6��  A7��  A8��  A9
/*	
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOF, &LCD_Pin_InitStructure);//��ʼ�� 
	//PG0��PG1��PG2��PG3��PG4��PG5
	//A10��A11��A12��A13��A14��A15
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//��ʼ�� 
*/
	//PF12
	//A6 = RS
	LCD_Pin_InitStructure.GPIO_Pin =  GPIO_Pin_12 ;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOF, &LCD_Pin_InitStructure);//��ʼ�� 

	/************************************/
	//�������ߡ�D0-D15
	/************************************/

	//###��D��
	//PD14 PD15 PD0 PD1 
	//D0   D1   D2  D3
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//��ʼ��

	//###��E��
	//PE7  PE8  PE9  PE10 PE11 PE12 PF13 PF14 PF15
	//D4   D5   D6   D7   D8   D9   D10  D11  D12
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOE, &LCD_Pin_InitStructure);//��ʼ��

	//###��D��
	//PD8 PD9 PD10 
	//D13 D14 D15  
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//��ʼ��

	/*******��ָ��������Ų��֡�****************/
	//PD4 PD5 PD6	
	//NOE NWE NWAIT
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 /*| GPIO_Pin_6*/;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//��ʼ��
	//NE4
	//PG12
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_12;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//��ʼ��
	//PB0
	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0;
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOB, &LCD_Pin_InitStructure);//��ʼ��

	LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_15;//PB15 �������,���Ʊ���
	LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	LCD_Pin_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
	LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOB, &LCD_Pin_InitStructure);//��ʼ�� //PB15 �������,���Ʊ���


//���ò���
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FSMC);//PD14����	D0
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FSMC);//PD15����	D1
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_FSMC);//PD0����	D2
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_FSMC);//PD1����	D3
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_FSMC);//PE7����	D4
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_FSMC);//PE8����	D5
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_FSMC);//PE9����	D6
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_FSMC);//PE10����	D7
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_FSMC);//PE11����	D8
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_FSMC);//PE12����	D9
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_FSMC);//PE13����	D10
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_FSMC);//PE14����	D11
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_FSMC);//PE15����	D12
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_FSMC);//PD8����	D13
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_FSMC);//PD9����	D14
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FSMC);//PD10����	D15

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_FSMC);//PD4����	NOE
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_FSMC);//PD5����	NWE

	GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_FSMC);//PF12����	A6 = RS
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource12, GPIO_AF_FSMC);//PG12����	NE4 = CS

	


}


void TFT_PinDetect(FunctionalState NewState) {
	if (NewState == DISABLE) {
		return;
	}
	else if (NewState == ENABLE) {
		GPIO_InitTypeDef LCD_Pin_InitStructure;
		//��⡾��ַ�ߡ�A0-A15
		//A0�� A1�� A2�� A3�� A4�� A5��   A6��  A7��  A8��  A9
		//PF0��PF1��PF2��PF3��PF4��PF5��  PF12��PF13��PF14��PF15
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5 |GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOF, &LCD_Pin_InitStructure);//��ʼ�� 
		//A10��A11��A12��A13��A14��A15
		//PG0��PG1��PG2��PG3��PG4��PG5
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1| GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 ;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//��ʼ�� 

		//FSMC NE4
		//PG12
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_12;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOG, &LCD_Pin_InitStructure);//��ʼ�� 


		//��⡾�����ߡ�D0-D7
		//PD14 PD15 PD0 PD1
		//D0   D1   D2  D3
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//��ʼ��
		//PE7  PE8  PE9  PE10
		//D4   D5   D6   D7
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOE, &LCD_Pin_InitStructure);//��ʼ��


		//PB0
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_0;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOB, &LCD_Pin_InitStructure);//��ʼ�� 
		//PD4 PD5
		LCD_Pin_InitStructure.GPIO_Pin = GPIO_Pin_4| GPIO_Pin_5;
		LCD_Pin_InitStructure.GPIO_Mode = GPIO_Mode_IN;//
		LCD_Pin_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//
		LCD_Pin_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(GPIOD, &LCD_Pin_InitStructure);//��ʼ�� 

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOG, ENABLE);

	}
	while (1) {
		//PB0����TFT_PinSTA[0]
		//PD4����TFT_PinSTA[1]
		//PD5����TFT_PinSTA[2]
		//PF12����TFT_PinSTA[3]
		//PG12����TFT_PinSTA[4]
		delay_ms(500);
		if(PBin(0) == 1) printf("PB0");
		if(PDin(4) == 1) printf("PD4");
		if(PDin(5) == 1) printf("PD5");
		if(PFin(12) == 1) printf("PF12");
		if(PGin(12) == 1) printf("PG12");

		//��ַ
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

		//����
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


