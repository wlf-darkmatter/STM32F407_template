#include "delay.h"
#include "sys.h"

#include <function_wlf.h>


////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////  
////////////////////////////////////////////////////////////////////////////////// 
 
static u8  fac_us=0;//us延时倍乘数			   
static u16 fac_ms=0;//ms延时倍乘数,在ucos下,代表每个节拍的ms数

#ifdef OS_CRITICAL_METHOD 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
//systick中断服务函数,使用ucos时用到
void SysTick_Handler(void)
{				   
	OSIntEnter();		//进入中断
    OSTimeTick();       //调用ucos的时钟服务程序               
    OSIntExit();        //触发任务切换软中断
}
#endif
			   
//初始化延迟函数
//当使用ucos的时候,此函数会初始化ucos的时钟节拍
//SYSTICK的时钟固定为HCLK时钟的1/8
//SYSCLK:需要输入的系统时钟（单位：MHz）
//锁相环压腔振荡器时钟PLL_VCO =HSE_VALUE / PLL_M * PLL_N = 8 / 8* 336 = 336MHz 
void delay_init(u8 SYSCLK)
{
#ifdef OS_CRITICAL_METHOD 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
	u32 reload;
#endif
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);//选用外部时钟HCLK/8
	fac_us=SYSCLK/8;		//不论是否使用ucos,fac_us都需要使用
	//在SYSCLK=168MHz 的情况下，SysTick=168/8=21MHz，fac_us=21，这表示SYSCLK每跳动21次，就过了 1 us
	    
#ifdef OS_CRITICAL_METHOD 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
	reload=SYSCLK/8;		//每秒钟的计数次数 单位为K	   
	reload*=1000000/OS_TICKS_PER_SEC;//根据OS_TICKS_PER_SEC设定溢出时间
							//reload为24位寄存器,最大值:16777216,在168M下,约合0.7989s左右	
	fac_ms=1000/OS_TICKS_PER_SEC;//代表ucos可以延时的最少单位	   
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD=reload; 	//每1/OS_TICKS_PER_SEC秒中断一次	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK
#else
	fac_ms=(u16)fac_us*1000;//非ucos下,代表每个ms需要的systick时钟数，在168MHz的情况下，需要21000次SYSCLK的跳动   
#endif
	STM32F407ZET6_info.fac_ms = fac_ms;
	STM32F407ZET6_info.fac_us = fac_us;
}								    

#ifdef OS_CRITICAL_METHOD 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
//延时nus
//nus:要延时的us数.		    								   
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;	//LOAD的值	    	 
	ticks=nus*fac_us; 			//需要的节拍数	  		 
	tcnt=0;
	OSSchedLock();				//阻止ucos调度，防止打断us延时
	told=SysTick->VAL;        	//刚进入时的计数器值
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;//时间超过/等于要延迟的时间,则退出.
		}  
	};
	OSSchedUnlock();			//开启ucos调度 									    
}
//延时nms
//nms:要延时的ms数
void delay_ms(u16 nms)
{	
	if(OSRunning == OS_TRUE && OSLockNesting == 0)//如果os已经在跑了	   
	{		  
		if(nms>=fac_ms)//延时的时间大于ucos的最少时间周期 
		{
   			OSTimeDly(nms/fac_ms);	//ucos延时
		}
		nms%=fac_ms;				//ucos已经无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((u32)(nms*1000));		//普通方式延时 
}
#else  //不用ucos时


//延时nus
//nus为要延时的us数.	
//注意:nus的值,不要大于798915us=79ms
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*fac_us; //时间加载，计数器里每放一个fac_us，就代表延时了1us	  		 
	SysTick->VAL=0x00;        //清空计数器
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //开始倒数 
	do
	{
		temp=SysTick->CTRL;//进行采样
	}
	while((temp&0x01)&&!(temp&(1<<16)));//等待时间到达，不断采样，采样过程可以中断，但是这不影响计时的进行。    
	//(temp&0x01)表示temp的比特0，也就是 STK_CTRL 的 Enable 比特位，<1>表示可用
	//temp&(1<<16)表示temp的比特16，也就是 STK_CTRL 的 CountFlag 比特位，<0>为一般情况，<1>表示计时到零了
	//(temp&0x01)&&!(temp&(1<<16))表示只要满足[Enable==1]&&[CountFlag==0]，就再次采样。

	//跳出循环，说明计时器【完成计时】，但也有可能是【时钟不可用】
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
	SysTick->VAL =0X00;       //清空计数器	 
}


//延时nms
//注意nms的范围
//SysTick->LOAD为24位寄存器,所以,最大延时为:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK单位为Hz,nms单位为ms
//对168M条件下,nms<=798ms 
void delay_xms(u16 nms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD=(u32)nms*fac_ms;//时间加载(SysTick->LOAD为24bit)
	SysTick->VAL =0x00;           //清空计数器
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //开始倒数  
	do
	{
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));//等待时间到达   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
	SysTick->VAL =0X00;       //清空计数器	  	    
} 


//延时nms 
//nms:0~65535
void delay_ms(u16 nms)
{	 	 
	u8 repeat=nms/540;	//这里用540,是考虑到某些客户可能超频使用,
						//比如超频到248M的时候,delay_xms最大只能延时541ms左右了
	//设定的nms很多时候都会大于1000ms，但是寄存器是24位的，
	//按照计算，能延时的时间 t = (1 / SysTick)*(0x00FFFFFF)。其中，SysTick = SYSCLK / AHBPresc / 8
	//								[s]		   <16,777,215‬>
	//综上，若AHBPresc=1（一般情况下）、SYSCLK是168MHz，不溢出延时的最大值是0.798915‬s，即798.915ms
	u16 remain=nms%540;
	while(repeat)
	{
		delay_xms(540);
		repeat--;
	}
	if(remain) delay_xms(remain);
	
} 
#endif
