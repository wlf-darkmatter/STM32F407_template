#ifndef __TIM_H
#define __TIM_H
#include "sys.h"



void TIM3_INT_Init(u32 Period, u16 Prescaler);//TIM3定时器中断的初始化函数

//计算APB1下的定时器的溢出时间（我真的是太闲了）
//【Period】是周期数
//【Prescaler】是分频系数
//【Ft】=定时器工作频率,单位:(MHz)
//定时器溢出时间计算方法:Tout=((Period+1)*(Prescaler+1))/Ft （us）,
//返回溢出时间（us）
u32 Get_TIMx_OutputTime(TIM_TypeDef* TIMx);





#endif
