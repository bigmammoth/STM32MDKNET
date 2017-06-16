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
  * @brief  驱动补丁，配置MCO引脚，输出50Mhz时钟信号给ETH芯片83848
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
  * @brief  设置网络参数
  * @param  dataInFlash - 指向flash中存储的数据块指针
  * @retval None
  *-------------------------------------------------------------------------*/
volatile bool ifDhcp = true;
extern uint8_t remoteServerIP[];
extern uint16_t remoteUdpPort;
void setupNetParm(_dataInFlash* dataInFlash)
{
	// 开启或关闭DHCP
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
	// 设置IP
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++) 
		ip_addr_bin[i] = (char)(dataInFlash->ipAddr>>((NET_ADDR_IP4_LEN-i-1)*8));
	netIF_SetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_Address, 
									(uint8_t*)ip_addr_bin, sizeof(ip_addr_bin));
	// 设置子网掩码
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++) 
		ip_addr_bin[i] = (char)(dataInFlash->ipMask>>((NET_ADDR_IP4_LEN-i-1)*8));
	netIF_SetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_SubnetMask, 
									(uint8_t*)ip_addr_bin, sizeof(ip_addr_bin));
	// 设置网关
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++) 
		ip_addr_bin[i] = (char)(dataInFlash->ipGate>>((NET_ADDR_IP4_LEN-i-1)*8));
	netIF_SetOption (NET_IF_CLASS_ETH, netIF_OptionIP4_DefaultGateway, 
									(uint8_t*)ip_addr_bin, sizeof(ip_addr_bin));
	// 远端服务器地址
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
		remoteServerIP[i] = dataInFlash->remoteAddr>>((NET_ADDR_IP4_LEN-i-1)*8) & 0xFF;
	// 远端服务器UDP端口
	remoteUdpPort = dataInFlash->udpPort;
}


// 数据包，Json格式
static const char jsonData[64] = "\
{\
\"toiletID\":%d,\
\"packageType\":%d,\
\"statusNumber\":%d,\
\"status\":[\
";

/*!---------------------------------------------------------------------------
 * @brief  UDP定时发送之定时器回调函数。
 * @param  argument - 未使用
 * @retval None
 *---------------------------------------------------------------------------*/
void TimerUdpSendCallback(void *args)
{
	osThreadFlagsSet(mainThreadId, 1);
}

/*!---------------------------------------------------------------------------
 * @brief  主线程，工作：1.完成网络初始化工作 2.创建其它线程 3.负责接收485总线传来
					 的数据，通过UDP数据包发送给远程主机。
 * @param  argument - 未使用
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
	// 初始化网络接口
	MC0_Configuration();
	netStatus status = netInitialize ();
	// 调入存入Flash中的数据，包括IP地址等
	_dataInFlash dataInFlash;
	if(loadData(&dataInFlash) == true)
	{	// 设置网络参数
		setupNetParm(&dataInFlash);
	}
	// 创建485线程
	RS485_Thread_ID = osThreadNew(RS485_Thread, NULL, NULL);
	
	// 初始化UDP socket，注册回调函数，监听端口19000
  udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
    netUDP_Open (udp_sock, 19000);
  }

	char* buff = malloc(512);
	// 创建定时器
	osTimerId_t timeUdpSend = osTimerNew((osTimerFunc_t)&TimerUdpSendCallback,
																			osTimerPeriodic, NULL, NULL);
	osTimerStart(timeUdpSend, 100);
	
  for (;;) 
	{
		osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
		// 包类型102，10个数据
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
