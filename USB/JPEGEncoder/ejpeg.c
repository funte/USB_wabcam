//---------------------------------------------------------------------------
//
//	JPEG Encoder for embeded application
//		By 	HECC. HuiZhou 	2010./2011
//  //PC机上仿真，移植到DM642 DSP上的版本记录：
//	V1.0:	#只支持单色和YUV422格式
//			#未处理块延拓问题，只支持MCU块大小N倍长宽的图像, 要求width/heigh
//			 单色Align-8, YUV422 Align-16
//			#写入JPEG Header优化
//	V1.1:	移植到DM642， 对DSP优化。
//	V1.11:  再次整理, 减小MCU buffer，增加APP 段插入功能
//  -------------------------------
//  V2.0:   2011-10-31, 移植到STM32成功。
//      测试结果：使用4KByte输出Buffer，数据流输出到FSMC扩展SDRAM，640x480图像，          
//          用ARM Level-3 for Time编译设置，编码时间约880ms; 640x480单色图像，编码
//          时间约530ms。无优化的Level-0编译，时间约1120ms/630ms, 优化作用一般。
//  V2.01:  2011-11-10, 增加缩放取样功能，输出接口改成可匹配双缓冲区接口。
//  v2.03:  2011-12-10, 增加APP0段，这对MJPEG是很重要的! 增加YUV420模式，去掉Mono
//      模式单独的ReadSrc，与YUV合成一个函数
//---------------------------------------------------------------------------
//------------------------------------
//#define		APP_DSP_CCS
//------------------------------------
//#include <stdio.h>
#include "ejpeg.h"
//#include "Pld_Intf.h"

#define JPG_STATIC_LOC      static
//============================================================================
//	quant_table
JPG_STATIC_LOC UInt8 std_Y_QT[64] =
{
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

JPG_STATIC_LOC UInt8 std_UV_QT[64] =
{
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

JPG_STATIC_LOC UInt8 zigzag_table[64] =
{
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};

JPG_STATIC_LOC UInt8 un_zigzag_table[64] =
{
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};


//送入JPEG流的huffman表数据, 按JPEG规定格式，带有FFC4标记和长度，4个Huffman表.
//UInt8 huff_markerdata[432] =
JPG_STATIC_LOC UInt8 huff_Y_marker[216] =
{
	0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
	0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04,
	0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03,
	0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
	0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1,
	0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A,
	0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34,
	0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64,
	0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93,
	0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
	0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
	0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3,
	0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5,
	0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA
};

JPG_STATIC_LOC UInt8 huff_UV_marker[216] =
{
	0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
	0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	0xFF, 0xC4, 0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03,
	0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02,
	0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
	0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1,
	0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24,
	0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29,
	0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
	0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4,
	0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
	0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
	0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4,
	0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA,
};

//huffman表
JPG_STATIC_LOC UInt16 luminance_dc_code_table[12] =
{
	0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
	0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
};

JPG_STATIC_LOC UInt8 luminance_dc_size_table[12] =
{
	0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
};

JPG_STATIC_LOC UInt16 chrominance_dc_code_table[12] =
{
	0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
	0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
};

JPG_STATIC_LOC UInt8 chrominance_dc_size_table[12] =
{
	0x02, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
};

//JPEG标准预设的AC_Huffman表
//	10个为一组，1组对应1个0游程值; 每组16个值，对应非零系数bit数1~16。首尾2个为特殊码
//UInt16 luminance_ac_code_table[] =
JPG_STATIC_LOC UInt16 luminance_ac_code_table[162] =
{
	0x000A,
	0x0000, 0x0001, 0x0004, 0x000B, 0x001A, 0x0078, 0x00F8, 0x03F6, 0xFF82, 0xFF83,
	0x000C, 0x001B, 0x0079, 0x01F6, 0x07F6, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88,
	0x001C, 0x00F9, 0x03F7, 0x0FF4, 0xFF89, 0xFF8A, 0xFF8b, 0xFF8C, 0xFF8D, 0xFF8E,
	0x003A, 0x01F7, 0x0FF5, 0xFF8F, 0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95,
	0x003B, 0x03F8, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D,
	0x007A, 0x07F7, 0xFF9E, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5,
	0x007B, 0x0FF6, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD,
	0x00FA, 0x0FF7, 0xFFAE, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5,
	0x01F8, 0x7FC0, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD,
	0x01F9, 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6,
	0x01FA, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
	0x03F9, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8,
	0x03FA, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1,
	0x07F8, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA,
	0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4,
	0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
	0x07F9
};

JPG_STATIC_LOC UInt8 luminance_ac_size_table[162] =
{
	0x04,
	0x02, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x0A, 0x10, 0x10,
	0x04, 0x05, 0x07, 0x09, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x05, 0x08, 0x0A, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x06, 0x09, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x06, 0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x07, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x07, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x08, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0B
};

JPG_STATIC_LOC  UInt16 chrominance_ac_code_table[162] =
{
	0x0000,
	0x0001, 0x0004, 0x000A, 0x0018, 0x0019, 0x0038, 0x0078, 0x01F4, 0x03F6, 0x0FF4,
	0x000B, 0x0039, 0x00F6, 0x01F5, 0x07F6, 0x0FF5, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B,
	0x001A, 0x00F7, 0x03F7, 0x0FF6, 0x7FC2, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F, 0xFF90,
	0x001B, 0x00F8, 0x03F8, 0x0FF7, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96,
	0x003A, 0x01F6, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E,
	0x003B, 0x03F9, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6,
	0x0079, 0x07F7, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE,
	0x007A, 0x07F8, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6,
	0x00F9, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
	0x01F7, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8,
	0x01F8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF, 0xFFD0, 0xFFD1,
	0x01F9, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA,
	0x01FA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3,
	0x07F9, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEb, 0xFFEC,
	0x3FE0, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5,
	0x7FC3, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
	0x03FA
};

JPG_STATIC_LOC UInt8 chrominance_ac_size_table[162] =
{
	0x02,
	0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x09, 0x0A, 0x0C,
	0x04, 0x06, 0x08, 0x09, 0x0B, 0x0C, 0x10, 0x10, 0x10, 0x10,
	0x05, 0x08, 0x0A, 0x0C, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x05, 0x08, 0x0A, 0x0C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x06, 0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x06, 0x0A, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x07, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x07, 0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x09, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0B, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0E, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x0A
};

//----------------------------------------------------------------
typedef struct tagJPGENC_STRUCT
{
	UInt16	mcu_width;
	UInt16	mcu_height;
	UInt16	hori_mcus;
	UInt16	vert_mcus;
	UInt8 	quality_factor;
	UInt8	img_format;

	UInt16	img_width;
	UInt16	img_height;
	UInt16  img_pitch;
	UInt16  img_xbeg;
	UInt16  img_ybeg;
	UInt16  img_scale;

	Int16	last_dc1;
	Int16	last_dc2;
	Int16	last_dc3;
} JPGENC_STRUCT;

JPGENC_STRUCT jpgenc_struct;
JPGENC_STRUCT *jpgenc;

UInt32 jpg_bitsbuf;				//bits to stream buffer
UInt16 jpg_bitindex;

UInt8 app_Y_QT[64];				//输出到Stream的量化表
UInt8 app_UV_QT[64];
UInt16 inv_Y_QT[64];			//快速量化用的倒数量化表
UInt16 inv_UV_QT[64];

Int16 jpg_mcubuff[64 * 6];			//MCU数据buffer，Src/DCT/QT共用
UInt8 *jpg_outstream;			//JPEG数据流输出指针
UInt8 *jpg_streamfptr = 0;		//为跳过JPEG Header，保存格式未改前图像流起始指针

Int32 jpg_outbufsize;
//CallBack函数
CallBack_OutStream jpg_FlushStream = 0;

#define JPG_APPDAT_BUFSIZE		64
UInt8 jpg_appdat_buf[JPG_APPDAT_BUFSIZE];


//---------------------------------------------------------------------------
//	整数FDCT，IN/OUT 11bit
//  结果覆盖源数据
void DCT(Int16 *data)
{
	UInt16 i;
	Int32 x0, x1, x2, x3, x4, x5, x6, x7, x8;

	//Cos系数表，按cos(i*PI/16)*sqrt(2)计算，i=0,1..7, 乘1024 Scale,即Q12, c0=0,c4=1
	static const UInt16 c1 = 1420;    // cos PI/16 * root(2) *1024
	static const UInt16 c2 = 1338;    // cos PI/8 * root(2)
	static const UInt16 c3 = 1204;    // cos 3PI/16 * root(2)
	static const UInt16 c5 = 805;     // cos 5PI/16 * root(2)
	static const UInt16 c6 = 554;     // cos 3PI/8 * root(2)
	static const UInt16 c7 = 283;     // cos 7PI/16 * root(2)
	static const UInt16 s1 = 3;
	static const UInt16 s2 = 10;
	static const UInt16 s3 = 13;

	for (i = 8; i > 0; i--)
	{
		x8 = data[0] + data[7];
		x0 = data[0] - data[7];
		x7 = data[1] + data[6];
		x1 = data[1] - data[6];
		x6 = data[2] + data[5];
		x2 = data[2] - data[5];
		x5 = data[3] + data[4];
		x3 = data[3] - data[4];

		x4 = x8 + x5;
		x8 = x8 - x5;
		x5 = x7 + x6;
		x7 = x7 - x6;

		data[0] = (Int16)(x4 + x5);
		data[4] = (Int16)(x4 - x5);
		data[2] = (Int16)((x8*c2 + x7*c6) >> s2);
		data[6] = (Int16)((x8*c6 - x7*c2) >> s2);
		data[7] = (Int16)((x0*c7 - x1*c5 + x2*c3 - x3*c1) >> s2);
		data[5] = (Int16)((x0*c5 - x1*c1 + x2*c7 + x3*c3) >> s2);
		data[3] = (Int16)((x0*c3 - x1*c7 - x2*c1 - x3*c5) >> s2);
		data[1] = (Int16)((x0*c1 + x1*c3 + x2*c5 + x3*c7) >> s2);

		data += 8;
	}

	data -= 64;
	for (i = 8; i > 0; i--)
	{
		x8 = data[0] + data[56];
		x0 = data[0] - data[56];
		x7 = data[8] + data[48];
		x1 = data[8] - data[48];
		x6 = data[16] + data[40];
		x2 = data[16] - data[40];
		x5 = data[24] + data[32];
		x3 = data[24] - data[32];

		x4 = x8 + x5;
		x8 = x8 - x5;
		x5 = x7 + x6;
		x7 = x7 - x6;

		data[0] = (Int16)((x4 + x5) >> s1);
		data[32] = (Int16)((x4 - x5) >> s1);
		data[16] = (Int16)((x8*c2 + x7*c6) >> s3);
		data[48] = (Int16)((x8*c6 - x7*c2) >> s3);

		data[56] = (Int16)((x0*c7 - x1*c5 + x2*c3 - x3*c1) >> s3);
		data[40] = (Int16)((x0*c5 - x1*c1 + x2*c7 + x3*c3) >> s3);
		data[24] = (Int16)((x0*c3 - x1*c7 - x2*c1 - x3*c5) >> s3);
		data[8] = (Int16)((x0*c1 + x1*c3 + x2*c5 + x3*c7) >> s3);

		data++;
	}
}

#if 0
UInt32 outptr_upmask;
#define	M_JPG_writebits {outstr_ptr = JPG_writebits(CodeVal, bitnum, outstr_ptr);    }
//---------------------------------------------------------------------------
//	输出Huffman编码数据流, pointer in ->pointer out
UInt8 *JPG_writebits(UInt32 val, UInt16 bitnum, UInt8 *outstr_ptr)
{
	Int16 bits_fornext;

	bits_fornext = (Int16)(jpg_bitindex + bitnum - 32);
	if (bits_fornext <0)
	{
		jpg_bitsbuf = (jpg_bitsbuf<<bitnum) | val;  	//未溢出，简单并入Buf中
		jpg_bitindex += bitnum;
	}
	else
	{
		jpg_bitsbuf = (jpg_bitsbuf << (32 - jpg_bitindex)) |(val>> bits_fornext);
		//32bit Word按Byte输出， 按编码规定，遇到0xFF迦?x00
		if ((*outstr_ptr++ = (UInt8)(jpg_bitsbuf >>24)) == 0xff)
		{
			(UInt32)outstr_ptr &= outptr_upmask;
			*outstr_ptr++ = 0;
		}
		if ((*outstr_ptr++ = (UInt8)(jpg_bitsbuf >>16)) == 0xff)
			*outstr_ptr++ = 0;
		if ((*outstr_ptr++ = (UInt8)(jpg_bitsbuf >>8)) == 0xff)
			*outstr_ptr++ = 0;
		if ((*outstr_ptr++ = (UInt8)jpg_bitsbuf) == 0xff)
			*outstr_ptr++ = 0;
		//剩余bits
		jpg_bitsbuf = val;
		jpg_bitindex = bits_fornext;
	}
	return(outstr_ptr);
}
#endif

//#if 0
#define	M_JPG_writebits												\
{																	\
    bits_fornext = (Int16)(jpg_bitindex + bitnum - 32);				\
    if (bits_fornext <0)    										\
	    {																\
    	jpg_bitsbuf = (jpg_bitsbuf<<bitnum) | CodeVal; 				\
		jpg_bitindex += bitnum;										\
		}    															\
		else															\
	{																\
        jpg_bitsbuf = (jpg_bitsbuf << (32 - jpg_bitindex)) |(CodeVal>> bits_fornext);	\
        if ((*outstr_ptr++ = (UInt8)(jpg_bitsbuf >>24)) == 0xff)	\
            *outstr_ptr++ = 0;										\
        if ((*outstr_ptr++ = (UInt8)(jpg_bitsbuf >>16)) == 0xff)	\
            *outstr_ptr++ = 0;										\
        if ((*outstr_ptr++ = (UInt8)(jpg_bitsbuf >>8)) == 0xff)		\
            *outstr_ptr++ = 0;										\
        if ((*outstr_ptr++ = (UInt8)jpg_bitsbuf) == 0xff)			\
            *outstr_ptr++ = 0;										\
        jpg_bitsbuf = CodeVal;										\
        jpg_bitindex = bits_fornext;								\
    }																\
}
//#endif

//---------------------------------------------------------------------------
//	对单个块(MCU)的量化结果进行huffman编码输出
//  输入：in_dat - 量化后的MCU数据
//        component - MCU类型标记，1-Y, 2-Cb, 3-Cr
//        outstr_ptr - 输出Buffer  
UInt8 *JPG_huffman(Int16 *in_dat, UInt8 component, UInt8 *outstr_ptr)
{
	UInt16 i;
	UInt16 *pDcCodeTable, *pAcCodeTable;
	UInt8 *pDcSizeTable, *pAcSizeTable;
	UInt8 *un_zigzag_ptr;
	Int16 Coeff, LastDc;
	UInt16 AbsCoeff, HuffCode, HuffSize, RunLength, DataSize, index, bitnum;
	UInt32 CodeVal; 	//need 32bit!
	Int16 bits_fornext;

	//选择huffman表
	Coeff = in_dat[0];        	//取DC值
	if (component == 1)
	{
		pDcCodeTable = (UInt16 *)luminance_dc_code_table;
		pDcSizeTable = (UInt8 *)luminance_dc_size_table;
		pAcCodeTable = (UInt16 *)luminance_ac_code_table;
		pAcSizeTable = (UInt8 *)luminance_ac_size_table;

		LastDc = jpgenc->last_dc1;		//保存DC值
		jpgenc->last_dc1 = Coeff;
	} else
	{
		pDcCodeTable = (UInt16 *)chrominance_dc_code_table;
		pDcSizeTable = (UInt8 *)chrominance_dc_size_table;
		pAcCodeTable = (UInt16 *)chrominance_ac_code_table;
		pAcSizeTable = (UInt8 *)chrominance_ac_size_table;
		if (component == 2)
		{
			LastDc = jpgenc->last_dc2;
			jpgenc->last_dc2 = Coeff;
		} else
		{
			LastDc = jpgenc->last_dc3;
			jpgenc->last_dc3 = Coeff;
		}
	}
	//DC系数编码: DPCM系数编码
	//取差分值
	Coeff -= LastDc;
	AbsCoeff = (Coeff < 0) ? -Coeff : Coeff;
	//计算系数绝对值的有效bit数
	DataSize = 0;
	while (AbsCoeff != 0)			//log2, 系数绝对值的有效bit数
	{
		AbsCoeff >>= 1;
		DataSize++;
	}
	//符号1:用huffman编码表示的"系数绝对值有效bit数"
	HuffCode = pDcCodeTable[DataSize];
	HuffSize = pDcSizeTable[DataSize];
	//符号2: 负值取反码,屏蔽冗余位
	if (Coeff < 0)
	{
		Coeff--;
		Coeff &= ((1 << DataSize) - 1);
	}
	//组合2个符号一起输出
	CodeVal = (HuffCode << DataSize) | Coeff;
	bitnum = HuffSize + DataSize;
	M_JPG_writebits;

	//AC系数编码： 零游程-系数编码
	RunLength = 0;
	un_zigzag_ptr = (UInt8 *)un_zigzag_table;
	for (i = 63; i > 0; i--)				//63个AC系数
	{
		un_zigzag_ptr++;
		Coeff = in_dat[(*un_zigzag_ptr)];
		if (Coeff != 0)
		{
			//遇到非0系数
			while (RunLength > 15)
			{
				//0游程长度达到16，输出一个跳码[16,0]
				RunLength -= 16;
				CodeVal = pAcCodeTable[161];
				bitnum = pAcSizeTable[161];
				M_JPG_writebits;
			}
			//系数的有效bit数
			AbsCoeff = (Coeff < 0) ? -Coeff : Coeff;

			DataSize = 0;
			while (AbsCoeff != 0)			//log2, 系数绝对值的有效bit数
			{
				AbsCoeff >>= 1;
				DataSize++;
			}
			//符号1: (到达非0系数前的)0游程长度x10 + 非0系数有效bit数
			//	符号可记为[RL, Bitnum]形式, RL范围0~15，系数值由于DCT限制
			//	1024范围，bitnum<10。
			index = RunLength * 10 + DataSize;
			HuffCode = pAcCodeTable[index];
			HuffSize = pAcSizeTable[index];
			//符号2: 负值取反码,屏蔽冗余位
			if (Coeff < 0)
			{
				Coeff--;
				Coeff &= (1 << DataSize) - 1;
			}
			//组合2个符号输出
			CodeVal = (HuffCode << DataSize) | Coeff;
			bitnum = HuffSize + DataSize;
			M_JPG_writebits;

			RunLength = 0;
		} else
		{
			RunLength++;		//0值，进行游程统计
		}
	}
	//输出块结束标记
	if (RunLength != 0)
	{
		//符号[0,0] EOB，后续全0，作为块结束标记
		CodeVal = pAcCodeTable[0];
		bitnum = pAcSizeTable[0];
		M_JPG_writebits;
	}
	return(outstr_ptr);
}

//---------------------------------------------------------------------------
//	对DCT系数量化. 量化结果覆盖源数据
void JPG_quantization(Int16*in_dat, UInt16* quant_table_ptr)
{
	Int16 i;
	Int32 f0, f1, f2, f3, f4, f5, f6, f7;

	for (i = 8; i > 0; i--)
	{
		//量化的方法是: value = data/QT
		//变除法为乘法: 使用倒数形式的量化表，INV = 0x8000/QT，这样
		//	value = data/QT = (v*INV)/0x8000
		//	加0x4000相当与加0.5
		f0 = in_dat[0] * quant_table_ptr[0] + 0x4000;
		f1 = in_dat[1] * quant_table_ptr[1] + 0x4000;
		f2 = in_dat[2] * quant_table_ptr[2] + 0x4000;
		f3 = in_dat[3] * quant_table_ptr[3] + 0x4000;
		f4 = in_dat[4] * quant_table_ptr[4] + 0x4000;
		f5 = in_dat[5] * quant_table_ptr[5] + 0x4000;
		f6 = in_dat[6] * quant_table_ptr[6] + 0x4000;
		f7 = in_dat[7] * quant_table_ptr[7] + 0x4000;

		in_dat[0] = (Int16)(f0 >> 15);
		in_dat[1] = (Int16)(f1 >> 15);
		in_dat[2] = (Int16)(f2 >> 15);
		in_dat[3] = (Int16)(f3 >> 15);
		in_dat[4] = (Int16)(f4 >> 15);
		in_dat[5] = (Int16)(f5 >> 15);
		in_dat[6] = (Int16)(f6 >> 15);
		in_dat[7] = (Int16)(f7 >> 15);
		in_dat += 8;
		quant_table_ptr += 8;
	}
}

//---------------------------------------------------------------------------
//	根据质量因子调整量化表，并产生快速量化用的倒数量化表
void JPG_setquality(UInt8 quality_factor)
{
	UInt16 i, index;
	UInt32 value, quality1;

	if (quality_factor < 1)
		quality_factor = 1;
	if (quality_factor > 8)
		quality_factor = 8;
	quality1 = quality_factor;
	//
	quality1 = ((quality1 * 3) - 2) * 128; //converts range[1:8] to [1:22]
	for (i = 0; i < 64; i++)
	{
		index = zigzag_table[i];
		// luminance quantization table * quality factor *
		//按 v = (QT*Zoom +512)/1024 转换，Zoom = factor*128 ->1024范围, 即相当于
		//	v = k*QT + 0.5, k在1左右
		value = quality1*std_Y_QT[i];
		value = (value + 0x200) >> 10;
		if (value < 2)
			value = 2;
		else if (value > 255)
			value = 255;
		//保存新量化表用于输出。
		app_Y_QT[index] = (UInt8)value;
		//产生倒数形式的量化表，INV = 0x8000/QT， 量化时 Q = v/QT = (v*INV)/0x8000
		//变除法为乘法
		inv_Y_QT[i] = 0x8000 / value;

		// chrominance quantization table * quality factor
		value = quality1*std_UV_QT[i];
		value = (value + 0x200) >> 10;
		if (value < 2)
			value = 2;
		else if (value > 255)
			value = 255;

		app_UV_QT[index] = (UInt8)value;
		inv_UV_QT[i] = 0x8000 / value;
	}
}

//---------------------------------------------------------------------------
//	写标准JPEG文? 按规定输出量化表和Huffman表
UInt8 *JPG_writeheader(UInt8 *outbuf)
{
	UInt16 i, header_length;
	UInt8 number_of_components;
	UInt8 *outstr_ptr;

	outstr_ptr = outbuf;
	// Start of image marker (SOI)标准文件头
	*outstr_ptr++ = 0xFF;
	*outstr_ptr++ = 0xD8;			//marker
	//---
	//	APPx --0xFFEx
	//---
#ifdef JPG_USING_APP0
	//Default APP0段，可选.
	//  JFIF标记，后续属性字节不能错。MJPEG用"AVI1"代替JFIF也可以，后续全0。
	*outstr_ptr++ = 0xFF;
	*outstr_ptr++ = 0xE0;			//marker, standard JFIF 
	*outstr_ptr++ = 0x00;
	*outstr_ptr++ = 0x10;			//length, 10byte
	/*    *outstr_ptr++ = 'A';            //JFIF mark, 5byte
		*outstr_ptr++ = 'V';
		*outstr_ptr++ = 'I';
		*outstr_ptr++ = '1';
		for(i=0; i<10; i++)
		*outstr_ptr++ = 0x00; */
	*outstr_ptr++ = 'J';            //JFIF mark, 5byte
	*outstr_ptr++ = 'F';
	*outstr_ptr++ = 'I';
	*outstr_ptr++ = 'F';
	*outstr_ptr++ = 0x00;
	*outstr_ptr++ = 0x01;           //Version, 2Byte, 1.1 or 1.2
	*outstr_ptr++ = 0x01;
	*outstr_ptr++ = 0x00;           //units, none=0, pixel/inch=1, pixle/cm=2
	*outstr_ptr++ = 0x00;           //X density
	*outstr_ptr++ = 0x01;
	*outstr_ptr++ = 0x00;           //Y density
	*outstr_ptr++ = 0x01;
	*outstr_ptr++ = 0x00;           //缩略图X Pixels
	*outstr_ptr++ = 0x00;           //缩略图Y Pixels 
#endif        

	//其他附加APP段
	/*#
		*outstr_ptr++ = 0xFF;
		*outstr_ptr++ = 0xE2;			//marker

		*outstr_ptr++ = 0x00;			//length, 2byte
		*outstr_ptr++ = JPG_APPDAT_BUFSIZE;		//0x40, 64Byte
		for(i=0; i<JPG_APPDAT_BUFSIZE-2; i++)
		{
		*outstr_ptr++ = jpg_appdat_buf[i];
		}
		*/
	//------------------
	// Start of frame marker (SOF) Frame头
	*outstr_ptr++ = 0xFF;
	*outstr_ptr++ = 0xC0;			//Baseline DCT frame

	if (jpgenc->img_format == JPG_IMGFMT_MONO)
		number_of_components = 1;
	else
		number_of_components = 3;
	header_length = (UInt16)(8 + 3 * number_of_components);
	// Frame header length
	*outstr_ptr++ = (UInt8)(header_length >> 8);
	*outstr_ptr++ = (UInt8)header_length;
	// Precision (P)
	*outstr_ptr++ = 0x08;
	// image height
	*outstr_ptr++ = (UInt8)((jpgenc->img_height / jpgenc->img_scale) >> 8);
	*outstr_ptr++ = (UInt8)(jpgenc->img_height / jpgenc->img_scale);
	// image width
	*outstr_ptr++ = (UInt8)((jpgenc->img_width / jpgenc->img_scale) >> 8);
	*outstr_ptr++ = (UInt8)(jpgenc->img_width / jpgenc->img_scale);
	// number of color component
	*outstr_ptr++ = number_of_components;
	// components feature
	if (jpgenc->img_format == JPG_IMGFMT_MONO)
	{
		*outstr_ptr++ = 0x01;		//component ID
		*outstr_ptr++ = 0x11;		//vertical & horitzontal sample factor
		*outstr_ptr++ = 0x00;		//quality-table ID
	} else
	{
		*outstr_ptr++ = 0x01;    	//for Y
		if (jpgenc->img_format == JPG_IMGFMT_YUV422)
			*outstr_ptr++ = 0x21;
		if (jpgenc->img_format == JPG_IMGFMT_YUV420)
			*outstr_ptr++ = 0x22;
		else
			*outstr_ptr++ = 0x11;       //YUV444
		*outstr_ptr++ = 0x00;

		*outstr_ptr++ = 0x02;		//for Cb
		*outstr_ptr++ = 0x11;
		*outstr_ptr++ = 0x01;

		*outstr_ptr++ = 0x03;		//for Cr
		*outstr_ptr++ = 0x11;
		*outstr_ptr++ = 0x01;
	}

	//------------------
	// Quantization table for Y
	*outstr_ptr++ = 0xFF;
	*outstr_ptr++ = 0xDB;
	// Quantization table length
	*outstr_ptr++ = 0x00;
	*outstr_ptr++ = 0x43;		//67 = 2+1+64
	// [Pq, Tq], Pq-数据精度,0-8Bit,1-16Bit; Tq - 量化表编号(0-1)
	*outstr_ptr++ = 0x00;
	// applicate luminance quality table
	for (i = 0; i < 64; i++)
		*outstr_ptr++ = app_Y_QT[i];

	// Quantization table for UV
	if (jpgenc->img_format != JPG_IMGFMT_MONO)
	{
		*outstr_ptr++ = 0xFF;
		*outstr_ptr++ = 0xDB;
		// Quantization table length
		*outstr_ptr++ = 0x00;
		*outstr_ptr++ = 0x43;
		// Pq, Tq
		*outstr_ptr++ = 0x01;
		// applicate chrominance quality table
		for (i = 0; i < 64; i++)
			*outstr_ptr++ = app_UV_QT[i];
	}

	//------------------
	// huffman table(DHT), 4个huffman表，直接取已编制好的数据
	for (i = 0; i < sizeof(huff_Y_marker); i++)
	{
		*outstr_ptr++ = huff_Y_marker[i];
	}
	if (jpgenc->img_format != JPG_IMGFMT_MONO)
	{
		for (i = 0; i < sizeof(huff_UV_marker); i++)
		{
			*outstr_ptr++ = huff_UV_marker[i];
		}
	}

	//------------------
	// Scan header(SOF)
	*outstr_ptr++ = 0xFF;
	*outstr_ptr++ = 0xDA;		// Start of scan marker

	header_length = (UInt16)(6 + (number_of_components << 1));
	// Scan header length
	*outstr_ptr++ = (UInt8)(header_length >> 8);
	*outstr_ptr++ = (UInt8)header_length;
	// number of color component
	*outstr_ptr++ = number_of_components;
	// component feature
	if (jpgenc->img_format == JPG_IMGFMT_MONO)
	{
		*outstr_ptr++ = 0x01;	//component ID, for Y
		*outstr_ptr++ = 0x00;	//[TdNs, TaNs]，4bit分别为DC/AC量化表的编号
	} else
	{
		*outstr_ptr++ = 0x01;
		*outstr_ptr++ = 0x00;

		*outstr_ptr++ = 0x02;	//for Cb
		*outstr_ptr++ = 0x11;

		*outstr_ptr++ = 0x03;	//for Cr
		*outstr_ptr++ = 0x11;
	}
	*outstr_ptr++ = 0x00;		//Ss
	*outstr_ptr++ = 0x3F;		//Se
	*outstr_ptr++ = 0x00;		//[Ah,Al]
	//
	return(outstr_ptr);
}

//---------------------------------------------------------------------------
//	结束输出数据流, 写文件尾
UInt8 *close_bitstream(UInt8 *outstr_ptr)
{
	UInt16 i, count;
	UInt8 *ptr;

	if (jpg_bitindex > 0)
	{
		jpg_bitsbuf <<= (32 - jpg_bitindex);

		count = (jpg_bitindex + 7) >> 3;	//最少Byte数, 不一定是4Byte
		ptr = (UInt8 *)&jpg_bitsbuf + 3;	//little endian模式，指向Uint32的最高Byte
		for (i = 0; i < count; i++)
		{
			if ((*outstr_ptr++ = *ptr--) == 0xff)
				*outstr_ptr++ = 0;
		}
	}
	// End of image marker
	*outstr_ptr++ = 0xFF;
	*outstr_ptr++ = 0xD9;
	return(outstr_ptr);
}

#if 0
//#old
//---------------------------------------------------------------------------
//	读源图像数据
//		单色or YUV422, 读1个MCU,64Word，
void JPG_readsrc(UInt8 *in_ptr, UInt32 offset, Int16* out_ptr, UInt32 img_pitch)
{
	Int32 i;
	UInt32 f0,f1,f2,f3,f4,f5,f6,f7;
	UInt8 *inptr;

	inptr = in_ptr + offset;
	for(i=8; i>0; i--)
	{
		f0 = inptr[0];
		f1 = inptr[1];
		f2 = inptr[2];
		f3 = inptr[3];
		f4 = inptr[4];
		f5 = inptr[5];
		f6 = inptr[6];
		f7 = inptr[7];
		out_ptr[0] = f0 - 128;
		out_ptr[1] = f1 - 128;
		out_ptr[2] = f2 - 128;
		out_ptr[3] = f3 - 128;
		out_ptr[4] = f4 - 128;
		out_ptr[5] = f5 - 128;
		out_ptr[6] = f6 - 128;
		out_ptr[7] = f7 - 128; 

		inptr += img_pitch;
		out_ptr += 8;
	}
}
#endif

//---------------------------------------------------------------------------
//	[Private]读源图像数据
//  yuv_mode: 0 - Mono, 1 - YUV420, 2 - YUV422
void JPG_readsrc_yuv(UInt32 xoffset, UInt32 yoffset, UInt32 yuv_mode, Int16 *mcubuf)
{
	UInt16 buf[16];
	Int16 *y1_ptr, *y2_ptr, *cb_ptr, *cr_ptr;
	Int32 i, j;
	UInt32 step_x, rownum;
	UInt16 *ex_ptr, *wordptr;
	UInt8 *byteptr;

	y1_ptr = mcubuf;
	y2_ptr = mcubuf + 64;
	if (yuv_mode == 1)
	{
		cb_ptr = y2_ptr + 3 * 64;
		rownum = 16;
	} else
	{
		cb_ptr = y2_ptr + 64;
		rownum = 8;
	}
	cr_ptr = cb_ptr + 64;

	step_x = jpgenc->img_scale * 2 - 1;
	for (i = 0; i < rownum; ++i)
	{
		//Pld_SelectImgRow((UInt32)(yoffset + (jpgenc->img_ybeg + i)*jpgenc->img_scale));
		//ex_ptr = (UInt16 *)Pld_PixelPtr((UInt32)(xoffset + jpgenc->img_xbeg*jpgenc->img_scale));
		//水平采样模式，相邻取2点，保持CbCr一致性，否则均匀取样会出现颜色偏差
		j = 0;
		while (j < 16)
		{
			buf[j++] = *ex_ptr++;
			buf[j++] = *ex_ptr;
			ex_ptr += step_x;
		}

		if ((yuv_mode == 0) || ((yuv_mode == 1) && ((i % 2) != 0)))
		{
			//只取亮度
			wordptr = buf;

			*y1_ptr++ = (UInt8)(*wordptr++) - 128;      //0
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;      //3
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;      //4
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;
			*y1_ptr++ = (UInt8)(*wordptr++) - 128;      //7

			*y2_ptr++ = (UInt8)(*wordptr++) - 128;      //0
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;      //3
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;      //4
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;
			*y2_ptr++ = (UInt8)(*wordptr++) - 128;      //7
			//构造Y3，Y4 MCU
			if ((yuv_mode == 1) && (i == 7))
			{
				y1_ptr += 64;
				y2_ptr += 64;
			}
		} else
		{
			byteptr = (UInt8 *)buf;
			//MCU-1
			*y1_ptr++ = *byteptr++ - 128;       //0   
			*cb_ptr++ = *byteptr++ - 128;
			*y1_ptr++ = *byteptr++ - 128;       //1 
			*cr_ptr++ = *byteptr++ - 128;
			*y1_ptr++ = *byteptr++ - 128;       //2   
			*cb_ptr++ = *byteptr++ - 128;
			*y1_ptr++ = *byteptr++ - 128;       //3 
			*cr_ptr++ = *byteptr++ - 128;

			*y1_ptr++ = *byteptr++ - 128;       //4   
			*cb_ptr++ = *byteptr++ - 128;
			*y1_ptr++ = *byteptr++ - 128;       //5 
			*cr_ptr++ = *byteptr++ - 128;
			*y1_ptr++ = *byteptr++ - 128;       //6   
			*cb_ptr++ = *byteptr++ - 128;
			*y1_ptr++ = *byteptr++ - 128;       //7 
			*cr_ptr++ = *byteptr++ - 128;
			//MCU-2
			*y2_ptr++ = *byteptr++ - 128;       //0   
			*cb_ptr++ = *byteptr++ - 128;
			*y2_ptr++ = *byteptr++ - 128;       //1 
			*cr_ptr++ = *byteptr++ - 128;
			*y2_ptr++ = *byteptr++ - 128;       //2   
			*cb_ptr++ = *byteptr++ - 128;
			*y2_ptr++ = *byteptr++ - 128;       //3 
			*cr_ptr++ = *byteptr++ - 128;

			*y2_ptr++ = *byteptr++ - 128;       //4   
			*cb_ptr++ = *byteptr++ - 128;
			*y2_ptr++ = *byteptr++ - 128;       //5 
			*cr_ptr++ = *byteptr++ - 128;
			*y2_ptr++ = *byteptr++ - 128;       //6   
			*cb_ptr++ = *byteptr++ - 128;
			*y2_ptr++ = *byteptr++ - 128;       //7 
			*cr_ptr++ = *byteptr++ - 128;
		}
	}
}


//---------------------------------------------------------------------------
//	[Public] 设置图像格式、质量因子、输出Buffer
// * 质量因子quality_factor范围[1,8]，1最好，8最差 *
void JPG_initImgFormat(UInt16 width, UInt16 height, UInt8 scale, UInt8 img_format,
					   UInt8 quality, UInt16 pitch, UInt16 xbeg, UInt16 ybeg)
{
	jpgenc = &jpgenc_struct;

	jpgenc->img_width = width;
	jpgenc->img_height = height;
	jpgenc->img_pitch = pitch;
	jpgenc->img_xbeg = xbeg;
	jpgenc->img_ybeg = ybeg;
	jpgenc->img_scale = scale;

	jpgenc->mcu_width = 8;
	jpgenc->mcu_height = 8;
	jpgenc->hori_mcus = ((width + 7) >> 3);
	jpgenc->vert_mcus = ((height + 7) >> 3);

	jpgenc->quality_factor = quality;
	jpgenc->img_format = img_format;

	JPG_setquality(quality);
}

//---------------------------------------------------------------------------
//	[Public] 设置输出数据流Buffer、流刷新Callback函数
//
void JPG_initOutStream(CallBack_OutStream streamFunc, UInt8 *outbuf, Int32 bufsize)
{
	jpg_FlushStream = streamFunc;
	jpg_outbufsize = bufsize;
	jpg_outstream = outbuf;
	//    jpg_streamfptr = 0;    				//要求重新输出JPEG header
	jpg_streamfptr = outbuf;
}

//---------------------------------------------------------------------------
//  [Public] 设置JPEG附加数据段
UInt8 JPG_WriteAppDat(UInt8 *in_dat, UInt16 size)
{
	UInt16 i;

	i = 0;
	while (i < size)
	{
		jpg_appdat_buf[i] = *in_dat++;
		if (jpg_appdat_buf[i] == 0xff)
		{
			i++;
			jpg_appdat_buf[i] = 0x00;
		}
		i++;
		if (i >= JPG_APPDAT_BUFSIZE)
			return(0);
	}
	return(1);
}

//---------------------------------------------------------------------------
//  [Public]
UInt8 *JPG_WriteHeader(UInt16 jpghd_offset)
{
	UInt8 *out_ptr;

	out_ptr = jpg_outstream;
	if (jpghd_offset == 0)
	{
		out_ptr = JPG_writeheader(out_ptr);
	} else
	{
		out_ptr += jpghd_offset;
	}
	jpg_streamfptr = out_ptr;
	return(out_ptr);
}

//---------------------------------------------------------------------------
//	[Public] JPEF Encoder interface
//  Return: 编码结果Byte数
Int32 JPG_Encode(void)
{
	UInt8 *out_ptr, *out_bufbase;
	UInt32 x, y, step_y;
	Int16 *srcptr;
	Int32 trunc_size, t_size = 0, size1;
	UInt32 yuv_mode;

	if (jpgenc->img_format == JPG_IMGFMT_YUV420)
	{
		step_y = 16 * jpgenc->img_scale;
		yuv_mode = 1;
	} else
	{
		step_y = 8 * jpgenc->img_scale;
		if (jpgenc->img_format == JPG_IMGFMT_MONO)
			yuv_mode = 0;
		else
			yuv_mode = 2;
	}

	//init Huffman Coder parameter
	jpg_bitsbuf = 0;
	jpg_bitindex = 0;
	jpgenc->last_dc1 = 0;
	jpgenc->last_dc2 = 0;
	jpgenc->last_dc3 = 0;
	//设定输出Buffer刷新限值，码流Byte数大于此值时，调用Callback函数输出
	//缓存的码流，从头开始使用缓冲区
	trunc_size = jpg_outbufsize - 64 * 4;
	//
	//write JPEG header
	// *如果图像格式、质量因子、输出Buffer未变，对相同格式图像的编码，QT和huffman表
	//	完全相同，则可以跳过写JPEG文件头的部分，直接跳到上次得到的图像流起点开始写
	out_bufbase = jpg_outstream;
	out_ptr = jpg_streamfptr;

	size1 = out_ptr - out_bufbase;  //672Byte, 要求Buffer >(672Byte + 4MCU 编码ByteNum)
	if (size1 > trunc_size)
	{
		out_bufbase = jpg_FlushStream(out_bufbase, size1);
		out_ptr = out_bufbase;
		t_size += size1;
	}

	//按MCU块编码图像，每次编码16x8Pixel图像，对应4个MCU
	for (y = 0; y < jpgenc->img_height; y += step_y)
	{
		for (x = 0; x < jpgenc->img_width; x += (16 * jpgenc->img_scale))
		{
			//Read source
			srcptr = jpg_mcubuff;
			JPG_readsrc_yuv(x, y, yuv_mode, srcptr);
			//Coder
			//Y1
			DCT(srcptr);
			JPG_quantization(srcptr, inv_Y_QT);
			out_ptr = JPG_huffman(srcptr, 1, out_ptr);
			//Y2
			srcptr += 64;
			DCT(srcptr);
			JPG_quantization(srcptr, inv_Y_QT);
			out_ptr = JPG_huffman(srcptr, 1, out_ptr);

			if (yuv_mode == 1)
			{
				//Y3
				srcptr += 64;
				DCT(srcptr);
				JPG_quantization(srcptr, inv_Y_QT);
				out_ptr = JPG_huffman(srcptr, 1, out_ptr);
				//Y4
				srcptr += 64;
				DCT(srcptr);
				JPG_quantization(srcptr, inv_Y_QT);
				out_ptr = JPG_huffman(srcptr, 1, out_ptr);
			}

			if (yuv_mode != 0)
			{
				//Cb
				srcptr += 64;
				DCT(srcptr);
				JPG_quantization(srcptr, inv_UV_QT);
				out_ptr = JPG_huffman(srcptr, 2, out_ptr);
				//Cr
				srcptr += 64;
				DCT(srcptr);
				JPG_quantization(srcptr, inv_UV_QT);
				out_ptr = JPG_huffman(srcptr, 3, out_ptr);
			}
			//write stream
			size1 = out_ptr - out_bufbase;
			if (size1 > trunc_size)
			{
				out_bufbase = jpg_FlushStream(out_bufbase, size1);
				out_ptr = out_bufbase;
				t_size += size1;
			}
		}
	}
	out_ptr = close_bitstream(out_ptr);
	size1 = out_ptr - out_bufbase;
	jpg_FlushStream(out_bufbase, size1);
	t_size += size1;
	return(t_size);
}

//---------------------------------------------------------------------------
//	End of file
//---------------------------------------------------------------------------

