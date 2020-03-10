#include <wlf_I2C.h>
#include <delay.h>
#include "usart.h"
u8 IIC_InitMode;//用于所有函数的初始化类型的判断，以方便选择合适的处理方式

//初始化IIC1,此处用的IIC的I/O引脚是【PB8】【PB9】
//初始化模式有【IIC_OD】（开漏）和【IIC_PP】（推挽）
void IIC_Init(void) {

    GPIO_InitTypeDef  IIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIOB时钟

    //GPIOB8,B9初始化设置
#if (IIC_INIT_MODE==0x01)
    IIC_InitStructure.GPIO_OType = GPIO_OType_OD;//开漏输出
#elif (IIC_INIT_MODE==0x00)
    IIC_InitStructure.GPIO_OType = GPIO_OType_PP;//开漏输出
#endif
    IIC_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    IIC_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
    IIC_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    IIC_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOB, &IIC_InitStructure);//初始化
    IIC_SCL = 0;
    IIC_SDA = 0;
    /****************************************************/
    /*  开漏模式                                        */
    /*【输出 = 1 】【浮空】（实际上被上拉）             */
    /*【输出 = 0 】【低电平】                           */
    /****************************************************/
}

//修改IIC的输出输入模式（推挽输出时才用得着）
#if (IIC_INIT_MODE==0x00)
void IIC_Mode(IIC_ModeType NewMode) {
    assert_param(IS_IIC_MODE_TYPE(NewMode));
    if (NewMode == IIC_Mode_In)     SDA_IN_Init();
    if (NewMode == IIC_Mode_Out)    SDA_OUT_Init();
}
#endif


/*开始IIC的运行
【SCL】¯¯¯¯¯¯¯|
【SDA】¯¯¯¯|___
SCL为【高】电平时，SDA由【高】电平向【低】电平转换，开始传送数据
*/
void IIC_Start(void){
#if (IIC_INIT_MODE==0x00)
    SDA_OUT_Init();
#endif
    //浮空双线输出拉高
    IIC_SDA = 1;
    IIC_SCL = 1;
    delay_us(1);
    //(当【SCL】为高时)【SDA】线由高变低
    IIC_SDA = 0;
    delay_us(1); 
    //钳住时间线
    IIC_SCL = 0;
}

/*结束IIC的运行
【SCL】____|¯¯¯¯¯|
【SDA】_____|¯¯¯¯¯
SCL为【高】电平时，SDA由【低】电平向【高】电平转换，结束传输数据
*/
void IIC_Stop(void) {
#if (IIC_INIT_MODE==0x00)
    SDA_OUT_Init();
#endif
    //浮空双线输出拉低
    IIC_SCL = 0;
    IIC_SDA = 0;
    delay_us(1);
    IIC_SCL = 1;//SCL为【高】电平时
 //   delay_us(1);
    IIC_SDA = 1;//SDA由【低】电平向【高】电平转换
 //   delay_us(1);
    //在IIC中规定，当SDA、SCL同时为高电平时，视为【空闲状态】。

}


//等待【Ack】应答信号
u8 IIC_WaitAck(void) {
    u8 ucErrTime = 0;
    //推挽模式下
#if (IIC_INIT_MODE==0x00)
    SDA_IN_Init();//SDA设置为输入 
#endif
//  delay_us(1);
    IIC_SCL = 1; 
    delay_us(1);
    while (IIC_SDA_Input) {//等待
        ucErrTime++;
        if (ucErrTime > 250) {//超时
            IIC_Stop();
            printf("Error,Ack no resposed!\n");
            return 1;
        }
    }
    IIC_SCL = 0;//时钟【低电平】
    return 0;
}

//产生ACK应答
//【SCL】____|¯¯¯|____
//【SDA】_____________
void IIC_Ack(void) {
    IIC_SCL = 0;
#if (IIC_INIT_MODE==0x00)
    SDA_OUT_Init();
#endif
    IIC_SDA = 0;
    delay_us(1);
    IIC_SCL = 1;
    delay_us(2);
    IIC_SCL = 0;
}

//产生"非"ACK应答
//【SCL】___|¯¯¯¯|___
//【SDA】¯¯¯¯¯¯¯¯¯¯¯¯
void IIC_NAck(void) {
    IIC_SCL = 0;
#if (IIC_INIT_MODE==0x00)
    SDA_OUT_Init();
#endif
    IIC_SDA = 1;
    delay_us(2);
    IIC_SCL = 1;
    delay_us(2);
    IIC_SCL = 0;
}

//IIC发送一个字节
void IIC_Send_Byte(u8 TxData) {
#if (IIC_INIT_MODE==0x00)
    SDA_OUT_Init();
#endif
    IIC_SCL = 0;//拉低时钟开始数据传输
    delay_us(1);
    for (u8 t = 0; t < 8; t++) {
        IIC_SDA = (TxData & 0x80) >> 7;
        TxData <<= 1;
        delay_us(1);   
        IIC_SCL = 1;//拉高时钟，激活该数据
        delay_us(1);
        IIC_SCL = 0;//再次拉低
        //delay_us(1);
    }
    IIC_WaitAck();
}

//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
//1：有应答
//0：无应答	
u8 IIC_Read_Byte(unsigned char ack) {
    u8 i, receive = 0;
#if (IIC_INIT_MODE==0x00)
    SDA_IN_Init();//SDA设置为输入
#endif
    for (i = 0; i < 8; i++) {
        IIC_SCL = 0;
        delay_us(2);
        IIC_SCL = 1;
        receive <<= 1;
        if (IIC_SDA_Input)  receive++;
        delay_us(1);
    }
    if (!ack)
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   
    return receive;
}

