#include "usmart.h"
#include "usmart_str.h"
////////////////////////////�û�������///////////////////////////////////////////////
//������Ҫ�������õ��ĺ�����������ͷ�ļ�(�û��Լ����) 

#include "SDIO_SDCard.h"
#include "fattester.h" 


#include <function_wlf.h>

//extern void led_set(u8 sta);
//extern void test_fun(void(*ledset)(u8),u8 sta);
//�������б��ʼ��(�û��Լ����)
//�û�ֱ������������Ҫִ�еĺ�����������Ҵ�
//�ر�ע�⣬����ṹ��������̫��ĺ����������ʹLCD��ʼ��ʧ�ܣ���Ҳ��֪��Ϊʲô��
struct _m_usmart_nametab usmart_nametab[]=
{
#if USMART_USE_WRFUNS==1 	//���ʹ���˶�д����
	(void*)Exit,"void Exit(void)",
	(void*)Debug,"void Debug(void)",
	/**************************************************/
	(void*)read_addr,"u32 read_addr(u32 addr)",
	(void*)write_addr,"void write_addr(u32 addr,u32 val)",	 
#endif		   
	(void*)delay_ms,"void delay_ms(u16 nms)",
 	(void*)delay_us,"void delay_us(u32 nus)",
	/******************  LCD **********************/
	(void*)LCD_Init,"void LCD_Init(void)",//����ᱨ������Ӳ���жϣ�Ŀǰ��֪��Ҫ��ν��
	(void*)LCD_Clear,"void LCD_Clear(u16 color)",
	(void*)LCD_Fill,"void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)",
	(void*)LCD_DrawLine,"void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)",
	(void*)LCD_DrawRectangle,"void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)",
	(void*)LCD_Draw_Circle,"void LCD_Draw_Circle(u16 x0,u16 y0,u8 r)",
	(void*)LCD_ShowNum,"void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)",
	(void*)LCD_ShowString,"void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)",/*�Ѽ��飬����ʶ��*/
	(void*)LCD_Fast_DrawPoint,"void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)",
	(void*)LCD_ReadPoint,"u16 LCD_ReadPoint(u16 x,u16 y)",							 
	(void*)LCD_Display_Dir,"void LCD_Display_Dir(u8 dir)",
	(void*)LCD_ShowxNum,"void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode)",
	(void*)LCD_Draw_setting,"LCD_Draw_setting(u16 color, u8 bold)",
	(void*)LCD_ShowString,"LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,char* p)",
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
	(void*)OLED_GUIGRAM_Init,"void OLED_GUIGRAM_Init(void)",
//	(void*)OLED_LocalRefresh,"void OLED_LocalRefresh(u8 x,u8 y,u8 dx,u8 dy)",
/*	(void*)SDIO_Clock_Set,"void SDIO_Clock_Set(u8 clkdiv)",
	(_SD*)SD_Init,"_SD SD_Init(void)",
	(_SD*)SD_PowerON,"_SD SD_PowerON(void)",
	(_SD*)SD_PowerOFF,"_SD SD_PowerOFF(void)",
	(_SD*)SD_InitializeCards,"_SD SD_InitializeCards(void)",
	(_SD*)SD_GetCardInfo,"_SD SD_GetCardInfo(SD_CardInfo* cardinfo)",
	(_SD*)SD_EnableWideBusOperation,"_SD SD_EnableWideBusOperation(u32 wmode)",
	(_SD*)SD_SetDeviceMode,"_SD SD_SetDeviceMode(u32 mode)",
	(_SD*)SD_SelectDeselect,"_SD SD_SelectDeselect(u32 addr)",
	(_SD*)SD_SendStatus,"_SD SD_SendStatus(uint32_t* pcardstatus)",
	(SDCardState*)SD_GetState,"SDCardState SD_GetState(void)",
	(_SD*)SD_ReadBlock,"_SD SD_ReadBlock(u8* buf,long long addr,u16 blksize)",
	(_SD*)SD_ReadMultiBlocks,"_SD SD_ReadMultiBlocks(u8* buf,long long addr,u16 blksize,u32 nblks)",
	(_SD*)SD_WriteBlock,"_SD SD_WriteBlock(u8* buf,long long addr,u16 blksize)",
	(_SD*)SD_WriteMultiBlocks,"_SD SD_WriteMultiBlocks(u8* buf,long long addr,u16 blksize,u32 nblks)",
	(_SD*)SD_ProcessIRQSrc,"_SD SD_ProcessIRQSrc(void)",
	(void*)SD_DMA_Config,"void SD_DMA_Config(u32* mbuf,u32 bufsize,u32 dir)",
	(u8*)SD_ReadDisk,"u8 SD_ReadDisk(u8* buf,u32 sector,u8 cnt)",
	(u8*)SD_WriteDisk,"u8 SD_WriteDisk(u8* buf,u32 sector,u8 cnt)",
	(void*)show_sdcard_info,"void show_sdcard_info(void)",
	*/
	/******************   WIFI **********************/
	(void*)ESP8266_init,"u8 ESP8266_init(void)",
	(void*)ESP8266_restart,"void ESP8266_restart(void)",
	(void*)ESP8266_send_cmd,"u8 ESP8266_send_cmd(char* cmd, char* ack, int waittime)",
	(void*)ESP8266_check_cmd,"u8* ESP8266_check_cmd(u8* str)",
	(void*)ESP8266_quit_trans,"u8 ESP8266_quit_trans(void)",
	(void*)usart2_init,"void usart2_init(u32 bound)",
	(void*)ESP8266_send_data,"u8* ESP8266_send_data(char* cmd, u16 waittime)",
	(void*)usart2_printf,"void usart2_printf(char* fmt, ...)",
	(void*)WiFi_Debug,"void WiFi_Debug(void)",
	/******************  RTC **********************/
	(void*)RTC_Set_Time,"u8 RTC_Set_Time(u8 hour,u8 min,u8 sec,u8 ampm)",
	(void*)RTC_Set_Date,"u8 RTC_Set_Date(u8 year,u8 month,u8 date,u8 week)",
	(void*)RTC_Set_AlarmA,"void RTC_Set_AlarmA(u8 week,u8 hour,u8 min,u8 sec)",
	(void*)RTC_Set_WakeUp,"void RTC_Set_WakeUp(u8 wksel,u16 cnt)",
	(void*)Show_RTC,"void Show_RTC(void)",
	/******************   SD **********************/
/*	(void*)mf_mount,"u8 mf_mount(u8* path,u8 mt)",
	(void*)mf_open,"u8 mf_open(u8*path,u8 mode)",
	(void*)mf_close,"u8 mf_close(void)",
	(void*)mf_read,"u8 mf_read(u16 len)",
	(void*)mf_write,"u8 mf_write(u8*dat,u16 len)",
	(void*)mf_opendir,"u8 mf_opendir(u8* path)",
	(void*)mf_closedir,"u8 mf_closedir(void)",
	(void*)mf_readdir,"u8 mf_readdir(void)",
	(void*)mf_scan_files,"u8 mf_scan_files(u8 * path)",
	(void*)mf_showfree,"u32 mf_showfree(u8 *drv)",
	(void*)mf_lseek,"u8 mf_lseek(u32 offset)",
	(void*)mf_tell,"u32 mf_tell(void)",
	(void*)mf_size,"u32 mf_size(void)",
	(void*)mf_mkdir,"u8 mf_mkdir(u8*pname)",
	(void*)mf_fmkfs,"u8 mf_fmkfs(u8* path,u8 mode,u16 au)",
	(void*)mf_unlink,"u8 mf_unlink(u8 *pname)",
	(void*)mf_rename,"u8 mf_rename(u8 *oldname,u8* newname)",
	(void*)mf_getlabel,"void mf_getlabel(u8 *path)",
	(void*)mf_setlabel,"void mf_setlabel(u8 *path)",
	(void*)mf_gets,"void mf_gets(u16 size)",
	(void*)mf_putc,"u8 mf_putc(u8 c)",
	(void*)mf_puts,"u8 mf_puts(u8*c)",*/


	(void*)show_picture,"u8 show_picture(const u8* filename, u8 fast)",

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



