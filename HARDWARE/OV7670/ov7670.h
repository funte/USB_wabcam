#ifndef _OV7670_H
#define _OV7670_H
#include "sys.h"
#include "sccb.h"
//////////////////////////////////////////////////////////////////////////	 
//本程序参考自网友guanfu_wang代码。
//ALIENTEK战舰STM32开发板
//OV7670 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/14
//版本：V1.0		    							    							  
//////////////////////////////////////////////////////////////////////////

#define OV7670_VSYNC  	PAin(8)			// 同步信号检测IO
#define OV7670_WRST		PDout(6)		// 写指针复位
#define OV7670_WREN		PBout(3)		// 写入FIFO使能
#define OV7670_RCK		PBout(4)		// 读数据时钟
#define OV7670_RRST		PGout(14)  		// 读指针复位
#define OV7670_CS		PGout(15)  		// 片选信号(OE)

#define OV7670_DATA   GPIOC->IDR&0x00FF  					//数据输入端口
//////////////////////////////////////////////////////////////////////////

#define OV_MAXBUF		320 * 240 * 2
extern const u32 ov_maxbuf;
typedef struct _OV_BUF
{
	u32 addr;
	u32 bsize;
	u8 sta;		// 可读?
} OV_BUF, *POV_BUF;

// 频率计数
extern u8 OV0_FPS;		// timer.c中定义
extern u8 FPS0Count;	// timer.c中定义
extern u8 USB_SPS;		// timer.c中定义
extern u8 SPSCount;		// timer.c中定义

// #0摄像头缓冲区
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
