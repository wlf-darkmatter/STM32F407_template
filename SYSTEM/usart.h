#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//5,�����ˡ�WiFi���Ĵ���ͨѶ
////////////////////////////////////////////////////////////////////////////////// 	
/*Ҫע�⣬��Arduino������ͨѶ��ʱ��Ҫ��Arduino��ͨѶ�����ʵ�Ϊ��STM32��Ӧ����һ��*/

//**�������ȼ�
#define NVIC_USART1_PreemptionPriority 1
#define NVIC_USART1_SubPriority 1

#define USART1_REC_LEN  			200  	//�����������ֽ��� 200


#define EN_USART1_RX 			1		//ʹ�ܣ�1��/��ֹ��0������1����
#define EN_USART1_TX			DISABLE		//����1����������ж�TCIE���Ƿ��

extern u8  USART1_RX_BUF[USART1_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16 USART1_RX_STA;         		//����״̬���
extern u8  USART1_TX_STA;				//�Լ�д��һ�����ڷ���״̬�ı�Ǳ���



//����봮���жϽ��գ��벻Ҫע�����º궨��
void usart1_init(u32 bound);




#endif


