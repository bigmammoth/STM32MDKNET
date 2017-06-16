/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2015 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    UDP_Socket.c
 * Purpose: UDP Socket Code Template
 * Rev.:    V7.0.0
 *----------------------------------------------------------------------------*/
//! [code_UDP_Socket]
#include "rl_net.h"

// 远端服务器IP地址
volatile uint8_t remoteServerIP[NET_ADDR_IP4_LEN] = {0, 0, 0, 0};
// 远端服务器端口
volatile uint16_t remoteUdpPort = 9010;

int32_t udp_sock;                       // UDP socket handle
 
// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const  NET_ADDR *addr, const uint8_t *buf, uint32_t len)
{ 
  // Data received
  /* Example
  if ((buf[0] == 0x01) && (len == 2)) {
    // Switch LEDs on and off
    // LED_out (buf[1]);
  }
  */
	//uint32_t port = addr->port;
  return (0);
}

// Send UDP data to destination client.
void send_udp_data (char* buff, uint32_t size) {
 
  if (udp_sock > 0)
	{
		NET_ADDR remoteServerAddr;
		remoteServerAddr.addr_type = NET_ADDR_IP4;
		for(int i = 0; i < NET_ADDR_IP6_LEN; i++)
		{
			remoteServerAddr.addr[i] = 0;
			if(i < NET_ADDR_IP4_LEN) remoteServerAddr.addr[i] = remoteServerIP[i];
		}
		remoteServerAddr.port = remoteUdpPort;
		uint8_t *sendBuf = NULL;
		sendBuf = netUDP_GetBuffer(size);
		for(int i = 0; i < size; i++) sendBuf[i] = buff[i];
		netStatus netSt;
		if(sendBuf != NULL)
			netSt = netUDP_Send(udp_sock, &remoteServerAddr, sendBuf, size);
		if(netSt == netOK)
		{
			int i = 0;
		}
  }
}

//! [code_UDP_Socket]
