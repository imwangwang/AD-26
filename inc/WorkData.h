/**
  ******************************************************************************
  * @file    WorkData.h
  * @author  ����
  * @version V1.0
  * @date    2015.12.31
  * @brief   Header file of WorkData
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _WorkData_H
#define _WorkData_H



/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"
#include "pstorage.h"

/* Exported types ------------------------------------------------------------*/
/*�̼��汾,2BYTE*/
#define VERSION			                                        1


/*�����汾*/
#define WORKDATA_VERSION	                                  1
#define LOGDATA_VERSION	                                    300

/*������־������������*/
#define LOG_STORE_MAX				             300



/*Ĭ���������*/
#define HEARTBEAT_DEF                                       60

/*Ĭ���豸ID*/
#define DEVICEID_DEF                                        "XYZ000001"
#define PRODUCT_NAME                                        "ES01"
#define FATORY_NAME                                         "SHXY"


/*�����豸�����ṹ,����4�ֽڶ���*/
typedef struct
{
  uint16_t      crc; 
  uint16_t      version;
  uint16_t      StoreLog_In;
  uint8_t       sleep_state;
  uint16_t      Total_dose;
  uint8_t       single_dose;
  char          BLE_name[8];
  char          DeviceID[16];
  char          Product_name[16];
  char          Factory_name[16];
} WorkParam_t;

/*���廹ɡ��¼�ṹ*/
typedef struct
{
  uint32_t            time;
  float               temperature;
  uint16_t            key_num;
} StoreLog_t;

/* Exported constants --------------------------------------------------------*/
extern WorkParam_t        WorkData;
extern pstorage_handle_t  psLog;


/* Exported macro ------------------------------------------------------------*/
#define STROE_LOG_POINT(i)				((StoreLog_t*)((uint8_t *)psLog.block_id + sizeof(StoreLog_t) * i))

/* Exported functions --------------------------------------------------------*/
void WorkData_Init(void);
void DataBackInit(BOOL reset);
BOOL WorkData_Update(void);

BOOL Write_StoreLog(StoreLog_t *dat);
void Clear_LogData(void);
uint32_t Wait_For_FlashOP(uint32_t op_in);

#endif /* _WorkData_H */

/************************ (C) *****END OF FILE****/

