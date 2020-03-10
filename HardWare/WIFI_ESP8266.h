#ifndef __WIFI_H
#define __WIFI_H

#include "sys.h"

//**�������ȼ�
//��֤��TIM7�������ȼ��ȡ�USART2��Ҫ��
#define NVIC_USART2_PreemptionPriority 2
#define NVIC_USART2_SubPriority 2

#define NVIC_TIM7_PreemptionPriority 2
#define NVIC_TIM7_SubPriority 1

//�����ַ�������
#define USART2_REC_LEN  			1000
#define USART2_TRA_LEN  			200

extern u8  USART2_RX_BUF[USART2_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u8  USART2_TX_BUF[USART2_TRA_LEN];
extern u16 USART2_RX_STA;         		//����״̬���
extern u8  USART2_TX_STA;				//�Լ�д��һ�����ڷ���״̬�ı�Ǳ���

extern char info[50];//����WiFi��ѯESP8266�ķ�����Ϣ

void usart2_init(u32 bound);//����WiFiͨѶ

void ESP8266_restart(void);
//��������WiFi
void ESP8266_WiFiConnect(char* SSID, char* passward);
//���������Լ�Ҫ���WiFi
void ESP8266_WiFiEmit(char* SSID, char* passward);
void ESP8266_TCP_Server(void/*char* addr, char* port*/);
void usart2_printf(char* fmt, ...);

int Info_fetch(char* Source, const char* String);

//��ESP8266��������
u8 ESP8266_send_cmd(char* cmd, char* ack, int waittime);
//ESP8266���������,�����յ���Ӧ��
u8* ESP8266_check_cmd(u8* str);
//ESP8266�˳�͸��ģʽ   ����ֵ:0,�˳��ɹ�;1,�˳�ʧ��
u8 ESP8266_quit_trans(void);


void ESP8266_init(void);

//��ESP8266��������
u8* ESP8266_send_data(char* cmd, u16 waittime);

void TIM7_INT_Init(u16 arr, u16 psc);

#endif // !__WIFI_H
