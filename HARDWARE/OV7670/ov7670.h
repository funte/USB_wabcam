#ifndef _OV7670_H
#define _OV7670_H
#include "sys.h"
#include "sccb.h"
//////////////////////////////////////////////////////////////////////////	 
//������ο�������guanfu_wang���롣
//ALIENTEKս��STM32������
//OV7670 ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/14
//�汾��V1.0		    							    							  
//////////////////////////////////////////////////////////////////////////

#define OV7670_VSYNC  	PAin(8)			// ͬ���źż��IO
#define OV7670_WRST		PDout(6)		// дָ�븴λ
#define OV7670_WREN		PBout(3)		// д��FIFOʹ��
#define OV7670_RCK		PBout(4)		// ������ʱ��
#define OV7670_RRST		PGout(14)  		// ��ָ�븴λ
#define OV7670_CS		PGout(15)  		// Ƭѡ�ź�(OE)

#define OV7670_DATA   GPIOC->IDR&0x00FF  					//��������˿�
//////////////////////////////////////////////////////////////////////////

#define OV_MAXBUF		320 * 240 * 2
extern const u32 ov_maxbuf;
typedef struct _OV_BUF
{
	u32 addr;
	u32 bsize;
	u8 sta;		// �ɶ�?
} OV_BUF, *POV_BUF;

// Ƶ�ʼ���
extern u8 OV0_FPS;		// timer.c�ж���
extern u8 FPS0Count;	// timer.c�ж���
extern u8 USB_SPS;		// timer.c�ж���
extern u8 SPSCount;		// timer.c�ж���

// #0����ͷ������
extern OV_BUF	ovbuf0;
extern OV_BUF	ovbuf1;
extern u8		ov0_sta;

//////////////////////////////////////////////////////////////////////////
u8   OV7670_Init (void);
void OV7670_Light_Mode (u8 mode);
void OV7670_Color_Saturation (u8 sat);
void OV7670_Brightness (u8 bright);
void OV7670_Contrast (u8 contrast);
void OV7670_Special_Effects (u8 eft);
void OV7670_Window_Set (u16 sx, u16 sy, u16 width, u16 height);
void OV7670_CaptureFrame(void);

#endif
