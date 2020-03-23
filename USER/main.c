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
#include <WIFI_ESP8266.h>
#include "SDIO_SDCard.h"
/*��Ҫʹ��PA13��PA14�����Ƿֱ��Ӧ�š�SW-DIO���͡�SW-CLK�����ұ���һֱ����AF����ģʽ*/
/*���������ʱ��Ҫ�ǵ���ʹ��ʱ�ӣ�Ȼ�������ã���Ϊ��Ҫһ��ʱ��ȴ�ʱ����������*/


int main(void) {
	u8 x = 0;
	u8 lcd_id[12];

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	usmart_init(168);
	delay_init(168);
	LED_Init();
//	Beep_Init();
	usart1_init(115200);
	LCD_Init();
	Key_Init();
	OLED_Init();
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);

	POINT_COLOR = RED;      //������ɫ����ɫ
	sprintf((char*)lcd_id, "LCD ID:%04X", lcddev.id);//��LCD ID��ӡ��lcd_id���顣				 	
	while (1) {
		switch (x)
		{
		case 0:LCD_Clear(WHITE); break;
		case 1:LCD_Clear(BLACK); break;
		case 2:LCD_Clear(BLUE); break;
		case 3:LCD_Clear(RED); break;
		case 4:LCD_Clear(MAGENTA); break;
		case 5:LCD_Clear(GREEN); break;
		case 6:LCD_Clear(CYAN); break;
		case 7:LCD_Clear(YELLOW); break;
		case 8:LCD_Clear(BRRED); break;
		case 9:LCD_Clear(GRAY); break;
		case 10:LCD_Clear(LGRAY); break;
		case 11:LCD_Clear(BROWN); break;
		}
		POINT_COLOR = RED;
		LCD_ShowString(30, 40, 210, 24, 24, "WLF- STM32F4");
		LCD_ShowString(30, 70, 200, 16, 16, "String test");
		LCD_ShowString(30, 90, 200, 16, 16, "LQ likes what?");
		LCD_ShowString(30, 110, 200, 16, 16, lcd_id);		//��ʾLCD ID	      					 
		LCD_ShowString(30, 130, 200, 12, 12, "Sleeping, of course.");
		x++;
		if (x == 12)x = 0;
		LED1 = !LED1;
		delay_ms(1000);
	}


//	ESP8266_init();	//esp8266���г�ʼ��

//	ESP8266_WiFiEmit("WLF-Control", "00000000");

//	ESP8266_WiFiConnect("�����غ������", "19981213");

//	ESP8266_WiFiConnect("��������", "wang123456");

//	ESP8266_TCP_Server();

//	TFT_PinDetect(ENABLE);
/*
	OLED_DrawChar(0, 0, 'P', 24, 1);
	OLED_DrawStr(0, 0, "I love you ",24,1);
	OLED_DrawStr(0, 24, "   'LQ'",24,1);
	*/
	
//	OLED_Refresh();
/*OLED_ShowGBK(0, 0, 0, 24, 1);
	OLED_ShowGBK(24, 0, 0, 24, 1);
	OLED_ShowGBK(48, 0, 1, 24, 1);
	OLED_ShowGBK(72, 0, 2, 24, 1);
	OLED_ShowGBK(96, 0, 3, 24, 1);
*/

	

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	while (1) {
		
	}

}


/*��PWM������

//PWM_TIM14_Init(500 - 1, 84 - 1);//����ʱ��Ϊ500us,��һ�������ǡ�ARR����Ҳ�С�period������ֵ
*/

/*��TIM3����������TIM3���ж�

TIM3_INT_Init(5000 - 1, 8400 - 1);//һ������£�Tout=(I+1)*(II+1)/84 (��λus)
*/


//ͨ�����ڴ�ӡSD�������Ϣ
void show_sdcard_info(void)
{
	switch (SDCardInfo.CardType)
	{
	case SDIO_STD_CAPACITY_SD_CARD_V1_1:printf("Card Type:SDSC V1.1\r\n"); break;
	case SDIO_STD_CAPACITY_SD_CARD_V2_0:printf("Card Type:SDSC V2.0\r\n"); break;
	case SDIO_HIGH_CAPACITY_SD_CARD:printf("Card Type:SDHC V2.0\r\n"); break;
	case SDIO_MULTIMEDIA_CARD:printf("Card Type:MMC Card\r\n"); break;
	}
	printf("Card ManufacturerID:%d\r\n", SDCardInfo.SD_cid.ManufacturerID);	//������ID
	printf("Card RCA:%d\r\n", SDCardInfo.RCA);								//����Ե�ַ
	printf("Card Capacity:%d MB\r\n", (u32)(SDCardInfo.CardCapacity >> 20));	//��ʾ����
	printf("Card BlockSize:%d\r\n\r\n", SDCardInfo.CardBlockSize);			//��ʾ���С
}



