#include "sys.h"
#include "usart.h"	
#include "LED_STM32F407ZET6.h"
#include "delay.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif

/*——————————————————————————————————————*/
/*         Universal Synchronous Asynchronous Receiver Transmitter            */
/*			                   通用同步异步收发器                             */
/*——————————————————————————————————————*/

//此为串口1
//PA(8)		- CK
//PA(9)		- TX
//PA(10)	- RX
//PA(11)	- STC
//PA(12)	- RTS

//此为串口2——WIFI
//PA(2)		- TX
//PA(3)		- RX


//##特别注意：链接此文件出问题的话，说明需要使用文件stm32f4_ustart.c
//##特别注意：USART_ITConfig只能使用一个中断标志！如果需要打开多个中断，需要在在stm32f4xx_conf.h中，打开标志并定义函数

/*#关键提示：这里要弄清楚，可以操作USART1中断标志位（也就是输入参数是USART的中断类型）的库函数有三个： 
USART_ITConfig：			使能 / 失能中断
USART_ GetITStatus：		读取中断状态
USART_ClearITPendingBit：	清除中断标志位
其中，清除中断标志位用： USART_ClearITPendingBit；
*/

//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug

//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式

//V1.5修改说明
//1,增加了对UCOSII的支持

//V1.6修改说明
//1,修正了启用TCIE中断后程序进入TCIE中断无限循环的BUG
//2,添加了程序的USART_Echo等待响应函数
//3,添加了中断执行程序中的【TC中断】判断
//4,修改了重定义fputc函数中的TC标志位的判断方法，采用了自己设计的USART_TX_STA来判断，以便之后能够利用TCIE

//V1.7修改说明
//1，添加了WIFI串口——USART2
////////////////////////////////////////////////////////////////////////////////// 	  


//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x){ 
	x = x; 
}

//重定义fputc函数 
int fputc(int ch, FILE *f){ 	
	USART1->DR = (u8)ch;
	while ((USART1->SR & 0X40) == 0);//循环发送,直到发送完毕   
	return ch;
	
	/*
	##注意
	如果打开了TCIE的话，那么在
	USART_SendData(USART1, (u8)ch);
	发送完毕的那一瞬间，TC置位，系统就进入了USART中断处理函数，
	（如果没有添加中断处理的话，TC标志位永远不会发生改变）
	要注意这个地方一定要有中断处理
	*/
	 

}
#endif


#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART1_RX_BUF[USART1_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.

//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目


//自己定义的接收状态标记
//0x8000	[15]位是接收状态，1：接收完成，0：未接收
//0x4000	[14]位是接收到【0x0D】的判断位
//0x3FFF	[13:0]位是接收字节计数器，最大可接收2^14次方个字节，即 16MB内容
u16 USART1_RX_STA = 0;

//自己定义的发送状态标记
//0x80		[7]位是发送状态，1：单次发送完成，0：正在发送或未发送
u8  USART1_TX_STA = 0;

/*
**************************************************************************************
*	串口定义的几个步骤
*	①	串口时钟使能，GPIO时钟使能*
*	②	设置引脚复用器映射
*	③	GPIO初始化设置——设置端口模式为【复用模式】
*	④	串口初始化设置——设置【波特率】【字长】【奇偶校验】
*	⑤	（开启中断）开启中断并且初始化 <NVIC>，使能中断 
*	⑥	使能串口
*	⑦	编写中断处理函数——函数格式应该为 USARTxIRQHandler（x为串口号）
*
*	
*
*	*串口1是挂载在APB2下面的外设，要启动串口，就要使能APB2外设
*	*串口2是挂载在APB1下面的外设，要启动串口，就要使能APB1外设
**************************************************************************************
*/

//初始化IO 串口1 
//bound:波特率
void usart1_init(u32 bound){
	
   //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	
	//①	串口1时钟使能，GPIOA时钟使能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟，处于APB2总线
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟，处于APB1总线


	//②	设置引脚复用器映射
	//需要复用两个端口
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1
	

	//③	GPIO初始化设置——设置端口模式为【复用模式】
	//USART1端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10


	//④	串口初始化设置——设置【波特率】【字长】【奇偶校验】
	//USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置，一般为9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式，激活RE与TE
	USART_Init(USART1, &USART_InitStructure); //初始化串口1


#if EN_USART1_TX
	//开启【发送完成TC中断】
	//#define EN_USART1_TX = ENABLE串口1【发送完成中断TCIE】是否打开
	USART_ITConfig(USART1, USART_IT_TC, EN_USART1_TX);
	
	//暂时不发送信息，所以TXE（发送数据寄存器为空）对应的IE位—TXEIE要关闭
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ClearFlag(USART1, USART_FLAG_TC);//必须及时复位（使能USART1之前）
	/*巨大BUG处
	******************************************************************
	*其实一但初始化USART1的时钟的时候，TC和RXNE就被置位了，这里必须及时复位（使能USART1之前），
	*否则一旦使能USART1后，直接跳进了中断，连等待复位的机会都没有
	￥￥并且这个bug连【调试】都无法发现
	******************************************************************
	*/
#endif


	//⑤	（开启中断）开启中断并且初始化 <NVIC>，使能<中断>
	//只有当接收RX打开的时候才有可能出现中断	
#if EN_USART1_RX	//该设置在usart.h中可以看到
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启【RXNE接收中断】使能寄存器
	/****************************************************************/
		//默认设置为2位优先级
#endif
	
	//⑥	使能串口
	USART_Cmd(USART1, ENABLE);  //使能串口1 
	/*USART_Cmd()该函数在寄存器上的操作是
	向USART_CR1寄存器中的 UE 位写ENABLE（1）或DISABLE（0）以控制USART的使能开关。*/

	//因为开启了TE,使能USART1时串口会自动发送一个空字节，于是就再次置位了TC的标志位，这时需要清除
	//这个清零的函数的运行一定比发送完一个空字节要早，所以要等待其发送完一个空字节后， 再清零

#ifdef EN_USART1_TX		//【发送完成中断TCIE】打开的话，就需要这个等待部分
	while (USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET) {	}//等待TC被置位
	USART_ClearFlag(USART1, USART_FLAG_TC);//TC复位
#endif // EN_USART1_TX	

	delay_ms(100);
	LED2=0;
	delay_ms(100);
	LED2=1;
	delay_ms(50);
	
	//Usart1 NVIC 初始化配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		//串配置USART1为中断信号源
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_USART1_PreemptionPriority;	//设置抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_USART1_SubPriority;		//设置响应优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ中断通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数配置NVIC
	/************************************************************/

	//USART_ClearITPendingBit(USART1,USART_IT_RXNE);

	/********************************************************/
	/*注意，开启了NVIC中断后，需要设置系统本身的中断的优先级*/
	/********************************************************/

	delay_ms(100);
	LED2=0;
	delay_ms(50);
	LED2=1;
	
	
}//uart_init初始化函数结束



//⑦	编写中断处理函数——函数格式应该为 USARTxIRQHandler（x为串口号）
void USART1_IRQHandler(void) //串口1中断服务程序，【*】该函数的调用位置在启动文件startup_stm32f40_41xxx.s中
{
	u8 Res;
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();
#endif

	//接收中断(接收到的数据必须是0x0D 0x0A结尾)——一个换行符+一个置顶符就是一个回车
	//0x0D = 1101B, 0x0A = 1010B
	//先判断是不是接收中断
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{	//USART_SR 比特5 RXNE位是【读取数据寄存器】不为空标志符，当RDR移位寄存器的内容已经传输到USART_DR寄存器时，该位硬件置1（SET）
		//0：未接收到数据
		//1：已准备好接收数据。
		//USART_ClearITPendingBit(USART1, USART_IT_RXNE);//RXEN清零（通过读DR寄存器其实就可以硬件复位）

		Res = USART_ReceiveData(USART1);//(USART1->DR);	//DR是接收到的数据或已发送的数据——放入Res
		//Res是8位的，DR是16位的，这里有了类型转换
		//这里Res得到了发送过来的数据

		if ((USART1_RX_STA & 0x8000) == 0)//接收未完成
		{
			if (USART1_RX_STA & 0x4000)//接收到了0x0d
			{
				if (Res != 0x0a)USART1_RX_STA = 0;//接收错误,重新开始
				else USART1_RX_STA |= 0x8000;	//接收完成了 
			}
			else //还没收到0X0D
			{
				if (Res == 0x0d)USART1_RX_STA |= 0x4000;
				else
				{
					USART1_RX_BUF[USART1_RX_STA & 0X3FFF] = Res;
					USART1_RX_STA++;
					if (USART1_RX_STA > (USART1_REC_LEN - 1))USART1_RX_STA = 0;//接收数据错误,重新开始接收	  
				}
			}
		}
		/*
		当接收到从电脑发过来的数据，把接收到的数据保存在USART_RX_BUF 中，
		同时在接收状态寄存器（USART_RX_STA）中计数接收到的有效数据个数，
		当收到回车（0X0D，0X0A）的第一个字节0X0D 时，计数器将不再增加，等待0X0A 的到来，
		而如果0X0A 没有来到，则认为这次接收失败，重新开始下一次接收。
		如果顺利接收到0X0A，则标记USART_RX_STA的第15位，这样完成一次接收，并等待该位被其他程序清除，从而开始下一次的接收，
		而如果迟迟没有收到0X0D，那么在接收数据超过 USART_REC_LEN 个了，则会丢弃前面的数据，重新接收。
		*/


	}



#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();
#endif
}


#endif
