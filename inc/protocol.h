/**
  ******************************************************************************
  * @file    protocol.h
  * @author  宋阳
  * @version V1.0
  * @date    2015.12.31
  * @brief   Header file of protocol
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _protocol_H
#define _protocol_H



/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"


/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define MSG_TYPE_CMD	                              0xAA
#define MSG_TYPE_DATA			                          0x55

#define CMD_TIME_RALIB		                          0xB1
#define CMD_PRODUCT_NAME			                      0xB3
#define CMD_FACTORY_NAME			                      0xB2
#define CMD_DEVICE_ID			                          0xB4
#define CMD_CLEAR_DATA		                          0x58
#define CMD_TOTAL_DOSE		                          0xB5
#define CMD_SINGLE_DOSE		                          0xB7
#define CMD_DATA		                                0xB8
#define CMD_INTERRUPT                               0x29


/*协议接收处理，接收缓存栈条目的最大数量，需为2的冥次方值 */
#define PROTOCOL_REC_STACK_MAX                      16

/*分包数据缓存的最大长度*/
#define MULITI_PACKET_LEN_MAX		                    512

/*蓝牙连接后无数据超时断开时间，单位秒*/
#define BLE_CONNECT_TIMEOUT			                    180

extern BOOL isAuthOK;
extern uint32_t AuthOK_TS;
extern uint8_t mulitiBuf[];
extern uint8_t Random_Running[];
extern uint8_t Key_Running[];
extern BOOL isInitOK;
extern uint8_t  Init_Status[5];
extern uint8_t  SendData_flag;
/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void Protocol_Init(void);

void Protocol_NewDate(uint8_t *dat, uint8_t len);
void Protocol_DateProcPoll(void);

void Send_Cmd(uint8_t cmd, uint8_t *arg, uint8_t len);
void Send_CmdDefaultkey(uint8_t cmd, uint8_t* arg, uint8_t len);
void Send_MulitiPacket(uint16_t index, uint8_t *data, uint8_t len);

void Report_SetM1(BOOL ret, uint32_t uid);
void Report_SetFPG(BOOL ret, uint8_t pageid);
uint8_t* Create_Key(uint8_t* random);
uint8_t* Create_Random(void);
void AesData_decrypt(uint8_t* data, uint8_t* key, uint16_t len);
bool check_InitStatus(void);
#endif /* _protocol_H */

/************************ (C)  *****END OF FILE****/

