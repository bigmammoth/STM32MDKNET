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
	{	// �������
		osThreadFlagsSet(RS485_Thread_ID, RS485_RECV_COMPL);
	}
	if(event & ARM_USART_EVENT_SEND_COMPLETE)
	{ // ���͵�Ӳ����������ɣ�����Ӳ����û�з������
		osThreadFlagsSet(RS485_Thread_ID, RS485_SEND_COMPL);
	}
	if(event & ARM_USART_EVENT_TRANSFER_COMPLETE)
	{ // TRANSFER���շ������Ƿ���
		osThreadFlagsSet(RS485_Thread_ID, RS485_TRANS_COMPL);
	}
	if(event & ARM_USART_EVENT_TX_COMPLETE)
	{	// Ӳ���������
		osThreadFlagsSet(RS485_Thread_ID, RS485_TX_COMPL);
	}
}

// ѯ�����ݰ�
typedef struct
{
	uint8_t start0;
	uint8_t start1;
	uint8_t moduleId;
	uint8_t inquiryType;
	uint16_t checkSum;
}rs485InquiryPack_t;

// ����״̬���ݰ�
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

// ģ����������ֵ32
uint16_t moduleQuantity = 8;
volatile uint32_t sonicDistance;
/*!---------------------------------------------------------------
	*@brief		RS485�߳�
	*@param		δʹ��
	*@retval		none
----------------------------------------------------------------*/
void RS485_Thread(const void* argument)
{
	static ARM_DRIVER_USART* USARTdrv = &Driver_USART3;
	// ��ʼ��USART3������ע��ص�����
	USARTdrv->Initialize(USART3_callback);
	// �ϵ�USART3����Ӳ��
	USARTdrv->PowerControl(ARM_POWER_FULL);
	// ����USART3��������9600
	USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
		ARM_USART_DATA_BITS_8 |
		ARM_USART_PARITY_NONE |
		ARM_USART_STOP_BITS_1	|
		ARM_USART_FLOW_CONTROL_NONE, 
		115200);
	// ������պͷ���
	USARTdrv->Control(ARM_USART_CONTROL_TX, 1);
	USARTdrv->Control(ARM_USART_CONTROL_RX, 1);
	
	// 485�շ�ѡ������PC12����
	GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable GPIOC clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// �������ݻ���
	rs485StatusPack_t statusPack;
	
	// ��ǰ��ѯ���ж�ģ��ID����ֵΪ0
	uint32_t currentInquiryId = 0;
	// ѯ�����ݰ�
	rs485InquiryPack_t inquiryPack;
	
	while(1)
	{
		// ����ѯ�����ݰ�
		inquiryPack.start0 = 0xFF;
		inquiryPack.start1 = 0x55;
		inquiryPack.moduleId = currentInquiryId;
		inquiryPack.inquiryType = MODULE_STATUS;
		inquiryPack.checkSum = inquiryPack.moduleId + inquiryPack.inquiryType;
		
		// ��ѯ�ʰ����ȴ��������
		SetSend();
		USARTdrv->Send(&inquiryPack, sizeof(inquiryPack));
		uint32_t ret = osThreadFlagsWait(RS485_TX_COMPL, osFlagsWaitAny, osWaitForever);
		
		// �ȴ�ģ��ظ����ȴ�ʱ��20ms
		SetReceive();
		USARTdrv->Receive(&statusPack, sizeof(statusPack));
		ret = osThreadFlagsWait(RS485_RECV_COMPL, osFlagsWaitAny, 10);

		if(ret == osFlagsErrorTimeout)
		{ // ��ʱ����һ��ģ��
			if(++currentInquiryId >= moduleQuantity) currentInquiryId = 0;
		}
		else if(ret & 0x80000000U)	// Error
		{
			while(1);
		}
		else
		{ // ������ɣ����У���
			uint16_t checkSum = statusPack.moduleId + statusPack.inquiryType + statusPack.sonicDistance;
			if(checkSum == statusPack.checkSum)
			{
				if(currentInquiryId == statusPack.moduleId)
				{ // У��ͼ�ģ��ž���ȷ
					sonicDistance = statusPack.sonicDistance;
				}
				else
				{ // ģ��Ų���
					int i = 0;
				}
			}
			else
			{ // У��ʹ���
				int i = 0;
			}
			// ��һ��ģ��
			if(++currentInquiryId >= moduleQuantity) currentInquiryId = 0;
		}
	}
}
