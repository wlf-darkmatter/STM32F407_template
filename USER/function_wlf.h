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

#define B2H(n) ((n>>21)&0x80)|((n>>18)&0x40)|((n>>15)&0x20)|((n>>12)&0x10)|((n>>9)&0x08)|((n>>6)&0x04)|((n>>3)&0x02)|((n)&0x01)
#define B(n) B2H(0x##n##l)

struct _STM32_INFO {
	u8 fac_us;
	u8 fac_ms;
	//[0]����1����ռ�ã����Ǳ���λ������ִ��printf
	//[7]����1�����ڽ������ݣ�0������
	u8 USART1_Busy;
	u16 Picture_totalnum;//��¼ͼƬ����
	u32 SD_total;
	u32 SD_free;
	
	u16* point_COLOR;//ָ��ǰϵͳ������ɫ��ָ��
	u16* back_COLOR;//ָ��ǰϵͳ������ɫ��ָ��
	u8* font_BOLD;//ָ��ǰϵͳ�����ϸ��ָ��


	char* current_picture;
};
extern struct _STM32_INFO STM32F407ZET6_info;

typedef struct  {
	u8 index;
	u8 cmd_num;
	const char* name;
}_RMT_CMD;
extern _RMT_CMD Remote_CmdStr[22];
//extern u8  USART1_Busy;
#define USART1_BUSY 1//�Ƿ�ʹ�ܴ��ڶ�ռ��0=��ʹ�ã�1=ʹ��

/********************************  �������  ***********************************/



extern OS_EVENT* Message_Input;
void APP_task(void* pdata);

void Dialog_set(u8 dialog_state);

/*--------------  KEY  -----------------*/
u8 Key_detect(void);

/*-------------  REMOTE  ---------------*/
u8 Remote_Scan(void);




/******************************* WiFi ���� ********************************/
#define WIFI_DEBUG_TASK_PRIO			3
#define WIFI_DEBUG_STK_SIZE				64

#define WIFI_TASK_PRIO					8
#define WIFI_STK_SIZE					64

void WiFi_Debug(void);
void WiFi_Debug_task(void* pdata);

/*******************************       SD       ****************************************/
//���SD�������Ƿ��Զ���ʽ����1=�ǣ�0=��
#define FORMAT_IF_ERROR 0
extern OS_EVENT* message_SD;			//SD����д�����¼���ָ��





/*******************************ͼƬ����********************************/

//u16 picture_index
//char picture_name[62]
struct _structure_picture_name {
	u16 picture_index;
	char picture_name[62];//֧��30��������ɵĳ�����
};
extern struct _structure_picture_name* pic_reference;

extern DIR PictureDir;
void SD_picinfo_write(FIL* pic_fil, u8 index, struct _structure_picture_name* write_Structure);
void SD_picinfo_read(FIL* pic_fil, u8 index, struct _structure_picture_name* read_Structure);
//��ȡͼƬ����
u8 PictureFile_Init(void);//��¼����ͼƬ��Ϣ
u16 pic_get_tnum(u8* path);


/*******************************  OLED GUI����  ********************************/
#define OLED_TASK_PRIO				6
#define OLED_STK_SIZE				64
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

#define USMART_APP_TASK_PRIO				3
#define USMART_APP_STK_SIZE					200//������������Ƚϳ����Ǿ���Ҫ��һ���ջ
extern OS_STK USMART_APP_TASK_STK[USMART_APP_STK_SIZE];
void USMART_APP(void* pdata);

void Debug(void);

void Exit(void);

#endif
#endif 




/*****************************   REMOTE    *******************************/
#define RDATA PAin(8)	 //�������������

//����ң��ʶ����(ID),ÿ��ң�����ĸ�ֵ��������һ��,��Ҳ��һ����.
//Carmp3��ң����ʶ����Ϊ1
#define REMOTE_ID 0x00    		   

extern u8 RmtCnt;	//�������µĴ���
extern u32 RmtRec;
void Remote_Init(void);    //���⴫��������ͷ���ų�ʼ��



/**********************      APP    ������     *************************/
#define APP_TASK_PRIO					5
#define APP_STK_SIZE					256
extern OS_EVENT* Message_APP_cmd; //������������������
extern OS_STK APP_TASK_STK[APP_STK_SIZE];

void APP_task(void* pdata);

/**********************      APP    �ⲿ����     *************************/

void lcd_ShowSystemInfo(void);
