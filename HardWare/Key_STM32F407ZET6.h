#ifndef __KEY_H
#define __KEY_H

#include "sys.h"

#define KEY0 PEin(4)	//KEY0��Ӧ��������PE4����һ������GND
#define KEY1 PEin(3)	//KEY1��Ӧ��������PE3����һ������GND
#define KEY2 PEin(2)	//������


#define WK_UP PAin(0)	//WK_UP��Ӧ��������PA0����һ������Vcc3.3

extern u8 KEY_STA;

void Key_IRQHandler(void);//�Լ������һ��ͳһ�İ���������
void Key_Init(void);	//���ð����ĳ�ʼ������
u8 Key_Scan(void);		//��ѯKey��״̬������һ��8λ״̬����
#endif
