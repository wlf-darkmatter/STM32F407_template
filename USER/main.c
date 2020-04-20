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

#define MAIN_TASK_PRIO					4
#define MAIN_STK_SIZE					64
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
void main_task(void* padta);



void STM32_init(void);
void STM32_init(void) {
	u32 total, free;
	u8 t = 0;
	u8 res = 0;
	delay_init(168);/*******************************************************************/
	LED_Init();/*******************************************************************/
	Key_Init();/*******************************************************************/
	Remote_Init();/*******************************************************************/
	My_RTC_Init();/*******************************************************************/
	RTC_Set_WakeUp(RTC_WakeUpClock_CK_SPRE_16bits, 0);
	usart1_init(115200);/*******************************************************************/
	delay_ms(500);//LCD��λ�Ƚ���
	usmart_init(84);/*******************************************************************/
	LCD_Init();/*******************************************************************/
	LCD_Init();/*******************************************************************/
	POINT_COLOR = BLACK;	//��������Ϊ��ɫ 									    
	LCD_ShowString(20, 20, 200, 16, 16, "Welcome [LiuQian].");
	LCD_ShowString(20, 40, 200, 16, 16, "LCD  Initialized.");
	sprintf(string_buff, "LCD_H=[%d]  LCD_W=[%d]",lcddev.height,lcddev.width);
	LCD_ShowString(20, 60, 200, 16, 16, string_buff);

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
	OLED_GUIGRAM_Init();
	LCD_ShowString(20, 150, 200, 16, 16, "FATFS OK!      ");
	LCD_ShowString(20, 170, 200, 16, 16, "SD Total Size:     MB");
	LCD_ShowString(20, 190, 200, 16, 16, "SD  Free Size:     MB");
	LCD_ShowNum(20 + 8 * 14, 170, total >> 10, 5, 16);//��ʾSD�������� MB
	POINT_COLOR = (free < total / 5) ? RED : BLUE;//��������
	LCD_ShowNum(20 + 8 * 14, 190, free >> 10, 5, 16);//��ʾSD��ʣ������ MB	
	STM32F407ZET6_info.SD_total = total;
	STM32F407ZET6_info.SD_free = free;
	STM32F407ZET6_info.SD_free = free;


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
	LCD_ShowString(20, 100, 200, 24, 24, "��ٻ,��ӭʹ��~");
	POINT_COLOR = BLACK;
	res=PictureFile_Init();/*******************************************************************/
	sprintf(string_buff, "ͼƬ��ȡ��ϣ���%d��", STM32F407ZET6_info.Picture_totalnum);

	printf("%s\r\n",string_buff);
	if(res==0) LCD_ShowString(20, 230, 200, 16, 16, string_buff);
	piclib_init();/*******************************************************************/
	
	OLED_GUIGRAM_Init();/*******************************************************************/
	OLED_Refresh();

	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����."); 
	delay_ms(200);
	LCD_ShowString(20, 246, 200, 24, 24, "������ʾ����.."); 
	delay_ms(200);

	
	LCD_Clear(PURPLE);
	show_picture("0:/PICTURE/ͷ��.bmp", 1);//��ʾͼƬ 

//	ESP8266_init();/*******************************************************************/


	STM32F407ZET6_info.point_COLOR=&POINT_COLOR;
	STM32F407ZET6_info.back_COLOR=&BACK_COLOR;
	STM32F407ZET6_info.font_BOLD=&FontBold;

	
	LQ_period_read();//��ȡ������ʱ��
}



//��ʼ����
void start_task(void* pdata)
{
	OS_CPU_SR cpu_sr = 0;
	pdata = pdata;
	message_SD = OSMboxCreate((void*)0);
	Message_Input = OSMboxCreate((void*)0);//���ݡ����⡿�ĺ͡���ť��������
	Message_APP_cmd = OSMboxCreate((void*)0);//���ݸ���������������������
	Message_LQ_clock = OSMboxCreate((void*)0);//���ݸ���LQ_clock����ʱ������
	OSStatInit();					//��ʼ��ͳ������.�������ʱ1��������
	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
	OSTaskCreate(USMART_APP,(void*)0,(OS_STK*)&USMART_APP_TASK_STK[USMART_APP_STK_SIZE-1],USMART_APP_TASK_PRIO);//3��
	OSTaskCreate(main_task, (void*)0, (OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1], MAIN_TASK_PRIO);//4��
	OSTaskCreate(APP_task, (void*)0, (OS_STK*)&APP_TASK_STK[APP_STK_SIZE-1], APP_TASK_PRIO);//5��
	OSTaskCreate(OLED_GUI_update, (void*)0, (OS_STK*)&OLED_TASK_STK[OLED_STK_SIZE - 1], OLED_TASK_PRIO);//6��
	OSTaskCreate(Show_LQ_CLOCK, (void*)0, (OS_STK*)&LQ_CLOCK_TASK_STK[LQ_CLOCK_STK_SIZE - 1], LQ_CLOCK_TASK_PRIO);//7��
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
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

void main_task(void* padta) {
	OS_CPU_SR cpu_sr;
	u8 err;
	u32 res;
	_RMT_CMD* cmd;
	u8 cmd_index;
	char* cmd_str;
	
	cmd_str=cmd_str;
	while (1) {
		
		while (1) {

			if ((u32)OSMboxPend(Message_Input, 800, &err) != 0) break;
			else OLED_DrawStr(0, 34, "    ", 24, 1);//800��ϵͳ�δ�û���յ���Ϣ���Զ�����
		}
		res = Remote_Scan();//�ȴ�500��ϵͳ�δ��ڴ��ڼ������ϵͳ����
		
		OS_ENTER_CRITICAL();

		if (res != 0) {
			switch (res)
		{
		default:	cmd = &Remote_CmdStr[0];
			break;
		case 162:	cmd = &Remote_CmdStr[1];
			break;
		case 98:	cmd = &Remote_CmdStr[2];
			break;
		case 226:	cmd = &Remote_CmdStr[3];
			break;
		case 34:	cmd = &Remote_CmdStr[4];
			break;
		case 2:		cmd = &Remote_CmdStr[5];
			break;
		case 194:	cmd = &Remote_CmdStr[6];
			break;
		case 224:	cmd = &Remote_CmdStr[7];
			break;
		case 168:	cmd = &Remote_CmdStr[8];
			break;
		case 144:	cmd = &Remote_CmdStr[9];
			break;
		case 104:	cmd = &Remote_CmdStr[10];
			break;
		case 152:	cmd = &Remote_CmdStr[11];
			break;
		case 176:	cmd = &Remote_CmdStr[12];
			break;
		case 48:	cmd = &Remote_CmdStr[13];
			break;
		case 24:	cmd = &Remote_CmdStr[14];
			break;
		case 122:	cmd = &Remote_CmdStr[15];
			break;
		case 16:	cmd = &Remote_CmdStr[16];
			break;
		case 56:	cmd = &Remote_CmdStr[17];
			break;
		case 90:	cmd = &Remote_CmdStr[18];
			break;
		case 66:	cmd = &Remote_CmdStr[19];
			break;
		case 74:	cmd = &Remote_CmdStr[20];
			break;
		case 82:	cmd = &Remote_CmdStr[21];
			break;
		}
			cmd_index = cmd->index;
//			cmd_str = (char*)(cmd->name);
			/*********************��APP������Ϣ***********************/

//			printf("%d\n",cmd_num);
			OSMboxPost(Message_APP_cmd, &cmd_index);//�����ź�
			OS_EXIT_CRITICAL();
			/*********************************************************/
			
		}

		OS_EXIT_CRITICAL();

		
	}
}
