/*********************************************************************/
//				�����Լ�����ġ�I2C�����ʵ�ֵĳ���

/*********************************************************************/
#ifndef __I2C_H
#define __I2C_H

#include "sys.h"
					/************************/
					//��SCL������SCL = ��PB8��
					//��SDA������SDA = ��PB9��
					//GPIO_OType_OD=0x01
					//GPIO_OType_PP=0x00
					/************************/
#define IIC_INIT_MODE 0x01 //�趨�����졢��©��




//IO��������(������ģʽ����)
#if (IIC_INIT_MODE==0x00)
#define SDA_IN_Init()  {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=0<<9*2;}	//PB9����ģʽ
#define SDA_OUT_Init() {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=1<<9*2;}	//PB9���ģʽ
//������������趨ö��
typedef enum {
	IIC_Mode_Out = 0, IIC_Mode_In = 1
}IIC_ModeType;
#define IS_IIC_MODE_TYPE(NewMode) (((NewMode) == IIC_Mode_Out) || ((NewMode) == IIC_Mode_In))
//�޸�IIC�ġ�������롿
void IIC_Mode(IIC_ModeType NewMode);
#endif

//IO��������	 
#define IIC_SCL	PBout(8) //SCL = PB8
#define IIC_SDA	PBout(9) //SDA��Ĭ��Ϊ�����= PB9
#define IIC_SDA_Input   PBin(9)  //SDA�����룩= PB9


//IIC����ʼ�����͡�
void IIC_Init(void);                //��ʼ��IIC��IO��







//����IIC��ʼ�ź�
void IIC_Start(void);
//����IICֹͣ�ź�
void IIC_Stop(void);
//IIC�ȴ�ACK�ź�
u8 IIC_WaitAck(void);
//����ACKӦ��
void IIC_Ack(void);
//������ACK�ź�
void IIC_NAck(void);
//IIC����һ���ֽ�	
void IIC_Send_Byte(u8 TxData);
/*



u8 IIC_Read_Byte(unsigned char ack);//IIC��ȡһ���ֽ�



void IIC_Ack(void);					//IIC����ACK�ź�



void IIC_Write_One_Byte(u8 daddr, u8 addr, u8 data);
u8 IIC_Read_One_Byte(u8 daddr, u8 addr);
*/



#endif // !__I2C_H
