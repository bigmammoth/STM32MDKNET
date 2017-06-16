
/*
 * Auto generated Run-Time-Environment Component Configuration File
 *      *** Do not modify ! ***
 *
 * Project: 'STM32F107MDKNET' 
 * Target:  'STM32F107' 
 */

#ifndef RTE_COMPONENTS_H
#define RTE_COMPONENTS_H


/*
 * Define the Device Header File: 
 */
#define CMSIS_device_header "stm32f10x.h"

#define RTE_CMSIS_RTOS2                 /* CMSIS-RTOS2 */
        #define RTE_CMSIS_RTOS2_RTX5            /* CMSIS-RTOS2 Keil RTX5 */
#define RTE_DEVICE_STDPERIPH_FLASH
#define RTE_DEVICE_STDPERIPH_FRAMEWORK
#define RTE_DEVICE_STDPERIPH_GPIO
#define RTE_DEVICE_STDPERIPH_RCC
#define RTE_Drivers_ETH_MAC0            /* Driver ETH_MAC0 */
#define RTE_Drivers_PHY_DP83848C        /* Driver PHY DP83848C */
#define RTE_Drivers_USART1              /* Driver USART1 */
        #define RTE_Drivers_USART2              /* Driver USART2 */
        #define RTE_Drivers_USART3              /* Driver USART3 */
        #define RTE_Drivers_USART4              /* Driver UART4  */
        #define RTE_Drivers_USART5              /* Driver UART5  */
#define RTE_Network_Core                /* Network Core */
          #define RTE_Network_IPv4                /* Network IPv4 Stack */
          #define RTE_Network_Release             /* Network Release Version */
#define RTE_Network_Interface_ETH_0     /* Network Interface ETH 0 */
#define RTE_Network_Socket_TCP          /* Network Socket TCP */
#define RTE_Network_Socket_UDP          /* Network Socket UDP */
#define RTE_Network_Web_Server_RO       /* Network Web Server with Read-only Web Resources */

#endif /* RTE_COMPONENTS_H */
