/*���ļ��ǵ�Ƭ���ϵ�����LED��ͷ�ļ�*/

#ifndef __LED_STM32F407ZET6
#define __LED_STM32F407ZET6

#include "sys.h"
//ÿ����ͬ�ĵ�Ƭ���в�ͬ�Ķ˿ڣ�������ֻ��Ӧ�Լ��ĵ�һ��F4��Ƭ��
#define LED1 PFout(9)
#define LED2 PFout(10)
#define D1 PFout(9)//����
#define D2 PFout(10)//����

void LED_Init(void);

#endif
