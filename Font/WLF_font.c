#include "WLF_font.h"
#include "usart.h"
#include "malloc.h"
#include "delay.h"



_font_info ftinfo;


//code �ַ�ָ�뿪ʼ
//���ֿ��в��ҳ���ģ
//code �ַ����Ŀ�ʼ��ַ,GBK��
//mat  ���ݴ�ŵ�ַ (size/8+((size%8)?1:0))*(size) bytes��С	
//size:�����С
void Get_HzMat(unsigned char* code, unsigned char* mat, u8 size)
{
	unsigned char qh, ql;
	unsigned char i;
	unsigned long foffset;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);//�õ�����һ���ַ���Ӧ������ռ���ֽ���	 
	qh = *code;
	ql = *(++code);
	if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)//�� ���ú���
	{
		for (i = 0; i < csize; i++)*mat++ = 0x00;//�������
		return; //��������
	}
	if (ql < 0x7f)ql -= 0x40;//ע��!
	else ql -= 0x41;
	qh -= 0x81;
	foffset = ((unsigned long)190 * qh + ql) * csize;	//�õ��ֿ��е��ֽ�ƫ����  		  
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


//��ʼ������
//����ֵ:0,�ֿ����.
//		 ����,�ֿⶪʧ
u8 font_init(void)
{
	u8 res = 0;
	u8 t = 0;
	FIL* fftemp;
	u8 rval = 0;
	char* fn;//�ļ���

	fftemp = (FIL*)mymalloc(SRAMIN, sizeof(FIL));	//�����ڴ�
	if (fftemp == NULL)
		rval = 1;
	
	while (t++ < 10)//������ȡ10��,���Ǵ���,˵��ȷʵ��������,�ø����ֿ���
	{
		printf("��ȡSD���е��ֿ�");
		POINT_COLOR = BLACK;
		LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializing.");
		res = f_opendir(&dir, (const TCHAR*)"FONT"); //��FONTĿ¼
		if (res == 0) {
			POINT_COLOR = BLACK;
			printf("\n�ҵ�FONT�ֿ��ļ���.\n");
		}
		else {
			printf("\nû���ҵ�FONT�ֿ��ļ���.\n");
			POINT_COLOR = RED;
			LCD_ShowString(30, 210, 200, 12, 12, "GBK Initializd failed.");
			return res;
		}

		printf("�ҵ��������ֿ��ļ� ");
		while (1)
		{
			res = f_readdir(&dir, &fileinfo);                   //��ȡĿ¼�µ�һ���ļ�
			if (res != FR_OK || fileinfo.fname[0] == 0) break;  //������/��ĩβ��,�˳�
			//if (fileinfo.fname[0] == '.') continue;             //�����ϼ�Ŀ¼
			fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
			//strlen()��¼���һ����'0'�ĵ�ַ-��ʼ��ַ��"0123456789"��Ӧ10
			if(strcmp(fn+strlen(fn)-2,"FON")==0)
			 printf("%s, ", fn);
		}
		printf("\n");



		//W25QXX_Read((u8*)&ftinfo, FONTINFOADDR, sizeof(ftinfo));//����ftinfo�ṹ������
		if (ftinfo.fontok == 0XAA)break;
		delay_ms(20);
	}
	if (ftinfo.fontok != 0XAA)return 1;
	return 0;
}
