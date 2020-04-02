#include "function_wlf.h"



/*��Ҫʹ��PA13��PA14�����Ƿֱ��Ӧ�š�SW-DIO���͡�SW-CLK�����ұ���һֱ����AF����ģʽ*/
/*���������ʱ��Ҫ�ǵ���ʹ��ʱ�ӣ�Ȼ�������ã���Ϊ��Ҫһ��ʱ��ȴ�ʱ����������*/


/*******************************��ʱ��****************************/
//����ʱ��4���Ǹ���USMART����
//����ʱ��7���Ǹ���USART2���ģ�����WiFi��������



/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
#define START_STK_SIZE  				128//���������ջ��С
OS_STK START_TASK_STK[START_STK_SIZE];//�����ջ	
void start_task(void* pdata);//������


#define FLOLAT_TASK_PRIO				5
#define FLOAT_STK_SIZE					128//���������ջ��С
#if __FPU_USED//���������ʹ��printf������ӡ�������Ļ�һ��Ҫ8�ֽڶ���
__align(8) OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];
#else
OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];
#endif
//������
//�����������
void float_task(void* pdata);

OS_STK FLOAT_TASK_STK[FLOAT_STK_SIZE];

//����function.c��
//#define WIFI_DEBUG_TASK_PRIO				2
//#define WIFI_DEBUG_STK_SIZE				128
/***************************************************************************************/

char lcd_string[64];//ָ����Ҫ��ӡ���ַ�����ָ��



void STM32_init(void);
void STM32_init(void) {

	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	Key_Init();/*******************************************************************/
	My_RTC_Init();/*******************************************************************/
	RTC_Set_WakeUp(RTC_WakeUpClock_CK_SPRE_16bits, 0);
	usart1_init(115200);/*******************************************************************/
	usmart_init(84);/*******************************************************************/
	delay_ms(200);//LCD��λ�Ƚ���
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
	sprintf(lcd_string, "ͼƬ��ȡ��ϣ���%d��", App_LCD.Picture_num);
	printf("%s\n",lcd_string);
	if(res==0) LCD_ShowString(20, 230, 200, 16, 16, lcd_string);
	piclib_init();/*******************************************************************/
	
	OLED_GUIGRAM_Init();/*******************************************************************/
	OLED_Refresh();

	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����."); 
	delay_ms(800);
	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����.."); 
	delay_ms(800);

	LCD_Clear(GREEN);
	show_picture("0:/PICTURE/ͷ��.bmp", 1);//��ʾͼƬ 

	ESP8266_init();/*******************************************************************/




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
	OSTaskCreate(USMART_APP, (void*)0, (OS_STK*)&USMART_APP_TASK_STK[USMART_APP_STK_SIZE - 1], USMART_APP_TASK_PRIO);
	OSTaskCreate(OLED_GUI_update, (void*)0, (OS_STK*)&OLED_TASK_STK[OLED_STK_SIZE - 1], OLED_TASK_PRIO);
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


/**********************************�����������****************************************/
void float_task(void* pdata) {
/*	OS_CPU_SR cpu_sr = 0;
	static float float_num = 0.01f;
	while (1) {
		float_num += 0.01f;
		OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
		printf("float_num��ֵΪ��%.4f\n", float_num);
		OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
		delay_ms(500);
	}
*/
}
/**************************************************************************************/




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

//��OLED.c��ʵ��
/*u8 OLED_GUI_Init(void) {
	OLED_Clear();
	OLED_DrawStr(0, 0, "CPU: 00 %", 16, 1);//������
	OLED_DrawStr(72, 0, " |12:13", 16, 1);//ʱ��
	OLED_DrawStr(0, 22, "User: QianQian", 12, 1);
	OLED_DrawStr(80, 16, "|03/31", 16, 1);
}*/


