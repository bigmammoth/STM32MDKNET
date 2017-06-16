/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2015 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server_CGI.c
 * Purpose: HTTP Server CGI Module
 * Rev.:    V7.0.0
 *----------------------------------------------------------------------------*/
//! [code_HTTP_Server_CGI]
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "rl_net.h"
#include "cmsis_os2.h"
#include "flash_rw.h"

bool LEDrun;
// Local variables.
static uint8_t P2;
static uint8_t ip_addr[NET_ADDR_IP4_LEN];
static char    ip_string[40];

extern uint8_t remoteServerIP[];
extern bool ifDhcp;
extern uint16_t remoteUdpPort;

// My structure of CGI status variable.
typedef struct {
  uint8_t idx;
  uint8_t unused[3];
} MY_BUF;
#define MYBUF(p)        ((MY_BUF *)p)
 
//! \brief Process query string received by GET request.
//! \param[in]     qstr          pointer to the query string.
//! \return        none.
void netCGI_ProcessQuery (const char *qstr) {
  netIF_Option opt;
  int16_t      typ;
  char var[40];
	uint32_t index;

  do {
    // Loop through all the parameters
    qstr = netCGI_GetEnvVar (qstr, var, sizeof (var));
    // Check return string, 'qstr' now points to the next parameter

    switch (var[0]) {
      case 'i': // Local IP address
        opt = netIF_OptionIP4_Address;
				index = 3;
        break;

      case 'm': // Local network mask
        opt = netIF_OptionIP4_SubnetMask;
				index = 4;
        break;

      case 'g': // Default gateway IP address
        opt = netIF_OptionIP6_DefaultGateway;
				index = 3;
        break;

      case 'p': // Primary DNS server IP address
				if(var[1] == 'g')
				{
					var[0] = '\0';
					break;
				}
        opt = netIF_OptionIP4_PrimaryDNS;
				index = 5;
        break;

      case 's': // Secondary DNS server IP address
        opt = netIF_OptionIP4_SecondaryDNS;
				index = 5;
        break;
 
      case 'r': // Remote server IP address
        var[0] = '\0';
				index = 5;
				netIP_aton(&var[index], typ, remoteServerIP);
        break;
			
			case 'd':	// DHCP enable or disable
				if(strcmp(var, "dhcp=on") == 0)
				{
					ifDhcp = true;
					netDHCP_Enable(0);
				}
				else
				{
					ifDhcp = false;
					netDHCP_Disable(0);
				}
				var[0] = '\0';
				break;
			
      default: var[0] = '\0'; break;
    }

		typ = NET_ADDR_IP4;
    if (var[0] != '\0')
		{
			netIP_aton (&var[index], typ, ip_addr);
			// Set required option
			netIF_SetOption (NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
    }
  } while (qstr);
}

//! \brief Process data received by POST request.
//! \param[in]     code          callback context:
//!                              - 0 = www-url-encoded form data,
//!                              - 1 = filename for file upload (null-terminated string),
//!                              - 2 = file upload raw data,
//!                              - 3 = end of file upload (file close requested),
//!                              - 4 = any other type of POST data (single or last stream),
//!                              - 5 = the same as 4, but with more data to follow.
//! \param[in]     data          pointer to POST data.
//! \param[in]     len           length of POST data.
//! \return        none.
void netCGI_ProcessData (uint8_t code, const char *data, uint32_t len) {
  char var[40],passw[12];

  if (code != 0) {
    // Ignore all other codes
    return;
  }

  P2 = 0;
//  LEDrun = true;
  if (len == 0) {
    // No data or all items (radio, checkbox) are off
    //LED_SetOut (P2);
    return;
  }
  passw[0] = 1;
	
	netIF_Option opt;
	uint32_t	index;
  do {
    // Parse all parameters
    data = netCGI_GetEnvVar (data, var, sizeof (var));
		
		if(strcmp(var, "pg=net") == 0)
		{
			do
			{
				data = netCGI_GetEnvVar(data, var, sizeof(var));
				if(strncmp(var, "dhcp=off", 8) == 0)
				{
					ifDhcp = false;
					netDHCP_Disable(0);
					continue;
				}
				else if(strncmp(var, "dhcp=on", 7) == 0)
				{
					ifDhcp = true;
					netDHCP_Enable(0);
					continue;
				}
				else if(strncmp(var, "ip=", 3) == 0)
				{
					opt = netIF_OptionIP4_Address;
					index = 3;
				}
				else if(strncmp(var, "msk=", 4) == 0)
				{
					opt = netIF_OptionIP4_SubnetMask;
					index = 4;
				}
				else if(strncmp(var, "gw=", 3) == 0)
				{
					opt = netIF_OptionIP6_DefaultGateway;
					index = 3;
				}
				else if(strncmp(var, "rmts=", 5) == 0)
				{
					index = 5;
					netIP_aton(&var[index], NET_ADDR_IP4, remoteServerIP);
					continue;
				}
				else if(strncmp(var, "pdns=", 5) == 0)
				{
					opt = netIF_OptionIP4_PrimaryDNS;
					index = 5;
				}
				else if(strncmp(var, "sdns=", 5) == 0)
				{
					opt = netIF_OptionIP4_SecondaryDNS;
					index = 5;
				}
				else if(strncmp(var, "port=", 5) == 0)
				{
					index = 5;
					char buff[16];
					remoteUdpPort = atoi(&var[index]);
					continue;
				}
				netIP_aton (&var[index], NET_ADDR_IP4, ip_addr);
				// Set required option
				netIF_SetOption (NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
			}while(data);
			saveNetworkData();
		}
		
    if (var[0] != 0) {
      // First character is non-null, string exists
      if (strcmp (var, "led0=on") == 0) {
        P2 |= 0x01;
      }
      else if (strcmp (var, "led1=on") == 0) {
        P2 |= 0x02;
      }
      else if (strcmp (var, "led2=on") == 0) {
        P2 |= 0x04;
      }
      else if (strcmp (var, "led3=on") == 0) {
        P2 |= 0x08;
      }
      else if (strcmp (var, "led4=on") == 0) {
        P2 |= 0x10;
      }
      else if (strcmp (var, "led5=on") == 0) {
        P2 |= 0x20;
      }
      else if (strcmp (var, "led6=on") == 0) {
        P2 |= 0x40;
      }
      else if (strcmp (var, "led7=on") == 0) {
        P2 |= 0x80;
      }
      else if (strcmp (var, "ctrl=Browser") == 0) {
//        LEDrun = false;
      }
      else if ((strncmp (var, "pw0=", 4) == 0) ||
               (strncmp (var, "pw2=", 4) == 0)) {
        // Change password, retyped password
        if (netHTTPs_LoginActive()) {
          if (passw[0] == 1) {
            strcpy (passw, var+4);
          }
          else if (strcmp (passw, var+4) == 0) {
            // Both strings are equal, change the password
            netHTTPs_SetPassword (passw);
          }
        }
      }
      else if (strncmp (var, "lcd1=", 5) == 0) {
        // LCD Module line 1 text
//        strcpy (lcd_text[0], var+5);
        //osSignalSet (TID_Display, 0x01);
      }
      else if (strncmp (var, "lcd2=", 5) == 0) {
        // LCD Module line 2 text
//        strcpy (lcd_text[1], var+5);
        //osSignalSet (TID_Display, 0x01);
      }
    }
  } while (data);
  //LED_SetOut (P2)
}
 
/// \brief Generate dynamic web data based on a CGI script.
/// \param[in]     env           environment string.
/// \param[out]    buf           output data buffer.
/// \param[in]     buf_len       size of output buffer (from 536 to 1440 bytes).
/// \param[in,out] pcgi          pointer to a session's local buffer of 4 bytes.
///                              - 1st call = cleared to 0,
///                              - 2nd call = not altered by the system,
///                              - 3rd call = not altered by the system, etc.
/// \return        number of bytes written to output buffer.
///                - return len | (1U<<31) = repeat flag, the system calls this function
///                                          again for the same script line.
///                - return len | (1U<<30) = force transmit flag, the system transmits
///                                          current packet immediately.
uint32_t netCGI_Script (const char *env, char *buf, uint32_t buf_len, uint32_t *pcgi) {
  int32_t socket;
  netTCP_State state;
  NET_ADDR r_client;
  const char *lang;
  uint32_t len = 0;
  uint8_t id;
  static uint32_t adv;
  netIF_Option opt;
  int16_t      typ;

  switch (env[0]) {
    // Analyze a 'c' script line starting position 2
    case 'a' :
      // Network parameters from 'network.cgi'
      switch (env[2]) {
        case 'l':
          // Link-local address
					return (0);
          break;

        case 'i':
          // Write local IP address (IPv4)
					opt = netIF_OptionIP4_Address;
          break;

        case 'm':
          // Write local network mask
					opt = netIF_OptionIP4_SubnetMask;
          break;

        case 'g':
          // Write default gateway IP address
					opt = netIF_OptionIP4_DefaultGateway;
          break;

        case 'p':
          // Write primary DNS server IP address
					opt = netIF_OptionIP4_PrimaryDNS;
          break;

        case 's':
          // Write secondary DNS server IP address
					opt = netIF_OptionIP4_SecondaryDNS;
          break;
				
				case 'r':	// 远程主机IP地址
					netIP_ntoa(typ, remoteServerIP, ip_string, sizeof(ip_string));
					len = sprintf(buf, &env[4], ip_string);
					return len;
					break;
				
				case 'd':	// DHCP开关状态
					len = sprintf(buf, &env[4], (ifDhcp) ? "" : "checked", (ifDhcp) ? "checked" : "");
					return len;
					break;
				
				case 'u': // 远端主机UDP端口
					len = sprintf(buf, &env[4], remoteUdpPort);
					return len;
					break;
      }

			typ = NET_ADDR_IP4;
			netIF_GetOption (NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
			netIP_ntoa (typ, ip_addr, ip_string, sizeof(ip_string));
			len = sprintf (buf, &env[4], ip_string);
      break;

    case 'b':
      // LED control from 'led.cgi'
      if (env[2] == 'c') {
        // Select Control
        len = sprintf (buf, &env[4], LEDrun ?     ""     : "selected",
                                     LEDrun ? "selected" :    ""     );
        break;
      }
      // LED CheckBoxes
      id = env[2] - '0';
      if (id > 7) {
        id = 0;
      }
      id = 1 << id;
      len = sprintf (buf, &env[4], (P2 & id) ? "checked" : "");
      break;

    case 'c':
      // TCP status from 'tcp.cgi'
      while ((len + 150) < buf_len) {
        socket = ++MYBUF(pcgi)->idx;
        state  = netTCP_GetState (socket);

        if (state == netTCP_StateINVALID) {
          /* Invalid socket, we are done */
          return (len);
        }

        // 'sprintf' format string is defined here
        len += sprintf (buf+len,   "<tr align=\"center\">");
        if (state <= netTCP_StateCLOSED) {
          len += sprintf (buf+len, "<td>%d</td><td>%d</td><td>-</td><td>-</td>"
                                   "<td>-</td><td>-</td></tr>\r\n",
                                   socket,
                                   netTCP_StateCLOSED);
        }
        else if (state == netTCP_StateLISTEN) {
          len += sprintf (buf+len, "<td>%d</td><td>%d</td><td>%d</td><td>-</td>"
                                   "<td>-</td><td>-</td></tr>\r\n",
                                   socket,
                                   netTCP_StateLISTEN,
                                   netTCP_GetLocalPort(socket));
        }
        else {
          netTCP_GetPeer (socket, &r_client, sizeof(r_client));

          netIP_ntoa (r_client.addr_type, r_client.addr, ip_string, sizeof (ip_string));
          
          len += sprintf (buf+len, "<td>%d</td><td>%d</td><td>%d</td>"
                                   "<td>%d</td><td>%s</td><td>%d</td></tr>\r\n",
                                   socket, netTCP_StateLISTEN, netTCP_GetLocalPort(socket),
                                   netTCP_GetTimer(socket), ip_string, r_client.port);
        }
      }
      /* More sockets to go, set a repeat flag */
      len |= (1u << 31);
      break;

    case 'd':
      // System password from 'system.cgi'
      switch (env[2]) {
        case '1':
          len = sprintf (buf, &env[4], netHTTPs_LoginActive() ? "Enabled" : "Disabled");
          break;
        case '2':
          len = sprintf (buf, &env[4], netHTTPs_GetPassword());
          break;
      }
      break;

    case 'e':
      // Browser Language from 'language.cgi'
      lang = netHTTPs_GetLanguage();
      if      (strncmp (lang, "en", 2) == 0) {
        lang = "English";
      }
      else if (strncmp (lang, "de", 2) == 0) {
        lang = "German";
      }
      else if (strncmp (lang, "fr", 2) == 0) {
        lang = "French";
      }
      else if (strncmp (lang, "sl", 2) == 0) {
        lang = "Slovene";
      }
      else {
        lang = "Unknown";
      }
      len = sprintf (buf, &env[2], lang, netHTTPs_GetLanguage());
      break;

    case 'f':
      // LCD Module control from 'lcd.cgi'
      switch (env[2]) {
        case '1':
//          len = sprintf (buf, &env[4], lcd_text[0]);
          break;
        case '2':
//          len = sprintf (buf, &env[4], lcd_text[1]);
          break;
      }
      break;

    case 'g':
      // AD Input from 'ad.cgi'
      switch (env[2]) {
        case '1':
          adv = 1;
          len = sprintf (buf, &env[4], adv);
          break;
        case '2':
          len = sprintf (buf, &env[4], (float)adv*3.3f/4096);
          break;
        case '3':
          adv = (adv * 100) / 4096;
          len = sprintf (buf, &env[4], adv);
          break;
      }
      break;

    case 'x':
      // AD Input from 'ad.cgx'
      adv = 1;
      len = sprintf (buf, &env[1], adv);
      break;

    case 'y':
      // Button state from 'button.cgx'
      len = sprintf (buf, "<checkbox><id>button%c</id><on>%s</on></checkbox>",
                     env[1], (1 & (1 << (env[1]-'0'))) ? "true" : "false");
      break;
  }
  return (len);
}
 
/// \brief Override default Content-Type for CGX script files.
/// \return        pointer to user defined Content-Type.
const char *netCGX_ContentType (void) {
  /* Example
  return ("text/xml; charset=utf-8");
  */
  return (NULL);
}
 
/// \brief Override default character encoding in html documents.
/// \return        pointer to user defined character set type.
const char *netCGI_Charset (void) {
  /* Example
  return ("utf-8");
  */
  return (NULL);
}
//! [code_HTTP_Server_CGI]
