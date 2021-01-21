#include "sys.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h" 
#include "beep.h"	 	 
#include "key.h"	 	 
#include "exti.h"	 	 
#include "timer.h"		 	 
#include "lcd.h"
#include "rtc.h"	 	 
#include "wkup.h"		 
#include "dac.h" 	  
#include "24cxx.h" 	 
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"	 
#include "usb_desc.h"
#include "ov7670.h"
#include "sram.h"


void usb_port_set(u8 enable);


int main(void)
{
	u8 lightmode = 0, saturation = 2, brightness = 2, contrast = 2;
	u8 effect = 0;
	u8 errcode;

	Stm32_Clock_Init(9);	// ϵͳʱ������
	delay_init(72);			// ��ʱ��ʼ��
	uart_init(72, 9600); 	// ����1��ʼ��
	LCD_Init();				// ��ʼ��Һ��
	LED_Init();         	// LED��ʼ��
	KEY_Init();				// ������ʼ��
	FSMC_SRAM_Init();		// ��ʼ���ⲿSRAM
	//////////////////////////////////////////////////////////////////////////

	//// ��ʼ��OV7670
	//errcode = OV7670_Init();
	//if (errcode)
	//{
	//	POINT_COLOR = RED;
	//	LCD_ShowString(10, 50, 200, 16, 16, "OV7670 Error!!");
	//	delay_ms(1000);
	//	return -1;
	//} else{
	//	POINT_COLOR = RED;
	//	LCD_ShowString(10, 50, 200, 16, 16, "OV7670 Init OK");
	//}
	//delay_ms(1000);
	//OV7670_Light_Mode(lightmode);
	//OV7670_Color_Saturation(saturation);
	//OV7670_Brightness(brightness);
	//OV7670_Contrast(contrast);
	//OV7670_Special_Effects(effect);
	//OV7670_Window_Set(176, 10, 320, 240);	//���ô���
	//OV7670_CS = 0;

	//// ʹ�ܶ�ʱ������(ʹ���ⲿ�ж���8����֡ͬ���ź�)
	//EXTI8_Init();							

	//////////////////////////////////////////////////////////////////////////

	// ��������USBģ��
	usb_port_set(0);
	delay_ms(300);
	usb_port_set(1);
	// USB����
	USB_Interrupts_Config();
	Set_USBClock();
	USB_Init();
	delay_ms(300);

	//////////////////////////////////////////////////////////////////////////
	
	// 10Khz����Ƶ��,1�����ж϶�ʱ��,ͳ�Ƹ���ˢ����
	TIM6_Int_Init (10000, 7199);				
	
	while (1)
	{
		OV7670_CaptureFrame();
	}
}


// ����USB ����/����
// enable:0,�Ͽ�
//        1,��������	   
void usb_port_set(u8 enable)
{
	RCC->APB2ENR |= 1 << 2;    				// ʹ��PORTAʱ��	   	 
	if (enable) 							// �˳��ϵ�ģʽ(0 == PDWN)
	{
		_SetCNTR(_GetCNTR()&(~(1 << 1))); 	// �˳��ϵ�ģʽ
	} else {		// ����ϵ�ģʽ(1 == PDWN)
		_SetCNTR(_GetCNTR() | (1 << 1));  	// �ϵ�ģʽ
		GPIOA->CRH &= 0XFFF00FFF;			// ���PA11��PA12
		GPIOA->CRH |= 0X00033000;			// 00��ͨ���������ģʽ ; 11 �����ģʽ������ٶ�50MH
		PAout(12) = 0;
	}
}


