#include "SDIO_SDCard.h"
#include "string.h"	 
#include "sys.h"	 
#include "usart.h"	 
#include "ff.h"

/*用于sdio初始化的结构体*/
SDIO_InitTypeDef SDIO_InitStructure;
SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
SDIO_DataInitTypeDef SDIO_DataInitStructure;   

_SD CmdError(void);  
_SD CmdResp7Error(void);
_SD CmdResp1Error(u8 cmd);
_SD CmdResp3Error(void);
_SD CmdResp2Error(void);
_SD CmdResp6Error(u8 cmd,u16*prca);  
_SD SDEnWideBus(u8 enx);	  
_SD IsCardProgramming(u8 *pstatus); 
_SD FindSCR(u16 rca,u32 *pscr);
u8 convert_from_bytes_to_power_of_two(u16 NumberOfBytes); 



static u8 CardType=SDIO_STD_CAPACITY_SD_CARD_V1_1;		//SD卡类型（默认为1.x卡）
static u32 CSD_Tab[4],CID_Tab[4],RCA=0;					//SD卡CSD,CID以及相对地址(RCA)数据
static u8 DeviceMode=SD_DMA_MODE;		   				//工作模式,注意,工作模式必须通过SD_SetDeviceMode,后才算数.这里只是定义一个默认的模式(SD_DMA_MODE)
static u8 StopCondition=0; 								//是否发送停止传输标志位,DMA多块读写的时候用到  
volatile _SD TransferError=SD_OK;					//数据传输错误标志,DMA读写时使用	    
volatile u8 TransferEnd=0;								//传输结束标志,DMA读写时使用
SD_CardInfo SDCardInfo;									//SD卡信息

//SD_ReadDisk/SD_WriteDisk函数专用buf,当这两个函数的数据缓存区地址不是4字节对齐的时候,
//需要用到该数组,确保数据缓存区地址是4字节对齐的.
__align(4) u8 SDIO_DATA_BUFFER[512];						  
 
FIL cc936_bin;
 
void SDIO_Register_Deinit()
{
	SDIO->POWER=0x00000000;
	SDIO->CLKCR=0x00000000;
	SDIO->ARG=0x00000000;
	SDIO->CMD=0x00000000;
	SDIO->DTIMER=0x00000000;
	SDIO->DLEN=0x00000000;
	SDIO->DCTRL=0x00000000;
	SDIO->ICR=0x00C007FF;
	SDIO->MASK=0x00000000;	 
}

//初始化SD卡
//返回值:错误代码;(0,无错误)
_SD SD_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	_SD errorstatus=SD_OK;	 
	u8 clkdiv=0;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_DMA2, ENABLE);//使能GPIOC,GPIOD DMA2时钟
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, ENABLE);//SDIO时钟使能
	
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, ENABLE);//SDIO复位
	
	
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12; 	//PC8,9,10,11,12复用功能输出	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100M
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);// PC8,9,10,11,12复用功能输出

	
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);//PD2复用功能输出
	
	 //引脚复用映射设置
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_SDIO); //PC8,AF12
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_SDIO);	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_SDIO);	
	
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, DISABLE);//SDIO结束复位
		
 	//SDIO外设寄存器设置为默认值 			   
	SDIO_Register_Deinit();
	
	NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
	
   	errorstatus=SD_PowerON();			//SD卡上电
 	if(errorstatus==SD_OK)errorstatus=SD_InitializeCards();			//初始化SD卡														  
  	if(errorstatus==SD_OK)errorstatus=SD_GetCardInfo(&SDCardInfo);	//获取卡信息
 	if(errorstatus==SD_OK)errorstatus=SD_SelectDeselect((u32)(SDCardInfo.RCA<<16));//选中SD卡   
   	if(errorstatus==SD_OK)errorstatus=SD_EnableWideBusOperation(SDIO_BusWide_4b);	//4位宽度,如果是MMC卡,则不能用4位模式 
  	if((errorstatus==SD_OK)||(SDIO_MULTIMEDIA_CARD==CardType))
	{  		    
		if(SDCardInfo.CardType==SDIO_STD_CAPACITY_SD_CARD_V1_1||SDCardInfo.CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0)
		{
			clkdiv=SDIO_TRANSFER_CLK_DIV+2;	//V1.1/V2.0卡，设置最高48/4=12Mhz
		}else clkdiv=SDIO_TRANSFER_CLK_DIV;	//SDHC等其他卡，设置最高48/2=24Mhz
		SDIO_Clock_Set(clkdiv);	//设置时钟频率,SDIO时钟计算公式:SDIO_CK时钟=SDIOCLK/[clkdiv+2];其中,SDIOCLK固定为48Mhz 
		//errorstatus=SD_SetDeviceMode(SD_DMA_MODE);	//设置为DMA模式
		errorstatus=SD_SetDeviceMode(SD_POLLING_MODE);//设置为查询模式
 	}
	return errorstatus;		 
}

//SDIO时钟初始化设置
//clkdiv:时钟分频系数
//CK时钟=SDIOCLK/[clkdiv+2];(SDIOCLK时钟固定为48Mhz)
void SDIO_Clock_Set(u8 clkdiv)
{
	u32 tmpreg=SDIO->CLKCR; 
  	tmpreg&=0XFFFFFF00; 
 	tmpreg|=clkdiv;   
	SDIO->CLKCR=tmpreg;
} 


//卡上电
//查询所有SDIO接口上的卡设备,并查询其电压和配置时钟
//返回值:错误代码;(0,无错误)
_SD SD_PowerON(void)
{
 	u8 i=0;
	_SD errorstatus=SD_OK;
	u32 response=0,count=0,validvoltage=0;
	u32 SDType=SD_STD_CAPACITY;
	
	 /*初始化时的时钟不能大于400KHz*/ 
  SDIO_InitStructure.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;	/* HCLK = 72MHz, SDIOCLK = 72MHz, SDIO_CK = HCLK/(178 + 2) = 400 KHz */
  SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;  //不使用bypass模式，直接用HCLK进行分频得到SDIO_CK
  SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;	// 空闲时不关闭时钟电源
  SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;	 				//1位数据线
  SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;//硬件流
  SDIO_Init(&SDIO_InitStructure);

	SDIO_SetPowerState(SDIO_PowerState_ON);	//上电状态,开启卡时钟   
  SDIO->CLKCR|=1<<8;			//SDIOCK使能  
 
 	for(i=0;i<74;i++)
	{
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;//发送CMD0进入IDLE STAGE模式命令.
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE; //cmd0
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;  //无响应
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;  //则CPSM在开始发送命令之前等待数据传输结束。 
		SDIO_SendCommand(&SDIO_CmdInitStructure);	  		//写命令进命令寄存器
		
		errorstatus=CmdError();
		
		if(errorstatus==SD_OK)break;
 	}
 	if(errorstatus)return errorstatus;//返回错误状态
	
	SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;	//发送CMD8,短响应,检查SD卡接口特性
	SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_SEND_IF_COND;	//cmd8
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;	 //r7
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;			 //关闭等待中断
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	
	errorstatus=CmdResp7Error();						//等待R7响应
	
 	if(errorstatus==SD_OK) 								//R7响应正常
	{
		CardType=SDIO_STD_CAPACITY_SD_CARD_V2_0;		//SD 2.0卡
		SDType=SD_HIGH_CAPACITY;			   			//高容量卡
	}
	  
	SDIO_CmdInitStructure.SDIO_Argument = 0x00;//发送CMD55,短响应	
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);		//发送CMD55,短响应	 
	
	errorstatus=CmdResp1Error(SD_CMD_APP_CMD); 		 	//等待R1响应   
	
	if(errorstatus==SD_OK)//SD2.0/SD 1.1,否则为MMC卡
	{																  
		//SD卡,发送ACMD41 SD_APP_OP_COND,参数为:0x80100000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{	   										   
			SDIO_CmdInitStructure.SDIO_Argument = 0x00;//发送CMD55,短响应
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;	  //CMD55
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);			//发送CMD55,短响应	 
			
			errorstatus=CmdResp1Error(SD_CMD_APP_CMD); 	 	//等待R1响应  
			
 			if(errorstatus!=SD_OK)return errorstatus;   	//响应错误

      //acmd41，命令参数由支持的电压范围及HCS位组成，HCS位置一来区分卡是SDSc还是sdhc
			SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;	//发送ACMD41,短响应	
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r3
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);
			
			errorstatus=CmdResp3Error(); 					//等待R3响应   
			
 			if(errorstatus!=SD_OK)return errorstatus;   	//响应错误 
			response=SDIO->RESP1;;			   				//得到响应
			validvoltage=(((response>>31)==1)?1:0);			//判断SD卡上电是否完成
			count++;
		}
		if(count>=SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}	 
		if(response&=SD_HIGH_CAPACITY)
		{
			CardType=SDIO_HIGH_CAPACITY_SD_CARD;
		}
 	}else//MMC卡
	{
		//MMC卡,发送CMD1 SDIO_SEND_OP_COND,参数为:0x80FF8000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{	   										   				   
			SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_MMC;//发送CMD1,短响应	   
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_OP_COND;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r3
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);
			
			errorstatus=CmdResp3Error(); 					//等待R3响应   
			
 			if(errorstatus!=SD_OK)return errorstatus;   	//响应错误  
			response=SDIO->RESP1;;			   				//得到响应
			validvoltage=(((response>>31)==1)?1:0);
			count++;
		}
		if(count>=SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}	 			    
		CardType=SDIO_MULTIMEDIA_CARD;	  
  	}  
  	return(errorstatus);		
}

//SD卡 Power OFF
//返回值:错误代码;(0,无错误)
_SD SD_PowerOFF(void)
{
 
  SDIO_SetPowerState(SDIO_PowerState_OFF);//SDIO电源关闭,时钟停止	

  return SD_OK;	  
}

//初始化所有的卡,并让卡进入就绪状态
//返回值:错误代码
_SD SD_InitializeCards(void)
{
 	_SD errorstatus=SD_OK;
	u16 rca = 0x01;
	
	if (SDIO_GetPowerState() == SDIO_PowerState_OFF)	//检查电源状态,确保为上电状态
	{
		errorstatus = SD_REQUEST_NOT_APPLICABLE;
		return(errorstatus);
	}

 	if(SDIO_SECURE_DIGITAL_IO_CARD!=CardType)			//非SECURE_DIGITAL_IO_CARD
	{
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;//发送CMD2,取得CID,长响应
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);//发送CMD2,取得CID,长响应	
		
		errorstatus=CmdResp2Error(); 					//等待R2响应 
		
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误		 
		
 		CID_Tab[0]=SDIO->RESP1;
		CID_Tab[1]=SDIO->RESP2;
		CID_Tab[2]=SDIO->RESP3;
		CID_Tab[3]=SDIO->RESP4;
	}
	if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_SECURE_DIGITAL_IO_COMBO_CARD==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))//判断卡类型
	{
		SDIO_CmdInitStructure.SDIO_Argument = 0x00;//发送CMD3,短响应 
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;	//cmd3
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short; //r6
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	//发送CMD3,短响应 
		
		errorstatus=CmdResp6Error(SD_CMD_SET_REL_ADDR,&rca);//等待R6响应 
		
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误		    
	}   
    if (SDIO_MULTIMEDIA_CARD==CardType)
    {

		SDIO_CmdInitStructure.SDIO_Argument = (u32)(rca<<16);//发送CMD3,短响应 
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;	//cmd3
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short; //r6
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	//发送CMD3,短响应 	
			
		errorstatus=CmdResp2Error(); 					//等待R2响应   
			
		if(errorstatus!=SD_OK)
			return errorstatus;   	//响应错误	 
    }
	if (SDIO_SECURE_DIGITAL_IO_CARD!=CardType)			//非SECURE_DIGITAL_IO_CARD
	{
		RCA = rca;
		
		SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)(rca << 16);//发送CMD9+卡RCA,取得CSD,长响应 
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_CSD;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		
		errorstatus=CmdResp2Error(); 					//等待R2响应   
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误		    
  		
		CSD_Tab[0]=SDIO->RESP1;
		CSD_Tab[1]=SDIO->RESP2;
		CSD_Tab[2]=SDIO->RESP3;						
		CSD_Tab[3]=SDIO->RESP4;					    
	}
	return SD_OK;//卡初始化成功
}

//得到卡信息
//cardinfo:卡信息存储区
//返回值:错误状态
_SD SD_GetCardInfo(SD_CardInfo *cardinfo)
{
 	_SD errorstatus=SD_OK;
	u8 tmp=0;	   
	cardinfo->CardType=(u8)CardType; 				//卡类型
	cardinfo->RCA=(u16)RCA;							//卡RCA值
	tmp=(u8)((CSD_Tab[0]&0xFF000000)>>24);
	cardinfo->SD_csd.CSDStruct=(tmp&0xC0)>>6;		//CSD结构
	cardinfo->SD_csd.SysSpecVersion=(tmp&0x3C)>>2;	//2.0协议还没定义这部分(为保留),应该是后续协议定义的
	cardinfo->SD_csd.Reserved1=tmp&0x03;			//2个保留位  
	tmp=(u8)((CSD_Tab[0]&0x00FF0000)>>16);			//第1个字节
	cardinfo->SD_csd.TAAC=tmp;				   		//数据读时间1
	tmp=(u8)((CSD_Tab[0]&0x0000FF00)>>8);	  		//第2个字节
	cardinfo->SD_csd.NSAC=tmp;		  				//数据读时间2
	tmp=(u8)(CSD_Tab[0]&0x000000FF);				//第3个字节
	cardinfo->SD_csd.MaxBusClkFrec=tmp;		  		//传输速度	   
	tmp=(u8)((CSD_Tab[1]&0xFF000000)>>24);			//第4个字节
	cardinfo->SD_csd.CardComdClasses=tmp<<4;    	//卡指令类高四位
	tmp=(u8)((CSD_Tab[1]&0x00FF0000)>>16);	 		//第5个字节
	cardinfo->SD_csd.CardComdClasses|=(tmp&0xF0)>>4;//卡指令类低四位
	cardinfo->SD_csd.RdBlockLen=tmp&0x0F;	    	//最大读取数据长度
	tmp=(u8)((CSD_Tab[1]&0x0000FF00)>>8);			//第6个字节
	cardinfo->SD_csd.PartBlockRead=(tmp&0x80)>>7;	//允许分块读
	cardinfo->SD_csd.WrBlockMisalign=(tmp&0x40)>>6;	//写块错位
	cardinfo->SD_csd.RdBlockMisalign=(tmp&0x20)>>5;	//读块错位
	cardinfo->SD_csd.DSRImpl=(tmp&0x10)>>4;
	cardinfo->SD_csd.Reserved2=0; 					//保留
 	if((CardType==SDIO_STD_CAPACITY_SD_CARD_V1_1)||(CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0)||(SDIO_MULTIMEDIA_CARD==CardType))//标准1.1/2.0卡/MMC卡
	{
		cardinfo->SD_csd.DeviceSize=(tmp&0x03)<<10;	//C_SIZE(12位)
	 	tmp=(u8)(CSD_Tab[1]&0x000000FF); 			//第7个字节	
		cardinfo->SD_csd.DeviceSize|=(tmp)<<2;
 		tmp=(u8)((CSD_Tab[2]&0xFF000000)>>24);		//第8个字节	
		cardinfo->SD_csd.DeviceSize|=(tmp&0xC0)>>6;
 		cardinfo->SD_csd.MaxRdCurrentVDDMin=(tmp&0x38)>>3;
		cardinfo->SD_csd.MaxRdCurrentVDDMax=(tmp&0x07);
 		tmp=(u8)((CSD_Tab[2]&0x00FF0000)>>16);		//第9个字节	
		cardinfo->SD_csd.MaxWrCurrentVDDMin=(tmp&0xE0)>>5;
		cardinfo->SD_csd.MaxWrCurrentVDDMax=(tmp&0x1C)>>2;
		cardinfo->SD_csd.DeviceSizeMul=(tmp&0x03)<<1;//C_SIZE_MULT
 		tmp=(u8)((CSD_Tab[2]&0x0000FF00)>>8);	  	//第10个字节	
		cardinfo->SD_csd.DeviceSizeMul|=(tmp&0x80)>>7;
 		cardinfo->CardCapacity=(cardinfo->SD_csd.DeviceSize+1);//计算卡容量
		cardinfo->CardCapacity*=(1<<(cardinfo->SD_csd.DeviceSizeMul+2));
		cardinfo->CardBlockSize=1<<(cardinfo->SD_csd.RdBlockLen);//块大小
		cardinfo->CardCapacity*=cardinfo->CardBlockSize;
	}else if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)	//高容量卡
	{
 		tmp=(u8)(CSD_Tab[1]&0x000000FF); 		//第7个字节	
		cardinfo->SD_csd.DeviceSize=(tmp&0x3F)<<16;//C_SIZE
 		tmp=(u8)((CSD_Tab[2]&0xFF000000)>>24); 	//第8个字节	
 		cardinfo->SD_csd.DeviceSize|=(tmp<<8);
 		tmp=(u8)((CSD_Tab[2]&0x00FF0000)>>16);	//第9个字节	
 		cardinfo->SD_csd.DeviceSize|=(tmp);
 		tmp=(u8)((CSD_Tab[2]&0x0000FF00)>>8); 	//第10个字节	
 		cardinfo->CardCapacity=(long long)(cardinfo->SD_csd.DeviceSize+1)*512*1024;//计算卡容量
		cardinfo->CardBlockSize=512; 			//块大小固定为512字节
	}	  
	cardinfo->SD_csd.EraseGrSize=(tmp&0x40)>>6;
	cardinfo->SD_csd.EraseGrMul=(tmp&0x3F)<<1;	   
	tmp=(u8)(CSD_Tab[2]&0x000000FF);			//第11个字节	
	cardinfo->SD_csd.EraseGrMul|=(tmp&0x80)>>7;
	cardinfo->SD_csd.WrProtectGrSize=(tmp&0x7F);
 	tmp=(u8)((CSD_Tab[3]&0xFF000000)>>24);		//第12个字节	
	cardinfo->SD_csd.WrProtectGrEnable=(tmp&0x80)>>7;
	cardinfo->SD_csd.ManDeflECC=(tmp&0x60)>>5;
	cardinfo->SD_csd.WrSpeedFact=(tmp&0x1C)>>2;
	cardinfo->SD_csd.MaxWrBlockLen=(tmp&0x03)<<2;	 
	tmp=(u8)((CSD_Tab[3]&0x00FF0000)>>16);		//第13个字节
	cardinfo->SD_csd.MaxWrBlockLen|=(tmp&0xC0)>>6;
	cardinfo->SD_csd.WriteBlockPaPartial=(tmp&0x20)>>5;
	cardinfo->SD_csd.Reserved3=0;
	cardinfo->SD_csd.ContentProtectAppli=(tmp&0x01);  
	tmp=(u8)((CSD_Tab[3]&0x0000FF00)>>8);		//第14个字节
	cardinfo->SD_csd.FileFormatGrouop=(tmp&0x80)>>7;
	cardinfo->SD_csd.CopyFlag=(tmp&0x40)>>6;
	cardinfo->SD_csd.PermWrProtect=(tmp&0x20)>>5;
	cardinfo->SD_csd.TempWrProtect=(tmp&0x10)>>4;
	cardinfo->SD_csd.FileFormat=(tmp&0x0C)>>2;
	cardinfo->SD_csd.ECC=(tmp&0x03);  
	tmp=(u8)(CSD_Tab[3]&0x000000FF);			//第15个字节
	cardinfo->SD_csd.CSD_CRC=(tmp&0xFE)>>1;
	cardinfo->SD_csd.Reserved4=1;		 
	tmp=(u8)((CID_Tab[0]&0xFF000000)>>24);		//第0个字节
	cardinfo->SD_cid.ManufacturerID=tmp;		    
	tmp=(u8)((CID_Tab[0]&0x00FF0000)>>16);		//第1个字节
	cardinfo->SD_cid.OEM_AppliID=tmp<<8;	  
	tmp=(u8)((CID_Tab[0]&0x000000FF00)>>8);		//第2个字节
	cardinfo->SD_cid.OEM_AppliID|=tmp;	    
	tmp=(u8)(CID_Tab[0]&0x000000FF);			//第3个字节	
	cardinfo->SD_cid.ProdName1=tmp<<24;				  
	tmp=(u8)((CID_Tab[1]&0xFF000000)>>24); 		//第4个字节
	cardinfo->SD_cid.ProdName1|=tmp<<16;	  
	tmp=(u8)((CID_Tab[1]&0x00FF0000)>>16);	   	//第5个字节
	cardinfo->SD_cid.ProdName1|=tmp<<8;		 
	tmp=(u8)((CID_Tab[1]&0x0000FF00)>>8);		//第6个字节
	cardinfo->SD_cid.ProdName1|=tmp;		   
	tmp=(u8)(CID_Tab[1]&0x000000FF);	  		//第7个字节
	cardinfo->SD_cid.ProdName2=tmp;			  
	tmp=(u8)((CID_Tab[2]&0xFF000000)>>24); 		//第8个字节
	cardinfo->SD_cid.ProdRev=tmp;		 
	tmp=(u8)((CID_Tab[2]&0x00FF0000)>>16);		//第9个字节
	cardinfo->SD_cid.ProdSN=tmp<<24;	   
	tmp=(u8)((CID_Tab[2]&0x0000FF00)>>8); 		//第10个字节
	cardinfo->SD_cid.ProdSN|=tmp<<16;	   
	tmp=(u8)(CID_Tab[2]&0x000000FF);   			//第11个字节
	cardinfo->SD_cid.ProdSN|=tmp<<8;		   
	tmp=(u8)((CID_Tab[3]&0xFF000000)>>24); 		//第12个字节
	cardinfo->SD_cid.ProdSN|=tmp;			     
	tmp=(u8)((CID_Tab[3]&0x00FF0000)>>16);	 	//第13个字节
	cardinfo->SD_cid.Reserved1|=(tmp&0xF0)>>4;
	cardinfo->SD_cid.ManufactDate=(tmp&0x0F)<<8;    
	tmp=(u8)((CID_Tab[3]&0x0000FF00)>>8);		//第14个字节
	cardinfo->SD_cid.ManufactDate|=tmp;		 	  
	tmp=(u8)(CID_Tab[3]&0x000000FF);			//第15个字节
	cardinfo->SD_cid.CID_CRC=(tmp&0xFE)>>1;
	cardinfo->SD_cid.Reserved2=1;	 
	return errorstatus;
}

//设置SDIO总线宽度(MMC卡不支持4bit模式)
//wmode:位宽模式.
//0=>1位数据宽度;
//1=>4位数据宽度;
//2=>8位数据宽度
//返回值:SD卡错误状态
_SD SD_EnableWideBusOperation(u32 WideMode)
{
  	_SD errorstatus=SD_OK;
  if (SDIO_MULTIMEDIA_CARD == CardType)
  {
    errorstatus = SD_UNSUPPORTED_FEATURE;
    return(errorstatus);
  }
	
 	else if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
	{
		 if (SDIO_BusWide_8b == WideMode)   //2.0 sd不支持8bits
    {
      errorstatus = SD_UNSUPPORTED_FEATURE;
      return(errorstatus);
    }
 		else   
		{
			errorstatus=SDEnWideBus(WideMode);
 			if(SD_OK==errorstatus)
			{
				SDIO->CLKCR&=~(3<<11);		//清除之前的位宽设置    
				SDIO->CLKCR|=WideMode;//1位/4位总线宽度 
				SDIO->CLKCR|=0<<14;			//不开启硬件流控制
			}
		}  
	}
	return errorstatus; 
}

//设置SD卡工作模式
//Mode:
//返回值:错误状态
_SD SD_SetDeviceMode(u32 Mode)
{
	_SD errorstatus = SD_OK;
 	if((Mode==SD_DMA_MODE)||(Mode==SD_POLLING_MODE))DeviceMode=Mode;
	else errorstatus=SD_INVALID_PARAMETER;
	return errorstatus;	    
}

//选卡
//发送CMD7,选择相对地址(rca)为addr的卡,取消其他卡.如果为0,则都不选择.
//addr:卡的RCA地址
_SD SD_SelectDeselect(u32 addr)
{

  SDIO_CmdInitStructure.SDIO_Argument =  addr;//发送CMD7,选择卡,短响应	
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);//发送CMD7,选择卡,短响应
	
 	return CmdResp1Error(SD_CMD_SEL_DESEL_CARD);	  
}

//SD卡读取一个块 
//buf:读数据缓存区(必须4字节对齐!!)
//addr:读取地址
//blksize:块大小
_SD SD_ReadBlock(u8 *buf,long long addr,u16 blksize)
{	  
	_SD errorstatus=SD_OK;
	u8 power;
	u32 count=0,*tempbuff=(u32*)buf;//转换为u32指针 
	u32 timeout=SDIO_DATATIMEOUT;   
	if(NULL==buf)
		return SD_INVALID_PARAMETER; 
	SDIO->DCTRL=0x0;	//数据控制寄存器清零(关DMA) 
  
	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//大容量卡
	{
		blksize=512;
		addr>>=9;
	}   
	SDIO_DataInitStructure.SDIO_DataBlockSize= SDIO_DataBlockSize_1b ;//清除DPSM状态机配置
	SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
	SDIO_DataConfig(&SDIO_DataInitStructure);
	
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//卡锁了
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);	
		
   
		SDIO_CmdInitStructure.SDIO_Argument =  blksize;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);//发送CMD16+设置数据长度为blksize,短响应
		

		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//等待R1响应 
		
		if(errorstatus!=SD_OK) return errorstatus;   	//响应错误	
		
	}else return SD_INVALID_PARAMETER;	  	 
	
	SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4 ;//清除DPSM状态机配置
	SDIO_DataInitStructure.SDIO_DataLength= blksize ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToSDIO;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
	SDIO_DataConfig(&SDIO_DataInitStructure);
	
	SDIO_CmdInitStructure.SDIO_Argument =  addr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);//发送CMD17+从addr地址出读取数据,短响应 
	
	errorstatus=CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);//等待R1响应   
	if(errorstatus!=SD_OK) return errorstatus;   		//响应错误	 
 	if(DeviceMode==SD_POLLING_MODE)						//查询模式,轮询数据	 
	{
 		INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
		while(!(SDIO->STA&((1<<5)|(1<<1)|(1<<3)|(1<<10)|(1<<9))))//无上溢/CRC/超时/完成(标志)/起始位错误
		{
			if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)						//接收区半满,表示至少存了8个字
			{
				for(count=0;count<8;count++)			//循环读取数据
				{
					*(tempbuff+count)=SDIO->FIFO;
				}
				tempbuff+=8;	 
				timeout=0X7FFFFF; 	//读数据溢出时间
			}else 	//处理超时
			{
				if(timeout==0)return SD_DATA_TIMEOUT;
				timeout--;
			}
		} 
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//数据超时错误
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//清错误标志
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//数据块CRC错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) 	//接收fifo上溢错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);		//清错误标志
			return SD_RX_OVERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//接收起始位错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//清错误标志
			return SD_START_BIT_ERR;		 
		}   
		while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)	//FIFO里面,还存在可用数据
		{
			*tempbuff=SDIO->FIFO;	//循环读取数据
			tempbuff++;
		}
		INTX_ENABLE();//开启总中断
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	 
	}
	else if(DeviceMode==SD_DMA_MODE)
	{
 		TransferError=SD_OK;
		StopCondition=0;			//单块读,不需要发送停止传输指令
		TransferEnd=0;				//传输结束标置位，在中断服务置1
		SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);	//配置需要的中断 
	 	SDIO->DCTRL|=1<<3;		 	//SDIO DMA使能 
 	    SD_DMA_Config((u32*)buf,blksize,DMA_DIR_PeripheralToMemory); 
 		while(((DMA2->LISR&(1<<27))==RESET)&&(TransferEnd==0)&&(TransferError==SD_OK)&&timeout)timeout--;//等待传输完成 
		if(timeout==0)return SD_DATA_TIMEOUT;//超时
		if(TransferError!=SD_OK)errorstatus=TransferError;  
    }   
 	return errorstatus; 
}

//SD卡读取多个块 
//buf:读数据缓存区
//addr:读取地址
//blksize:块大小
//nblks:要读取的块数
//返回值:错误状态
/*__align(4)*/ u32 *tempbuff;
_SD SD_ReadMultiBlocks(u8 *buf,long long addr,u16 blksize,u32 nblks)
{
  _SD errorstatus=SD_OK;
	u8 power;
  u32 count=0;
	u32 timeout=SDIO_DATATIMEOUT;  
	tempbuff=(u32*)buf;//转换为u32指针
	
  SDIO->DCTRL=0x0;		//数据控制寄存器清零(关DMA)   
	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//大容量卡
	{
		blksize=512;
		addr>>=9;
	}  
	
	  SDIO_DataInitStructure.SDIO_DataBlockSize= 0; ;//清除DPSM状态机配置
	  SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	  SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	  SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	  SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	  SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
    SDIO_DataConfig(&SDIO_DataInitStructure);
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//卡锁了
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);	    
		
	  SDIO_CmdInitStructure.SDIO_Argument =  blksize;//发送CMD16+设置数据长度为blksize,短响应 
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//等待R1响应  
		
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误	 
		
	}else return SD_INVALID_PARAMETER;	  
	
	if(nblks>1)											//多块读  
	{									    
 	  	if(nblks*blksize>SD_MAX_DATA_LENGTH)return SD_INVALID_PARAMETER;//判断是否超过最大接收长度 
		
		   SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4; ;//nblks*blksize,512块大小,卡到控制器
			 SDIO_DataInitStructure.SDIO_DataLength= nblks*blksize ;
			 SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
			 SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
			 SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToSDIO;
			 SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
			 SDIO_DataConfig(&SDIO_DataInitStructure);

       SDIO_CmdInitStructure.SDIO_Argument =  addr;//发送CMD18+从addr地址出读取数据,短响应 
	     SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;
		   SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		   SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		   SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		   SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_READ_MULT_BLOCK);//等待R1响应 
		
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误	 
		
 		if(DeviceMode==SD_POLLING_MODE)
		{
			INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
			while(!(SDIO->STA&((1<<5)|(1<<1)|(1<<3)|(1<<8)|(1<<9))))//无上溢/CRC/超时/完成(标志)/起始位错误
			{
				if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)						//接收区半满,表示至少存了8个字
				{
					for(count=0;count<8;count++)			//循环读取数据
					{
						*(tempbuff+count)=SDIO->FIFO;
					}
					tempbuff+=8;	 
					timeout=0X7FFFFF; 	//读数据溢出时间
				}else 	//处理超时
				{
					if(timeout==0)return SD_DATA_TIMEOUT;
					timeout--;
				}
			}  
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//数据超时错误
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//清错误标志
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//数据块CRC错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) 	//接收fifo上溢错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);		//清错误标志
			return SD_RX_OVERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//接收起始位错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//清错误标志
			return SD_START_BIT_ERR;		 
		}   
	    
		while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)	//FIFO里面,还存在可用数据
		{
			*tempbuff=SDIO->FIFO;	//循环读取数据
			tempbuff++;
		}
	 		if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)		//接收结束
			{
				if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
				{				
					SDIO_CmdInitStructure.SDIO_Argument =  0;//发送CMD12+结束传输
				  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
					SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
					SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
					SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
					SDIO_SendCommand(&SDIO_CmdInitStructure);	
					
					errorstatus=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);//等待R1响应   
					
					if(errorstatus!=SD_OK)return errorstatus;	 
				}
 			}
			INTX_ENABLE();//开启总中断
	 		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
 		}else if(DeviceMode==SD_DMA_MODE)
		{
	   		TransferError=SD_OK;
			StopCondition=1;			//多块读,需要发送停止传输指令 
			TransferEnd=0;				//传输结束标置位，在中断服务置1
			SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);	//配置需要的中断 
		 	SDIO->DCTRL|=1<<3;		 						//SDIO DMA使能 
	 	    SD_DMA_Config((u32*)buf,nblks*blksize,DMA_DIR_PeripheralToMemory); 
	 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//等待传输完成 
			if(timeout==0)return SD_DATA_TIMEOUT;//超时
			while((TransferEnd==0)&&(TransferError==SD_OK)); 
			if(TransferError!=SD_OK)errorstatus=TransferError;  	 
		}		 
  	}
	return errorstatus;
}			    																  
//SD卡写1个块 
//buf:数据缓存区
//addr:写地址
//blksize:块大小	  
//返回值:错误状态
_SD SD_WriteBlock(u8 *buf,long long addr,  u16 blksize)
{
	_SD errorstatus = SD_OK;
	
	u8  power=0,cardstate=0;
	
	u32 timeout=0,bytestransferred=0;
	
	u32 cardstatus=0,count=0,restwords=0;
	
	u32	tlen=blksize;						//总长度(字节)
	
	u32*tempbuff=(u32*)buf;					
	
 	if(buf==NULL)return SD_INVALID_PARAMETER;//参数错误  
	
  SDIO->DCTRL=0x0;							//数据控制寄存器清零(关DMA)
	
	SDIO_DataInitStructure.SDIO_DataBlockSize= 0; ;//清除DPSM状态机配置
	SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
  SDIO_DataConfig(&SDIO_DataInitStructure);
	
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//卡锁了
 	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)	//大容量卡
	{
		blksize=512;
		addr>>=9;
	}    
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);	
		
		SDIO_CmdInitStructure.SDIO_Argument = blksize;//发送CMD16+设置数据长度为blksize,短响应 	
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//等待R1响应  
		
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误	 
		
	}else return SD_INVALID_PARAMETER;	
	
			SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA<<16;//发送CMD13,查询卡的状态,短响应 	
		  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);	

	  errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);		//等待R1响应  
	
	if(errorstatus!=SD_OK)return errorstatus;
	cardstatus=SDIO->RESP1;													  
	timeout=SD_DATATIMEOUT;
   	while(((cardstatus&0x00000100)==0)&&(timeout>0)) 	//检查READY_FOR_DATA位是否置位
	{
		timeout--;  
		
		SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA<<16;//发送CMD13,查询卡的状态,短响应
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);	//等待R1响应   
		
		if(errorstatus!=SD_OK)return errorstatus;		
		
		cardstatus=SDIO->RESP1;													  
	}
	if(timeout==0)return SD_ERROR;

			SDIO_CmdInitStructure.SDIO_Argument = addr;//发送CMD24,写单块指令,短响应 	
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);	
	
	errorstatus=CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);//等待R1响应  
	
	if(errorstatus!=SD_OK)return errorstatus;   	 
	
	StopCondition=0;									//单块写,不需要发送停止传输指令 

	SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4; ;	//blksize, 控制器到卡	
	SDIO_DataInitStructure.SDIO_DataLength= blksize ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
  SDIO_DataConfig(&SDIO_DataInitStructure);
	
	
	timeout=SDIO_DATATIMEOUT;
	
	if (DeviceMode == SD_POLLING_MODE)
	{
		INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
		while(!(SDIO->STA&((1<<10)|(1<<4)|(1<<1)|(1<<3)|(1<<9))))//数据块发送成功/下溢/CRC/超时/起始位错误
		{
			if(SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)							//发送区半空,表示至少存了8个字
			{
				if((tlen-bytestransferred)<SD_HALFFIFOBYTES)//不够32字节了
				{
					restwords=((tlen-bytestransferred)%4==0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);
					
					for(count=0;count<restwords;count++,tempbuff++,bytestransferred+=4)
					{
						SDIO->FIFO=*tempbuff;
					}
				}else
				{
					for(count=0;count<8;count++)
					{
						SDIO->FIFO=*(tempbuff+count);
					}
					tempbuff+=8;
					bytestransferred+=32;
				}
				timeout=0X3FFFFFFF;	//写数据溢出时间
			}else
			{
				if(timeout==0)return SD_DATA_TIMEOUT;
				timeout--;
			}
		} 
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//数据超时错误
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//清错误标志
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//数据块CRC错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) 	//接收fifo下溢错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);		//清错误标志
			return SD_TX_UNDERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//接收起始位错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//清错误标志
			return SD_START_BIT_ERR;		 
		}   
	      
		INTX_ENABLE();//开启总中断
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记  
	}else if(DeviceMode==SD_DMA_MODE)
	{
   		TransferError=SD_OK;
		StopCondition=0;			//单块写,不需要发送停止传输指令 
		TransferEnd=0;				//传输结束标置位，在中断服务置1
		SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);	//配置产生数据接收完成中断
		SD_DMA_Config((u32*)buf,blksize,DMA_DIR_MemoryToPeripheral);				//SDIO DMA配置
 	 	SDIO->DCTRL|=1<<3;								//SDIO DMA使能.  
 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//等待传输完成 
		if(timeout==0)
		{
  			SD_Init();	 					//重新初始化SD卡,可以解决写入死机的问题
			return SD_DATA_TIMEOUT;			//超时	 
 		}
		timeout=SDIO_DATATIMEOUT;
		while((TransferEnd==0)&&(TransferError==SD_OK)&&timeout)timeout--;
 		if(timeout==0)return SD_DATA_TIMEOUT;			//超时	 
  		if(TransferError!=SD_OK)return TransferError;
 	}  
 	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
 	errorstatus=IsCardProgramming(&cardstate);
 	while((errorstatus==SD_OK)&&((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)))
	{
		errorstatus=IsCardProgramming(&cardstate);
	}   
	return errorstatus;
}

//SD卡写多个块 
//buf:数据缓存区
//addr:写地址
//blksize:块大小
//nblks:要写入的块数
//返回值:错误状态												   
_SD SD_WriteMultiBlocks(u8 *buf,long long addr,u16 blksize,u32 nblks)
{
	_SD errorstatus = SD_OK;
	u8  power = 0, cardstate = 0;
	u32 timeout=0,bytestransferred=0;
	u32 count = 0, restwords = 0;
	u32 tlen=nblks*blksize;				//总长度(字节)
	u32 *tempbuff = (u32*)buf;  
  if(buf==NULL)return SD_INVALID_PARAMETER; //参数错误  
  SDIO->DCTRL=0x0;							//数据控制寄存器清零(关DMA)   
	
	SDIO_DataInitStructure.SDIO_DataBlockSize= 0; ;	//清除DPSM状态机配置	
	SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
  SDIO_DataConfig(&SDIO_DataInitStructure);
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//卡锁了
 	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//大容量卡
	{
		blksize=512;
		addr>>=9;
	}    
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);
		
		SDIO_CmdInitStructure.SDIO_Argument = blksize;	//发送CMD16+设置数据长度为blksize,短响应
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//等待R1响应  
		
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误	 
		
	}else return SD_INVALID_PARAMETER;	 
	if(nblks>1)
	{					  
		if(nblks*blksize>SD_MAX_DATA_LENGTH)return SD_INVALID_PARAMETER;   
     	if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
    	{
			//提高性能
				SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA<<16;		//发送ACMD55,短响应 	
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);	
				
			errorstatus=CmdResp1Error(SD_CMD_APP_CMD);		//等待R1响应 
				
			if(errorstatus!=SD_OK)return errorstatus;				 
				
				SDIO_CmdInitStructure.SDIO_Argument =nblks;		//发送CMD23,设置块数量,短响应 	 
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);
			  
				errorstatus=CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);//等待R1响应 
				
			if(errorstatus!=SD_OK)return errorstatus;		
		    
		} 

				SDIO_CmdInitStructure.SDIO_Argument =addr;	//发送CMD25,多块写指令,短响应 	  
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);	

 		errorstatus=CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);	//等待R1响应   		   
	
		if(errorstatus!=SD_OK)return errorstatus;

        SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4; ;	//blksize, 控制器到卡	
				SDIO_DataInitStructure.SDIO_DataLength= nblks*blksize ;
				SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
				SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
				SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
				SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
				SDIO_DataConfig(&SDIO_DataInitStructure);
				
		if(DeviceMode==SD_POLLING_MODE)
	    {
			timeout=SDIO_DATATIMEOUT;
			INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
			while(!(SDIO->STA&((1<<4)|(1<<1)|(1<<8)|(1<<3)|(1<<9))))//下溢/CRC/数据结束/超时/起始位错误
			{
				if(SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)							//发送区半空,表示至少存了8字(32字节)
				{	  
					if((tlen-bytestransferred)<SD_HALFFIFOBYTES)//不够32字节了
					{
						restwords=((tlen-bytestransferred)%4==0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);
						for(count=0;count<restwords;count++,tempbuff++,bytestransferred+=4)
						{
							SDIO->FIFO=*tempbuff;
						}
					}else 										//发送区半空,可以发送至少8字(32字节)数据
					{
						for(count=0;count<SD_HALFFIFO;count++)
						{
							SDIO->FIFO=*(tempbuff+count);
						}
						tempbuff+=SD_HALFFIFO;
						bytestransferred+=SD_HALFFIFOBYTES;
					}
					timeout=0X3FFFFFFF;	//写数据溢出时间
				}else
				{
					if(timeout==0)return SD_DATA_TIMEOUT; 
					timeout--;
				}
			} 
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//数据超时错误
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//清错误标志
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//数据块CRC错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) 	//接收fifo下溢错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);		//清错误标志
			return SD_TX_UNDERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//接收起始位错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//清错误标志
			return SD_START_BIT_ERR;		 
		}   
	      										   
			if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)		//发送结束
			{															 
				if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
				{   
					SDIO_CmdInitStructure.SDIO_Argument =0;//发送CMD12+结束传输 	  
					SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
					SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
					SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
					SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
					SDIO_SendCommand(&SDIO_CmdInitStructure);	
					
					errorstatus=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);//等待R1响应   
					if(errorstatus!=SD_OK)return errorstatus;	 
				}
			}
			INTX_ENABLE();//开启总中断
	 		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	    }else if(DeviceMode==SD_DMA_MODE)
		{
	   	TransferError=SD_OK;
			StopCondition=1;			//多块写,需要发送停止传输指令 
			TransferEnd=0;				//传输结束标置位，在中断服务置1
			SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);	//配置产生数据接收完成中断
			SD_DMA_Config((u32*)buf,nblks*blksize,DMA_DIR_MemoryToPeripheral);		//SDIO DMA配置
	 	 	SDIO->DCTRL|=1<<3;								//SDIO DMA使能. 
			timeout=SDIO_DATATIMEOUT;
	 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//等待传输完成 
			if(timeout==0)	 								//超时
			{									  
  				SD_Init();	 					//重新初始化SD卡,可以解决写入死机的问题
	 			return SD_DATA_TIMEOUT;			//超时	 
	 		}
			timeout=SDIO_DATATIMEOUT;
			while((TransferEnd==0)&&(TransferError==SD_OK)&&timeout)timeout--;
	 		if(timeout==0)return SD_DATA_TIMEOUT;			//超时	 
	 		if(TransferError!=SD_OK)return TransferError;	 
		}
  	}
 	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
 	errorstatus=IsCardProgramming(&cardstate);
 	while((errorstatus==SD_OK)&&((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)))
	{
		errorstatus=IsCardProgramming(&cardstate);
	}   
	return errorstatus;	   
}

//SDIO中断服务函数		  
void SDIO_IRQHandler(void) 
{											
 	SD_ProcessIRQSrc();//处理所有SDIO相关中断
}	 																    

//SDIO中断处理函数
//处理SDIO传输过程中的各种中断事务
//返回值:错误代码
_SD SD_ProcessIRQSrc(void)
{
	if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)//接收完成中断
	{	 
		if (StopCondition==1)
		{  
				SDIO_CmdInitStructure.SDIO_Argument =0;//发送CMD12+结束传输 	  
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);	
					
			TransferError=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
		}else TransferError = SD_OK;	
 		SDIO->ICR|=1<<8;//清除完成中断标记
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
 		TransferEnd = 1;
		return(TransferError);
	}
 	if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)//数据CRC错误
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//清错误标志
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
	    TransferError = SD_DATA_CRC_FAIL;
	    return(SD_DATA_CRC_FAIL);
	}
 	if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)//数据超时错误
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);  			//清中断标志
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
	    TransferError = SD_DATA_TIMEOUT;
	    return(SD_DATA_TIMEOUT);
	}
  	if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)//FIFO上溢错误
	{
		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);  			//清中断标志
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
	    TransferError = SD_RX_OVERRUN;
	    return(SD_RX_OVERRUN);
	}
   	if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)//FIFO下溢错误
	{
		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);  			//清中断标志
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
	    TransferError = SD_TX_UNDERRUN;
	    return(SD_TX_UNDERRUN);
	}
	if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)//起始位错误
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);  		//清中断标志
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
	    TransferError = SD_START_BIT_ERR;
	    return(SD_START_BIT_ERR);
	}
	return(SD_OK);
}
  
//检查CMD0的执行状态
//返回值:sd卡错误码
_SD CmdError(void)
{
	_SD errorstatus = SD_OK;
	u32 timeout=SDIO_CMD0TIMEOUT;	   
	while(timeout--)
	{
		if(SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) != RESET)break;	//命令已发送(无需响应)	 
	}	    
	if(timeout==0)return SD_CMD_RSP_TIMEOUT;  
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	return errorstatus;
}	 

//检查R7响应的错误状态
//返回值:sd卡错误码
_SD CmdResp7Error(void)
{
	_SD errorstatus=SD_OK;
	u32 status;
	u32 timeout=SDIO_CMD0TIMEOUT;
 	while(timeout--)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
	}
 	if((timeout==0)||(status&(1<<2)))	//响应超时
	{																				    
		errorstatus=SD_CMD_RSP_TIMEOUT;	//当前卡不是2.0兼容卡,或者不支持设定的电压范围
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 			//清除命令响应超时标志
		return errorstatus;
	}	 
	if(status&1<<6)						//成功接收到响应
	{								   
		errorstatus=SD_OK;
		SDIO_ClearFlag(SDIO_FLAG_CMDREND); 				//清除响应标志
 	}
	return errorstatus;
}	   

//检查R1响应的错误状态
//cmd:当前命令
//返回值:sd卡错误码
_SD CmdResp1Error(u8 cmd)
{	  
   	u32 status; 
	while(1)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)
	} 
	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)					//响应超时
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 				//清除命令响应超时标志
		return SD_CMD_RSP_TIMEOUT;
	}	
 	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)					//CRC错误
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL); 				//清除标志
		return SD_CMD_CRC_FAIL;
	}		
	if(SDIO->RESPCMD!=cmd)return SD_ILLEGAL_CMD;//命令不匹配 
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	return (_SD)(SDIO->RESP1&SD_OCR_ERRORBITS);//返回卡响应
}

//检查R2响应的错误状态
//返回值:错误状态
_SD CmdResp2Error(void)
{
	_SD errorstatus = SD_OK;
	u32 status;
	u32 timeout = SDIO_CMD0TIMEOUT;
	while (timeout--)
	{
		status = SDIO->STA;
		if (status & ((1 << 0) | (1 << 2) | (1 << 6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
	}
	if ((timeout == 0) || (status & (1 << 2)))	//响应超时
	{
		errorstatus = SD_CMD_RSP_TIMEOUT;
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 		//清除命令响应超时标志
		return errorstatus;
	}
	if (SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)						//CRC错误
	{
		errorstatus = SD_CMD_CRC_FAIL;
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);		//清除响应标志
	}
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	return errorstatus;
}

//检查R3响应的错误状态
//返回值:错误状态
_SD CmdResp3Error(void)
{
	u32 status;						 
 	while(1)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
	}
 	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)					//响应超时
	{											 
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);			//清除命令响应超时标志
		return SD_CMD_RSP_TIMEOUT;
	}	 
   SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
 	return SD_OK;								  
}


//检查R6响应的错误状态
//cmd:之前发送的命令
//prca:卡返回的RCA地址
//返回值:错误状态
_SD CmdResp6Error(u8 cmd,u16*prca)
{
	_SD errorstatus=SD_OK;
	u32 status;					    
	u32 rspr1;
 	while(1)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
	}
	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)					//响应超时
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);			//清除命令响应超时标志
		return SD_CMD_RSP_TIMEOUT;
	}	 	 
	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)						//CRC错误
	{								   
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);					//清除响应标志
 		return SD_CMD_CRC_FAIL;
	}
	if(SDIO->RESPCMD!=cmd)				//判断是否响应cmd命令
	{
 		return SD_ILLEGAL_CMD; 		
	}	    
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	rspr1=SDIO->RESP1;					//得到响应 	 
	if(SD_ALLZERO==(rspr1&(SD_R6_GENERAL_UNKNOWN_ERROR|SD_R6_ILLEGAL_CMD|SD_R6_COM_CRC_FAILED)))
	{
		*prca=(u16)(rspr1>>16);			//右移16位得到,rca
		return errorstatus;
	}
   	if(rspr1&SD_R6_GENERAL_UNKNOWN_ERROR)return SD_GENERAL_UNKNOWN_ERROR;
   	if(rspr1&SD_R6_ILLEGAL_CMD)return SD_ILLEGAL_CMD;
   	if(rspr1&SD_R6_COM_CRC_FAILED)return SD_COM_CRC_FAILED;
	return errorstatus;
}

//SDIO使能宽总线模式
//enx:0,不使能;1,使能;
//返回值:错误状态
_SD SDEnWideBus(u8 enx)
{
	_SD errorstatus = SD_OK;
 	u32 scr[2]={0,0};
	u8 arg=0X00;
	if(enx)arg=0X02;
	else arg=0X00;
 	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//SD卡处于LOCKED状态		    
 	errorstatus=FindSCR(RCA,scr);						//得到SCR寄存器数据
 	if(errorstatus!=SD_OK)return errorstatus;
	if((scr[1]&SD_WIDE_BUS_SUPPORT)!=SD_ALLZERO)		//支持宽总线
	{
		  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16;//发送CMD55+RCA,短响应	
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
		
	 	errorstatus=CmdResp1Error(SD_CMD_APP_CMD);
		
	 	if(errorstatus!=SD_OK)return errorstatus; 
		
		  SDIO_CmdInitStructure.SDIO_Argument = arg;//发送ACMD6,短响应,参数:10,4位;00,1位.	
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
			
     errorstatus=CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
		
		return errorstatus;
	}else return SD_REQUEST_NOT_APPLICABLE;				//不支持宽总线设置 	 
}												   

//检查卡是否正在执行写操作
//pstatus:当前状态.
//返回值:错误代码
_SD IsCardProgramming(u8 *pstatus)
{
 	vu32 respR1 = 0, status = 0;  
  
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16; //卡相对地址参数
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;//发送CMD13 	
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);	
 	
	status=SDIO->STA;
	
	while(!(status&((1<<0)|(1<<6)|(1<<2))))status=SDIO->STA;//等待操作完成
   	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)			//CRC检测失败
	{  
	  SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);	//清除错误标记
		return SD_CMD_CRC_FAIL;
	}
   	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)			//命令超时 
	{
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);			//清除错误标记
		return SD_CMD_RSP_TIMEOUT;
	}
 	if(SDIO->RESPCMD!=SD_CMD_SEND_STATUS)return SD_ILLEGAL_CMD;
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	respR1=SDIO->RESP1;
	*pstatus=(u8)((respR1>>9)&0x0000000F);
	return SD_OK;
}

//读取当前卡状态
//pcardstatus:卡状态
//返回值:错误代码
_SD SD_SendStatus(uint32_t *pcardstatus)
{
	_SD errorstatus = SD_OK;
	if(pcardstatus==NULL)
	{
		errorstatus=SD_INVALID_PARAMETER;
		return errorstatus;
	}
	
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16;//发送CMD13,短响应		 
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);	
	
	errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);	//查询响应状态 
	if(errorstatus!=SD_OK)return errorstatus;
	*pcardstatus=SDIO->RESP1;//读取响应值
	return errorstatus;
} 

//返回SD卡的状态
//返回值:SD卡状态
SDCardState SD_GetState(void)
{
	u32 resp1=0;
	if(SD_SendStatus(&resp1)!=SD_OK)return SD_CARD_ERROR;
	else return (SDCardState)((resp1>>9) & 0x0F);
}

//查找SD卡的SCR寄存器值
//rca:卡相对地址
//pscr:数据缓存区(存储SCR内容)
//返回值:错误状态		   
_SD FindSCR(u16 rca,u32 *pscr)
{ 
	u32 index = 0; 
	_SD errorstatus = SD_OK;
	u32 tempscr[2]={0,0};  
	
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)8;	 //发送CMD16,短响应,设置Block Size为8字节	
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN; //	 cmd16
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r1
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
 	errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	
 	if(errorstatus!=SD_OK)return errorstatus;	 
	
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16; 
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//发送CMD55,短响应 	
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
 	errorstatus=CmdResp1Error(SD_CMD_APP_CMD);
 	if(errorstatus!=SD_OK)return errorstatus;
	
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = 8;  //8个字节长度,block为8字节,SD卡到SDIO.
  SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_8b  ;  //块大小8byte 
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);		

  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;	//发送ACMD51,短响应,参数为0	
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r1
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
 	errorstatus=CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);
 	if(errorstatus!=SD_OK)return errorstatus;							   
 	while(!(SDIO->STA&(SDIO_FLAG_RXOVERR|SDIO_FLAG_DCRCFAIL|SDIO_FLAG_DTIMEOUT|SDIO_FLAG_DBCKEND|SDIO_FLAG_STBITERR)))
	{ 
		if(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)//接收FIFO数据可用
		{
			*(tempscr+index)=SDIO->FIFO;	//读取FIFO内容
			index++;
			if(index>=2)break;
		}
	}
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//数据超时错误
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//清错误标志
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//数据块CRC错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) 	//接收fifo上溢错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);		//清错误标志
			return SD_RX_OVERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//接收起始位错误
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//清错误标志
			return SD_START_BIT_ERR;		 
		}  
   SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记
	//把数据顺序按8位为单位倒过来.   	
	*(pscr+1)=((tempscr[0]&SD_0TO7BITS)<<24)|((tempscr[0]&SD_8TO15BITS)<<8)|((tempscr[0]&SD_16TO23BITS)>>8)|((tempscr[0]&SD_24TO31BITS)>>24);
	*(pscr)=((tempscr[1]&SD_0TO7BITS)<<24)|((tempscr[1]&SD_8TO15BITS)<<8)|((tempscr[1]&SD_16TO23BITS)>>8)|((tempscr[1]&SD_24TO31BITS)>>24);
 	return errorstatus;
}

//得到NumberOfBytes以2为底的指数.
//NumberOfBytes:字节数.
//返回值:以2为底的指数值
u8 convert_from_bytes_to_power_of_two(u16 NumberOfBytes)
{
	u8 count=0;
	while(NumberOfBytes!=1)
	{
		NumberOfBytes>>=1;
		count++;
	}
	return count;
} 	 

//配置SDIO DMA  
//mbuf:存储器地址
//bufsize:传输数据量
//dir:方向;DMA_DIR_MemoryToPeripheral  存储器-->SDIO(写数据);DMA_DIR_PeripheralToMemory SDIO-->存储器(读数据);
void SD_DMA_Config(u32*mbuf,u32 bufsize,u32 dir)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
	while (DMA_GetCmdStatus(DMA2_Stream3) != DISABLE){}//等待DMA可配置 

	DMA_DeInit(DMA2_Stream3);//清空之前该stream3上的所有中断标志
	
 
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;  //通道选择
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SDIO->FIFO;//DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)mbuf;//DMA 存储器0地址
	DMA_InitStructure.DMA_DIR = dir;//存储器到外设模式
	DMA_InitStructure.DMA_BufferSize = 0;//数据传输量 
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;//外设数据长度:32位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;//存储器数据长度:32位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;// 使用普通模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;//最高优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;   //FIFO使能      
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;//全FIFO
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;//外设突发4次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;//存储器突发4次传输
	DMA_Init(DMA2_Stream3, &DMA_InitStructure);//初始化DMA Stream

	DMA_FlowControllerConfig(DMA2_Stream3,DMA_FlowCtrl_Peripheral);//外设流控制 
	DMA_Cmd(DMA2_Stream3 ,ENABLE);//开启DMA传输	 

}   


//读SD卡
//buf:读数据缓存区
//sector:扇区地址
//cnt:扇区个数	
//返回值:错误状态;0,正常;其他,错误代码;
u8 SD_ReadDisk(u8*buf,u32 sector,u8 cnt)
{
	// buf = mymalloc(0, 512);		//申请内存
	// SD_ReadDisk(buf, 0, 1)	//读取0扇区的内容
	u8 sta=SD_OK;
	long long lsector=sector;
	u8 n;
	if(CardType!=SDIO_STD_CAPACITY_SD_CARD_V1_1)lsector<<=9;
	if((u32)buf%4!=0)
	{
	 	for(n=0;n<cnt;n++)
		{
		 	sta=SD_ReadBlock(SDIO_DATA_BUFFER,lsector+512*n,512);//单个sector的读操作
			memcpy(buf,SDIO_DATA_BUFFER,512);
			buf+=512;
		} 
	}else
	{
		if(cnt==1)sta=SD_ReadBlock(buf,lsector,512);    	//单个sector的读操作
		else sta=SD_ReadMultiBlocks(buf,lsector,512,cnt);//多个sector  
	}
	return sta;
}

//读SD卡的字节
//Bbuf:读数据缓存区
//sector:扇区地址（其实是簇的起始扇区地址）
//startBtye_offset:该扇区需要读的字节的起始地址（相对于扇区开始地址，0~512）
//【↑】****最好是4的倍数，如果出错了，就找这里
//length 读取的长度(几个字节)，
//返回值:错误状态;0,正常;其他,错误代码;	
//注意，需要提前声明Bbuf的 大小，函数把SD卡的内容放进Bbuf指向的地址里
u8 SD_ReakBytes(u8*Bbuf,u32 sector,u16 startBtye_offset ,u16 length) {
	u8 sta = SD_OK;
	long long lsector = sector;
	u16 n = 1;
	if (CardType != SDIO_STD_CAPACITY_SD_CARD_V1_1) lsector <<= 9;
//startBtye_offset其实也要求被4整除
//SD_ReadBlock读取的length必须是2的幂级数
	if ((u32)Bbuf % 4 != 0 || (length & length - 1) != 0)//四字节对齐吗，如果四字节对齐，但是读取的不是2幂级数，也同样
	{
//		n = startBtye_offset % 4 ;
		//既非4倍数地址又读取>=512字节，懒得弄了，直接用别的函数吧
		if ((n != 0 && length >= 512)||length==0) return SD_ERROR;
			//取最接近的length的2幂级数
//		n = length;
/*		for (t = 1; t <9 ; t++) {
			if (n == 1) {
				n = 1 << t;//n取最接近length，且比length大的数，最大是512
				break;
			}
				n >>= 1;
		}*/
		
		sta = SD_ReadBlock(SDIO_DATA_BUFFER, lsector , 512);//读取512字节，全部读！妈的
		memcpy(Bbuf, SDIO_DATA_BUFFER + startBtye_offset , length);
		

	}
	else
	{//四字节对齐的情况下
		sta = SD_ReadBlock(Bbuf, lsector + startBtye_offset, length);    	//单个sector的读操作
	}

	return sta;
}


//写SD卡
//buf:写数据缓存区
//sector:扇区地址
//cnt:扇区个数	
//返回值:错误状态;0,正常;其他,错误代码;	
u8 SD_WriteDisk(u8*buf,u32 sector,u8 cnt)
{
	u8 sta=SD_OK;
	u8 n;
	long long lsector=sector;
	if(CardType!=SDIO_STD_CAPACITY_SD_CARD_V1_1)lsector<<=9;
	if((u32)buf%4!=0)
	{
	 	for(n=0;n<cnt;n++)
		{
			memcpy(SDIO_DATA_BUFFER,buf,512);
		 	sta=SD_WriteBlock(SDIO_DATA_BUFFER,lsector+512*n,512);//单个sector的写操作
			buf+=512;
		} 
	}else
	{
		if(cnt==1)sta=SD_WriteBlock(buf,lsector,512);    	//单个sector的写操作
		else sta=SD_WriteMultiBlocks(buf,lsector,512,cnt);	//多个sector  
	}
	return sta;
}



void show_sdcard_info(void)
{
	switch (SDCardInfo.CardType)
	{
	case SDIO_STD_CAPACITY_SD_CARD_V1_1:printf("Card Type:SDSC V1.1\r\n"); break;
	case SDIO_STD_CAPACITY_SD_CARD_V2_0:printf("Card Type:SDSC V2.0\r\n"); break;
	case SDIO_HIGH_CAPACITY_SD_CARD:printf("Card Type:SDHC V2.0\r\n"); break;
	case SDIO_MULTIMEDIA_CARD:printf("Card Type:MMC Card\r\n"); break;
	}
	printf("Card ManufacturerID:%d\r\n", SDCardInfo.SD_cid.ManufacturerID);	//制造商ID
	printf("Card RCA:%d\r\n", SDCardInfo.RCA);								//卡相对地址
	printf("Card Capacity:%d MB\r\n", (u32)(SDCardInfo.CardCapacity >> 20));	//显示容量
	printf("Card BlockSize:%d\r\n\r\n", SDCardInfo.CardBlockSize);			//显示块大小
}

FRESULT cc936_Init(void) {
	FRESULT res;
	res=f_open(&cc936_bin, "/FONT/cc936.bin", FA_OPEN_EXISTING | FA_READ);
	/*if(res!=0) {printf("字库文件打开失败\n");return;}*/
	//f_close(&cc936_bin);
	if (res == 6) printf("FR_INVALID_NAME\n");
	else if (res == 4) printf("FR_NO_FILE\n");
	else if (res == 9) printf("FR_INVALID_OBJECT\n");

	return res;
}

FRESULT cc936_Deinit(void) {
	return f_close(&cc936_bin);
}

WCHAR ff_convert(	/* Converted code, 0 means conversion error */
	WCHAR	src,	/* Character code to be converted */
	UINT	dir		/* 0: Unicode to OEMCP, 1: OEMCP to Unicode */
)
{
	WCHAR t[2];
	WCHAR c;
	u32 i, li, hi;
	u16 n;
	u32 gbk2uni_offset = 0;
	UINT br;
	if (src < 0x80)c = src;//ASCII,直接不用转换.
	else
	{
		if (dir)	//GBK 2 UNICODE
		{
			gbk2uni_offset = cc936_bin.fsize / 2;
		}
		else	//UNICODE 2 GBK  
		{
			gbk2uni_offset = 0;
		}
		/* Unicode to OEMCP */
		hi = cc936_bin.fsize / 2;//对半开.
		hi = hi / 4 - 1;
		li = 0;
		for (n = 16; n; n--)
		{
			i = li + (hi - li) / 2;
			f_lseek(&cc936_bin, i * 4+ gbk2uni_offset);
			f_read(&cc936_bin, (u8*)&t, 4, &br);//读出4个字节
//			W25QXX_Read((u8*)&t, ftinfo.ugbkaddr + i * 4 + gbk2uni_offset, 4);//读出4个字节  
			if (src == t[0]) break;
			if (src > t[0])li = i;
			else hi = i;
		}
		c = n ? t[1] : 0;
	}
	return c;
}



WCHAR ff_wtoupper(	/* Upper converted character */
	WCHAR chr		/* Input character */
)
{
	static const WCHAR tbl_lower[] = {
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
		0xA1, 0x00A2, 0x00A3, 0x00A5, 0x00AC, 0x00AF,
		0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
		0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0x0FF,
		0x101, 0x103, 0x105, 0x107, 0x109, 0x10B, 0x10D, 0x10F,
		0x111, 0x113, 0x115, 0x117, 0x119, 0x11B, 0x11D, 0x11F,
		0x121, 0x123, 0x125, 0x127, 0x129, 0x12B, 0x12D, 0x12F,
		0x131, 0x133, 0x135, 0x137, 0x13A, 0x13C, 0x13E,
		0x140, 0x142, 0x144, 0x146, 0x148, 0x14B, 0x14D, 0x14F,
		0x151, 0x153, 0x155, 0x157, 0x159, 0x15B, 0x15D, 0x15F,
		0x161, 0x163, 0x165, 0x167, 0x169, 0x16B, 0x16D, 0x16F,
		0x171, 0x173, 0x175, 0x177, 0x17A, 0x17C, 0x17E,
		0x192,
		0x3B1, 0x3B2, 0x3B3, 0x3B4, 0x3B5, 0x3B6, 0x3B7, 0x3B8, 0x3B9, 0x3BA, 0x3BB, 0x3BC, 0x3BD, 0x3BE, 0x3BF,
		0x3C0, 0x3C1, 0x3C3, 0x3C4, 0x3C5, 0x3C6, 0x3C7, 0x3C8, 0x3C9, 0x3CA,
		0x430, 0x431, 0x432, 0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439,
		0x43A, 0x43B, 0x43C, 0x43D, 0x43E, 0x43F,
		0x440, 0x441, 0x442, 0x443, 0x444, 0x445, 0x446, 0x447, 0x448, 0x449,
		0x44A, 0x44B, 0x44C, 0x44D, 0x44E, 0x44F,
		0x451, 0x452, 0x453, 0x454, 0x455, 0x456, 0x457, 0x458, 0x459, 0x45A, 0x45B, 0x45C, 0x45E, 0x45F,
		0x2170, 0x2171, 0x2172, 0x2173, 0x2174, 0x2175, 0x2176, 0x2177, 0x2178, 0x2179,
		0x217A, 0x217B, 0x217C, 0x217D, 0x217E, 0x217F,
		0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49,
		0xFF4A, 0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F,
		0xFF50, 0xFF51, 0xFF52, 0xFF53, 0xFF54, 0xFF55, 0xFF56, 0xFF57, 0xFF58, 0xFF59, 0xFF5A, 0
	};
	static const WCHAR tbl_upper[] = {
		0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
		0x21,
		0xFFE0, 0xFFE1, 0xFFE5, 0xFFE2, 0xFFE3,
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
		0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
		0x178, 0x100, 0x102, 0x104, 0x106, 0x108, 0x10A, 0x10C, 0x10E,
		0x110, 0x112, 0x114, 0x116, 0x118, 0x11A, 0x11C, 0x11E, 0x120, 0x122, 0x124, 0x126, 0x128,
		0x12A, 0x12C, 0x12E, 0x130, 0x132, 0x134, 0x136, 0x139, 0x13B, 0x13D, 0x13F,
		0x141, 0x143, 0x145, 0x147, 0x14A, 0x14C, 0x14E, 0x150, 0x152, 0x154, 0x156, 0x158,
		0x15A, 0x15C, 0x15E, 0x160, 0x162, 0x164, 0x166, 0x168, 0x16A, 0x16C, 0x16E,
		0x170, 0x172, 0x174, 0x176, 0x179, 0x17B, 0x17D,
		0x191,
		0x391, 0x392, 0x393, 0x394, 0x395, 0x396, 0x397, 0x398, 0x399,
		0x39A, 0x39B, 0x39C, 0x39D, 0x39E, 0x39F,
		0x3A0, 0x3A1, 0x3A3, 0x3A4, 0x3A5, 0x3A6, 0x3A7, 0x3A8, 0x3A9,
		0x3AA, 0x410, 0x411, 0x412, 0x413, 0x414, 0x415, 0x416, 0x417, 0x418, 0x419,
		0x41A, 0x41B, 0x41C, 0x41D, 0x41E, 0x41F,
		0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427, 0x428, 0x429,
		0x42A, 0x42B, 0x42C, 0x42D, 0x42E, 0x42F,
		0x401, 0x402, 0x403, 0x404, 0x405, 0x406, 0x407, 0x408, 0x409,
		0x40A, 0x40B, 0x40C, 0x40E, 0x40F,
		0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169,
		0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
		0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29,
		0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
		0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39,
		0xFF3A, 0
	};
	int i;


	for (i = 0; tbl_lower[i] && chr != tbl_lower[i]; i++);

	return tbl_lower[i] ? tbl_upper[i] : chr;
}

