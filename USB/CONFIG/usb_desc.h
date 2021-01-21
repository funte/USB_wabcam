/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : usb_desc.h
* Author             : MCD Application Team
* Version            : V2.2.1
* Date               : 09/22/2008
* Description        : Descriptor Header for Virtual COM Port Device
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_DESC_H
#define __USB_DESC_H

/* Includes ------------------------------------------------------------------*/
#include "usb_type.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define   UNCOMPRESS        0

#define MAKE_WORD(x)        (u8)((x)&0xFF),(u8)(((x)>>8)&0xFF)
#define MAKE_DWORD(x)       (u8)((x)&0xFF),(u8)(((x)>>8)&0xFF),(u8)(((x)>>16)&0xFF),(u8)(((x)>>24)&0xFF)

/* Exported define -----------------------------------------------------------*/
//#define CAMERA_FORMAT_MJPEG							// 定义此宏使用JPEG视频流
#define CAMERA_FORMAT_UNCOMPRESSED						// 定义此宏使用未压缩的视频流

// Video Descriptor Types(描述符类型)
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01		// DEVICE
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02		// CONFIGRATION
#define USB_STRING_DESCRIPTOR_TYPE              0x03		// STRING
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04		// INTERFACE
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05		// EP
#define USB_ASSOCIATION_DESCRIPTOR_TYPE         0x0B		// IAD

// Video String Descriptor Size(字符描述符大小)
#define CAMERA_SIZ_DEVICE_DESC                  18
#define CAMERA_SIZ_STRING_LANGID                4
#define CAMERA_SIZ_STRING_VENDOR                14
#define CAMERA_SIZ_STRING_PRODUCT               26
#define CAMERA_SIZ_STRING_SERIAL                26

// Video Class-Specific VS Interface Descriptor Subtypes(视频流类型)
#define VS_FORMAT_UNCOMPRESSED					0x04
#define VS_FRAME_UNCOMPRESSED					0x05
#define VS_FORMAT_MJPEG							0x06

// YUV Format(GUID used to identify stream-encoding format, 16 Bytes)
#define YUV422									0x59,0x55,0x59,0x32,\
												0x00,0x00,\
												0x10,0x00,\
												0x80, 0x00,\
												0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
#define YUY2									YUV422		// GUID: 32595559-0000-0010-8000-00AA00389B71
#define YUV420									0x4e, 0x56, 0x31, 0x32,\
												0x00,0x00,\
												0x10, 0x00,\
												0x80,0x00,\
												0x00,0xaa,0x00,0x38,0x9b,0x71
#define NV12									YUV420		// GUID:3231564E-0000-0010-8000-00AA00389B71

#define YUV_BITS_PER_PIX						0x10

#ifdef CAMERA_FORMAT_MJPEG
// MJPEG视频流帧率,带宽控制(15帧/s)
#define IMG_WIDTH								640
#define IMG_HEIGHT								480
#define IMG_VIDEO_SCALE							2			// MJPEG size = IMG_WIDTH / IMG_VIDEO_SCALE
#define IMG_MJPG_FRAMERATE						15			// 预定义MJPEG视频帧率
#define MIN_BIT_RATE							(20*1024*IMG_MJPG_FRAMERATE)
#define MAX_BIT_RATE							(500*1024*IMG_MJPG_FRAMERATE)
#define MAX_FRAME_SIZE							(200*1024)	// 每帧最大字节数，对应Host要求的Buffer Size
#define FRAME_INTERVEL							(10000000ul/IMG_MJPG_FRAMERATE)     // 帧间间隔时间，单位100ns
 
#define PACKET_SIZE                             0xB0		// 176 
#endif

#ifdef CAMERA_FORMAT_UNCOMPRESSED
// UNCOMPRESSED视频流帧率,带宽控制(5帧/s)
#define IMG_WIDTH								640
#define IMG_HEIGHT								480
#define IMG_VIDEO_SCALE							2			// MJPEG size = IMG_WIDTH / IMG_VIDEO_SCALE
#define IMG_UNCOMP_FRAMERATE					15			// 预定义MJPEG视频帧率
#define MIN_BIT_RATE							(20*1024*IMG_UNCOMP_FRAMERATE)	// 每帧最小字节*帧率
#define MAX_BIT_RATE							(400*1024*IMG_UNCOMP_FRAMERATE)	// 每帧最大字节*帧率
#define MAX_FRAME_SIZE							(200*1024)	// 最大每帧字节数，对应Host要求的Buffer Size
#define FRAME_INTERVEL							(10000000ul/IMG_UNCOMP_FRAMERATE)     // 帧间间隔时间，单位100ns

#define PACKET_SIZE                             0xB0		// 176 
#endif

/* Exported functions ------------------------------------------------------- */
extern const u8 Camera_DeviceDescriptor[CAMERA_SIZ_DEVICE_DESC];
#ifdef CAMERA_FORMAT_MJPEG
#define CAMERA_SIZ_CONFIG_DESC					144
extern const u8 Camera_ConfigDescriptor[CAMERA_SIZ_CONFIG_DESC];		// MPEG视频流使用的配置描述符
#endif
#ifdef CAMERA_FORMAT_UNCOMPRESSED
#define CAMERA_SIZ_CONFIG_DESC					160
extern const u8 Camera_ConfigDescriptor[CAMERA_SIZ_CONFIG_DESC];		// UNCOMPRESSED视频流使用的描述符
#endif
extern const u8 Camera_StringLangID[CAMERA_SIZ_STRING_LANGID];
extern const u8 Camera_StringVendor[CAMERA_SIZ_STRING_VENDOR];
extern const u8 Camera_StringProduct[CAMERA_SIZ_STRING_PRODUCT];
extern u8 Camera_StringSerial[CAMERA_SIZ_STRING_SERIAL];

#endif /* __USB_DESC_H */
/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
