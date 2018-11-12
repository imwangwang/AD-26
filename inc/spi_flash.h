/**
 * **********************************************************************
 *             Copyright (c) 2016 temp. All Rights Reserved.
 * @file spi_flash.h
 * @author 宋阳
 * @version V1.0
 * @date 2016.4.1
 * @brief spi flash驱动函数头文件.
 *
 * **********************************************************************
 * @note
 *
 * **********************************************************************
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _SPI_SFLASH_H
#define _SPI_SFLASH_H


/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"

/* Exported define -----------------------------------------------------------*/
/*flash交换区的大小*/
#define SECTOR_SWAP_NUM           10

/*flash保护扇区大小*/
#define SECTOR_SAFE_NUM           2

/*扇区与页的大小定义*/
#define SFLASH_SECTOR_SIZE        0x1000
#define SFLASH_PAGE_SIZE          0x100
#define SFLASH_32K_SIZE           0x8000
#define SFLASH_64K_SIZE           0x10000


/* Exported types ------------------------------------------------------------*/
/*flash 信息参数结构体*/
typedef struct
{
    uint8_t   ManFID;
    uint8_t   Type;
    uint8_t   DevID;
    uint32_t  Capacity;
    uint16_t  PageSize;
    uint16_t  SectorSize;
    uint16_t  SectorCount;
} SPI_Flash_Info_t;

/* Exported constants --------------------------------------------------------*/
#define SFLASH_CMD_WRITE          0x02  /*!< Write to Memory instruction */
#define SFLASH_CMD_WRSR           0x01  /*!< Write Status Register instruction */
#define SFLASH_CMD_WREN           0x06  /*!< Write enable instruction */

#define SFLASH_CMD_READ           0x03  /*!< Read from Memory instruction */
#define SFLASH_CMD_RDSR           0x05  /*!< Read Status Register instruction  */
#define SFLASH_CMD_RDID           0x9F  /*!< Read identification */

#define SFLASH_CMD_RDSR2          0x35

#define SFLASH_CMD_RPSUSP         0x75
#define SFLASH_CMD_RPRESU         0x7A

#define SFLASH_CMD_SE             0x20  /*!< Sector Erase instruction */
#define SFLASH_CMD_BE             0xC7  /*!< Bulk Erase instruction */

#define SFLASH_CMD_DUMMY          0xFF

#define SFLASH_CMD_PWR_DOWN         0xB9
#define SFLASH_CMD_RELEASE_PWR_DOWN 0xAB

#define SFLASH_CMD_SE             0x20  /*!< Sector Erase instruction */
#define SFLASH_CMD_32KE           0x52
#define SFLASH_CMD_64KE           0xD8
#define SFLASH_CMD_BE             0xC7  /*!< Bulk Erase instruction */

#define SFLASH_WIP_FLAG           0x01  /*!< Write In Progress (WIP) flag */


/* Exported macro ------------------------------------------------------------*/
#define SECTOR_ADDR(n)      (uint32_t)(SFLASH_SECTOR_SIZE * (n))

/* Exported variables --------------------------------------------------------*/
extern SPI_Flash_Info_t     SFlash_Info;

/* Exported functions --------------------------------------------------------*/
void SFlash_Init(void);

BOOL SFlash_GetSwapSector(uint32_t *addr);

BOOL SFlash_Read(uint32_t addr, uint8_t *rdat, uint32_t len);
BOOL SFlash_Write(uint32_t addr, uint8_t *wdat, uint32_t len);
BOOL SFlash_Write_NotCheck(uint32_t addr, uint8_t *wdat, uint32_t len);

BOOL SFlash_EraseChip(void);
BOOL SFlash_EraseSectors(uint32_t addr, uint16_t nums);
BOOL SFlash_EraseSectors_NotCheck(uint32_t addr, uint16_t nums);

BOOL SFlash_Sleep(void);
BOOL SFlash_Wakeup(void);

#endif


