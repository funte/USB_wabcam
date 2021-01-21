/*============================================================================
//  File Name       : ejpeg.h
//  Author          : HECC. DuckWeed Tech.
//                  @HuiZhou, 2011
//  email:  hcc21cn@163.com	
//  Version         : V2.0
//  Description     : JPEG编码器
//  使用说明：
//      1).用JPG_initImgFormat()设定源图像尺寸
//         用JPG_initOutStream()设定编码器的数据流输出缓冲区和CallBack函数。建议
//      缓冲区不小于1K SRAM。
//         用JPG_WriteAppDat()设定附加数据，这是可选功能。
//      2).调用JPG_Encode()执行图像编码。返回结果数据ByteNum。中间自动调用
//      CallBack_OutStream写入最终存储介质。
==============================================================================*/
#ifndef __ejpeg_H_
#define __ejpeg_H_
//
//#include "stm32f10x_lib.h"
//#include "stm32f10x_type.h"

typedef signed long  Int32;
typedef signed short Int16;
typedef signed char  Int8;

typedef unsigned long  UInt32;
typedef unsigned short UInt16;
typedef unsigned char  UInt8;

//---------------------------------------------------------------------------
//  编译控制
#define JPG_USING_APP0

//--------------------------------------------------------------------------
#define	JPG_IMGFMT_MONO		    0
#define JPG_IMGFMT_YUV422       1
#define JPG_IMGFMT_YUV420       2
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
typedef UInt8* (*CallBack_OutStream)(UInt8 *bufptr, Int32 byteNum);

//---------------------------------------------------------------------------

    //质量因子quality_factor范围[1,8]，1最好，8最差 
void JPG_initImgFormat(UInt16 width, UInt16 height, UInt8 scale, UInt8 img_format, 
                UInt8 quality, UInt16 pitch, UInt16 xbeg, UInt16 ybeg);
void JPG_initOutStream(CallBack_OutStream streamFunc, UInt8 *outbuf, Int32 bufsize);
UInt8 JPG_WriteAppDat(UInt8 *in_dat, UInt16 size);
UInt8 *JPG_WriteHeader(UInt16 jpghd_offset);
Int32 JPG_Encode(void);


#endif
