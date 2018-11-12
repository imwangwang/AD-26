/**
  ******************************************************************************
  * @file    fpg.h
  * @author  宋阳
  * @version V1.0
  * @date    2017.2.15
  * @brief   指纹传感器驱动函数头文件
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _FPG_H
#define _FPG_H

/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"

/* Exported define -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    uint8_t stsus_reg;
    uint8_t sensor;
    uint8_t memorysize;
    uint8_t security;
    uint32_t addr;
    uint8_t packetsize;
    uint8_t bdr;
} FPG_SysPara_t;

/* Exported constants --------------------------------------------------------*/
#define PS_GetImage 0x01    //从传感器上读入图像存于图像缓冲区
#define PS_GenChar 0x02     //根据原始图像生成指纹特征存于 CharBuffer1 或 CharBuffer2
#define PS_Match 0x03       //精确比对 CharBuffer1 与 CharBuffer2 中的特征文件
#define PS_Search 0x04      //以 CharBuffer1 或 CharBuffer2 中的特征文件搜索整个或部分指纹库
#define PS_RegModel 0x05    //将 CharBuffer1 与 CharBuffer2 中的特征文件合并生成模板存于 CharBuffer2
#define PS_StoreChar 0x06   //将特征缓冲区中的文件储存到 flash 指纹库中
#define PS_LoadChar 0x07    //从 flash 指纹库中读取一个模板到特征缓冲区
#define PS_UpChar 0x08      //将特征缓冲区中的文件上传给上位机
#define PS_DownChar 0x09    //从上位机下载一个特征文件到特征缓冲区
#define PS_UpImage 0x0a     //上传原始图像
#define PS_DownImage 0x0b   //下载原始图像
#define PS_DeletChar 0x0c   // 删除 flash 指纹库中的一个特征文件
#define PS_Empty 0x0d       //清空 flash 指纹库
#define PS_WriteReg 0x0e    //写模块系统寄存器
#define PS_ReadSysPara 0x0f //读系统基本参数
#define PS_Enroll 0x10      //注册模板
#define PS_Identify 0x11    //验证指纹
#define PS_SetPwd 0x12      //设置设备握手口令
#define PS_VfyPwd 0x13      //验证设备握手口令
#define PS_GetRandomCode 0x14 //采样随机数
#define PS_ReadINFpage 0x16         //读取 FLASH Information Page 内容
#define PS_HighSpeedSearch 0x1b     //高速搜索 FLASH
#define PS_ValidTempleteNum 0x1d   //读有效模板个数

#define PS_TYPE_CMD             0x01
#define PS_TYPE_DATA            0x02
#define PS_TYPE_DATA_LAST       0x08
#define PS_TYPE_ACK             0x07
#define PS_HEAD                 0xEF01
#define PS_ADDR_DEF             0xFFFFFFFF
#define PS_PWD_DEF              0x00

#define PACKET_SIZE_32          0
#define PACKET_SIZE_64          1
#define PACKET_SIZE_128         2
#define PACKET_SIZE_256         3

#define REG_BDR                 4
#define REG_LEAVEL              5
#define REG_PACKET_SIZE         6

/* Exported macro ------------------------------------------------------------*/
#define FPG_BDR(bdr)        (bdr / 9600)

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void FPG_init(void);
uint8_t FPG_PS_GetImage(void);
uint8_t FPG_PS_GenChar(uint8_t charbuffer);
uint8_t FPG_PS_Match(uint16_t *pscore);
uint8_t FPG_PS_Search(uint8_t charbuffer, uint16_t startpage, uint16_t pagenum, uint16_t *pageid, uint16_t *pscore);
uint8_t FPG_PS_RegModel(void);
uint8_t FPG_PS_StoreChar(uint8_t charbuffer, uint16_t pageid);
uint8_t FPG_PS_LoadChar(uint8_t charbuffer, uint16_t pageid);
uint8_t FPG_PS_UpChar(uint8_t charbuffer, uint8_t *data);
uint8_t FPG_PS_DownChar(uint8_t charbuffer, uint8_t *data);
BOOL FPG_PS_UpImage(void);
BOOL FPG_PS_DownImage(void);
uint8_t FPG_PS_DeletChar(uint16_t pageid, uint16_t number);
uint8_t FPG_PS_Empty(void);
uint8_t FPG_PS_WriteReg(uint8_t reg, uint8_t res);
BOOL FPG_PS_ReadSysPara(FPG_SysPara_t *par);
uint8_t FPG_PS_Enroll(uint16_t *pageid);
uint8_t FPG_PS_Identify(uint16_t *pageid, uint16_t *pscore);
uint8_t FPG_PS_SetPwd(uint32_t pwd);
uint8_t FPG_PS_VfyPwd(uint32_t pwd);
uint8_t FPG_PS_GetRandomCode(uint32_t *radom);
BOOL FPG_PS_ReadINFpage(void);
uint8_t FPG_PS_HighSpeedSearch(uint8_t charbuffer, uint16_t startpage, uint16_t pagenum, uint16_t *pageid, uint16_t *pscore);
uint8_t FPG_PS_ValidTempleteNum(uint16_t *number);
char* FPG_GetResultString(uint8_t retnum);

#endif


/************************ (C)  *****END OF FILE****/

