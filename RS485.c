#include "Driver_USART.h"
#include "cmsis_os2.h"
#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO


void RS485_Thread(void const *argument);
osThreadId_t RS485_Thread_ID;

/* USART Driver */
extern ARM_DRIVER_USART Driver_USART3;

#define RS485_RECV_COMPL		(0x00000001U<<1)
#define RS485_SEND_COMPL		(0x00000001U<<2)
#define RS485_TRANS_COMPL		(0x00000001U<<3)
#define RS485_TX_COMPL			(0x00000001U<<4)

inline void SetSend(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
}

inline void SetReceive(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_12);
}

inline void SendRs485Pack(const void * data, uint32_t num)
{
}

void USART3_callback(uint32_t event)
{
	if(event & ARM_USART_EVENT_RECEIVE_COMPLETE)
	{	// 接收完成
		osThreadFlagsSet(RS485_Thread_ID, RS485_RECV_COMPL);
	}
	if(event & ARM_USART_EVENT_SEND_COMPLETE)
	{ // 发送到硬件缓冲区完成，但是硬件并没有发送完毕
		osThreadFlagsSet(RS485_Thread_ID, RS485_SEND_COMPL);
	}
	if(event & ARM_USART_EVENT_TRANSFER_COMPLETE)
	{ // TRANSFER是收发，不是发送
		osThreadFlagsSet(RS485_Thread_ID, RS485_TRANS_COMPL);
	}
	if(event & ARM_USART_EVENT_TX_COMPLETE)
	{	// 硬件发送完成
		osThreadFlagsSet(RS485_Thread_ID, RS485_TX_COMPL);
	}
}

// 询问数据包
typedef struct
{
	uint8_t start0;
	uint8_t start1;
	uint8_t moduleId;
	uint8_t inquiryType;
	uint16_t checkSum;
}rs485InquiryPack_t;

// 接收状态数据包
typedef struct
{
	uint8_t moduleId;
	uint8_t inquiryType;
	uint16_t sonicDistance;
	uint16_t checkSum;
}rs485StatusPack_t;

enum InquiryType
{
	MODULE_STATUS = 0x01,
	MODULE_RESET
};

// 模块总数，初值32
uint16_t moduleQuantity = 8;
volatile uint32_t sonicDistance;
/*!---------------------------------------------------------------
	*@brief		RS485线程
	*@param		未使用
	*@retval		none
----------------------------------------------------------------*/
void RS485_Thread(const void* argument)
{
	static ARM_DRIVER_USART* USARTdrv = &Driver_USART3;
	// 初始化USART3驱动，注册回调函数
	USARTdrv->Initialize(USART3_callback);
	// 上电USART3外设硬件
	USARTdrv->PowerControl(ARM_POWER_FULL);
	// 配置USART3，波特率9600
	USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
		ARM_USART_DATA_BITS_8 |
		ARM_USART_PARITY_NONE |
		ARM_USART_STOP_BITS_1	|
		ARM_USART_FLOW_CONTROL_NONE, 
		115200);
	// 允许接收和发送
	USARTdrv->Control(ARM_USART_CONTROL_TX, 1);
	USARTdrv->Control(ARM_USART_CONTROL_RX, 1);
	
	// 485收发选择引脚PC12设置
	GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable GPIOC clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// 接收数据缓存
	rs485StatusPack_t statusPack;
	
	// 当前查询的中断模块ID，初值为0
	uint32_t currentInquiryId = 0;
	// 询问数据包
	rs485InquiryPack_t inquiryPack;
	
	while(1)
	{
		// 设置询问数据包
		inquiryPack.start0 = 0xFF;
		inquiryPack.start1 = 0x55;
		inquiryPack.moduleId = currentInquiryId;
		inquiryPack.inquiryType = MODULE_STATUS;
		inquiryPack.checkSum = inquiryPack.moduleId + inquiryPack.inquiryType;
		
		// 发询问包，等待发送完成
		SetSend();
		USARTdrv->Send(&inquiryPack, sizeof(inquiryPack));
		uint32_t ret = osThreadFlagsWait(RS485_TX_COMPL, osFlagsWaitAny, osWaitForever);
		
		// 等待模块回复，等待时长20ms
		SetReceive();
		USARTdrv->Receive(&statusPack, sizeof(statusPack));
		ret = osThreadFlagsWait(RS485_RECV_COMPL, osFlagsWaitAny, 10);

		if(ret == osFlagsErrorTimeout)
		{ // 超时，下一个模块
			if(++currentInquiryId >= moduleQuantity) currentInquiryId = 0;
		}
		else if(ret & 0x80000000U)	// Error
		{
			while(1);
		}
		else
		{ // 接收完成，检查校验和
			uint16_t checkSum = statusPack.moduleId + statusPack.inquiryType + statusPack.sonicDistance;
			if(checkSum == statusPack.checkSum)
			{
				if(currentInquiryId == statusPack.moduleId)
				{ // 校验和及模块号均正确
					sonicDistance = statusPack.sonicDistance;
				}
				else
				{ // 模块号不对
					int i = 0;
				}
			}
			else
			{ // 校验和错误
				int i = 0;
			}
			// 下一个模块
			if(++currentInquiryId >= moduleQuantity) currentInquiryId = 0;
		}
	}
}
