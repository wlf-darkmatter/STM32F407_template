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
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);//�õ�����һ���ַ���Ӧ������ռ���ֽ���
	qh = *char_1; ql = *(++char_1);
	if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)//�� ���ú���
	{
		for (i = 0; i < csize; i++) *mat++ = 0x00;//�������
		return; //��������
	}

	//�������λ���ת�������ֽ�=160+���룻���ֽ�=160+λ��
	//�ֿ��Ǵӵ�1����1λ��ʼ�ģ�ÿ����94�����֣���ô��j����kλ�����֣������൱�ڵ�(j-1)*94+k������
	//1~15��û�к��֣�����ȫ�ǵı�����,
	//��94������94��λ


	AC = qh - 160; BC = ql - 160;
	offset = ((AC - 1) * 94 + BC - 1) * csize;//Ҫ�ĵ������ʼ��ַ

	total_sector = offset / (512);                       //�õ��ܵ�������������
	secoff = (unsigned int)offset % (512);           //�����ڵ��ֽ���ƫ��
	offset = 0;
	total_cluster = (unsigned int)total_sector / (fs[0]->csize);   //�õ��ܵĴ��� //fs[0]->csize��ÿ����ӵ�е�������
	BC = (unsigned char)total_sector % (fs[0]->csize);

	offset = clust2sect(fs[0], FontStartClust + total_cluster); //ȡ���ֿ�Ĵ�����������ַ,
																//֮����Ҫ������һȦ������Ϊ�ز���������,����һ�����������п��ܿ��

	if (secoff + csize <= 512) {
		SD_ReakBytes(mat, offset, secoff, csize);
	}
	else {//��������
		SD_ReakBytes(mat, offset, secoff, 512 - secoff);
		//���ͬʱ���Ҳ�����
		if (++BC > (fs[0]->csize))
		{
			BC = 0;
			total_cluster = (unsigned int)((total_sector++) / (fs[0]->csize));
			offset = clust2sect(fs[0], FontStartClust + total_cluster);
		}
		SD_ReakBytes(mat + 512 - secoff, offset, 0, csize - (512 - secoff));

	}


}

//��ʼ������
//����ֵ:0,�ֿ����.
//		 ����,�ֿⶪʧ
u8 font_init(void)
{
	u8 res = 0;
	u8 t = 0;
	char* fn;//�ļ���
	u8 sizeinfo = 0;
	while (t++ < 10)//������ȡ10��,���Ǵ���,˵��ȷʵ��������,�ø����ֿ���
	{
		printf("��ȡSD���е��ֿ�\n");
		POINT_COLOR = BLACK;
		LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializing...");

		fileinfo.lfsize = _MAX_LFN * 2 + 1;
		fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
		res = f_opendir(&dir, (const TCHAR*)"FONT"); //��FONTĿ¼
		if (res == 0) {
			POINT_COLOR = BLACK;
			printf("�ҵ�FONT�ֿ��ļ���.\n");
			wlf_ftinfo.fontok = 0xAA;
		}
		else {
			printf("û���ҵ�FONT�ֿ��ļ���.\n");
			POINT_COLOR = RED;
			LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializd failed.");
			return res;
		}
	}
	if (wlf_ftinfo.fontok != 0XAA)return 1;
	return 0;
}
