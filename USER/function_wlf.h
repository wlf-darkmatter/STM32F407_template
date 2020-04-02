#ifndef __FUNCTION_WLF_H
#define __FUNCTION_WLF_H

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
#include <WLF_font.h>
#include <RTC.h>
#include <WIFI_ESP8266.h>
#include "SDIO_SDCard.h"
#include "malloc.h"
#include "includes.h"
#include "piclib.h"

#define USART1_BUSY 1//�Ƿ�ʹ�ܴ��ڶ�ռ��0=��ʹ�ã�1=ʹ��
extern u8  USART1_Busy;					//���Ϊ0���Ǿ��Ǳ�ռ�ã����Ǳ���λ������ִ��printf
/******************************* WiFi ���� ********************************/
void WiFi_Debug(void);
void WiFi_Debug_task(void* pdata);






/*******************************ͼƬ����********************************/
struct _app_LCD {
	u16 Picture_num;
};
extern struct _app_LCD App_LCD;
extern DIR PictureDir;
//��ȡͼƬ����
u8 PictureFile_Init(void);//��¼����ͼƬ��Ϣ
u16 pic_get_tnum(u8* path);


/*******************************OLED GUI����********************************/
void OLED_GUIGRAM_Init(void);
void OLED_GUI_update(void* pdata);


/*******************************RTC**********************************/
extern RTC_TimeTypeDef RTC_TimeStruct;
extern RTC_DateTypeDef RTC_DateStruct;

#endif 
