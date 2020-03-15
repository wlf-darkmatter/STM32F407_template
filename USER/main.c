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
/*不要使用PA13和PA14，它们分别对应着SW-DIO和SW-CLK，且本身一直处于AF复用模式*/


int main(void) {

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	
	delay_init(168);
	LED_Init();
//	Beep_Init();
	usart1_init(57600);
	

	Key_Init();
	OLED_Init();
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);

	
//	ESP8266_init();	//esp8266进行初始化

//	ESP8266_WiFiEmit("WLF-Control", "00000000");
	
//	ESP8266_WiFiConnect("东南沿海王大哥", "19981213");

//	ESP8266_WiFiConnect("地球暗物质", "wang123456");
	
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





