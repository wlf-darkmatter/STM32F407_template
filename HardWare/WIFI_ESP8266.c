#include <WIFI_ESP8266.h>
#include <LED_STM32F407ZET6.h>
#include <OLED.h>
/*
1��ESP8266��RXD�����ݵĽ��նˣ���Ҫ����USBתTTLģ���TXD��TXD�����ݵķ��Ͷˣ���Ҫ����USBתTTLģ���RXD�����ǻ����ģ�
2������VCC��ѡȡ����USBתTTLģ������3.3V��5V�������ſ�����ΪVCC������һ��ѡȡ5V��ΪVCC�����ѡȡ3.3V�����ܻ���Ϊ���粻������𲻶ϵ��������Ӷ���ͣ�ĸ�λ��
*/
/*
��ESP8266��������һ��Ϊ115200
ATָ����ִ�Сд�����Իس������н�β��������ܳ��õ�ATָ�*/
/*
ָ����					��Ӧ				����
AT						OK					����ָ��
AT+CWMODE=<mode>		OK					����Ӧ��ģʽ����������Ч��
AT+CWMODE_DEF=1  # ���� Wi-Fi Ϊ Station ģʽ�����浽 Flash�� 2 Ϊ AP ģʽ
AT+CWMODE?				+CWMODE:<mode>		��õ�ǰӦ��ģʽ
AT+CWLAP				+CWLAP:<ecn>,<ssid>,<rssi>	����Ŀǰ��AP�б�
AT+CWJAP=<ssid>,<pwd>	OK					����ĳһAP
AT+CWJAP?				+CWJAP:<ssid>		���ص�ǰ�����AP
AT+CWQAP				OK					�˳���ǰ�����AP
AT+CIPSTART=<type>,<addr>,<port>	OK		����TCP/UDP����
AT+CIPMUX=<mode>		OK					�Ƿ����ö�����
AT+CIPSEND=<param>		OK					��������
AT+CIPMODE=<mode>		OK					�Ƿ����͸��ģʽ

��AT+CWMODE=1�����ù���ģʽ��STAģʽ��
��AT+RST��ģ����������Ч����ģʽ��
��AT+CWJAP="111","11111111"�����ӵ�ǰ������WIFI�ȵ㣨�ȵ��������룩
��AT+CIPMUX=0�����õ�·����ģʽ
��AT+CIPSTART="TCP","xxx.xxx.xxx.xxx",xxxx������TCP����
��AT+CIPMODE=1������͸��ģʽ
��AT+CIPSEND��͸��ģʽ�£���������
��+++���˳�͸��ģʽ

//��Ϊ����2������WiFi��
//PA(2)		- TX
//PA(3)		- RX

//EC:FA:BC:59:32:96ΪESP-12F�ĵ�ַ

*/

#include "delay.h"
#include "usart.h"
#include "stdarg.h"
#include "string.h"
#include "Tim.h"


u8 USART2_RX_BUF[USART2_REC_LEN];
u8 USART2_TX_BUF[USART2_TRA_LEN];
u8 USART2_TX_STA = 0;
u16 USART2_RX_STA = 0;
//���ڵ��Իظ�����Ϣ
char info[50];

void ESP8266_restart(void) {
	//��Wifiģ������������
	printf("������������\n");
	for (int i = 0; i < 10; i++) {//�ظ�����������10*3��
		if (ESP8266_send_cmd("AT+RST", "OK", 4000) == 0) {//�ȴ�4��
			printf("�������\n");
//			OLED_DrawStr(0, 24, "WiFi Activated.         ", 12, 1);
			return;
		}
	}
	printf("����ʧ�ܣ�\n");
//	printf("3s �ȴ�����\n");
	
	
}


//ռ��OLEDһС����������ʾ
//���ù���ģʽ 1��stationģʽ   2��APģʽ  3������ AP+stationģʽ	
void ESP8266_init(void) {
	
	TIM7_INT_Init(1000 - 1, 840 - 1);		//10ms�ж�
	usart2_init(115200);
	int timeout = (int)Get_TIMx_OutputTime(TIM7);
	printf("TIM7�������ʱ��Ϊ��%d ms = %d us\n", timeout / 1000, timeout);


	//����
	if (ESP8266_send_cmd("AT", "OK", 500) == 0)
		printf("AT ���Գɹ�\n");
	else
		printf("AT ����ʧ��\n");
	//���ù���ģʽ 1��stationģʽ   2��APģʽ  3������ AP+stationģʽ
	
	ESP8266_send_cmd("AT+CWMODE_DEF=3", "OK", 500);


	/*
ָ����					��Ӧ				����
AT						OK					����ָ��
AT+CWMODE=<mode>		OK					����Ӧ��ģʽ����������Ч��
AT+CWMODE?				+CWMODE:<mode>		��õ�ǰӦ��ģʽ
AT+CWLAP				+CWLAP:<ecn>,<ssid>,<rssi>	����Ŀǰ��AP�б�
AT+CWJAP=<ssid>,<pwd>	WIFI GOT IP			����ĳһAP
AT+CWJAP?				+CWJAP:<ssid>		���ص�ǰ�����AP
AT+CWQAP				OK					�˳���ǰ�����AP
AT+CIPSTART=<type>,<addr>,<port>	OK		����TCP/UDP����
AT+CIPMUX=<mode>		OK					�Ƿ����ö�����
AT+CIPSEND=<param>		OK					��������
AT+CIPMODE=<mode>		OK					�Ƿ����͸��ģʽ
*/
//��ģ���������Լ���·��
	ESP8266_restart();
}

void ESP8266_WiFiConnect(char* SSID, char* passward) {
	char CWJAP_str[60];//���ڴ�ſ�������
	memset(CWJAP_str, '\0', 60 * sizeof(char));//����

	strcat(CWJAP_str,"AT+CWJAP=\"");//AT+CWJAP="
	strcat(CWJAP_str, SSID);//AT+CWJAP="SSID
	strcat(CWJAP_str, "\",\"");//AT+CWJAP="SSID","
	strcat(CWJAP_str, passward);//AT+CWJAP="SSID","passward
	strcat(CWJAP_str, "\"");//AT+CWJAP="SSID","passward"

//"AT+CWJAP=\"�����غ������\",\"19981213\""
	if (ESP8266_send_cmd(CWJAP_str, "WIFI GOT IP", 20000) == 0) {

	}
	else {
		printf("20��û��������ָ����WiFi��\n%s", USART2_RX_BUF);
		OLED_DrawStr(0, 24, "WiFi connection failed.", 12, 0);
		OLED_DrawStr(0, 36, "I'm so Sorry.          ", 12, 0);
		OLED_DrawStr(0, 48, "Please contact WLF.   ", 12, 0);
		return;
	}

	if (ESP8266_send_cmd("AT+CIFSR", "OK", 2500) == 0) printf("������WLAN\n");//(�鿴ģ���ַ)
	else printf("û������������ָ����WiFi\n%s", USART2_RX_BUF);
//	printf("%s", USART2_RX_BUF);
//	printf("***********************\n%s\n***********************", USART2_RX_BUF);

	
	OLED_DrawStr(0, 12, "WiFi connected.      ", 12, 0);

}
//AT+ CWSAP= <ssid>,<pwd>,<chl>,<ecn>
//	ssid:���������
//	pwd:����
//	chl : ͨ����
//	ecn : ���ܷ�ʽ:��0 - OPEN�� 1 - WEP�� 2 - WPA_PSK�� 3 - WPA2_PSK�� 4 - WPA_WPA2_PSK��
void ESP8266_WiFiEmit(char* SSID, char* passward) {
	
	char CWSAP_str[60];//���ڴ�ſ�������
	memset(CWSAP_str, '\0', 60 * sizeof(char));//����

	strcat(CWSAP_str, "AT+CWSAP=\"");//AT+CWSAP="
	strcat(CWSAP_str, SSID);//AT+CWSAP="SSID
	strcat(CWSAP_str, "\",\"");//AT+CWSAP="SSID","
	strcat(CWSAP_str, passward);//AT+CWSAP="SSID","passward
	strcat(CWSAP_str, "\",1,4");//AT+CWSAP="SSID","passward",1,4   ***����û�����ţ�����
	//�á�ͨ��1���͡�WPA_WPA2_PSK��

	if (ESP8266_send_cmd(CWSAP_str, "OK", 40000) == 0) {
		printf("�Ѵ���WLAN\nWiFi����%s\n���룺%s\n", SSID, passward);

		OLED_DrawStr(0, 24, "WiFi:", 12, 0);
		OLED_DrawStr(30, 24, SSID, 12, 0);
	}
	else {
		printf("20��û�д���ָ����WiFi��\n%s", USART2_RX_BUF);//���ش�����Ϣ
		OLED_DrawStr(0, 24, "WiFi create failed.   ", 12, 0);
		OLED_DrawStr(0, 36, "I'm so Sorry.          ", 12, 0);
		OLED_DrawStr(0, 48, "Please contact WLF.   ", 12, 0);
		return;
	}
	ESP8266_restart();
}

void ESP8266_TCP_Server(void) {
	char TCP_info[50];
	memset(TCP_info, '\0', 50 * sizeof(char));//����

	//=0����·����ģʽ     =1����·����ģʽ
	ESP8266_send_cmd("AT+CIPMUX=1", "OK", 200);
	
	/**************************************************************************
	ָ�
	1)��·����ʱ(+CIPMUX=0)��ָ��Ϊ��AT+CIPSTART=<type>,<addr>,<port>
	2)��·����ʱ(+CIPMUX=1)��ָ��Ϊ��AT+CIPSTART=<id>,<type>,<addr>,<port>
	port����Ҫ����
	�����ʽ��ȷ�����ӳɹ������� OK�����򷵻� ERROR
	��������Ѿ����ڣ�����ALREAY?CONNECT
	˵����
	<id>:0-4�����ӵ�id��
	<type>:�ַ��������������������ͣ���TCP��-����tcp���ӣ���UDP��-����UDP����
	<addr>:�ַ���������Զ�̷�����IP��ַ
	<port>:Զ�̷������˿ں�
	*************************************************************************
	ע������������ģʽ�ķ��� 
	������·���ӣ�					AT+CIPMUX=1 
	����������ģʽ���˿�Ϊ8080��ȱʡֵΪ333��	AT+CIPSERVER=1,8080
	*************************************************************************
	ָ�AT+CIPSERVER=mode,[port] 
	˵����
	mode:0-�ر�serverģʽ��1-����serverģʽ ������
	port:�˿ںţ�ȱʡֵΪ333 ��Ӧ��OK 
	*************************************************************************
	˵����
	(1) AT+CIPMUX=1ʱ���ܿ������������ر�serverģʽ��Ҫ���� ������
	(2)����server���Զ�����server����,����client������Զ���˳��ռ��һ�����ӡ�
	*************************************************************************
	*/
	if (ESP8266_send_cmd("AT+CIPSERVER=1,525", "OK", 200) == 0) {//����������
		OLED_DrawStr(0, 48, "TCP Established.      ", 12, 1);
		delay_ms(4000);

		/*���磺
		CIFSR:APIP,"192.168.4.1"
		CIFSR:APMAC,"de:4f:22:7d:49:18"
		CIFSR:STAIP,"192.168.1.104"
		CIFSR:STAMAC,"dc:4f:22:7d:49:18"
		*/
		ESP8266_send_cmd("AT+CIFSR","OK", 200);//��ѯ
		if (Info_fetch((char*)USART2_RX_BUF, "CIFSR:STAIP") == 0) {
			printf("STA IP: %s\n", info);
			strcat(TCP_info, "STA IP:");
			strcat(TCP_info, info);
		OLED_DrawStr(0, 36, TCP_info, 12, 1);
		}
		else {
			printf("��ѯʧ��\n");
			OLED_DrawStr(0, 36, "TCP IP error       ", 12, 1);
		}

		OLED_DrawStr(0, 48, "TCP port: 525        ", 12, 1);
	}
	else {
		OLED_DrawStr(0, 48, "TCP server error.    ", 12, 1);
	}



/*�ͻ��ˡ���������������
	char CIPSTART_str[60];//���ڴ�ſ�������
	for (int i = 0; i < 60; i++) CIPSTART_str[i] = 0;

	strcat(CIPSTART_str, "AT+CIPSTART=\"TCP\",\"");//AT+CIPSTART="TCP","
	strcat(CIPSTART_str, addr);//AT+CWJAP="TCP","addr
	strcat(CIPSTART_str, "\",");//AT+CWJAP="TCP","addr",

	strcat(CIPSTART_str, port);//AT+CWJAP="TCP","addr",port
*/

/*
	//����TCP����  ������ֱ������ Ҫ���ӵ�ID��0~4   ��������  Զ�̷�����IP��ַ   Զ�̷������˿ں�
	while (ESP8266_send_cmd(CIPSTART_str, "CONNECT", 2000));
	//�Ƿ���͸��ģʽ  0����ʾ�ر� 1����ʾ����͸��
	ESP8266_send_cmd("AT+CIPMODE=1", "OK", 2000);
	//͸��ģʽ�� ��ʼ�������ݵ�ָ�� ���ָ��֮��Ϳ���ֱ�ӷ�������
	ESP8266_send_cmd("AT+CIPSEND", "OK", 500);
	*/
}


//char info[50];��ȡ��Ӧ�ַ��䶺�ź�Ĳ��֣���������ţ��򲻰�������
//return 1����û���ҵ����ϵĻ�Ӧ
//return 2�������ϵĻ�Ӧ����û�ж���
//return 3�������ҵ�Ŀ���ַ����������޷��������Ľ�β����
int Info_fetch(char* Source, const char* String) {
	memset(info, '\0', 50 * sizeof(char));//����
	char* head = Source;
	char* tail = NULL;
	/*�����ʾ������  
	AT+CIFSR
	+CIFSR:APIP,"192.168.4.1"
	+CIFSR:APMAC,"de:4f:22:7d:49:18"
	+CIFSR:STAIP,"192.168.1.104"
	+CIFSR:STAMAC,"dc:4f:22:7d:49:18"
	*/
	printf("**************\nInfo_fetch()\n %s\n**************", Source);
	//ȡCIFSR:STAIP֮����ַ�������
	//head����ͷ����
	//tail����β����
	head = strstr(Source, String);
	if (head == NULL) {
		printf("Info_fetchʧ�ܣ�û���ҵ����������Ļ�Ӧ��\n");
		return 1;
	}
	head = strstr(head, ",") + 1; //����ͷ���������š���
	if (head == NULL) {
		printf("Info_fetchʧ�ܣ����ϵĻ�Ӧ����û�ж��š�\n");
		return 2;
	}
	tail = strstr(head, "\r\n")-1;//���Ҵ˾�֮�������ĵ�һ�����з�������β��,������\r����
	if (tail == NULL) {
		printf("Info_fetchʧ�ܣ����ҵ�Ŀ���ַ����������޷��������Ľ�β����\n");
		return 3;
	}
//����Ƿ�������
	if (*head == '\"') {
		head += 1;
		tail -= 1;
	}
	strncpy(info, head, tail - head + 1); //������info
	return 0;
}



//��ESP8266��������
//cmd:���͵������ַ���;ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��;waittime:�ȴ�ʱ��(��λ:ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����);1,����ʧ��
u8 ESP8266_send_cmd(char* cmd, char* ack, int waittime) {
	int waittime_copy = waittime;
	waittime /= 10;//����һ�£�������delay_ms(10); 
	for (int i = 0; i < 3; i++) {
		USART2_RX_STA = 0;
		usart2_printf("%s\r\n", cmd);	//��������
		//	printf("%s\r\n", cmd);
		if (ack && waittime) {		//��Ҫ�ȴ�Ӧ��
			while (--waittime) {	//�ȴ�����ʱ
				delay_ms(10); 
				if (USART2_RX_STA & 0X8000) {//TIM7��ʱ�Ƿ����,������ȡ�Ѷ�����
					if (ESP8266_check_cmd((u8*)ack)) {
						printf("%s\r\n",ack);
						return (u8)0;//�õ���Ч���� 
					}
					else printf(".");
					USART2_RX_STA = 0;
				}
			}
			if (waittime == 0) {
				printf("�ȴ���ʱ> %d (ms)��3�����ԣ�������ʧ��ָ��: %s\n", (int)waittime_copy * 10, cmd);
			}
		}
	}
	return 1;
}

//ESP8266���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����;����,�ڴ�Ӧ������λ��(str��λ��)
u8* ESP8266_check_cmd(u8* str) {
	char* strx = 0;
	if (USART2_RX_STA & 0X8000)		//���յ�һ��������
	{
		USART2_RX_BUF[USART2_RX_STA & 0X7FFF] = 0;//��ӽ�����
		strx = strstr((const char*)USART2_RX_BUF, (const char*)str);
	}
	return (u8*)strx;
}

//ESP8266�˳�͸��ģʽ   ����ֵ:0,�˳��ɹ�;1,�˳�ʧ��
//����wifiģ�飬ͨ����wifiģ����������3��+��ÿ��+��֮�� ����10ms,������Ϊ���������η���+��
u8 ESP8266_quit_trans(void)
{
	u8 result = 1;
	usart2_printf("+++");
	delay_ms(1000);					//�ȴ�500ms̫�� Ҫ1000ms�ſ����˳�
	result = ESP8266_send_cmd("AT", "OK", 20);//�˳�͸���ж�.
	if (result)
		printf("quit_trans failed!");
	else
		printf("quit_trans success!");
	return result;
}



//��ʼ��IO ����2
//bound:115200 (ESP8266��Ĭ��ֵ)
void usart2_init(u32 bound) {
	//PA(2)		- TX
	//PA(3)		- RX

	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;


	//��	����1ʱ��ʹ�ܣ�GPIOAʱ��ʹ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);//ʹ��USART2ʱ�ӣ�����APB1����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //ʹ��GPIOAʱ�ӣ�����AHB1����


	//��	�������Ÿ�����ӳ��
	//��Ҫ���������˿�
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //GPIOA2����ΪUSART2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //GPIOA3����ΪUSART2


	//��	GPIO��ʼ�����á������ö˿�ģʽΪ������ģʽ��
	//USART2�˿�����
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA9��GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA2��PA3


	//��	���ڳ�ʼ�����á������á������ʡ����ֳ�������żУ�顿
	//USART2 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//���������ã�115200 (ESP8266��Ĭ��ֵ)
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ������RE��TE
	USART_Init(USART2, &USART_InitStructure); //��ʼ������1



	//�������������TC�жϡ�
	//#define EN_USART2_TX = ENABLE����1����������ж�TCIE���Ƿ��
	USART_ITConfig(USART2, USART_IT_TC, ENABLE);

	//��ʱ��������Ϣ������TXE���������ݼĴ���Ϊ�գ���Ӧ��IEλ��TXEIEҪ�ر�
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	USART_ClearFlag(USART2, USART_FLAG_TC);//���뼰ʱ��λ��ʹ��USART2֮ǰ��
	/*�޴�BUG��
	******************************************************************
	*��ʵһ����ʼ��USART2��ʱ�ӵ�ʱ��TC��RXNE�ͱ���λ�ˣ�������뼰ʱ��λ��ʹ��USART2֮ǰ����
	*����һ��ʹ��USART2��ֱ���������жϣ����ȴ���λ�Ļ��ᶼû��
	�����������bug�������ԡ����޷�����
	******************************************************************
	*/



	//��	�������жϣ������жϲ��ҳ�ʼ�� <NVIC>��ʹ��<�ж�>
	//ֻ�е�����RX�򿪵�ʱ����п��ܳ����ж�	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//������RXNE�����жϡ�ʹ�ܼĴ���
	/****************************************************************/
		//Ĭ������Ϊ2λ���ȼ�


	//��	ʹ�ܴ���
	USART_Cmd(USART2, ENABLE);  //ʹ�ܴ���2 
	/*USART_Cmd()�ú����ڼĴ����ϵĲ�����
	��USART_CR1�Ĵ����е� UE λдENABLE��1����DISABLE��0���Կ���USART��ʹ�ܿ��ء�*/

	//��Ϊ������TE,ʹ��USART2ʱ���ڻ��Զ�����һ�����ֽڣ����Ǿ��ٴ���λ��TC�ı�־λ����ʱ��Ҫ���
	//�������ĺ���������һ���ȷ�����һ�����ֽ�Ҫ�磬����Ҫ�ȴ��䷢����һ�����ֽں� ������

	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET) {}//�ȴ�TC����λ
	USART_ClearFlag(USART2, USART_FLAG_TC);//TC��λ


	//Usart2 NVIC ��ʼ������
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;		//������USART2Ϊ�ж��ź�Դ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_USART2_PreemptionPriority;	//������ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_USART2_SubPriority;		//������Ӧ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ�ж�ͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�������NVIC
	/************************************************************/





}//uart_init��ʼ����������


//��Period��arr������������
//��Prescaler���Ƿ�Ƶϵ��
//��Ft��=��ʱ������Ƶ��,��λ:(MHz)
//��ʱ�����ʱ����㷽��:Tout=((Period+1)*(Prescaler+1))/Ft ��us��,�������ʱ�䣨us��
void TIM7_INT_Init(u16 arr, u16 psc) {


	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7ʱ��ʹ��    

	//��ʱ��TIM7��ʼ��
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

	TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE); //ʹ��ָ����TIM7�ж�,��������ж�
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
	TIM_Cmd(TIM7, DISABLE);			//�رն�ʱ��7
	
	USART2_RX_STA = 0;		//����


	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_TIM7_PreemptionPriority;//��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_TIM7_SubPriority;		//�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	
	
}



//��ESP8266��������
//cmd:���͵������ַ���;waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:�������ݺ󣬷������ķ�����֤��
u8* ESP8266_send_data(char* cmd, u16 waittime)
{
	char temp[5];
	char* ack = temp;
	USART2_RX_STA = 0;
	usart2_printf("%s", cmd);	//��������
	if (waittime)		//��Ҫ�ȴ�Ӧ��
	{
		while (--waittime)	//�ȴ�����ʱ
		{
			delay_ms(10);
			if (USART2_RX_STA & 0X8000)//���յ��ڴ���Ӧ����
			{
				USART2_RX_BUF[USART2_RX_STA & 0X7FFF] = 0;//��ӽ�����
				ack = (char*)USART2_RX_BUF;
				printf("ack:%s\r\n", (u8*)ack);
				USART2_RX_STA = 0;
				break;//�õ���Ч���� 
			}
		}
	}
	return (u8*)ack;
}




void usart2_printf(char* fmt, ...) {
	u16 len, j;
	va_list ap;
	va_start(ap, fmt);
	vsprintf((char*)USART2_TX_BUF, fmt, ap);
	va_end(ap);
	len = strlen((const char*)USART2_TX_BUF);		//�˴η������ݵĳ���
	for (j = 0; j < len; j++)	{						//ѭ����������
	
		USART_SendData(USART2, USART2_TX_BUF[j]);

		//printf("%c",USART2_TX_BUF[j]);//*******************����ط�����Ҫ��������֪��Ϊʲô��û������ͻ�һֱ����*********
		//delay_ms(10);//�ȴ���ŷ���USART2_TX_STA[7]����λ

		while ((USART2_TX_STA & 0x80) == RESET); //ѭ������,ֱ���������
		USART2_TX_STA &= ~(0x80);
	}

}


//��ʱ��7�жϷ������		    
void TIM7_IRQHandler(void) {
	//��������жϱ�ʾWiFi�������ڶ�Ӧʱ����û�н��յ���Ϣ�ˣ��ж�������ɡ�
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET) {//�Ǹ����ж�
		USART2_RX_STA |= 1 << 15;	//��ǽ������
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);  //���TIM7�����жϱ�־
		
		TIM_Cmd(TIM7, DISABLE);  //�ر�TIM7 
		D1 = !D1;
	}
}

//��	��д�жϴ���������������ʽӦ��Ϊ USARTxIRQHandler��xΪ���ںţ�
void USART2_IRQHandler(void) { //����1�жϷ�����򣬡�*���ú����ĵ���λ���������ļ�startup_stm32f40_41xxx.s��

	u8 Res;
#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntEnter();
#endif

	//���ж��ǲ��ǽ����ж�
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		//USART_SR ����5 RXNEλ�ǡ���ȡ���ݼĴ�������Ϊ�ձ�־������RDR��λ�Ĵ����������Ѿ����䵽USART_DR�Ĵ���ʱ����λӲ����1��SET��
		//0��δ���յ�����
		//1����׼���ý������ݡ�
		//USART_ClearITPendingBit(USART2, USART_IT_RXNE);//RXEN���㣨ͨ����DR�Ĵ�����ʵ�Ϳ���Ӳ����λ��

		Res = USART_ReceiveData(USART2);//(USART2->DR);	//DR�ǽ��յ������ݻ��ѷ��͵����ݡ�������Res
		//Res��8λ�ģ�DR��16λ�ģ�������������ת��
		//����Res�õ��˷��͹���������

		//ģ�鷢�ͻ�������Ϣ���кܶ��еģ��ж�ģ�鷢����ϵı�׼���ǡ�һ��ʱ�䡿��û���ٴη����µ���Ϣ
		if ((USART2_RX_STA & 0x8000) == 0) {//USART_RX_STA[15]==0  ����δ���
			if (USART2_RX_STA < USART2_REC_LEN)	{//�����Խ�������
				TIM_SetCounter(TIM7, 0);//���������
				if (USART2_RX_STA == 0) {
					TIM_Cmd(TIM7, ENABLE);//ʹ�ܶ�ʱ��7���ж� 
				}
				USART2_RX_BUF[USART2_RX_STA++] = Res;	//��¼���յ���ֵ	 
			}

			else {
				USART2_RX_STA |= 1 << 15;				//ǿ�Ʊ�ǽ������
				printf("WiFi�����������\n");
				TIM_Cmd(TIM7, DISABLE);
			}
		}
	}

	//���ж��ǲ��Ƿ����ж�
	if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) {
		USART_ClearITPendingBit(USART2, USART_IT_TC);//�����ض����printf()ʱ�������ظ�����TC�ж���ѭ��
		USART2_TX_STA |= 0x80;//��λ[7]��˵���������жϣ�������TC�жϡ�
	}

#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntExit();
#endif
}

