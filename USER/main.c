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
#include "includes.h"
#include "piclib.h"

/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				128
//�����ջ	
OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void* pdata);


//�����������
#define FLOLAT_TASK_PRIO				5
//���������ջ��С
#define FLOAT_STK_SIZE					128
//���������ʹ��printf������ӡ�������Ļ�һ��Ҫ8�ֽڶ���
/*__align(8)*/ OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];
//������
void float_task(void* pdata);

/*******************************        SD      ****************************************/
//���SD�������Ƿ��Զ���ʽ����1=�ǣ�0=��
#define FORMAT_IF_ERROR 0
OS_EVENT* message_SD;			//SD����д�����¼���ָ��
/***************************************************************************************/

char lcd_string[128];//ָ����Ҫ��ӡ���ַ�����ָ��

DIR picdir;
u16* picindextbl; //���ڴ��ͼƬ����
int mytemp;//�Զ����һ��ֻ������һ���������ⲿ������Ϣ�ı���

void STM32_init(void);
u16 pic_get_tnum(u8* path);//��ȡͼƬ����
u8 PictureFile_Init(void);//��¼����ͼƬ��Ϣ
u8 OLED_GUI_Init(void);
void OLED_CPUstate(void);//���CPU��Ϣ

/*��Ҫʹ��PA13��PA14�����Ƿֱ��Ӧ�š�SW-DIO���͡�SW-CLK�����ұ���һֱ����AF����ģʽ*/
/*���������ʱ��Ҫ�ǵ���ʹ��ʱ�ӣ�Ȼ�������ã���Ϊ��Ҫһ��ʱ��ȴ�ʱ����������*/
void STM32_init(void) {


	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	Key_Init();/*******************************************************************/
	usart1_init(115200);/*******************************************************************/
	usmart_init(168);/*******************************************************************/
	LCD_Init();/*******************************************************************/
	POINT_COLOR = BLACK;	//��������Ϊ��ɫ 									    
	LCD_ShowString(20, 20, 200, 16, 16, "Welcome [LiuQian].");
	LCD_ShowString(20, 40, 200, 16, 16, "LCD  Initialized.");
	sprintf(lcd_string, "LCD_H=[%d]  LCD_W=[%d]",lcddev.height,lcddev.width);
	LCD_ShowString(20, 60, 200, 16, 16, lcd_string);

	OLED_Init();/*******************************************************************/
	OLED_Clear();
	OLED_DrawStr(0, 0, "Welcome [LiuQian].", 12, 1);
	OLED_DrawStr(0, 12, "OLED Initialized", 12, 1);
	LCD_ShowString(20, 80, 200, 16, 16, "OLED Initialized.");


	SD_Init();/*******************************************************************/
	show_sdcard_info();	//��ӡSD�������Ϣ
	LCD_ShowString(20, 150, 200, 16, 16, "SD Card OK    ");
	POINT_COLOR = RED;
	LCD_ShowNum(20 + 13 * 8, 170, SDCardInfo.CardCapacity >> 20, 5, 16);//��ʾSD������
	//��ʼ���ڲ��ڴ��
	my_mem_init(SRAMIN);/*******************************************************************/
	//Ϊfatfs��ر��������ڴ�
	exfuns_init();/*******************************************************************/				 
	res = f_mount(fs[0], "0:", 1);//����SD�� 
	if (res == 0X0D)//SD��,FAT�ļ�ϵͳ����,
	{
		printf("SD������\r");
		POINT_COLOR = RED;
		LCD_ShowString(20, 80, 200, 20, 20, "SD card !ERROR!.");
		//���¸�ʽ��SD
#if FORMAT_IF_ERROR==1
		LCD_ShowString(20, 150, 200, 16, 16, "Flash Disk Formatting...");	//��ʽ��SD
		res = f_mkfs("0:", 1, 4096);//��ʽ��SD,0,�̷�;0,����Ҫ������,8������Ϊ1����
		if (res == 0)
		{
			f_setlabel((const TCHAR*)"0:WLF_SD");	//����SD���̵�����Ϊ��WLF_SD
			LCD_ShowString(20, 150, 200, 16, 16, "Flash Disk Format Finish");	//��ʽ�����
		}
		else LCD_ShowString(20, 150, 200, 16, 16, "Flash Disk Format Error ");	//��ʽ��ʧ��
		delay_ms(1000);
#endif
	}

	t = 0;//�õ�SD������������ʣ������
	while ((t++>100)||exf_getfree("0", &total, &free))	
	{
		LCD_ShowString(20, 150, 200, 16, 16, "SD Card FATFS Error!");
		delay_ms(200);
		LED1 != LED1;
	}
	t = 0;
	POINT_COLOR = BLACK;//��������   
	LCD_ShowString(20, 150, 200, 16, 16, "FATFS OK!      ");
	LCD_ShowString(20, 170, 200, 16, 16, "SD Total Size:     MB");
	LCD_ShowString(20, 190, 200, 16, 16, "SD  Free Size:     MB");
	LCD_ShowNum(20 + 8 * 14, 170, total >> 10, 5, 16);//��ʾSD�������� MB
	POINT_COLOR = (free < total / 5) ? RED : BLUE;//��������
	LCD_ShowNum(20 + 8 * 14, 190, free >> 10, 5, 16);//��ʾSD��ʣ������ MB	

	//��ʼ��cc936
	cc936_Init();/*******************************************************************/

	//��ʼ���ֿ�
	POINT_COLOR = BLACK;
	res=font_init();/*******************************************************************/	
	if(res==0) LCD_ShowString(20, 210, 200, 16, 16, "�ֿ��ʼ����ϣ�");
	else {
		POINT_COLOR = RED;
		LCD_ShowString(20, 210, 200, 16, 16, "Chinese font failed!"); 
		
	}
	POINT_COLOR = BLACK;
	res=PictureFile_Init();/*******************************************************************/
	sprintf(lcd_string, "ͼƬ��ȡ��ϣ���%d��", mytemp);
	printf("%s\n",lcd_string);
	if(res==0) LCD_ShowString(20, 230, 200, 16, 16, lcd_string);
	piclib_init();/*******************************************************************/
	
	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����."); 
	delay_ms(800);
	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����.."); 
	delay_ms(800);
/*	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����..."); 
	delay_ms(800);
	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����...."); 
	delay_ms(800);
	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����....."); 
	delay_ms(800);
	*/
	LCD_Clear(GREEN);
	show_picture("0:/PICTURE/��.bmp", 1);//��ʾͼƬ 







	//myfree(SRAMIN, lcd_string);
}






//��ʼ����
void start_task(void* pdata)
{
	OS_CPU_SR cpu_sr = 0;
	message_SD = OSMboxCreate((void*)0);
	OSStatInit();					//��ʼ��ͳ������.�������ʱ1��������
	pdata = pdata;
	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
//	OSTaskCreate(led0_task, (void*)0, (OS_STK*)&LED0_TASK_STK[LED0_STK_SIZE - 1], LED0_TASK_PRIO);
//	OSTaskCreate(led1_task, (void*)0, (OS_STK*)&LED1_TASK_STK[LED1_STK_SIZE - 1], LED1_TASK_PRIO);
	OSTaskCreate(float_task, (void*)0, (OS_STK*)&FLOAT_TASK_STK[FLOAT_STK_SIZE - 1], FLOLAT_TASK_PRIO);
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
}

/*��PWM������

//PWM_TIM14_Init(500 - 1, 84 - 1);//����ʱ��Ϊ500us,��һ�������ǡ�ARR����Ҳ�С�period������ֵ
*/

/*��TIM3����������TIM3���ж�

TIM3_INT_Init(5000 - 1, 8400 - 1);//һ������£�Tout=(I+1)*(II+1)/84 (��λus)
*/


//ͨ�����ڴ�ӡSD�������Ϣ


//�����������
void float_task(void* pdata) {
	OS_CPU_SR cpu_sr = 0;
	static float float_num = 0.01;
	while (1) {
		float_num += 0.01f;
		OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
		OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
		printf("float_num��ֵΪ��%.4f\n", float_num);
		delay_ms(500);
	}
}


int main(void) {
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2

	//���������ʼ��
	STM32_init();
	//ϵͳ��ʼ��
	OSInit();
	//������ʼ����
	OSTaskCreate(start_task, (void*)0, (OS_STK*)&START_TASK_STK[START_STK_SIZE - 1], START_TASK_PRIO);
	//��ʼ����
	OSStart();

}

u8 OLED_GUI_Init(void) {
	OLED_Clear();

}
u8 PictureFile_Init(void) {
	FILINFO picfileinfo;//��¼ͼƬ���ļ���Ϣ
	u16 totpicnum;
	u8* pname;//��·�����ļ���
	u8 res;
	u16 curindex;//�ļ���ǰ����
	u16 temp;
	u8* fn;//���ļ���
	u8 time = 0;
	while (f_opendir(&picdir, "0:/PICTURE"))//��ͼƬ�ļ���
	{
		LCD_ShowString(20, 230, 200, 16, 16, "PICTURE�ļ��д���!");
		delay_ms(20);
		if (time++ >= 0xff) return 1;
	}
	totpicnum = pic_get_tnum("0:/PICTURE"); //�õ�����Ч�ļ���
	time = 0;
	while (totpicnum == NULL)//ͼƬ�ļ�Ϊ0		
	{
		LCD_ShowString(20, 230, 200, 16, 16, "û��ͼƬ�ļ�!");
		delay_ms(20);
		if (time++ >= 0xff) return 2;
	}
	picfileinfo.lfsize = _MAX_LFN * 2 + 1;						//���ļ�����󳤶�
	picfileinfo.lfname = mymalloc(SRAMIN, picfileinfo.lfsize);	//Ϊ���ļ������������ڴ�
	pname = mymalloc(SRAMIN, picfileinfo.lfsize);				//Ϊ��·�����ļ��������ڴ�
	picindextbl = mymalloc(SRAMIN, 2 * totpicnum);				//����2*totpicnum���ֽڵ��ڴ�,���ڴ��ͼƬ����
	time = 0;
	while (picfileinfo.lfname == NULL || pname == NULL || picindextbl == NULL)//�ڴ�������
	{
		LCD_ShowString(20, 230, 200, 16, 16, "�ڴ����ʧ��!");
		delay_ms(20);
		if (time++ >= 0xff) return 3;
	}
	//��¼����
	res = f_opendir(&picdir, "0:/PICTURE"); //��Ŀ¼
	if (res == FR_OK)
	{
		curindex = 0;//��ǰ����Ϊ0
		while (1)//ȫ����ѯһ��
		{
			temp = picdir.index;								//��¼��ǰindex
			res = f_readdir(&picdir, &picfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
			if (res != FR_OK || picfileinfo.fname[0] == 0) break;	//������/��ĩβ��,�˳�		  
			fn = (u8*)(*picfileinfo.lfname ? picfileinfo.lfname : picfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				picindextbl[curindex] = temp;//��¼����
				curindex++;
			}
		}
	}
	mytemp = curindex;
	myfree(SRAMIN, picfileinfo.lfname);	//�ͷ��ڴ�			    
	myfree(SRAMIN, pname);				//�ͷ��ڴ�			    
	myfree(SRAMIN, picindextbl);		//�ͷ��ڴ�		
	return 0;
}
u16 pic_get_tnum(u8* path)
{
	u8 res;
	u16 rval = 0;
	DIR tdir;	 		//��ʱĿ¼
	FILINFO tfileinfo;	//��ʱ�ļ���Ϣ	
	u8* fn;
	res = f_opendir(&tdir, (const TCHAR*)path); 	//��Ŀ¼
	tfileinfo.lfsize = _MAX_LFN * 2 + 1;				//���ļ�����󳤶�
	tfileinfo.lfname = mymalloc(SRAMIN, tfileinfo.lfsize);//Ϊ���ļ������������ڴ�
	if (res == FR_OK && tfileinfo.lfname != NULL)
	{
		while (1)//��ѯ�ܵ���Ч�ļ���
		{
			res = f_readdir(&tdir, &tfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
			if (res != FR_OK || tfileinfo.fname[0] == 0) break;	//������/��ĩβ��,�˳�		  
			fn = (u8*)(*tfileinfo.lfname ? tfileinfo.lfname : tfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				rval++;//��Ч�ļ�������1
			}
		}
	}
	return rval;
}


