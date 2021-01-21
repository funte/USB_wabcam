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


//����USBʱ��,USBclk=48Mhz
void Set_USBClock(void)
{
	RCC->CFGR &= ~(1 << 22); //USBclk=PLLclk/1.5=48Mhz	    
	RCC->APB1ENR |= 1 << 23; //USBʱ��ʹ��					 
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

//USB�ж�����
void USB_Interrupts_Config(void)
{
	EXTI->IMR |= 1 << 18;//  ������18�ϵ��ж�
	EXTI->RTSR |= 1 << 18;//line 18���¼��������ش���	 
	MY_NVIC_Init(1, 0, USB_LP_CAN1_RX0_IRQn, 2);//��2�����ȼ���֮ 
	MY_NVIC_Init(0, 0, USBWakeUp_IRQn, 2);     //��2�����ȼ����	 	 
}

extern u8 SPSCount;									// timer.c�ж���
#define CAMERA_SIZ_STREAMHD			2				// PAYLOAD Header Size
u32 sendsize;										// �ѷ����ֽ���
u8 packetbuf[PACKET_SIZE] = { 0x02, 0x01, 0x0 };	// ���ݰ�������
OV_BUF* povbuf;										// ���ڷ��͵�JPG������

void myMemcpy(const u8* src, u8* dst, u32 bsize)
{
	u32 i = 0;
	for (i = 0; i < bsize; ++i) dst[i] = src[i];
}

/*******************************************************************************
* Function Name  : UsbCamera_Fillbuf
* Description    : ׼�������͵���Ƶ��������
* Input          :
* Output         :
* Return         :
*******************************************************************************/
void UsbCamera_Fillbuf(void)
{
	u32 datalen = 0;		// ���η����ֽ���
	
	if (0 == sendsize)		// ͼ���װ�
	{
		if (ovbuf0.sta)
		{
			povbuf = &ovbuf0;
		} else if (ovbuf1.sta){
			povbuf = &ovbuf1;
		} else{
			return;
		}
		// ��ʼ��PayloadDataHeaderr
		packetbuf[1] &= 0x01;		// ���BFH
		packetbuf[1] ^= 0x01;		// �л�FID

		// ��ȡ��������
		datalen = PACKET_SIZE - CAMERA_SIZ_STREAMHD;
		FSMC_SRAM_ReadBuffer(packetbuf + CAMERA_SIZ_STREAMHD, povbuf->addr, datalen);

		sendsize = datalen;
		datalen += CAMERA_SIZ_STREAMHD;
	} else{					// ͼ�������
		datalen = PACKET_SIZE - CAMERA_SIZ_STREAMHD;
		// �ж��Ƿ�Ϊ���һ��
		if (sendsize + datalen > povbuf->bsize)
		{
			datalen = povbuf->bsize - sendsize;
			packetbuf[1] |= 0x02;		// EOF��λ
		}
		// ��ȡ��������
		FSMC_SRAM_ReadBuffer(packetbuf + CAMERA_SIZ_STREAMHD, povbuf->addr + sendsize, datalen);

		sendsize += datalen;
		datalen += CAMERA_SIZ_STREAMHD;
	}

	// �������ݵ�PMA
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
	_ToggleDTOG_TX(ENDP1);					// ��תDTOG_TX
	_SetEPTxStatus(ENDP1, EP_TX_VALID);		// �������ݷ���

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
