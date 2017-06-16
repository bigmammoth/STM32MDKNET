#include "RTE_Device.h"
#include "stm32f10x_flash.h"
#include "flash_rw.h"
#include "rl_net.h" 

#define FLASH_SIZE 256         //所选MCU的FLASH容量大小(单位为K)

#if FLASH_SIZE < 256
  #define SECTOR_SIZE           1024    //字节
#else 
  #define SECTOR_SIZE           2048    //字节
#endif

// 定义数据存储页地址
#define START_ADDR				(FLASH_BASE+1024*(FLASH_SIZE-SECTOR_SIZE/1024))

//读取指定地址的半字(16位数据)
uint16_t FLASH_ReadHalfWord(uint32_t address)
{
  return *(__IO uint16_t*)address; 
}

//读取指定地址的全字(32位数据)
uint32_t FLASH_ReadWord(uint32_t address)
{
  uint32_t temp1,temp2;
  temp1=*(__IO uint16_t*)address; 
  temp2=*(__IO uint16_t*)(address+2); 
  return (temp2<<16)+temp1;
}

//从指定地址开始读取多个数据
void FLASH_ReadMoreData(uint32_t startAddress,uint16_t *readData,uint16_t countToRead)
{
  uint16_t dataIndex;
  for(dataIndex=0;dataIndex<countToRead;dataIndex++)
  {
    readData[dataIndex]=FLASH_ReadHalfWord(startAddress+dataIndex*2);
  }
}

//从指定地址开始写入多个数据
void FLASH_WriteMoreData(uint32_t startAddress,uint16_t *writeData,uint16_t countToWrite)
{
  if(startAddress < FLASH_BASE || ((startAddress+countToWrite*2) >= (FLASH_BASE+1024*FLASH_SIZE)))
  {
    return;//非法地址
  }
  FLASH_Unlock();         //解锁写保护
  uint32_t offsetAddress=startAddress-FLASH_BASE;               //计算去掉0X08000000后的实际偏移地址
  uint32_t sectorPosition=offsetAddress/SECTOR_SIZE;            //计算扇区地址，对于STM32F103VET6为0~255
  
  uint32_t sectorStartAddress=sectorPosition*SECTOR_SIZE+FLASH_BASE;    //对应扇区的首地址

  FLASH_ErasePage(sectorStartAddress);//擦除这个扇区
  
  uint16_t dataIndex;
  for(dataIndex=0; dataIndex < countToWrite; dataIndex++)
  {
    FLASH_ProgramHalfWord(startAddress+dataIndex*2,writeData[dataIndex]);
  }
  
  FLASH_Lock();//上锁写保护
}

/*!--------------------------------------------------------------------------
  * @brief  写入存储数据块到FLASH中
  * @param  指向数据块的指针
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
  * @brief  读取存储在FLASH中的数据块
  * @param  指向数据块的指针
  * @retval 如果校验不正确，则返回false，否则返回true
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
  * @brief  保存网络参数到FLASH中
  * @param  指向数据块的指针
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
