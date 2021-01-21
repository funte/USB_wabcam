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

	Stm32_Clock_Init(9);	// 系统时钟设置
	delay_init(72);			// 延时初始化
	uart_init(72, 9600); 	// 串口1初始化
	LCD_Init();				// 初始化液晶
	LED_Init();         	// LED初始化
	KEY_Init();				// 按键初始化
	FSMC_SRAM_Init();		// 初始化外部SRAM
	//////////////////////////////////////////////////////////////////////////

	//// 初始化OV7670
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
	//OV7670_Window_Set(176, 10, 320, 240);	//设置窗口
	//OV7670_CS = 0;

	//// 使能定时器捕获(使用外部中断线8捕获帧同步信号)
	//EXTI8_Init();							

	//////////////////////////////////////////////////////////////////////////

	// 重新启动USB模块
	usb_port_set(0);
	delay_ms(300);
	usb_port_set(1);
	// USB配置
	USB_Interrupts_Config();
	Set_USBClock();
	USB_Init();
	delay_ms(300);

	//////////////////////////////////////////////////////////////////////////
	
	// 10Khz计数频率,1秒钟中断定时器,统计各种刷新率
	TIM6_Int_Init (10000, 7199);				
	
	while (1)
	{
		OV7670_CaptureFrame();
	}
}


// 设置USB 连接/断线
// enable:0,断开
//        1,允许连接	   
void usb_port_set(u8 enable)
{
	RCC->APB2ENR |= 1 << 2;    				// 使能PORTA时钟	   	 
	if (enable) 							// 退出断电模式(0 == PDWN)
	{
		_SetCNTR(_GetCNTR()&(~(1 << 1))); 	// 退出断电模式
	} else {		// 进入断电模式(1 == PDWN)
		_SetCNTR(_GetCNTR() | (1 << 1));  	// 断电模式
		GPIOA->CRH &= 0XFFF00FFF;			// 清除PA11和PA12
		GPIOA->CRH |= 0X00033000;			// 00：通用推挽输出模式 ; 11 ：输出模式，最大速度50MH
		PAout(12) = 0;
	}
}


