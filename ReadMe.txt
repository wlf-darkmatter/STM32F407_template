#该文件夹下的文件已被适配，修改为只对应STM32F407单片机

version = 1.08.3
2020/03/19/17:36
当前版本已经可以实现【LCD】的初始化设置以及字符的正常显示。
原子哥的初始化颜色变化程序可以正常运行。
接下来的更新将删去不含9341驱动的代码以节约存储资源。

version = 1.09.0
2020/03/23/16:28
当前版本已经加入了USMART这一控制台程序

version = 1.10.0
2020/03/23/22:35
当前版本添加了SD卡的读写

version = 1.10.1
2020/03/23/22:35
换回了lcd.h

version = 1.10.2
2020/03/23/22:58
添加了malloc内存管理
添加了总初始化函数STM32_init()

version = 1.11.0
2020/03/24/00:23
新建FAT文件系统

version = 1.11.0
2020/03/24/00:23
加入必要的文件

version = 1.11.1
2020/03/25/20:23
大规模修改前准备

version = 1.11.2
2020/03/26/9:202
大规模修改前准备
中文字库准备

version = 1.11.3
2020/03/26/9:20
中文字库准备

version = 1.11.4
2020/03/26/12:21
支持了16中字体库，每种字体库都有三种字体大小
改写了部分字库的初始化优化设置

version = 1.11.5
2020/03/26/12:26
修改了名字识别BUG

version = 1.11.6
2020/03/27/02:41
在ff中添加了clust2sect
在lcd.c中修改了LCD_ShowString
在lcd.c中添加了LCD_GetBGKMat

version = 1.11.7
2020/03/28/01:23
可以显示中文了，但是存在读取SD卡时的4字节对齐问题
中文显示存在不匹配的问题
一 = D2BB——显示为【耀】D2AB
二 = B6FE——显示为【额】B6EE

version = 1.11.8
2020/03/28/11:43
可以显示中文了，
SD_ReakBytes(mat, offset+BC, secoff, csize);//这里的BC没有加上去，一直都是错误，【已修复！】

version = 1.11.9
2020/03/28/12:04
secoff = (unsigned int)(offset % (512));
【secoff】 => 【startBtye_offset】其实也要求被4整除，否则在函数
u8 SD_ReakBytes(u8*Bbuf,u32 sector,u16 【startBtye_offset】 ,u16 length)中的
SD_ReadBlock(u8 *buf,long long 【addr】,u16 blksize)
会发生读取错误。
修复12号，24号不是2的幂级数，SD_ReadBlock无法正常使用的BUG
12号的csize为 24B
16号的csize为 32B
24号的csize为 72B
直接读取512字节了，他妈的，不优化了

*********************************************************************
version = 1.12.0
2020/03/28/21:29
为fatfs相关变量申请内存
创建了void cc936_init(void)，用于开始时锁定cc936

version = 1.12.1
2020/03/28/21:29
大规模修改前准备，今晚熬夜！4月前弄好程序，开始封装设备

version = 1.12.2
2020/03/28/22:51
成功移动了cc936文件到SD卡中
熟悉了f_read和f_open
复刻不同粗细的字体文件
添加了函数
//设置颜色和字体粗细
//color为颜色
//bold为粗细，有0，16，32，64，128 其中0为默认
void LCD_Font_setting(u16 color, u8 bold)
