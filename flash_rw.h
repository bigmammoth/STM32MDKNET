#ifndef _FLASH_RW_H
#define _FLASH_RW_H
#include <stdint.h>
#include <stdbool.h>

// 存储数据块
typedef struct 
{
	// 网络数据
	uint32_t macAddrH;				// MAC地址
	uint32_t macAddrL;
	uint32_t ipAddr;					// IP地址
	uint32_t ipMask;					// 子网掩码
	uint32_t ipGate;					// 网关地址
	uint32_t ipPDns;					// 主DNS
	uint32_t ipSDns;					// 从DNS
	uint32_t remoteAddr;			// 远端服务器IP
	uint32_t udpPort;					// 远端服务器UDP端口
	uint32_t ifDhcp;					// 是否启用DHCP
	// 探测器数据
	uint32_t sensorNum;				// 探测器模块数量
	
	uint32_t checkSum;				// 校验和
}_dataInFlash;

void saveData(_dataInFlash*);
bool loadData(_dataInFlash*);
void saveNetworkData(void);

#endif /* _FLASH_RW_H */
