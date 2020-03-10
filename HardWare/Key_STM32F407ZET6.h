#ifndef __KEY_H
#define __KEY_H

#include "sys.h"

#define KEY0 PEin(4)	//KEY0对应的是引脚PE4，另一边拉到GND
#define KEY1 PEin(3)	//KEY1对应的是引脚PE3，另一边拉到GND
#define KEY2 PEin(2)	//非内置


#define WK_UP PAin(0)	//WK_UP对应的是引脚PA0，另一边拉到Vcc3.3

extern u8 KEY_STA;

void Key_IRQHandler(void);//自己定义的一个统一的按键处理函数
void Key_Init(void);	//内置按键的初始化函数
u8 Key_Scan(void);		//查询Key的状态，返回一个8位状态变量
#endif
