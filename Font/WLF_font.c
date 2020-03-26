#include "WLF_font.h"
#include "usart.h"
#include "malloc.h"
#include "delay.h"



_font_info ftinfo;


//code 字符指针开始
//从字库中查找出字模
//code 字符串的开始地址,GBK码
//mat  数据存放地址 (size/8+((size%8)?1:0))*(size) bytes大小	
//size:字体大小
void Get_HzMat(unsigned char* code, unsigned char* mat, u8 size)
{
	unsigned char qh, ql;
	unsigned char i;
	unsigned long foffset;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);//得到字体一个字符对应点阵集所占的字节数	 
	qh = *code;
	ql = *(++code);
	if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)//非 常用汉字
	{
		for (i = 0; i < csize; i++)*mat++ = 0x00;//填充满格
		return; //结束访问
	}
	if (ql < 0x7f)ql -= 0x40;//注意!
	else ql -= 0x41;
	qh -= 0x81;
	foffset = ((unsigned long)190 * qh + ql) * csize;	//得到字库中的字节偏移量  		  
	switch (size)
	{
	case 12:
//		W25QXX_Read(mat, foffset + ftinfo.f12addr, csize);
		break;
	case 16:
//		W25QXX_Read(mat, foffset + ftinfo.f16addr, csize);
		break;
	case 24:
//		W25QXX_Read(mat, foffset + ftinfo.f24addr, csize);
		break;

	}
}


//初始化字体
//返回值:0,字库完好.
//		 其他,字库丢失
u8 font_init(void)
{
	u8 res = 0;
	u8 t = 0;
	FIL* fftemp;
	u8 rval = 0;
	char* fn;//文件名

	fftemp = (FIL*)mymalloc(SRAMIN, sizeof(FIL));	//分配内存
	if (fftemp == NULL)
		rval = 1;
	
	while (t++ < 10)//连续读取10次,都是错误,说明确实是有问题,得更新字库了
	{
		printf("读取SD卡中的字库");
		POINT_COLOR = BLACK;
		LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializing.");
		res = f_opendir(&dir, (const TCHAR*)"FONT"); //打开FONT目录
		if (res == 0) {
			POINT_COLOR = BLACK;
			printf("\n找到FONT字库文件夹.\n");
		}
		else {
			printf("\n没有找到FONT字库文件夹.\n");
			POINT_COLOR = RED;
			LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializd failed.");
			return res;
		}

		printf("找到了以下字库文件 ");
		while (1)
		{
			res = f_readdir(&dir, &fileinfo);                   //读取目录下的一个文件
			if (res != FR_OK || fileinfo.fname[0] == 0) break;  //错误了/到末尾了,退出
			//if (fileinfo.fname[0] == '.') continue;             //忽略上级目录
			fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
			//strlen()记录最后一个非'0'的地址-初始地址，"0123456789"对应10
			if(strcmp(fn+strlen(fn)-2,"FON")==0)
			 printf("%s, ", fn);
		}
		printf("\n");



		//W25QXX_Read((u8*)&ftinfo, FONTINFOADDR, sizeof(ftinfo));//读出ftinfo结构体数据
		if (ftinfo.fontok == 0XAA)break;
		delay_ms(20);
	}
	if (ftinfo.fontok != 0XAA)return 1;
	return 0;
}
