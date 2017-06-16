/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/
 
#include "RTE_Components.h"
#include  CMSIS_device_header
#include "cmsis_os2.h"
#include "rl_net.h"                     // Keil.MDK-Plus::Network:CORE
#include "stm32f10x.h"                  // Device header
#include "GPIO_STM32F10x.h"             // Keil::Device:GPIO
#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO
#include "flash_rw.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
#ifdef RTE_Compiler_EventRecorder
#include "EventRecorder.h"
#endif

osThreadId_t mainThreadId = NULL;

/*!--------------------------------------------------------------------------
  * @brief  ��������������MCO���ţ����50Mhzʱ���źŸ�ETHоƬ83848
  * @param  None
  * @retval None
  *-------------------------------------------------------------------------*/
void MC0_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable GPIOA clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	/* MCO pin configuration------------------------------------------------- */
  /* Configure MCO (PA8) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  /* Set PLL3 clock output to 50MHz (25MHz /5 *10 =50MHz) */
  RCC_PLL3Config(RCC_PLL3Mul_10);
  /* Enable PLL3 */
  RCC_PLL3Cmd(ENABLE);
  /* Wait till PLL3 is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_PLL3RDY) == RESET)
  {}
  /* Get PLL3 clock on PA8 pin (MCO) */
  RCC_MCOConfig(RCC_MCO_PLL3CLK);
}

/*!--------------------------------------------------------------------------
  * @brief  �����������
  * @param  dataInFlash - ָ��flash�д洢�����ݿ�ָ��
  * @retval None
  *-------------------------------------------------------------------------*/
volatile bool ifDhcp = true;
extern uint8_t remoteServerIP[];
extern uint16_t remoteUdpPort;
void setupNetParm(_dataInFlash* dataInFlash)
{
	// ������ر�DHCP
	if(dataInFlash->ifDhcp)
	{
		netDHCP_Enable(0);
		ifDhcp = true;
	}
	else
	{
		netDHCP_Disable(0);
		ifDhcp = false;
	}
	
	char ip_addr_bin[NET_ADDR_IP4_LEN];
	// ����IP
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++) 
		ip_addr_bin[i] = (char)(dataInFlash->ipAddr>>((NET_ADDR_IP4_LEN-i-1)*8));
	netIF_SetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_Address, 
									(uint8_t*)ip_addr_bin, sizeof(ip_addr_bin));
	// ������������
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++) 
		ip_addr_bin[i] = (char)(dataInFlash->ipMask>>((NET_ADDR_IP4_LEN-i-1)*8));
	netIF_SetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_SubnetMask, 
									(uint8_t*)ip_addr_bin, sizeof(ip_addr_bin));
	// ��������
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++) 
		ip_addr_bin[i] = (char)(dataInFlash->ipGate>>((NET_ADDR_IP4_LEN-i-1)*8));
	netIF_SetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_DefaultGateway, 
									(uint8_t*)ip_addr_bin, sizeof(ip_addr_bin));
	// Զ�˷�������ַ
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
		remoteServerIP[i] = dataInFlash->remoteAddr>>((NET_ADDR_IP4_LEN-i-1)*8) & 0xFF;
	// Զ�˷�����UDP�˿�
	remoteUdpPort = dataInFlash->udpPort;
}


// ���ݰ���Json��ʽ
static const char jsonData[64] = "\
{\
\"toiletID\":%d,\
\"packageType\":%d,\
\"statusNumber\":%d,\
\"status\":[\
";

/*!---------------------------------------------------------------------------
 * @brief  UDP��ʱ����֮��ʱ���ص�������
 * @param  argument - δʹ��
 * @retval None
 *---------------------------------------------------------------------------*/
void TimerUdpSendCallback(void *args)
{
	osThreadFlagsSet(mainThreadId, 1);
}

/*!---------------------------------------------------------------------------
 * @brief  ���̣߳�������1.��������ʼ������ 2.���������߳� 3.�������485���ߴ���
					 �����ݣ�ͨ��UDP���ݰ����͸�Զ��������
 * @param  argument - δʹ��
 * @retval None
 *---------------------------------------------------------------------------*/
extern uint32_t udp_cb_func (int32_t socket, const  NET_ADDR *addr, 
	const uint8_t *buf, uint32_t len);
extern int32_t udp_sock;
extern uint32_t sonicDistance;
extern osThreadId_t RS485_Thread_ID;
extern void RS485_Thread(void* argument);
extern void send_udp_data (char* buff, uint32_t size);
void app_main (void *argument)
{
	// ��ʼ������ӿ�
	MC0_Configuration();
	netStatus status = netInitialize ();
	// �������Flash�е����ݣ�����IP��ַ��
	_dataInFlash dataInFlash;
	if(loadData(&dataInFlash) == true)
	{	// �����������
		setupNetParm(&dataInFlash);
	}
	// ����485�߳�
	RS485_Thread_ID = osThreadNew(RS485_Thread, NULL, NULL);
	
	// ��ʼ��UDP socket��ע��ص������������˿�19000
  udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
    netUDP_Open (udp_sock, 19000);
  }

	char* buff = malloc(512);
	// ������ʱ��
	osTimerId_t timeUdpSend = osTimerNew((osTimerFunc_t)&TimerUdpSendCallback,
																			osTimerPeriodic, NULL, NULL);
	osTimerStart(timeUdpSend, 100);
	
  for (;;) 
	{
		osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
		// ������102��10������
		//sprintf(buff, jsonData, 5, 102, 10);
		//strcat(buff, "1,0,1,0,1,0,1,0,1,0]}");
		sprintf(buff, "%d", sonicDistance);
		uint32_t size = 0;
		while(buff[size++] != 0);
		send_udp_data(buff, size);
		osThreadYield();
	}
}

int main (void) {

  // System Initialization
  SystemCoreClockUpdate();
#ifdef RTE_Compiler_EventRecorder
  // Initialize and start Event Recorder
  EventRecorderInitialize(EventRecordError, 1U);
#endif
  // ...	
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  mainThreadId = osThreadNew(app_main, NULL, NULL);    // Create application main thread
  osKernelStart();                      // Start thread execution

  for (;;) {}
}
