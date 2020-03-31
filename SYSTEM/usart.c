#include "sys.h"
#include "usart.h"	
#include "LED_STM32F407ZET6.h"
#include "delay.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos ʹ��	  
#endif

/*����������������������������������������������������������������������������*/
/*         Universal Synchronous Asynchronous Receiver Transmitter            */
/*			                   ͨ��ͬ���첽�շ���                             */
/*����������������������������������������������������������������������������*/

//��Ϊ����1
//PA(8)		- CK
//PA(9)		- TX
//PA(10)	- RX
//PA(11)	- STC
//PA(12)	- RTS

//��Ϊ����2����WIFI
//PA(2)		- TX
//PA(3)		- RX


//##�ر�ע�⣺���Ӵ��ļ�������Ļ���˵����Ҫʹ���ļ�stm32f4_ustart.c
//##�ر�ע�⣺USART_ITConfigֻ��ʹ��һ���жϱ�־�������Ҫ�򿪶���жϣ���Ҫ����stm32f4xx_conf.h�У��򿪱�־�����庯��

/*#�ؼ���ʾ������ҪŪ��������Բ���USART1�жϱ�־λ��Ҳ�������������USART���ж����ͣ��Ŀ⺯���������� 
USART_ITConfig��			ʹ�� / ʧ���ж�
USART_ GetITStatus��		��ȡ�ж�״̬
USART_ClearITPendingBit��	����жϱ�־λ
���У�����жϱ�־λ�ã� USART_ClearITPendingBit��
*/

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

//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��

//V1.6�޸�˵��
//1,����������TCIE�жϺ�������TCIE�ж�����ѭ����BUG
//2,����˳����USART_Echo�ȴ���Ӧ����
//3,������ж�ִ�г����еġ�TC�жϡ��ж�
//4,�޸����ض���fputc�����е�TC��־λ���жϷ������������Լ���Ƶ�USART_TX_STA���жϣ��Ա�֮���ܹ�����TCIE

//V1.7�޸�˵��
//1�������WIFI���ڡ���USART2
////////////////////////////////////////////////////////////////////////////////// 	  


//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE{ 
	int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
void _sys_exit(int x){ 
	x = x; 
}

//�ض���fputc���� 
int fputc(int ch, FILE *f){ 	
	USART1->DR = (u8)ch;
	while ((USART1->SR & 0X40) == 0);//ѭ������,ֱ���������   
	return ch;
	
	/*
	##ע��
	�������TCIE�Ļ�����ô��
	USART_SendData(USART1, (u8)ch);
	������ϵ���һ˲�䣬TC��λ��ϵͳ�ͽ�����USART�жϴ�������
	�����û������жϴ���Ļ���TC��־λ��Զ���ᷢ���ı䣩
	Ҫע������ط�һ��Ҫ���жϴ���
	*/
	 

}
#endif


#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART1_RX_BUF[USART1_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.

//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ


//�Լ�����Ľ���״̬���
//0x8000	[15]λ�ǽ���״̬��1��������ɣ�0��δ����
//0x4000	[14]λ�ǽ��յ���0x0D�����ж�λ
//0x3FFF	[13:0]λ�ǽ����ֽڼ����������ɽ���2^14�η����ֽڣ��� 16MB����
u16 USART1_RX_STA = 0;

//�Լ�����ķ���״̬���
//0x80		[7]λ�Ƿ���״̬��1�����η�����ɣ�0�����ڷ��ͻ�δ����
u8  USART1_TX_STA = 0;

/*
**************************************************************************************
*	���ڶ���ļ�������
*	��	����ʱ��ʹ�ܣ�GPIOʱ��ʹ��*
*	��	�������Ÿ�����ӳ��
*	��	GPIO��ʼ�����á������ö˿�ģʽΪ������ģʽ��
*	��	���ڳ�ʼ�����á������á������ʡ����ֳ�������żУ�顿
*	��	�������жϣ������жϲ��ҳ�ʼ�� <NVIC>��ʹ���ж� 
*	��	ʹ�ܴ���
*	��	��д�жϴ���������������ʽӦ��Ϊ USARTxIRQHandler��xΪ���ںţ�
*
*	
*
*	*����1�ǹ�����APB2��������裬Ҫ�������ڣ���Ҫʹ��APB2����
*	*����2�ǹ�����APB1��������裬Ҫ�������ڣ���Ҫʹ��APB1����
**************************************************************************************
*/

//��ʼ��IO ����1 
//bound:������
void usart1_init(u32 bound){
	
   //GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	
	//��	����1ʱ��ʹ�ܣ�GPIOAʱ��ʹ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//ʹ��USART1ʱ�ӣ�����APB2����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //ʹ��GPIOAʱ�ӣ�����APB1����


	//��	�������Ÿ�����ӳ��
	//��Ҫ���������˿�
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9����ΪUSART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10����ΪUSART1
	

	//��	GPIO��ʼ�����á������ö˿�ģʽΪ������ģʽ��
	//USART1�˿�����
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9��GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOA,&GPIO_InitStructure); //��ʼ��PA9��PA10


	//��	���ڳ�ʼ�����á������á������ʡ����ֳ�������żУ�顿
	//USART1 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//���������ã�һ��Ϊ9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ������RE��TE
	USART_Init(USART1, &USART_InitStructure); //��ʼ������1


#if EN_USART1_TX
	//�������������TC�жϡ�
	//#define EN_USART1_TX = ENABLE����1����������ж�TCIE���Ƿ��
	USART_ITConfig(USART1, USART_IT_TC, EN_USART1_TX);
	
	//��ʱ��������Ϣ������TXE���������ݼĴ���Ϊ�գ���Ӧ��IEλ��TXEIEҪ�ر�
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ClearFlag(USART1, USART_FLAG_TC);//���뼰ʱ��λ��ʹ��USART1֮ǰ��
	/*�޴�BUG��
	******************************************************************
	*��ʵһ����ʼ��USART1��ʱ�ӵ�ʱ��TC��RXNE�ͱ���λ�ˣ�������뼰ʱ��λ��ʹ��USART1֮ǰ����
	*����һ��ʹ��USART1��ֱ���������жϣ����ȴ���λ�Ļ��ᶼû��
	�����������bug�������ԡ����޷�����
	******************************************************************
	*/
#endif


	//��	�������жϣ������жϲ��ҳ�ʼ�� <NVIC>��ʹ��<�ж�>
	//ֻ�е�����RX�򿪵�ʱ����п��ܳ����ж�	
#if EN_USART1_RX	//��������usart.h�п��Կ���
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//������RXNE�����жϡ�ʹ�ܼĴ���
	/****************************************************************/
		//Ĭ������Ϊ2λ���ȼ�
#endif
	
	//��	ʹ�ܴ���
	USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���1 
	/*USART_Cmd()�ú����ڼĴ����ϵĲ�����
	��USART_CR1�Ĵ����е� UE λдENABLE��1����DISABLE��0���Կ���USART��ʹ�ܿ��ء�*/

	//��Ϊ������TE,ʹ��USART1ʱ���ڻ��Զ�����һ�����ֽڣ����Ǿ��ٴ���λ��TC�ı�־λ����ʱ��Ҫ���
	//�������ĺ���������һ���ȷ�����һ�����ֽ�Ҫ�磬����Ҫ�ȴ��䷢����һ�����ֽں� ������

#ifdef EN_USART1_TX		//����������ж�TCIE���򿪵Ļ�������Ҫ����ȴ�����
	while (USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET) {	}//�ȴ�TC����λ
	USART_ClearFlag(USART1, USART_FLAG_TC);//TC��λ
#endif // EN_USART1_TX	

	delay_ms(100);
	LED2=0;
	delay_ms(100);
	LED2=1;
	delay_ms(50);
	
	//Usart1 NVIC ��ʼ������
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		//������USART1Ϊ�ж��ź�Դ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_USART1_PreemptionPriority;	//������ռ���ȼ�
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_USART1_SubPriority;		//������Ӧ���ȼ�
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ�ж�ͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�������NVIC
	/************************************************************/

	//USART_ClearITPendingBit(USART1,USART_IT_RXNE);

	/********************************************************/
	/*ע�⣬������NVIC�жϺ���Ҫ����ϵͳ������жϵ����ȼ�*/
	/********************************************************/

	delay_ms(100);
	LED2=0;
	delay_ms(50);
	LED2=1;
	
	
}//uart_init��ʼ����������



//��	��д�жϴ���������������ʽӦ��Ϊ USARTxIRQHandler��xΪ���ںţ�
void USART1_IRQHandler(void) //����1�жϷ�����򣬡�*���ú����ĵ���λ���������ļ�startup_stm32f40_41xxx.s��
{
	u8 Res;
#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntEnter();
#endif

	//�����ж�(���յ������ݱ�����0x0D 0x0A��β)����һ�����з�+һ���ö�������һ���س�
	//0x0D = 1101B, 0x0A = 1010B
	//���ж��ǲ��ǽ����ж�
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{	//USART_SR ����5 RXNEλ�ǡ���ȡ���ݼĴ�������Ϊ�ձ�־������RDR��λ�Ĵ����������Ѿ����䵽USART_DR�Ĵ���ʱ����λӲ����1��SET��
		//0��δ���յ�����
		//1����׼���ý������ݡ�
		//USART_ClearITPendingBit(USART1, USART_IT_RXNE);//RXEN���㣨ͨ����DR�Ĵ�����ʵ�Ϳ���Ӳ����λ��

		Res = USART_ReceiveData(USART1);//(USART1->DR);	//DR�ǽ��յ������ݻ��ѷ��͵����ݡ�������Res
		//Res��8λ�ģ�DR��16λ�ģ�������������ת��
		//����Res�õ��˷��͹���������

		if ((USART1_RX_STA & 0x8000) == 0)//����δ���
		{
			if (USART1_RX_STA & 0x4000)//���յ���0x0d
			{
				if (Res != 0x0a)USART1_RX_STA = 0;//���մ���,���¿�ʼ
				else USART1_RX_STA |= 0x8000;	//��������� 
			}
			else //��û�յ�0X0D
			{
				if (Res == 0x0d)USART1_RX_STA |= 0x4000;
				else
				{
					USART1_RX_BUF[USART1_RX_STA & 0X3FFF] = Res;
					USART1_RX_STA++;
					if (USART1_RX_STA > (USART1_REC_LEN - 1))USART1_RX_STA = 0;//�������ݴ���,���¿�ʼ����	  
				}
			}
		}
		/*
		�����յ��ӵ��Է����������ݣ��ѽ��յ������ݱ�����USART_RX_BUF �У�
		ͬʱ�ڽ���״̬�Ĵ�����USART_RX_STA���м������յ�����Ч���ݸ�����
		���յ��س���0X0D��0X0A���ĵ�һ���ֽ�0X0D ʱ�����������������ӣ��ȴ�0X0A �ĵ�����
		�����0X0A û������������Ϊ��ν���ʧ�ܣ����¿�ʼ��һ�ν��ա�
		���˳�����յ�0X0A������USART_RX_STA�ĵ�15λ���������һ�ν��գ����ȴ���λ����������������Ӷ���ʼ��һ�εĽ��գ�
		������ٳ�û���յ�0X0D����ô�ڽ������ݳ��� USART_REC_LEN ���ˣ���ᶪ��ǰ������ݣ����½��ա�
		*/


	}



#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntExit();
#endif
}


#endif
