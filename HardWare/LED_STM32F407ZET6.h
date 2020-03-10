/*该文件是单片机上的内置LED的头文件*/

#ifndef __LED_STM32F407ZET6
#define __LED_STM32F407ZET6

#include "sys.h"
//每个不同的单片机有不同的端口，改设置只对应自己的第一块F4单片机
#define LED1 PFout(9)
#define LED2 PFout(10)
#define D1 PFout(9)//别名
#define D2 PFout(10)//别名

void LED_Init(void);

#endif
