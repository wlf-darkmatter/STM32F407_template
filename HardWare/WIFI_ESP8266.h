#ifndef __WIFI_H
#define __WIFI_H

#include "sys.h"

//**定义优先级
//保证【TIM7】的优先级比【USART2】要高
#define NVIC_USART2_PreemptionPriority 2
#define NVIC_USART2_SubPriority 2

#define NVIC_TIM7_PreemptionPriority 2
#define NVIC_TIM7_SubPriority 1

//定义字符串容量
#define USART2_REC_LEN  			1000
#define USART2_TRA_LEN  			200

extern u8  USART2_RX_BUF[USART2_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u8  USART2_TX_BUF[USART2_TRA_LEN];
extern u16 USART2_RX_STA;         		//接收状态标记
extern u8  USART2_TX_STA;				//自己写的一个关于发送状态的标记变量

extern char info[50];//用于WiFi查询ESP8266的返回信息

void usart2_init(u32 bound);//用于WiFi通讯

void ESP8266_restart(void);
//用于连接WiFi
void ESP8266_WiFiConnect(char* SSID, char* passward);
//用于配置自己要搭建的WiFi
void ESP8266_WiFiEmit(char* SSID, char* passward);
void ESP8266_TCP_Server(void/*char* addr, char* port*/);
void usart2_printf(char* fmt, ...);

int Info_fetch(char* Source, const char* String);

//向ESP8266发送命令
u8 ESP8266_send_cmd(char* cmd, char* ack, int waittime);
//ESP8266发送命令后,检测接收到的应答
u8* ESP8266_check_cmd(u8* str);
//ESP8266退出透传模式   返回值:0,退出成功;1,退出失败
u8 ESP8266_quit_trans(void);


void ESP8266_init(void);

//向ESP8266发送数据
u8* ESP8266_send_data(char* cmd, u16 waittime);

void TIM7_INT_Init(u16 arr, u16 psc);

#endif // !__WIFI_H
