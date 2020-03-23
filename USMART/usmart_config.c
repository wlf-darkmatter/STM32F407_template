#include "usmart.h"
#include "usmart_str.h"
////////////////////////////�û�������///////////////////////////////////////////////
//������Ҫ�������õ��ĺ�����������ͷ�ļ�(�û��Լ����) 
#include "delay.h"		
#include "sys.h"
#include "lcd.h"
#include <OLED.h>
#include <usart.h>
#include "SDIO_SDCard.h"

//extern void led_set(u8 sta);
//extern void test_fun(void(*ledset)(u8),u8 sta);
//�������б��ʼ��(�û��Լ����)
//�û�ֱ������������Ҫִ�еĺ�����������Ҵ�
struct _m_usmart_nametab usmart_nametab[]=
{
#if USMART_USE_WRFUNS==1 	//���ʹ���˶�д����
	(void*)read_addr,"u32 read_addr(u32 addr)",
	(void*)write_addr,"void write_addr(u32 addr,u32 val)",	 
#endif		   
	(void*)delay_ms,"void delay_ms(u16 nms)",
 	(void*)delay_us,"void delay_us(u32 nus)",	

	(void*)LCD_Clear,"void LCD_Clear(u16 color)",
	(void*)LCD_Fill,"void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)",
	(void*)LCD_DrawLine,"void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)",
	(void*)LCD_DrawRectangle,"void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)",
	(void*)LCD_Draw_Circle,"void Draw_Circle(u16 x0,u16 y0,u8 r)",
	(void*)LCD_ShowNum,"void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)",
	(void*)LCD_ShowString,"void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)",
	(void*)LCD_Fast_DrawPoint,"void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)",
	(void*)LCD_ReadPoint,"u16 LCD_ReadPoint(u16 x,u16 y)",							 
	(void*)LCD_Display_Dir,"void LCD_Display_Dir(u8 dir)",
	(void*)LCD_ShowxNum,"void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode)",
//	(void*)led_set,"void led_set(u8 sta)",
//	(void*)test_fun,"void test_fun(void(*ledset)(u8),u8 sta)",
	(void*)OLED_Cmd,"void OLED_Cmd(u8 command)",
	(void*)OLED_Data,"void OLED_Data(u8 data)",
	(void*)OLED_Init,"void OLED_Init(void)",
	(void*)OLED_On,"void OLED_On(void)",
	(void*)OLED_Fill,"void OLED_Fill(unsigned char fill_Data)",
	(void*)OLED_SetPos,"void OLED_SetPos(unsigned char x, unsigned char y)",
	(void*)OLED_Refresh,"void OLED_Refresh(void)",
	(void*)OLED_DrawPoint,"void OLED_DrawPoint(u8 x, u8 y, unsigned char mode)",
	(void*)OLED_DrawChar,"void OLED_DrawChar(u8 x, u8 y, u8 chr, u8 size, u8 mode)",
	(void*)OLED_DrawStr,"void OLED_DrawStr(u8 x, u8 y, char* str, u8 size, u8 mode)",
	(void*)OLED_ShowGBK,"void OLED_ShowGBK(u8 x, u8 y, u8 num, u8 size, u8 mode)",
	(void*)OLED_Clear,"void OLED_Clear(void)",






};						  
///////////////////////////////////END///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//�������ƹ�������ʼ��
//�õ������ܿغ���������
//�õ�����������
struct _m_usmart_dev usmart_dev= {
	usmart_nametab,//������ָ��
	usmart_init,//void (*init)(u8);		��ʼ��
	usmart_cmd_rec,//u8 (*cmd_rec)(u8*str);	ʶ��������������������ֵ���� ( * ָ�������) ([�β��б�]);
	usmart_exe,//void (*exe)(void);ִ��
	usmart_scan,//void (*scan)(void);ɨ��
	sizeof(usmart_nametab)/sizeof(struct _m_usmart_nametab),//u8 fnum;��������
	0,	  	//u8 pnum;��������
	0,	 	//u8 id;����ID
	1,		//u8 sptype;������ʾ����,0,10����;1,16����
	0,		//u16 parmtype;��������.bitx:,0,����;1,�ַ���	    
	0,	  	//u8  plentbl[MAX_PARM];ÿ�������ĳ����ݴ��,��ҪMAX_PARM��0��ʼ��
	0,		//u8  parm[PARM_LEN];�����Ĳ���,��ҪPARM_LEN��0��ʼ��
	//u8 runtimeflag;����0,��ͳ�ƺ���ִ��ʱ��;1,ͳ�ƺ���ִ��ʱ��,ע��:�˹��ܱ�����USMART_ENTIMX_SCANʹ�ܵ�ʱ��,������
	//u32 runtime;����ʱ��,��λ:0.1ms,�����ʱʱ��Ϊ��ʱ��CNTֵ��2��*0.1ms
};   



