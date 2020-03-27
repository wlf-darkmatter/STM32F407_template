#include "WLF_font.h"
#include "usart.h"
#include "malloc.h"
#include "delay.h"



_font_info wlf_ftinfo;
FIL fs_hz;
DWORD FontStartClust;

void Font_GetGBKMat(u8* char_1, u8* mat, u8 size) {
	u16 i = 0;
	unsigned char qh, ql;
	u8 AC = 0, BC = 0;
	u32 offset = 0;
	unsigned int  total_sector, total_cluster, secoff;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);//得到字体一个字符对应点阵集所占的字节数
	qh = *char_1; ql = *(++char_1);
	if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)//非 常用汉字
	{
		for (i = 0; i < csize; i++) *mat++ = 0x00;//填充满格
		return; //结束访问
	}

	//内码和区位码的转换；高字节=160+区码；低字节=160+位码
	//字库是从第1区第1位开始的，每个区94个文字，那么第j区第k位的文字，它就相当于第(j-1)*94+k个文字
	//1~15区没有汉字，而是全角的标点符号,
	//共94个区，94个位


	AC = qh - 160; BC = ql - 160;
	offset = ((AC - 1) * 94 + BC - 1) * csize;//要的点阵的起始地址

	total_sector = offset / (512);                       //得到总的完整的扇区数
	secoff = (unsigned int)offset % (512);           //扇区内的字节数偏移
	offset = 0;
	total_cluster = (unsigned int)total_sector / (fs[0]->csize);   //得到总的簇数 //fs[0]->csize是每个簇拥有的扇区数
	BC = (unsigned char)total_sector % (fs[0]->csize);

	offset = clust2sect(fs[0], FontStartClust + total_cluster); //取汉字库的簇数的扇区地址,
																//之所以要这样绕一圈，是因为簇不是连续的,跨了一个扇区，就有可能跨簇

	if (secoff + csize <= 512) {
		SD_ReakBytes(mat, offset, secoff, csize);
	}
	else {//跨扇区了
		SD_ReakBytes(mat, offset, secoff, 512 - secoff);
		//如果同时真的也跨簇了
		if (++BC > (fs[0]->csize))
		{
			BC = 0;
			total_cluster = (unsigned int)((total_sector++) / (fs[0]->csize));
			offset = clust2sect(fs[0], FontStartClust + total_cluster);
		}
		SD_ReakBytes(mat + 512 - secoff, offset, 0, csize - (512 - secoff));

	}


}

//初始化字体
//返回值:0,字库完好.
//		 其他,字库丢失
u8 font_init(void)
{
	u8 res = 0;
	u8 t = 0;
	char* fn;//文件名
	u8 sizeinfo = 0;
	while (t++ < 10)//连续读取10次,都是错误,说明确实是有问题,得更新字库了
	{
		printf("读取SD卡中的字库\n");
		POINT_COLOR = BLACK;
		LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializing...");

		fileinfo.lfsize = _MAX_LFN * 2 + 1;
		fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
		res = f_opendir(&dir, (const TCHAR*)"FONT"); //打开FONT目录
		if (res == 0) {
			POINT_COLOR = BLACK;
			printf("找到FONT字库文件夹.\n");
			wlf_ftinfo.fontok = 0xAA;
		}
		else {
			printf("没有找到FONT字库文件夹.\n");
			POINT_COLOR = RED;
			LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializd failed.");
			return res;
		}
	}
	if (wlf_ftinfo.fontok != 0XAA)return 1;
	return 0;
}
