#ifndef __BEEP_H
#define __BEEP_H
#endif
//WLF���ư�,�������Ŀ��ƶ˿���PF0

#include <sys.h>

#define BEEP PFout(0)



void Beep_Init(void);

//void Beep_Ring(uint16_t frequence);
void Beep_Ring(void);
void Beep_Sec(control _state);

