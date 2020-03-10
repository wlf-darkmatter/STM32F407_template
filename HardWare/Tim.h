#ifndef __TIM_H
#define __TIM_H
#include "sys.h"



void TIM3_INT_Init(u32 Period, u16 Prescaler);//TIM3��ʱ���жϵĳ�ʼ������

//����APB1�µĶ�ʱ�������ʱ�䣨�������̫���ˣ�
//��Period����������
//��Prescaler���Ƿ�Ƶϵ��
//��Ft��=��ʱ������Ƶ��,��λ:(MHz)
//��ʱ�����ʱ����㷽��:Tout=((Period+1)*(Prescaler+1))/Ft ��us��,
//�������ʱ�䣨us��
u32 Get_TIMx_OutputTime(TIM_TypeDef* TIMx);





#endif
