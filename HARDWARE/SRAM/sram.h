#ifndef __SRAM_H
#define __SRAM_H
#include <stm32f10x.h>	    
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//�ⲿSRAM ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/16
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////


void FSMC_SRAM_Init(void);
void FSMC_SRAM_WriteBuffer(u8* pBuffer, u32 WriteAddr, u32 BytesToWrite);
void FSMC_SRAM_ReadBuffer(u8* pBuffer, u32 ReadAddr, u32 BytesToWrite);

#endif

