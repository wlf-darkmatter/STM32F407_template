#include "function_wlf.h"



struct _app_LCD App_LCD;


void OLED_GUI_update(void* pdata) {
	OS_CPU_SR cpu_sr = 0;
	char strtemp[8];
	//OLED_GUI_Init();
	while (1) {
		//		OLED_DrawStr(0, 0, "CPU:    %", 16, 1);//利用率
		sprintf(strtemp, "%02d", OSCPUUsage);
		OLED_DrawStr_manual(40, 0, strtemp, 16, 1);//利用率

		OLED_DrawStr_manual(72, 0, " |03/31", 16, 1);
		//	OLED_DrawStr(0, 22, "User QianQian", 12, 1);
		OLED_DrawStr_manual(80, 16, "|12:13", 16, 1);//时间
		OS_ENTER_CRITICAL();
		OLED_Refresh();
		OS_EXIT_CRITICAL();
		delay_ms(1000);
	}
}

DIR PictureDir;//之后每次打开一个文件，都需要这个，所以就保存为实体了

u8 PictureFile_Init(void) {
	FILINFO picfileinfo;
	u16 totpicnum;
	u8* pname;//带路径的文件名
	u8 res;
	u16 curindex;//文件当前索引
	u16 temp;
	u8* fn;//长文件名
	u8 time = 0;
	u16* picindextbl; //用于存放图片索引

	while (f_opendir(&PictureDir, "0:/PICTURE"))//打开图片文件夹
	{
		LCD_ShowString(20, 230, 200, 16, 16, "PICTURE文件夹错误!");
		delay_ms(20);
		if (time++ >= 0xff) return 1;
	}
	totpicnum = pic_get_tnum("0:/PICTURE"); //得到总有效文件数
	time = 0;
	while (totpicnum == NULL)//图片文件为0		
	{
		LCD_ShowString(20, 230, 200, 16, 16, "没有图片文件!");
		delay_ms(20);
		if (time++ >= 0xff) return 2;
	}
	picfileinfo.lfsize = _MAX_LFN * 2 + 1;						//长文件名最大长度
	picfileinfo.lfname = mymalloc(SRAMIN, picfileinfo.lfsize);	//为长文件缓存区分配内存
	pname = mymalloc(SRAMIN, picfileinfo.lfsize);				//为带路径的文件名分配内存
	picindextbl = mymalloc(SRAMIN, 2 * totpicnum);				//申请2*totpicnum个字节的内存,用于存放图片索引
	time = 0;
	while (picfileinfo.lfname == NULL || pname == NULL || picindextbl == NULL)//内存分配出错
	{
		LCD_ShowString(20, 230, 200, 16, 16, "内存分配失败!");
		delay_ms(20);
		if (time++ >= 0xff) return 3;
	}
	//记录索引
	res = f_opendir(&PictureDir, "0:/PICTURE"); //打开目录
	if (res == FR_OK)
	{
		curindex = 0;//当前索引为0
		while (1)//全部查询一遍
		{
			temp = PictureDir.index;								//记录当前index
			res = f_readdir(&PictureDir, &picfileinfo);       		//读取目录下的一个文件
			if (res != FR_OK || picfileinfo.fname[0] == 0) break;	//错误了/到末尾了,退出		  
			fn = (u8*)(*picfileinfo.lfname ? picfileinfo.lfname : picfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//取高四位,看看是不是图片文件	
			{
				picindextbl[curindex] = temp;//记录索引
				curindex++;
			}
		}
	}
	App_LCD.Picture_num = curindex;
	myfree(SRAMIN, picfileinfo.lfname);	//释放内存			    
	myfree(SRAMIN, pname);				//释放内存			    
	myfree(SRAMIN, picindextbl);		//释放内存		
	return 0;
}
u16 pic_get_tnum(u8* path)
{
	u8 res;
	u16 rval = 0;
	DIR tdir;	 		//临时目录
	FILINFO tfileinfo;	//临时文件信息	
	u8* fn;
	res = f_opendir(&tdir, (const TCHAR*)path); 	//打开目录
	tfileinfo.lfsize = _MAX_LFN * 2 + 1;				//长文件名最大长度
	tfileinfo.lfname = mymalloc(SRAMIN, tfileinfo.lfsize);//为长文件缓存区分配内存
	if (res == FR_OK && tfileinfo.lfname != NULL)
	{
		while (1)//查询总的有效文件数
		{
			res = f_readdir(&tdir, &tfileinfo);       		//读取目录下的一个文件
			if (res != FR_OK || tfileinfo.fname[0] == 0) break;	//错误了/到末尾了,退出		  
			fn = (u8*)(*tfileinfo.lfname ? tfileinfo.lfname : tfileinfo.fname);
			res = f_typetell(fn);
			if ((res & 0XF0) == 0X50)//取高四位,看看是不是图片文件	
			{
				rval++;//有效文件数增加1
			}
		}
	}
	return rval;
}


