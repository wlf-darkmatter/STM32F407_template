/*********************************************************************/
//				这是自己定义的【I2C】软件实现的程序

/*********************************************************************/
#ifndef __I2C_H
#define __I2C_H

#include "sys.h"
					/************************/
					//【SCL】——SCL = 【PB8】
					//【SDA】——SDA = 【PB9】
					//GPIO_OType_OD=0x01
					//GPIO_OType_PP=0x00
					/************************/
#define IIC_INIT_MODE 0x01 //设定【推挽、开漏】




//IO方向设置(【推挽模式】下)
#if (IIC_INIT_MODE==0x00)
#define SDA_IN_Init()  {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=0<<9*2;}	//PB9输入模式
#define SDA_OUT_Init() {GPIOB->MODER&=~(3<<(9*2));GPIOB->MODER|=1<<9*2;}	//PB9输出模式
//【输入输出】设定枚举
typedef enum {
	IIC_Mode_Out = 0, IIC_Mode_In = 1
}IIC_ModeType;
#define IS_IIC_MODE_TYPE(NewMode) (((NewMode) == IIC_Mode_Out) || ((NewMode) == IIC_Mode_In))
//修改IIC的【输出输入】
void IIC_Mode(IIC_ModeType NewMode);
#endif

//IO操作函数	 
#define IIC_SCL	PBout(8) //SCL = PB8
#define IIC_SDA	PBout(9) //SDA（默认为输出）= PB9
#define IIC_SDA_Input   PBin(9)  //SDA（输入）= PB9


//IIC【初始化类型】
void IIC_Init(void);                //初始化IIC的IO口







//发送IIC开始信号
void IIC_Start(void);
//发送IIC停止信号
void IIC_Stop(void);
//IIC等待ACK信号
u8 IIC_WaitAck(void);
//产生ACK应答
void IIC_Ack(void);
//不发送ACK信号
void IIC_NAck(void);
//IIC发送一个字节	
void IIC_Send_Byte(u8 TxData);
/*



u8 IIC_Read_Byte(unsigned char ack);//IIC读取一个字节



void IIC_Ack(void);					//IIC发送ACK信号



void IIC_Write_One_Byte(u8 daddr, u8 addr, u8 data);
u8 IIC_Read_One_Byte(u8 daddr, u8 addr);
*/



#endif // !__I2C_H
