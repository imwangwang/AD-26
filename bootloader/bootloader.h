/**
  ******************************************************************************
  * @file    bootloader.h
  * @author  ËÎÑô
  * @version V1.0
  * @date    2015.12.31
  * @brief   Header file of bootloader
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _BOOT_H
#define _BOOT_H



/* Includes ------------------------------------------------------------------*/
#include "includes.h"


/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

#define DFU_NO_IMAGE                    0x00
#define DFU_FLAG_NONE                   0x01
#define DFU_FLASH_FAULT									0x02
#define DFU_IMAGE_CRC_FAULT             0x40
#define DFU_COMPLETE                    0x41


#define EMPTY_FLASH_MASK                0xFFFFFFFF

#define BOOTLOADER_ADDR                 0x1B000
#define APP_ADDRESS                     0x21000
#define DFU_TOTAL_SIZE                  0x13000
#define DFU_INFO_ADDR                   (0x3E000 + 12)
#define WDATA_DFU_INFO                  (*((DFU_Info_t*)DFU_INFO_ADDR))
#define WDATA_KEY                       ((uint8_t*)(0x3E000 + 12))

extern DFU_Info_t DFUInfo;

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void Bootloader_Init(void);
void Bootloader_app_jump(void);
void Save_DFU_Info(void);

uint8_t Bootloader_app_status(void);

void DFU_Clear(uint32_t size);
void DFU_End(void);
void DFU_Write_Data(uint8_t* data, uint16_t size);



#endif /* _WorkData_H */

/************************ (C)  *****END OF FILE****/

