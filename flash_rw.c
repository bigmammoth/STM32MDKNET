#include "RTE_Device.h"
#include "stm32f10x_flash.h"
#include "flash_rw.h"
#include "rl_net.h" 

#define FLASH_SIZE 256         //��ѡMCU��FLASH������С(��λΪK)

#if FLASH_SIZE < 256
  #define SECTOR_SIZE           1024    //�ֽ�
#else 
  #define SECTOR_SIZE           2048    //�ֽ�
#endif

// �������ݴ洢ҳ��ַ
#define START_ADDR				(FLASH_BASE+1024*(FLASH_SIZE-SECTOR_SIZE/1024))

//��ȡָ����ַ�İ���(16λ����)
uint16_t FLASH_ReadHalfWord(uint32_t address)
{
  return *(__IO uint16_t*)address; 
}

//��ȡָ����ַ��ȫ��(32λ����)
uint32_t FLASH_ReadWord(uint32_t address)
{
  uint32_t temp1,temp2;
  temp1=*(__IO uint16_t*)address; 
  temp2=*(__IO uint16_t*)(address+2); 
  return (temp2<<16)+temp1;
}

//��ָ����ַ��ʼ��ȡ�������
void FLASH_ReadMoreData(uint32_t startAddress,uint16_t *readData,uint16_t countToRead)
{
  uint16_t dataIndex;
  for(dataIndex=0;dataIndex<countToRead;dataIndex++)
  {
    readData[dataIndex]=FLASH_ReadHalfWord(startAddress+dataIndex*2);
  }
}

//��ָ����ַ��ʼд��������
void FLASH_WriteMoreData(uint32_t startAddress,uint16_t *writeData,uint16_t countToWrite)
{
  if(startAddress < FLASH_BASE || ((startAddress+countToWrite*2) >= (FLASH_BASE+1024*FLASH_SIZE)))
  {
    return;//�Ƿ���ַ
  }
  FLASH_Unlock();         //����д����
  uint32_t offsetAddress=startAddress-FLASH_BASE;               //����ȥ��0X08000000���ʵ��ƫ�Ƶ�ַ
  uint32_t sectorPosition=offsetAddress/SECTOR_SIZE;            //����������ַ������STM32F103VET6Ϊ0~255
  
  uint32_t sectorStartAddress=sectorPosition*SECTOR_SIZE+FLASH_BASE;    //��Ӧ�������׵�ַ

  FLASH_ErasePage(sectorStartAddress);//�����������
  
  uint16_t dataIndex;
  for(dataIndex=0; dataIndex < countToWrite; dataIndex++)
  {
    FLASH_ProgramHalfWord(startAddress+dataIndex*2,writeData[dataIndex]);
  }
  
  FLASH_Lock();//����д����
}

/*!--------------------------------------------------------------------------
  * @brief  д��洢���ݿ鵽FLASH��
  * @param  ָ�����ݿ��ָ��
  * @retval None
  *-------------------------------------------------------------------------*/
void saveData(_dataInFlash* dataInFl)
{
	for(int i = 0; i < sizeof(_dataInFlash)/2-2; i++)
	{
		dataInFl->checkSum += ((uint16_t*)dataInFl)[i];
	}
	FLASH_WriteMoreData(START_ADDR, (uint16_t*)dataInFl, sizeof(_dataInFlash)/2);
}

/*!--------------------------------------------------------------------------
  * @brief  ��ȡ�洢��FLASH�е����ݿ�
  * @param  ָ�����ݿ��ָ��
  * @retval ���У�鲻��ȷ���򷵻�false�����򷵻�true
  *-------------------------------------------------------------------------*/
bool loadData(_dataInFlash* dataInFl)
{
	FLASH_ReadMoreData(START_ADDR, (uint16_t*)dataInFl, sizeof(_dataInFlash)/2);
	uint32_t checkSum = 0;
	for(int i = 0; i < sizeof(_dataInFlash)/2-2; i++)
	{
		checkSum += ((uint16_t*)dataInFl)[i];
	}
	if(checkSum == dataInFl->checkSum && checkSum != 0) return true;
	else return false;
}

/*!--------------------------------------------------------------------------
  * @brief  �������������FLASH��
  * @param  ָ�����ݿ��ָ��
  * @retval None
  *-------------------------------------------------------------------------*/
extern uint8_t remoteServerIP[];
extern uint16_t remoteUdpPort;
extern bool ifDhcp;
void saveNetworkData(void)
{
	_dataInFlash dataInFlash;
	if(!loadData(&dataInFlash))
	{
		for(int i = 0; i < sizeof(dataInFlash)/4; i++)
			((uint32_t*)&dataInFlash)[i] = 0;
	}
	
	uint8_t ip4_addr[NET_ADDR_IP4_LEN];
	if(netIF_GetOption(NET_IF_CLASS_ETH, netIF_OptionIP4_Address,
		ip4_addr, NET_ADDR_IP4_LEN) == netOK)
	{
		dataInFlash.ipAddr = 0;
		for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
			dataInFlash.ipAddr |= ip4_addr[i] << ((NET_ADDR_IP4_LEN-i-1)*8);
	}
	
	if(netIF_GetOption(NET_IF_CLASS_ETH, netIF_OptionIP4_SubnetMask,
		ip4_addr, NET_ADDR_IP4_LEN) == netOK)
	{
		dataInFlash.ipMask = 0;
		for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
			dataInFlash.ipMask |= ip4_addr[i] << ((NET_ADDR_IP4_LEN-i-1)*8);
	}

	if(netIF_GetOption(NET_IF_CLASS_ETH, netIF_OptionIP4_DefaultGateway,
		ip4_addr, NET_ADDR_IP4_LEN) == netOK)
	{
		dataInFlash.ipGate = 0;
		for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
			dataInFlash.ipGate |= ip4_addr[i] << ((NET_ADDR_IP4_LEN-i-1)*8);
	}

	if(netIF_GetOption(NET_IF_CLASS_ETH, netIF_OptionIP4_PrimaryDNS,
		ip4_addr, NET_ADDR_IP4_LEN) == netOK)
	{
		dataInFlash.ipPDns = 0;
		for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
			dataInFlash.ipPDns |= ip4_addr[i] << ((NET_ADDR_IP4_LEN-i-1)*8);
	}

	if(netIF_GetOption(NET_IF_CLASS_ETH, netIF_OptionIP4_SecondaryDNS,
		ip4_addr, NET_ADDR_IP4_LEN) == netOK)
	{
		dataInFlash.ipSDns = 0;
		for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
			dataInFlash.ipSDns |= ip4_addr[i] << ((NET_ADDR_IP4_LEN-i-1)*8);
	}

	dataInFlash.remoteAddr = 0;
	for(int i = 0; i < NET_ADDR_IP4_LEN; i++)
		dataInFlash.remoteAddr |= remoteServerIP[i] << ((NET_ADDR_IP4_LEN-i-1)*8);
	
	dataInFlash.udpPort = remoteUdpPort;

	uint8_t mac_addr[NET_ADDR_ETH_LEN];
	if(netIF_GetOption(NET_IF_CLASS_ETH, netIF_OptionMAC_Address,
		mac_addr, NET_ADDR_ETH_LEN) == netOK)
	{
		dataInFlash.macAddrH = 0; dataInFlash.macAddrL = 0;
		for(int i = 0; i < NET_ADDR_ETH_LEN; i++)
		{
			if(i < 4)
				dataInFlash.macAddrH |= mac_addr[i] << ((NET_ADDR_ETH_LEN-i-3)*8);
			else
				dataInFlash.macAddrL |= mac_addr[i] << ((NET_ADDR_ETH_LEN-i+1)*8);
		}
	}
	
	dataInFlash.ifDhcp = ifDhcp;
	saveData(&dataInFlash);
}
