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
extern u8  USART1_Busy;
/********************************  �������  ***********************************/
#define INPUT_TASK_PRIO				4
#define INPUT_STK_SIZE				32
extern OS_STK KEY_TASK_STK[INPUT_STK_SIZE];

void InputCommand_task(void* pdata);

/*--------------  KEY  -----------------*/
u8 Key_detect(void);

/*-------------  REMOTE  ---------------*/
u8 Remote_Scan(void);


/******************************* WiFi ���� ********************************/
#define WIFI_DEBUG_TASK_PRIO				2
#define WIFI_DEBUG_STK_SIZE					128
#define WIFI_TASK_PRIO			8
#define WIFI_STK_SIZE			128

void WiFi_Debug(void);
void WiFi_Debug_task(void* pdata);

/*******************************       SD       ****************************************/
//���SD�������Ƿ��Զ���ʽ����1=�ǣ�0=��
#define FORMAT_IF_ERROR 0
extern OS_EVENT* message_SD;			//SD����д�����¼���ָ��





/*******************************ͼƬ����********************************/
struct _app_LCD {
	u16 Picture_num;
};
extern struct _app_LCD App_LCD;
extern DIR PictureDir;
//��ȡͼƬ����
u8 PictureFile_Init(void);//��¼����ͼƬ��Ϣ
u16 pic_get_tnum(u8* path);


/*******************************  OLED GUI����  ********************************/
#define OLED_TASK_PRIO				6
#define OLED_STK_SIZE					128
extern OS_STK OLED_TASK_STK[OLED_STK_SIZE];
extern OS_EVENT* message_OLED;			//OLED����д�����¼���ָ��

void OLED_GUIGRAM_Init(void);
void OLED_GUI_update(void* pdata);


/*******************************RTC**********************************/
extern RTC_TimeTypeDef RTC_TimeStruct;
extern RTC_DateTypeDef RTC_DateStruct;
void Show_RTC(void);

/*******************************  USMART  **********************************/
#if USE_SMART_APP==1

#define USMART_APP_TASK_PRIO				1
#define USMART_APP_STK_SIZE					128
extern OS_STK USMART_APP_TASK_STK[USMART_APP_STK_SIZE];
void USMART_APP(void* pdata);

#endif
#endif 




/*****************************   REMOTE    *******************************/
#define RDATA PAin(8)	 //�������������

//����ң��ʶ����(ID),ÿ��ң�����ĸ�ֵ��������һ��,��Ҳ��һ����.
//����ѡ�õ�ң����ʶ����Ϊ0
#define REMOTE_ID 0      		   

extern u8 RmtCnt;	//�������µĴ���
extern u32 RmtRec;
void Remote_Init(void);    //���⴫��������ͷ���ų�ʼ��



