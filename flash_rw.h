#ifndef _FLASH_RW_H
#define _FLASH_RW_H
#include <stdint.h>
#include <stdbool.h>

// �洢���ݿ�
typedef struct 
{
	// ��������
	uint32_t macAddrH;				// MAC��ַ
	uint32_t macAddrL;
	uint32_t ipAddr;					// IP��ַ
	uint32_t ipMask;					// ��������
	uint32_t ipGate;					// ���ص�ַ
	uint32_t ipPDns;					// ��DNS
	uint32_t ipSDns;					// ��DNS
	uint32_t remoteAddr;			// Զ�˷�����IP
	uint32_t udpPort;					// Զ�˷�����UDP�˿�
	uint32_t ifDhcp;					// �Ƿ�����DHCP
	// ̽��������
	uint32_t sensorNum;				// ̽����ģ������
	
	uint32_t checkSum;				// У���
}_dataInFlash;

void saveData(_dataInFlash*);
bool loadData(_dataInFlash*);
void saveNetworkData(void);

#endif /* _FLASH_RW_H */
