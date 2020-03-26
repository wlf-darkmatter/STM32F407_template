#include "sys.h"
#include "delay.h"
#include "usart.h"
#include <LED_STM32F407ZET6.h>
#include <Key_STM32F407ZET6.h>
#include <lcd.h>
#include <Beep.h>
#include "usmart.h"
#include <Tim.h>
#include <PWM.h>
#include <wlf_I2C.h>
#include <OLED.h>
#include <WLF_font.h>

#include <WIFI_ESP8266.h>
#include "SDIO_SDCard.h"
#include "malloc.h"

//���SD�������Ƿ��Զ���ʽ����1=�ǣ�0=��
#define FORMAT_IF_ERROR 0

/*��Ҫʹ��PA13��PA14�����Ƿֱ��Ӧ�š�SW-DIO���͡�SW-CLK�����ұ���һֱ����AF����ģʽ*/
/*���������ʱ��Ҫ�ǵ���ʹ��ʱ�ӣ�Ȼ�������ã���Ϊ��Ҫһ��ʱ��ȴ�ʱ����������*/
void STM32_init(void) {
	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	usart1_init(115200);/*******************************************************************/
	usmart_init(168);/*******************************************************************/
	Key_Init();/*******************************************************************/
	LCD_Init();/*******************************************************************/
	POINT_COLOR = BLACK;	//��������Ϊ��ɫ 									    
	LCD_ShowString(30, 20, 200, 16, 16, "Welcome [LiuQian].");
	LCD_ShowString(30, 40, 200, 16, 16, "LCD  Initialized.");

	OLED_Init();/*******************************************************************/
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);
	LCD_ShowString(30, 60, 200, 16, 16, "OLED Initialized.");



	SD_Init();/*******************************************************************/
	show_sdcard_info();	//��ӡSD�������Ϣ
	LCD_ShowString(30, 150, 200, 16, 16, "SD Card OK    ");
	POINT_COLOR = RED;
	LCD_ShowNum(30 + 13 * 8, 170, SDCardInfo.CardCapacity >> 20, 5, 16);//��ʾSD������
	
	//Ϊfatfs��ر��������ڴ�
	exfuns_init();/*******************************************************************/				 
	res = f_mount(fs[0], "0:", 1);//����SD�� 

	if (res == 0X0D)//SD��,FAT�ļ�ϵͳ����,
	{
		printf("SD������\r");
		POINT_COLOR = RED;
		LCD_ShowString(30, 80, 200, 20, 20, "SD card !ERROR!.");
		//���¸�ʽ��SD
#if FORMAT_IF_ERROR==1
		LCD_ShowString(30, 150, 200, 16, 16, "Flash Disk Formatting...");	//��ʽ��SD
		res = f_mkfs("0:", 1, 4096);//��ʽ��SD,0,�̷�;0,����Ҫ������,8������Ϊ1����
		if (res == 0)
		{
			f_setlabel((const TCHAR*)"0:WLF_SD");	//����SD���̵�����Ϊ��WLF_SD
			LCD_ShowString(30, 150, 200, 16, 16, "Flash Disk Format Finish");	//��ʽ�����
		}
		else LCD_ShowString(30, 150, 200, 16, 16, "Flash Disk Format Error ");	//��ʽ��ʧ��
		delay_ms(1000);
#endif
	}

	t = 0;
	while ((t++>100)||exf_getfree("0", &total, &free))	//�õ�SD������������ʣ������
	{
		LCD_ShowString(30, 150, 200, 16, 16, "SD Card FATFS Error!");
		delay_ms(200);
		LED1 != LED1;
	}
	t = 0;
	POINT_COLOR = BLACK;//��������   
	LCD_ShowString(30, 150, 200, 16, 16, "FATFS OK!   ");
	LCD_ShowString(30, 170, 200, 16, 16, "SD Total Size:     MB");
	LCD_ShowString(30, 190, 200, 16, 16, "SD  Free Size:     MB");
	LCD_ShowNum(30 + 8 * 14, 170, total >> 10, 5, 16);//��ʾSD�������� MB
	POINT_COLOR = (free < total / 5) ? RED : BLUE;//��������
	LCD_ShowNum(30 + 8 * 14, 190, free >> 10, 5, 16);//��ʾSD��ʣ������ MB	


	//��ʼ���ֿ�
	font_init();/*******************************************************************/	





	
}

int main(void) {

	u32 sd_size;

	u8* buf;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	
	
	STM32_init();




//��ȡSD��ʾ��
/*
	
	buf = mymalloc(0, 512);		//�����ڴ�
	if (SD_ReadDisk(buf, 0, 1) == 0)	//��ȡ0����������
	{
		POINT_COLOR = BROWN;
		LCD_ShowString(30, 230, 200, 16, 16, "USART1 Sending Data...");
		printf("SECTOR 0 DATA:\r\n");
		for (sd_size = 0; sd_size < 512; sd_size++)printf("%x ", buf[sd_size]);//��ӡ0��������    	   
		printf("\r\nDATA ENDED\r\n");
		LCD_ShowString(30, 230, 200, 16, 16, "USART1 Send Data Over!");
	}
	myfree(0, buf);//�ͷ��ڴ�	   
	*/

	



	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	while (1) {
		
	}

}


/*��PWM������

//PWM_TIM14_Init(500 - 1, 84 - 1);//����ʱ��Ϊ500us,��һ�������ǡ�ARR����Ҳ�С�period������ֵ
*/

/*��TIM3����������TIM3���ж�

TIM3_INT_Init(5000 - 1, 8400 - 1);//һ������£�Tout=(I+1)*(II+1)/84 (��λus)
*/


//ͨ�����ڴ�ӡSD�������Ϣ




