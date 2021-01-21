/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : hw_config.c
* Author             : MCD Application Team
* Version            : V2.2.0
* Date               : 06/13/2008
* Description        : Hardware Configuration & Setup
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "sys.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "platform_config.h"
#include "usb_pwr.h"
#include "lcd.h"
#include "ov7670.h"
#include "sram.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


//配置USB时钟,USBclk=48Mhz
void Set_USBClock(void)
{
	RCC->CFGR &= ~(1 << 22); //USBclk=PLLclk/1.5=48Mhz	    
	RCC->APB1ENR |= 1 << 23; //USB时钟使能					 
}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode.
* Description    : Power-off system clocks and power while entering suspend mode.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
	/* Set the device state to suspend */
	bDeviceState = SUSPENDED;
	/* Request to enter STOP mode with regulator in low power mode */
	//PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode.
* Description    : Restores system clocks and power while exiting suspend mode.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
	DEVICE_INFO *pInfo = &Device_Info;


	/* Set the device state to the correct state */
	if (pInfo->Current_Configuration != 0)
	{
		/* Device configured */
		bDeviceState = CONFIGURED;
	} else
	{
		bDeviceState = ATTACHED;
	}
}

//USB中断配置
void USB_Interrupts_Config(void)
{
	EXTI->IMR |= 1 << 18;//  开启线18上的中断
	EXTI->RTSR |= 1 << 18;//line 18上事件上升降沿触发	 
	MY_NVIC_Init(1, 0, USB_LP_CAN1_RX0_IRQn, 2);//组2，优先级次之 
	MY_NVIC_Init(0, 0, USBWakeUp_IRQn, 2);     //组2，优先级最高	 	 
}

extern u8 SPSCount;									// timer.c中定义
#define CAMERA_SIZ_STREAMHD			2				// PAYLOAD Header Size
u32 sendsize;										// 已发送字节数
u8 packetbuf[PACKET_SIZE] = { 0x02, 0x01, 0x0 };	// 数据包缓冲区
OV_BUF* povbuf;										// 正在发送的JPG缓冲区

void myMemcpy(const u8* src, u8* dst, u32 bsize)
{
	u32 i = 0;
	for (i = 0; i < bsize; ++i) dst[i] = src[i];
}

/*******************************************************************************
* Function Name  : UsbCamera_Fillbuf
* Description    : 准备待发送的视频流缓冲区
* Input          :
* Output         :
* Return         :
*******************************************************************************/
void UsbCamera_Fillbuf(void)
{
	u32 datalen = 0;		// 本次发送字节数
	
	if (0 == sendsize)		// 图像首包
	{
		if (ovbuf0.sta)
		{
			povbuf = &ovbuf0;
		} else if (ovbuf1.sta){
			povbuf = &ovbuf1;
		} else{
			return;
		}
		// 初始化PayloadDataHeaderr
		packetbuf[1] &= 0x01;		// 清除BFH
		packetbuf[1] ^= 0x01;		// 切换FID

		// 读取发送数据
		datalen = PACKET_SIZE - CAMERA_SIZ_STREAMHD;
		FSMC_SRAM_ReadBuffer(packetbuf + CAMERA_SIZ_STREAMHD, povbuf->addr, datalen);

		sendsize = datalen;
		datalen += CAMERA_SIZ_STREAMHD;
	} else{					// 图像后续包
		datalen = PACKET_SIZE - CAMERA_SIZ_STREAMHD;
		// 判断是否为最后一包
		if (sendsize + datalen > povbuf->bsize)
		{
			datalen = povbuf->bsize - sendsize;
			packetbuf[1] |= 0x02;		// EOF置位
		}
		// 读取发送数据
		FSMC_SRAM_ReadBuffer(packetbuf + CAMERA_SIZ_STREAMHD, povbuf->addr + sendsize, datalen);

		sendsize += datalen;
		datalen += CAMERA_SIZ_STREAMHD;
	}

	// 复制数据到PMA
	if (_GetENDPOINT(ENDP1) & EP_DTOG_TX)
	{
		// User use buffer0
		UserToPMABufferCopy(packetbuf, ENDP1_BUF0Addr, datalen);
		SetEPDblBuf0Count(ENDP1, EP_DBUF_IN, datalen);
	} else{
		// User use buffer1
		UserToPMABufferCopy(packetbuf, ENDP1_BUF1Addr, datalen);
		SetEPDblBuf1Count(ENDP1, EP_DBUF_IN, datalen);
	}
	_ToggleDTOG_TX(ENDP1);					// 反转DTOG_TX
	_SetEPTxStatus(ENDP1, EP_TX_VALID);		// 允许数据发送

	if (sendsize >= povbuf->bsize)
	{
		povbuf->sta = 0;
		++SPSCount;
		sendsize = 0;
	}

	return;
}

/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_SerialNum(void)
{
	u32 Device_Serial0, Device_Serial1, Device_Serial2;

	Device_Serial0 = *(u32*)(0x1FFFF7E8);
	Device_Serial1 = *(u32*)(0x1FFFF7EC);
	Device_Serial2 = *(u32*)(0x1FFFF7F0);

	if (Device_Serial0 != 0)
	{
		Camera_StringSerial[2] = (u8)(Device_Serial0 & 0x000000FF);
		Camera_StringSerial[4] = (u8)((Device_Serial0 & 0x0000FF00) >> 8);
		Camera_StringSerial[6] = (u8)((Device_Serial0 & 0x00FF0000) >> 16);
		Camera_StringSerial[8] = (u8)((Device_Serial0 & 0xFF000000) >> 24);

		Camera_StringSerial[10] = (u8)(Device_Serial1 & 0x000000FF);
		Camera_StringSerial[12] = (u8)((Device_Serial1 & 0x0000FF00) >> 8);
		Camera_StringSerial[14] = (u8)((Device_Serial1 & 0x00FF0000) >> 16);
		Camera_StringSerial[16] = (u8)((Device_Serial1 & 0xFF000000) >> 24);

		Camera_StringSerial[18] = (u8)(Device_Serial2 & 0x000000FF);
		Camera_StringSerial[20] = (u8)((Device_Serial2 & 0x0000FF00) >> 8);
		Camera_StringSerial[22] = (u8)((Device_Serial2 & 0x00FF0000) >> 16);
		Camera_StringSerial[24] = (u8)((Device_Serial2 & 0xFF000000) >> 24);
	}
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
