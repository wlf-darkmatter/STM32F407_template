#include "WLF_font.h"
#include "usart.h"
#include "malloc.h"
#include "delay.h"
#include <SDIO_SDCard.h>



FIL fs_hz;
DWORD FontStartClust;
u8 fontok;

void Font_GetGBKMat(u8* char_1, u8* mat, u8 size) {
	u16 i = 0;
	unsigned char qh, ql;
	u8 AC = 0, BC = 0;
	u32 offset = 0;
	unsigned int  total_sector, total_cluster, secoff;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);//得到字体一个字符对应点阵集所占的字节数
	qh = *char_1; ql = *(char_1+1);
	if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)//非 常用汉字
	{
		for (i = 0; i < csize; i++) *mat++ = 0x00;//填充满格
		return; //结束访问
	}
	/*
	当GBKL<0X7F时 Hp=((GBKH-0x81)×190+GBKL-0X40)×(sizex2)
	当GBKL>0X80时 Hp=((GBKH-0x81)×190+GBKL-0X41)×(sizex2)
	*/
	AC = qh - 0x81;
	if (ql < 0x7F)
		BC = ql - 0x40;//注意!
	else
		BC = ql - 0x41;
	offset = ((unsigned long)AC * 190 + BC) * csize;	//得到字库中的字节偏移量  		
	
	//每个GBK 码由2 个字节组成，第一个字节为0X81～0XFE，第二个字节分为两部分，一是0X40～0X7E，二是0X80～0XFE。
	//其中与GB2312相同的区域，字完全相同。
	/*********************************************/
//	AC = qh - 0x80; BC = ql - 0x80;
//	offset = ((AC - 1) * 94 + BC - 1) * csize;//要的点阵的起始地址
/*****************************************************/

	total_sector = offset / (512);                       //得到总的完整的扇区数
	secoff = (unsigned int)(offset % (512));           //扇区内的字节数偏移
	offset = 0;
	total_cluster = (unsigned int)total_sector / (fs[0]->csize);   //得到总的簇数 //fs[0]->csize是每个簇拥有的扇区数
	BC = (unsigned char)total_sector % (fs[0]->csize);//簇内扇区偏移，重复利用BC

	offset = clust2sect(fs[0], FontStartClust + total_cluster); //取汉字库的簇数的首扇区地址,
																//之所以要这样绕一圈，是因为簇不是连续的,跨了一个扇区，就有可能跨簇

	if (secoff + csize <= 512) {
		SD_ReakBytes(mat, offset+BC, secoff, csize);//这里的BC没有加上去，一直都是错误，【已修复！】
	}
	else {//跨扇区了
		SD_ReakBytes(mat, offset+BC, secoff, 512 - secoff);
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
//	char* fn;//文件名
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
			fontok = 0xAA;
			return res;
		}
		else {
			printf("没有找到FONT字库文件夹.\n");
			POINT_COLOR = RED;
			LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializd failed.");
			return res;
		}
	}
	if (fontok != 0XAA)return 1;
	return 0;
}
