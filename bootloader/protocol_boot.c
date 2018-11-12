/**
    ******************************************************************************
    * @file    protocol.c
    * @author  宋阳
    * @version V1.0
    * @date    2015.11.31
    * @brief   命令处理相关函数.
    *
    ******************************************************************************
    */

/* Includes ------------------------------------------------------------------*/
#include "includes.h"
#include "ble_nus.h"
#include "bootloader.h"

/** @addtogroup firmwave_F2_BLE
    * @{
    */



/** @defgroup protocol
    * @brief 命令处理函数
    * @{
    */


/* Private typedef -----------------------------------------------------------*/

/** @defgroup protocol_Private_Typedef protocol Private Typedef
    * @{
    */
typedef struct
{
  uint8_t(* pStack)[BLE_NUS_MAX_DATA_LEN];
  uint16_t  sizeMask;
  uint16_t  rpos;
  uint16_t  wpos;
} protocol_fifo_t;

/**
    * @}
    */

/* Private define ------------------------------------------------------------*/


/* Private macros ------------------------------------------------------------*/

#define STACK_LENGTH()   (ProtocolFifo.wpos - ProtocolFifo.rpos)


/* Private variables ---------------------------------------------------------*/


BOOL isAuthOK = FALSE;
uint32_t AuthOK_TS = 0;
uint8_t mulitiBuf[2048];
uint16_t authRunIndex = 0, authStep = 0;
uint8_t Random_Running[8];
uint8_t Key_Running[16];

uint8_t const Key_Default[] = { 0xAF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };

static uint8_t recStack[PROTOCOL_REC_STACK_MAX][BLE_NUS_MAX_DATA_LEN], mulitiCMD = 0;
static uint16_t multiIndex = 0, dfuBufIndex = 0, dfuIndex = 0;

static protocol_fifo_t  ProtocolFifo;


/* Private function prototypes -----------------------------------------------*/
static void Protocol_Analyse(uint8_t* dat, uint8_t len);
static void Protocol_Auth(uint8_t* dat, uint8_t len);
static void Protocol_Cmd_Analy(uint8_t* dat, uint8_t len);
static void Protocol_Packet_Analy(uint8_t* dat, uint8_t len);

/* Exported functions ---------------------------------------------------------*/


/**
 * 初始化命令处理.
 */
void Protocol_Init(void) {
  /* 初始化FIFO */
  if (IS_POWER_OF_TWO(PROTOCOL_REC_STACK_MAX)) {
    ProtocolFifo.sizeMask = PROTOCOL_REC_STACK_MAX - 1;
    ProtocolFifo.rpos = 0;
    ProtocolFifo.wpos = 0;
    ProtocolFifo.pStack = recStack;
  }
}

/**
    * @brief  命令接收处理轮询.
    */
void Protocol_DateProcPoll(void) {
  uint8_t* p = NULL, len = 0, ack = 0;
  static BOOL volatile connect = FALSE;

  if (STACK_LENGTH()) {
    p = &(ProtocolFifo.pStack[ProtocolFifo.rpos & ProtocolFifo.sizeMask][0]);
    len = *p++;
    ProtocolFifo.rpos++;
    Protocol_Analyse(p, len);
  }

  /*超时未鉴权断开蓝牙连接*/
  if (BLE_Connect && RTC_ReadCount() - AuthOK_TS > BLE_CONNECT_TIMEOUT) {
    AuthOK_TS = RTC_ReadCount();
    user_BLE_Disconnected();
  }
  if (!BLE_Connect) {
    mulitiCMD = 0;
    isAuthOK = FALSE;
    AuthOK_TS = RTC_ReadCount();
    connect = FALSE;
  }
  /*上报BOOT就绪*/
  else if (isAuthOK && connect == FALSE) {
    ack = 0x21;
    Send_Cmd(CMD_DFU, &ack, 1);
    connect = TRUE;
  }
}

/**
    * @brief  命令接收入队.
    */
void Protocol_NewDate(uint8_t* dat, uint8_t len) {
  uint8_t* p = NULL;

  if (STACK_LENGTH() <= ProtocolFifo.sizeMask) {
    p = &(ProtocolFifo.pStack[ProtocolFifo.wpos & ProtocolFifo.sizeMask][0]);
    *p++ = len;
    memcpy(p, dat, len);
    ProtocolFifo.wpos++;
  }
}

/**
 * 发送命令
 * 
 * @param cmd    命令
 * @param arg    参数内容
 * @param len    参数长度
 */
void Send_Cmd(uint8_t cmd, uint8_t* arg, uint8_t len) {
  uint8_t buf[20];

  DBG_LOG("Send_Cmd:%#x, len:%u", cmd, len);
  buf[0] = cmd << 2;
  buf[0] |= MSG_TYPE_CMD;
  if (arg != NULL && len > 0 && len <= 19) {
    memcpy(&buf[1], arg, len);
  }
  len += 1;
  AuthOK_TS = RTC_ReadCount();
#if USE_AES == 1
  nrf_ecb_hal_data_t ecb_data;

  if (len > 16) {
    len = 16;
  }
  memset(ecb_data.cleartext, 0, 16);
  memcpy(ecb_data.cleartext, buf, len);
  memcpy(ecb_data.key, Key_Running, 16);
  sd_ecb_block_encrypt(&ecb_data);
  user_BLE_Send(ecb_data.ciphertext, 16);
#else
  user_BLE_Send(buf, len);
#endif
}

/**
 * 使用默认密钥发送命令
 * 
 * @param cmd    命令
 * @param arg    参数内容
 * @param len    参数长度
 */
void Send_CmdDefaultkey(uint8_t cmd, uint8_t* arg, uint8_t len) {
  nrf_ecb_hal_data_t ecb_data;
  uint8_t buf[20];

  DBG_LOG("Send_Cmd:%#x, len:%u", cmd, len);
  buf[0] = cmd << 2;
  buf[0] |= MSG_TYPE_CMD;
  if (arg != NULL && len > 0 && len <= 19) {
    memcpy(&buf[1], arg, len);
  }
  len += 1;
  AuthOK_TS = RTC_ReadCount();

  if (len > 16) {
    len = 16;
  }
  memset(ecb_data.cleartext, 0, 16);
  memcpy(ecb_data.cleartext, buf, len);
  memcpy(ecb_data.key, Key_Default, 16);
  sd_ecb_block_encrypt(&ecb_data);
  user_BLE_Send(ecb_data.ciphertext, 16);
}

/**
 * 发送多包数据
 * 
 * @param index  包序号
 * @param data   单包数据内容
 * @param len    单包数据长度
 */
void Send_MulitiPacket(uint16_t index, uint8_t* data, uint8_t len) {

  uint8_t buf[20];

  buf[0] = (uint8_t)(index << 2);
  buf[0] |= MSG_TYPE_CMD;
  buf[1] = (uint8_t)(index >> 6);

  if (data != NULL && len > 0 && len <= 18) {
    memcpy(&buf[1], data, len);
  }
  user_BLE_Send(buf, len + 1);
}

/**
 * 根据随机数从密码池中生成新的密钥
 * 
 * @param random 8byte随机数数组
 */
uint8_t* Create_Key(uint8_t* random) {
  int i;

  for (i = 0; i < 8; i++) {
    Key_Running[i] = WDATA_KEY[random[i] % 128];
    Key_Running[i + 8] = WDATA_KEY[(random[i] + random[3]) % 128];
  }
  return Key_Running;
}


/**
 * 生成8BYTE随机数
 * 
 * @return 返回随机数指针
 */
uint8_t* Create_Random(void) {
  uint8_t  available = 0;

  while (available < 8) {
    nrf_drv_rng_bytes_available(&available);
  }
  nrf_drv_rng_rand(Random_Running, 8);
  return Random_Running;
}

/**
 * AES解密数据
 * 
 * @param data   数据内容
 * @param key    密钥
 * @param len    数据长度，需为16的倍数
 */
void AesData_decrypt(uint8_t* data, uint8_t* key, uint16_t len) {
  int i;
  uint8_t buf[16];

  if (data != NULL && key != NULL && len >= 16 && len % 16 == 0) {
    for (i = 0; i < len; i += 16) {
      AES128_ECB_decrypt(data, key, buf);
      memcpy(data, buf, 16);
      data += 16;
    }
  }
}

/**
    * @brief  命令接收处理轮询.
    * @param  none.
    * @retval none
    */
static void Protocol_Analyse(uint8_t* dat, uint8_t len) {

  /*鉴权处理*/
  if (isAuthOK == FALSE && dat[0] == 0 && (dat[1] & 0x03) == MSG_TYPE_CMD) {
    dat++;
    len--;
    Protocol_Auth(dat, len);
  } else {
    /*多包数据*/
    if ((dat[0] & 0x03) == MSG_TYPE_DATA) {
      Protocol_Packet_Analy(dat, len);
    }
    /*单包命令*/
    else if (dat[0] == 0 && (dat[1] & 0x03) == MSG_TYPE_CMD) {
      dat++;
      len--;
#if USE_AES == 1
      AesData_decrypt(dat, Key_Running, 16);
#endif
      Protocol_Cmd_Analy(dat, len);
    }
    /*重置时间戳*/
    AuthOK_TS = RTC_ReadCount();
  }

  /*非法访问断开蓝牙*/
  if (isAuthOK == FALSE && authStep == 0) {
    DBG_LOG("Not auth disconnect.");
    user_BLE_Disconnected();
  }
}

/**
 * 鉴权处理
 * 
 * @param dat    接收到的数据
 * @param len    接收到的数据长度
 */
static void Protocol_Auth(uint8_t* dat, uint8_t len) {
  uint8_t rsp[8], cmd;

#if USE_AES == 1
  uint16_t run = 0;

  if (len != 16) {
    return;
  }

  if (authStep == 0) {
    AesData_decrypt(dat, (uint8_t*)Key_Default, 16);
  } else {
    AesData_decrypt(dat, Key_Running, 16);
  }
  cmd = dat[0] >> 2;
  if (cmd == CMD_AUTH) {
    AuthOK_TS = RTC_ReadCount();

    /*主机握手*/
    if (dat[1] == 0x01 && authStep == 0) {
      memcpy((uint8_t*)&run, &dat[10], 2);
      /*比较同步计数值*/
      if (run - authRunIndex >= 1 &&  run - authRunIndex < 5) {
        authStep = 1;
        rsp[0] = 0x01;
        memcpy(&rsp[1], Create_Random(), 8);
        Send_CmdDefaultkey(CMD_AUTH, rsp, 9);
        Create_Key(Random_Running);
        DBG_LOG("Device auth handshake.");
      } else {
        DBG_LOG("auth index falut, recevive:%u, store:%u", run, authRunIndex);
      }
      authRunIndex = run;
    }
    /*主机认证*/
    else if (dat[1] == 0x02 && authStep == 1) {
      if (memcmp(&dat[2], Random_Running, 8) == 0) {
        isAuthOK = TRUE;
        rsp[0] = 0x11;
        rsp[1] = VERSION;
        rsp[2] = VERSION >> 8;
        Send_Cmd(CMD_AUTH, rsp, 3);
        DBG_LOG("Device auth OK.");
      } else {
        DBG_LOG("auth random falut, recevive:%.*s, store:%.*s", 8, &dat[2], 8, Random_Running);
      }
      authStep = 0;
    }
  } else if (authStep != 0) {
    authStep = 0;
    DBG_LOG("Handshake Failed.");
  }
#else
  cmd = dat[0] >> 2;
  if (cmd == CMD_AUTH) {
    AuthOK_TS = RTC_ReadCount();
    isAuthOK = TRUE;
    rsp[0] = 0x11;
    rsp[1] = VERSION;
    rsp[2] = VERSION >> 8;
    Send_Cmd(CMD_AUTH, rsp, 3);
  }
#endif
}

/**
 * 单包命令处理
 * 
 * @param dat    输入的数据
 * @param len    数据长度
 */
static void Protocol_Cmd_Analy(uint8_t* dat, uint8_t len) {

  uint8_t cmd = 0, ack = 1, utc[4];

  /*命令处理*/
  cmd = dat[0] >> 2;
  dat++;
  len--;

  switch (cmd) {
    case CMD_DFU:
      memcpy(utc, &dat[0], 2);
      DFUInfo.newver = *(uint16_t*)utc;
      memcpy(utc, &dat[2], 4);
      DFUInfo.size = *(uint32_t*)utc;
      memcpy(utc, &dat[6], 4);
      DFUInfo.crc = *(uint32_t*)utc;

      DBG_LOG("DFU start, version:%u, size:%u, crc:%#x, ",
              DFUInfo.newver, DFUInfo.size, DFUInfo.crc);

      DFU_Clear(DFUInfo.size);
      ack = 0x31;
      Send_Cmd(CMD_DFU, &ack, 1);
      Save_DFU_Info();

      /*准备接收多包数据*/
      mulitiCMD = CMD_DFU;
      multiIndex = 0;
      dfuBufIndex = 0;
      dfuIndex = 0;
    default:
      break;
  }
}

/**
 * 多包数据处理
 * 
 * @param dat    接收到的数据
 * @param len    数据长度
 */
static void Protocol_Packet_Analy(uint8_t* dat, uint8_t len) {
  uint8_t rsp[6], ack;
  int16_t index = 0, i;

  index = (dat[0] >> 2) | (dat[1] << 6);
  dat += 2;
  len -= 2;
  if (index == multiIndex) {
    multiIndex++;
    if (dfuBufIndex <= 2048 - len) {
      memcpy(&mulitiBuf[dfuBufIndex], dat, len);
      dfuBufIndex += len;
    }
    /*写入到flash*/
    if (mulitiCMD == CMD_DFU) {
      /*最后一个包*/
      if (DFUInfo.size - dfuIndex == dfuBufIndex && dfuBufIndex <= 1024) {
#if USE_AES == 1
        AesData_decrypt(mulitiBuf, Key_Running, dfuBufIndex);
#endif
        DFU_Write_Data(mulitiBuf, dfuBufIndex);
        i = Bootloader_app_status();
        DBG_LOG("DFU receive OK, verify status:%#x", i);
        DBG_LOG("Image crc:%#x, DFU crc:%#x", CRC_32(0, (uint8_t*)APP_ADDRESS, DFUInfo.size), DFUInfo.crc);
        if (i == DFU_COMPLETE) {
          ack = 0x41;
          Send_Cmd(CMD_DFU, &ack, 1);
          /*重启进入APP*/
          nrf_delay_ms(50);
          user_BLE_Disconnected();
          nrf_delay_ms(10);
          NVIC_SystemReset();
        } else {
          ack = 0x40;
          Send_Cmd(CMD_DFU, &ack, 1);
        }
      } else if (dfuBufIndex >= 1024) {
        DBG_SEND(">", 1);
#if USE_AES == 1
        AesData_decrypt(mulitiBuf, Key_Running, 1024);
#endif
        DFU_Write_Data(mulitiBuf, 1024);
        dfuIndex += 1024;
        dfuBufIndex -= 1024;
        if (dfuBufIndex > 0) {
          memmove(&mulitiBuf[0], &mulitiBuf[1024], dfuBufIndex);
        }
      }
    }
  } else {
    DBG_LOG("MultiPacket index fault, multiIndex:%u, recindex:%u",
            multiIndex, index);
    rsp[0] = 0x60;
    rsp[1] = index;
    rsp[2] = index >> 8;
    Send_Cmd(mulitiCMD, &rsp[0], 3);
  }
}



/************************ (C) COPYRIGHT  *****END OF FILE****/
