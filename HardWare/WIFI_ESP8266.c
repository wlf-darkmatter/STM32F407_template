#include <WIFI_ESP8266.h>
#include <LED_STM32F407ZET6.h>
#include <OLED.h>
/*
1、ESP8266的RXD（数据的接收端）需要连接USB转TTL模块的TXD，TXD（数据的发送端）需要连接USB转TTL模块的RXD，这是基本的；
2、关于VCC的选取，在USB转TTL模块上有3.3V和5V两个引脚可以作为VCC，但是一般选取5V作为VCC。如果选取3.3V，可能会因为供电不足而引起不断的重启，从而不停的复位。
*/
/*
【ESP8266】波特率一般为115200
AT指令不区分大小写，均以回车、换行结尾。下面介绍常用的AT指令：*/
/*
指令名					响应				含义
AT						OK					测试指令
AT+CWMODE=<mode>		OK					设置应用模式（需重启生效）
AT+CWMODE_DEF=1  # 设置 Wi-Fi 为 Station 模式并保存到 Flash。 2 为 AP 模式
AT+CWMODE?				+CWMODE:<mode>		获得当前应用模式
AT+CWLAP				+CWLAP:<ecn>,<ssid>,<rssi>	返回目前的AP列表
AT+CWJAP=<ssid>,<pwd>	OK					加入某一AP
AT+CWJAP?				+CWJAP:<ssid>		返回当前加入的AP
AT+CWQAP				OK					退出当前加入的AP
AT+CIPSTART=<type>,<addr>,<port>	OK		建立TCP/UDP连接
AT+CIPMUX=<mode>		OK					是否启用多连接
AT+CIPSEND=<param>		OK					发送数据
AT+CIPMODE=<mode>		OK					是否进入透传模式

①AT+CWMODE=1：设置工作模式（STA模式）
②AT+RST：模块重启（生效工作模式）
③AT+CWJAP="111","11111111"：连接当前环境的WIFI热点（热点名，密码）
④AT+CIPMUX=0：设置单路连接模式
⑤AT+CIPSTART="TCP","xxx.xxx.xxx.xxx",xxxx：建立TCP连接
⑥AT+CIPMODE=1：开启透传模式
⑦AT+CIPSEND：透传模式下，传输数据
⑧+++：退出透传模式

//此为串口2——【WiFi】
//PA(2)		- TX
//PA(3)		- RX

//EC:FA:BC:59:32:96为ESP-12F的地址

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
//用于调试回复的信息
char info[50];

void ESP8266_restart(void) {
	//让Wifi模块重启的命令
	printf("进入重启程序\n");
	for (int i = 0; i < 10; i++) {//重复次数不大于10*3次
		if (ESP8266_send_cmd("AT+RST", "OK", 4000) == 0) {//等待4秒
			printf("重启完毕\n");
//			OLED_DrawStr(0, 24, "WiFi Activated.         ", 12, 1);
			return;
		}
	}
	printf("重启失败！\n");
//	printf("3s 等待结束\n");
	
	
}


//占用OLED一小部分用于显示
//设置工作模式 1：station模式   2：AP模式  3：兼容 AP+station模式	
void ESP8266_init(void) {
	
	TIM7_INT_Init(1000 - 1, 840 - 1);		//10ms中断
	usart2_init(115200);
	int timeout = (int)Get_TIMx_OutputTime(TIM7);
	printf("TIM7单次溢出时间为：%d ms = %d us\n", timeout / 1000, timeout);


	//测试
	if (ESP8266_send_cmd("AT", "OK", 500) == 0)
		printf("AT 测试成功\n");
	else
		printf("AT 测试失败\n");
	//设置工作模式 1：station模式   2：AP模式  3：兼容 AP+station模式
	
	ESP8266_send_cmd("AT+CWMODE_DEF=3", "OK", 500);


	/*
指令名					响应				含义
AT						OK					测试指令
AT+CWMODE=<mode>		OK					设置应用模式（需重启生效）
AT+CWMODE?				+CWMODE:<mode>		获得当前应用模式
AT+CWLAP				+CWLAP:<ecn>,<ssid>,<rssi>	返回目前的AP列表
AT+CWJAP=<ssid>,<pwd>	WIFI GOT IP			加入某一AP
AT+CWJAP?				+CWJAP:<ssid>		返回当前加入的AP
AT+CWQAP				OK					退出当前加入的AP
AT+CIPSTART=<type>,<addr>,<port>	OK		建立TCP/UDP连接
AT+CIPMUX=<mode>		OK					是否启用多连接
AT+CIPSEND=<param>		OK					发送数据
AT+CIPMODE=<mode>		OK					是否进入透传模式
*/
//让模块连接上自己的路由
	ESP8266_restart();
}

void ESP8266_WiFiConnect(char* SSID, char* passward) {
	char CWJAP_str[60];//用于存放控制命令
	memset(CWJAP_str, '\0', 60 * sizeof(char));//清零

	strcat(CWJAP_str,"AT+CWJAP=\"");//AT+CWJAP="
	strcat(CWJAP_str, SSID);//AT+CWJAP="SSID
	strcat(CWJAP_str, "\",\"");//AT+CWJAP="SSID","
	strcat(CWJAP_str, passward);//AT+CWJAP="SSID","passward
	strcat(CWJAP_str, "\"");//AT+CWJAP="SSID","passward"

//"AT+CWJAP=\"东南沿海王大哥\",\"19981213\""
	if (ESP8266_send_cmd(CWJAP_str, "WIFI GOT IP", 20000) == 0) {

	}
	else {
		printf("20秒没有连接上指定的WiFi。\n%s", USART2_RX_BUF);
		OLED_DrawStr(0, 24, "WiFi connection failed.", 12, 0);
		OLED_DrawStr(0, 36, "I'm so Sorry.          ", 12, 0);
		OLED_DrawStr(0, 48, "Please contact WLF.   ", 12, 0);
		return;
	}

	if (ESP8266_send_cmd("AT+CIFSR", "OK", 2500) == 0) printf("已连接WLAN\n");//(查看模块地址)
	else printf("没有真正连接上指定的WiFi\n%s", USART2_RX_BUF);
//	printf("%s", USART2_RX_BUF);
//	printf("***********************\n%s\n***********************", USART2_RX_BUF);

	
	OLED_DrawStr(0, 12, "WiFi connected.      ", 12, 0);

}
//AT+ CWSAP= <ssid>,<pwd>,<chl>,<ecn>
//	ssid:接入点名称
//	pwd:密码
//	chl : 通道号
//	ecn : 加密方式:（0 - OPEN， 1 - WEP， 2 - WPA_PSK， 3 - WPA2_PSK， 4 - WPA_WPA2_PSK）
void ESP8266_WiFiEmit(char* SSID, char* passward) {
	
	char CWSAP_str[60];//用于存放控制命令
	memset(CWSAP_str, '\0', 60 * sizeof(char));//清零

	strcat(CWSAP_str, "AT+CWSAP=\"");//AT+CWSAP="
	strcat(CWSAP_str, SSID);//AT+CWSAP="SSID
	strcat(CWSAP_str, "\",\"");//AT+CWSAP="SSID","
	strcat(CWSAP_str, passward);//AT+CWSAP="SSID","passward
	strcat(CWSAP_str, "\",1,4");//AT+CWSAP="SSID","passward",1,4   ***这里没有引号！！！
	//用【通道1】和【WPA_WPA2_PSK】

	if (ESP8266_send_cmd(CWSAP_str, "OK", 40000) == 0) {
		printf("已创建WLAN\nWiFi名：%s\n密码：%s\n", SSID, passward);

		OLED_DrawStr(0, 24, "WiFi:", 12, 0);
		OLED_DrawStr(30, 24, SSID, 12, 0);
	}
	else {
		printf("20秒没有创建指定的WiFi。\n%s", USART2_RX_BUF);//返回错误信息
		OLED_DrawStr(0, 24, "WiFi create failed.   ", 12, 0);
		OLED_DrawStr(0, 36, "I'm so Sorry.          ", 12, 0);
		OLED_DrawStr(0, 48, "Please contact WLF.   ", 12, 0);
		return;
	}
	ESP8266_restart();
}

void ESP8266_TCP_Server(void) {
	char TCP_info[50];
	memset(TCP_info, '\0', 50 * sizeof(char));//清零

	//=0：单路连接模式     =1：多路连接模式
	ESP8266_send_cmd("AT+CIPMUX=1", "OK", 200);
	
	/**************************************************************************
	指令：
	1)单路连接时(+CIPMUX=0)，指令为：AT+CIPSTART=<type>,<addr>,<port>
	2)多路连接时(+CIPMUX=1)，指令为：AT+CIPSTART=<id>,<type>,<addr>,<port>
	port不需要引号
	如果格式正确且连接成功，返回 OK，否则返回 ERROR
	如果连接已经存在，返回ALREAY?CONNECT
	说明：
	<id>:0-4，连接的id号
	<type>:字符串参数，表明连接类型，“TCP”-建立tcp连接，”UDP”-建立UDP连接
	<addr>:字符串参数，远程服务器IP地址
	<port>:远程服务器端口号
	*************************************************************************
	注：开启服务器模式的方法 
	开启多路连接：					AT+CIPMUX=1 
	开启服务器模式，端口为8080，缺省值为333：	AT+CIPSERVER=1,8080
	*************************************************************************
	指令：AT+CIPSERVER=mode,[port] 
	说明：
	mode:0-关闭server模式，1-开启server模式 　　　
	port:端口号，缺省值为333 响应：OK 
	*************************************************************************
	说明：
	(1) AT+CIPMUX=1时才能开启服务器；关闭server模式需要重启 　　　
	(2)开启server后自动建立server监听,当有client接入会自动按顺序占用一个连接。
	*************************************************************************
	*/
	if (ESP8266_send_cmd("AT+CIPSERVER=1,525", "OK", 200) == 0) {//创建服务器
		OLED_DrawStr(0, 48, "TCP Established.      ", 12, 1);
		delay_ms(4000);

		/*例如：
		CIFSR:APIP,"192.168.4.1"
		CIFSR:APMAC,"de:4f:22:7d:49:18"
		CIFSR:STAIP,"192.168.1.104"
		CIFSR:STAMAC,"dc:4f:22:7d:49:18"
		*/
		ESP8266_send_cmd("AT+CIFSR","OK", 200);//查询
		if (Info_fetch((char*)USART2_RX_BUF, "CIFSR:STAIP") == 0) {
			printf("STA IP: %s\n", info);
			strcat(TCP_info, "STA IP:");
			strcat(TCP_info, info);
		OLED_DrawStr(0, 36, TCP_info, 12, 1);
		}
		else {
			printf("查询失败\n");
			OLED_DrawStr(0, 36, "TCP IP error       ", 12, 1);
		}

		OLED_DrawStr(0, 48, "TCP port: 525        ", 12, 1);
	}
	else {
		OLED_DrawStr(0, 48, "TCP server error.    ", 12, 1);
	}



/*客户端————————
	char CIPSTART_str[60];//用于存放控制命令
	for (int i = 0; i < 60; i++) CIPSTART_str[i] = 0;

	strcat(CIPSTART_str, "AT+CIPSTART=\"TCP\",\"");//AT+CIPSTART="TCP","
	strcat(CIPSTART_str, addr);//AT+CWJAP="TCP","addr
	strcat(CIPSTART_str, "\",");//AT+CWJAP="TCP","addr",

	strcat(CIPSTART_str, port);//AT+CWJAP="TCP","addr",port
*/

/*
	//建立TCP连接  这四项分别代表了 要连接的ID号0~4   连接类型  远程服务器IP地址   远程服务器端口号
	while (ESP8266_send_cmd(CIPSTART_str, "CONNECT", 2000));
	//是否开启透传模式  0：表示关闭 1：表示开启透传
	ESP8266_send_cmd("AT+CIPMODE=1", "OK", 2000);
	//透传模式下 开始发送数据的指令 这个指令之后就可以直接发数据了
	ESP8266_send_cmd("AT+CIPSEND", "OK", 500);
	*/
}


//char info[50];读取对应字符其逗号后的部分，如果有引号，则不包含引号
//return 1——没有找到符合的回应
//return 2——符合的回应后面没有逗号
//return 3——查找到目标字符串，但是无法收敛语句的结尾处。
int Info_fetch(char* Source, const char* String) {
	memset(info, '\0', 50 * sizeof(char));//清零
	char* head = Source;
	char* tail = NULL;
	/*【检测示例】：  
	AT+CIFSR
	+CIFSR:APIP,"192.168.4.1"
	+CIFSR:APMAC,"de:4f:22:7d:49:18"
	+CIFSR:STAIP,"192.168.1.104"
	+CIFSR:STAMAC,"dc:4f:22:7d:49:18"
	*/
	printf("**************\nInfo_fetch()\n %s\n**************", Source);
	//取CIFSR:STAIP之后的字符串部分
	//head跟踪头部；
	//tail收敛尾部；
	head = strstr(Source, String);
	if (head == NULL) {
		printf("Info_fetch失败，没有找到符合条件的回应。\n");
		return 1;
	}
	head = strstr(head, ",") + 1; //收敛头部到“逗号”处
	if (head == NULL) {
		printf("Info_fetch失败，符合的回应后面没有逗号。\n");
		return 2;
	}
	tail = strstr(head, "\r\n")-1;//查找此句之后遇到的第一个换行符，收敛尾部,收敛到\r处，
	if (tail == NULL) {
		printf("Info_fetch失败，查找到目标字符串，但是无法收敛语句的结尾处。\n");
		return 3;
	}
//检查是否有引号
	if (*head == '\"') {
		head += 1;
		tail -= 1;
	}
	strncpy(info, head, tail - head + 1); //拷贝到info
	return 0;
}



//向ESP8266发送命令
//cmd:发送的命令字符串;ack:期待的应答结果,如果为空,则表示不需要等待应答;waittime:等待时间(单位:ms)
//返回值:0,发送成功(得到了期待的应答结果);1,发送失败
u8 ESP8266_send_cmd(char* cmd, char* ack, int waittime) {
	int waittime_copy = waittime;
	waittime /= 10;//折算一下，后面有delay_ms(10); 
	for (int i = 0; i < 3; i++) {
		USART2_RX_STA = 0;
		usart2_printf("%s\r\n", cmd);	//发送命令
		//	printf("%s\r\n", cmd);
		if (ack && waittime) {		//需要等待应答
			while (--waittime) {	//等待倒计时
				delay_ms(10); 
				if (USART2_RX_STA & 0X8000) {//TIM7计时是否结束,结束则取已读内容
					if (ESP8266_check_cmd((u8*)ack)) {
						printf("%s\r\n",ack);
						return (u8)0;//得到有效数据 
					}
					else printf(".");
					USART2_RX_STA = 0;
				}
			}
			if (waittime == 0) {
				printf("等待超时> %d (ms)（3次重试），发送失败指令: %s\n", (int)waittime_copy * 10, cmd);
			}
		}
	}
	return 1;
}

//ESP8266发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果;其他,期待应答结果的位置(str的位置)
u8* ESP8266_check_cmd(u8* str) {
	char* strx = 0;
	if (USART2_RX_STA & 0X8000)		//接收到一次数据了
	{
		USART2_RX_BUF[USART2_RX_STA & 0X7FFF] = 0;//添加结束符
		strx = strstr((const char*)USART2_RX_BUF, (const char*)str);
	}
	return (u8*)strx;
}

//ESP8266退出透传模式   返回值:0,退出成功;1,退出失败
//配置wifi模块，通过想wifi模块连续发送3个+（每个+号之间 超过10ms,这样认为是连续三次发送+）
u8 ESP8266_quit_trans(void)
{
	u8 result = 1;
	usart2_printf("+++");
	delay_ms(1000);					//等待500ms太少 要1000ms才可以退出
	result = ESP8266_send_cmd("AT", "OK", 20);//退出透传判断.
	if (result)
		printf("quit_trans failed!");
	else
		printf("quit_trans success!");
	return result;
}



//初始化IO 串口2
//bound:115200 (ESP8266的默认值)
void usart2_init(u32 bound) {
	//PA(2)		- TX
	//PA(3)		- RX

	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;


	//①	串口1时钟使能，GPIOA时钟使能
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);//使能USART2时钟，处于APB1总线
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //使能GPIOA时钟，处于AHB1总线


	//②	设置引脚复用器映射
	//需要复用两个端口
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //GPIOA2复用为USART2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //GPIOA3复用为USART2


	//③	GPIO初始化设置——设置端口模式为【复用模式】
	//USART2端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2，PA3


	//④	串口初始化设置——设置【波特率】【字长】【奇偶校验】
	//USART2 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置，115200 (ESP8266的默认值)
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式，激活RE与TE
	USART_Init(USART2, &USART_InitStructure); //初始化串口1



	//开启【发送完成TC中断】
	//#define EN_USART2_TX = ENABLE串口1【发送完成中断TCIE】是否打开
	USART_ITConfig(USART2, USART_IT_TC, ENABLE);

	//暂时不发送信息，所以TXE（发送数据寄存器为空）对应的IE位—TXEIE要关闭
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	USART_ClearFlag(USART2, USART_FLAG_TC);//必须及时复位（使能USART2之前）
	/*巨大BUG处
	******************************************************************
	*其实一但初始化USART2的时钟的时候，TC和RXNE就被置位了，这里必须及时复位（使能USART2之前），
	*否则一旦使能USART2后，直接跳进了中断，连等待复位的机会都没有
	￥￥并且这个bug连【调试】都无法发现
	******************************************************************
	*/



	//⑤	（开启中断）开启中断并且初始化 <NVIC>，使能<中断>
	//只有当接收RX打开的时候才有可能出现中断	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启【RXNE接收中断】使能寄存器
	/****************************************************************/
		//默认设置为2位优先级


	//⑥	使能串口
	USART_Cmd(USART2, ENABLE);  //使能串口2 
	/*USART_Cmd()该函数在寄存器上的操作是
	向USART_CR1寄存器中的 UE 位写ENABLE（1）或DISABLE（0）以控制USART的使能开关。*/

	//因为开启了TE,使能USART2时串口会自动发送一个空字节，于是就再次置位了TC的标志位，这时需要清除
	//这个清零的函数的运行一定比发送完一个空字节要早，所以要等待其发送完一个空字节后， 再清零

	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET) {}//等待TC被置位
	USART_ClearFlag(USART2, USART_FLAG_TC);//TC复位


	//Usart2 NVIC 初始化配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;		//串配置USART2为中断信号源
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_USART2_PreemptionPriority;	//设置抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_USART2_SubPriority;		//设置响应优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ中断通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数配置NVIC
	/************************************************************/





}//uart_init初始化函数结束


//【Period（arr）】是周期数
//【Prescaler】是分频系数
//【Ft】=定时器工作频率,单位:(MHz)
//定时器溢出时间计算方法:Tout=((Period+1)*(Prescaler+1))/Ft （us）,返回溢出时间（us）
void TIM7_INT_Init(u16 arr, u16 psc) {


	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7时钟使能    

	//定时器TIM7初始化
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

	TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE); //使能指定的TIM7中断,允许更新中断
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
	TIM_Cmd(TIM7, DISABLE);			//关闭定时器7
	
	USART2_RX_STA = 0;		//清零


	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_TIM7_PreemptionPriority;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_TIM7_SubPriority;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	
	
}



//向ESP8266发送数据
//cmd:发送的命令字符串;waittime:等待时间(单位:10ms)
//返回值:发送数据后，服务器的返回验证码
u8* ESP8266_send_data(char* cmd, u16 waittime)
{
	char temp[5];
	char* ack = temp;
	USART2_RX_STA = 0;
	usart2_printf("%s", cmd);	//发送命令
	if (waittime)		//需要等待应答
	{
		while (--waittime)	//等待倒计时
		{
			delay_ms(10);
			if (USART2_RX_STA & 0X8000)//接收到期待的应答结果
			{
				USART2_RX_BUF[USART2_RX_STA & 0X7FFF] = 0;//添加结束符
				ack = (char*)USART2_RX_BUF;
				printf("ack:%s\r\n", (u8*)ack);
				USART2_RX_STA = 0;
				break;//得到有效数据 
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
	len = strlen((const char*)USART2_TX_BUF);		//此次发送数据的长度
	for (j = 0; j < len; j++)	{						//循环发送数据
	
		USART_SendData(USART2, USART2_TX_BUF[j]);

		//printf("%c",USART2_TX_BUF[j]);//*******************这个地方很重要，不过不知道为什么，没有这里就会一直错误*********
		//delay_ms(10);//等待后才发现USART2_TX_STA[7]被置位

		while ((USART2_TX_STA & 0x80) == RESET); //循环发送,直到发送完毕
		USART2_TX_STA &= ~(0x80);
	}

}


//定时器7中断服务程序		    
void TIM7_IRQHandler(void) {
	//进入这个中断表示WiFi接收器在对应时间内没有接收到信息了，判定接收完成。
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET) {//是更新中断
		USART2_RX_STA |= 1 << 15;	//标记接收完成
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);  //清除TIM7更新中断标志
		
		TIM_Cmd(TIM7, DISABLE);  //关闭TIM7 
		D1 = !D1;
	}
}

//⑦	编写中断处理函数——函数格式应该为 USARTxIRQHandler（x为串口号）
void USART2_IRQHandler(void) { //串口1中断服务程序，【*】该函数的调用位置在启动文件startup_stm32f40_41xxx.s中

	u8 Res;
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();
#endif

	//先判断是不是接收中断
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		//USART_SR 比特5 RXNE位是【读取数据寄存器】不为空标志符，当RDR移位寄存器的内容已经传输到USART_DR寄存器时，该位硬件置1（SET）
		//0：未接收到数据
		//1：已准备好接收数据。
		//USART_ClearITPendingBit(USART2, USART_IT_RXNE);//RXEN清零（通过读DR寄存器其实就可以硬件复位）

		Res = USART_ReceiveData(USART2);//(USART2->DR);	//DR是接收到的数据或已发送的数据——放入Res
		//Res是8位的，DR是16位的，这里有了类型转换
		//这里Res得到了发送过来的数据

		//模块发送回来的信息是有很多行的，判定模块发送完毕的标准就是【一定时间】内没有再次发送新的消息
		if ((USART2_RX_STA & 0x8000) == 0) {//USART_RX_STA[15]==0  接收未完成
			if (USART2_RX_STA < USART2_REC_LEN)	{//还可以接收数据
				TIM_SetCounter(TIM7, 0);//计数器清空
				if (USART2_RX_STA == 0) {
					TIM_Cmd(TIM7, ENABLE);//使能定时器7的中断 
				}
				USART2_RX_BUF[USART2_RX_STA++] = Res;	//记录接收到的值	 
			}

			else {
				USART2_RX_STA |= 1 << 15;				//强制标记接收完成
				printf("WiFi接收数据溢出\n");
				TIM_Cmd(TIM7, DISABLE);
			}
		}
	}

	//再判断是不是发送中断
	if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) {
		USART_ClearITPendingBit(USART2, USART_IT_TC);//利用重定义的printf()时，不想重复进入TC中断死循环
		USART2_TX_STA |= 0x80;//置位[7]，说明进入了中断，并且是TC中断。
	}

#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();
#endif
}

