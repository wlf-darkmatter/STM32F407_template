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

#include <WIFI_ESP8266.h>
#include "SDIO_SDCard.h"
#include "malloc.h"
#include "includes.h"
#include "piclib.h"

extern DIR PictureDir;

struct _app_LCD {
	u16 Picture_num;
};
extern struct _app_LCD App_LCD;



u16 pic_get_tnum(u8* path);//获取图片数量
u8 PictureFile_Init(void);//记录所有图片信息
void OLED_GUI_Init(void);





#endif // __FUNCTION_WLF_H