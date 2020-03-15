#include "sys.h"
#include "delay.h"
#include "usart.h"
#include <LED_STM32F407ZET6.h>
#include <Beep.h>
#include <Key_STM32F407ZET6.h>
#include <Tim.h>
#include <PWM.h>
#include <wlf_I2C.h>
#include <OLED.h>
#include <WIFI_ESP8266.h>
#include <TFT_LCD_2.8in.h>
/*��Ҫʹ��PA13��PA14�����Ƿֱ��Ӧ��SW-DIO��SW-CLK���ұ���һֱ����AF����ģʽ*/


int main(void) {

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	
	delay_init(168);
	LED_Init();
//	Beep_Init();
	usart1_init(57600);
	

	Key_Init();
	OLED_Init();
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);

	
//	ESP8266_init();	//esp8266���г�ʼ��

//	ESP8266_WiFiEmit("WLF-Control", "00000000");
	
//	ESP8266_WiFiConnect("�����غ������", "19981213");

//	ESP8266_WiFiConnect("��������", "wang123456");
	
//	ESP8266_TCP_Server();

	TFT_PinDetect(ENABLE);
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





