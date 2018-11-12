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

/** @addtogroup firmwave_F2_BLE
    * @{
    */



/** @defgroup protocol
    * @brief 命令处理函数
    * @{
    */
uint8_t const Key_Default[] = { 0xAF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };


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
BOOL isInitOK   = FALSE;

uint32_t AuthOK_TS = 0;
uint8_t  Init_Status[5];
uint8_t  SendData_flag = 0;

static uint8_t recStack[PROTOCOL_REC_STACK_MAX][BLE_NUS_MAX_DATA_LEN];



static protocol_fifo_t  ProtocolFifo;


/* Private function prototypes -----------------------------------------------*/
static void Protocol_Analyse(uint8_t* dat, uint8_t len);

static void Protocol_Cmd_Analy(uint8_t* dat, uint8_t len);

static void funProtocol(int argc, char* argv[]);

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
  CMD_ENT_DEF(protocol, funProtocol);
  Cmd_AddEntrance(CMD_ENT(protocol));
}

/**
    * @brief  命令接收处理轮询.
    */
void Protocol_DateProcPoll(void) {
  uint8_t* p = NULL, len = 0;

  if (STACK_LENGTH()) {
    p = &(ProtocolFifo.pStack[ProtocolFifo.rpos & ProtocolFifo.sizeMask][0]);
    len = *p++;
    ProtocolFifo.rpos++;
    Protocol_Analyse(p, len);
  }

}

/**
    * @brief  命令接收入队.
    */
void Protocol_NewDate(uint8_t* dat, uint8_t len) {
  uint8_t* p = NULL;

  if (STACK_LENGTH() <= ProtocolFifo.sizeMask) {
    p = &(ProtocolFifo.pStack[ProtocolFifo.wpos & ProtocolFifo.sizeMask][0]);       //这里将数据存在了ProtocolFifo里面。
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

  buf[0] = MSG_TYPE_CMD;
  buf[1] = cmd;
  buf[2] = len;
  if (arg != NULL && len > 0 && len <= 16) {
    memcpy(&buf[3], arg, len);
    buf[3 + len] = MSG_TYPE_DATA;
  }
  else buf[3] = MSG_TYPE_DATA;
  

  user_BLE_Send(buf, len + 4);

}



/**
    * @brief  命令接收处理轮询.
    * @param  none.
    * @retval none
    */
static void Protocol_Analyse(uint8_t* dat, uint8_t len) {
  
    int i;
    //char buf[5];
  for(uint8_t i=0;i<len;i++)  
      DBG_LOG("date is :dat[%u] = 0x%02x",i,dat[i]);
      //DBG_LOG("please enter a correct cmd!\n");
  if(dat[0] == 0x1A && dat[len - 1] == 0xA1 && dat[1] == 0x2F){
      Protocol_Cmd_Analy(dat, len);
      printf("charge time is 0x%02x",dat[2]);
      dat[1] = 0xF2;
      //user_BLE_Send();
      dat[1] = 0xFF;      
      user_BLE_Send(dat,len);
      //printf("time is : %d\n",dat[2]);
  }
  else{
    //  dat[5] = {0x1A,0xFF,0x78,0x40,0xA1};
      dat[0] = 0x1A;
      dat[1] = 0xFF;
      dat[2] = 0x78;
      dat[3] = 0x40;
      dat[4] = 0xA1;
          
      len = sizeof(dat)/sizeof(int);
      printf("len is %d\n",len);
      
      for(i=0;i<=len-1;i++)
      {
        printf("name = 0x%x ",dat[i]);
      }
    //  user_BLE_Send(dat,5);
  }

}
/**
 * 交换数组中的数据位置.
 */
void interte_array(uint8_t * array,uint8_t len){
    uint8_t temp[16],*p,length;
    length = len-1;
    p = array;
    for(uint8_t i=0;i<len;--length,++i){
      temp[i] = array[length];
    }
    memset(array,0,len);
    memcpy(p,temp, len);
}
/**
 * 字符数字对应的16进制的数字.
 */
uint8_t charTodata(char *str)
{
    uint32_t r = 0;

    if (str != NULL) {
        if(isdigit(*str)) {
            r = r * 10 + (*str - 0x30);
        }
    }
    return r;
}
/**
 * 一个16进制的字符数字对应的16进制的数字.
 */
uint8_t charTo16data(char *str)
{
    uint8_t n, r = 0;

    if (str == NULL) return 0;

    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) str += 2;
    for(uint8_t i=0;i<2;i++) {
        if (isdigit(*str)) n = *str - '0';
        else if (*str >= 'A' && *str <= 'F') n = *str - 'A' + 10;
        else if (*str >= 'a' && *str <= 'f') n = *str - 'a' + 10;
        else break;
        r = r * 16 + n;
        str++;
    }
    return r;
}
/**
 * 一个16进制的字符串对应一个数组的16进制的数字.
 */
uint8_t stringToarray(char *string, uint8_t *array){
    uint8_t length = 0,value = 0;
    while(string[value] != '\0'){
        *(array + length) = charTo16data(&string[value]);
        length++;
        value += 4;
    }
    return length;
}

			
/**
 * 单包命令处理
 * 
 * @param dat    输入的数据
 * @param len    数据长度
 */
static void Protocol_Cmd_Analy(uint8_t* dat, uint8_t len) {

  uint8_t cmd = 0,utc[4],Data[16],i = 0,length = 0;
  timeRTC_t time,current_time;
  
  cmd = dat[1];
  DBG_LOG("Receive command 0x%X.", cmd);

  switch (cmd) {
    /*校时*/
    case CMD_TIME_RALIB:
		DBG_LOG("timing ...... ");
		memcpy(utc, (uint8_t*)&dat[3], 4);
		RTC_TickToTime(*(uint32_t*)utc,&current_time);
		RTC_ReadTime(&time);
    if(current_time.year != time.year || current_time.seconds != time.seconds){

        RTC_SetCount(*(uint32_t*)utc);
        if((*(uint32_t*)utc) != RTC_ReadCount()){
            Data[0] = 0x00;
            Data[1] = BatvolADC;
            Data[2] = BatvolADC >> 8;
            Send_Cmd(0xC1,&Data[0],3);
            DBG_LOG("timing fail.....");
        }
        else{
            Data[0] = 0x01;
            Data[1] = BatvolADC;
            Data[2] = BatvolADC >> 8;
            Send_Cmd(0xC1,&Data[0],3);
            DBG_LOG("timing success.....");
        }
    }
    else{
        Data[0] = 0x55;
        Data[1] = BatvolADC;
        Data[2] = BatvolADC >> 8;
        Send_Cmd(0xC1,&Data[0],3);
        DBG_LOG("don't need timing.....");
    }
      break;
      /*产品名称命令*/
    case CMD_PRODUCT_NAME:
    DBG_LOG("product name is %s",WorkData.Product_name);
	  for(i=0;WorkData.Product_name[i] != '\0';i++){
			    length++;
				
      		Data[i] = WorkData.Product_name[i];
	  }
      interte_array(Data,i);
      Send_Cmd(0xC3,&Data[0],length);
      DBG_LOG("send product name");
      break;
      /*生产工厂名称命令*/
    case CMD_FACTORY_NAME:
     DBG_LOG("product name is %s",WorkData.Factory_name);
      for(i=0;WorkData.Factory_name[i] != '\0';i++)
      Data[i] = WorkData.Factory_name[i];
      interte_array(Data,i);
      Send_Cmd(0xC2,&Data[0],i);
      DBG_LOG("send factory name");
      break;
      /*设备序列号*/
    case CMD_DEVICE_ID:
     DBG_LOG("product name is %s",WorkData.DeviceID);
	  for(i=0;WorkData.DeviceID[i] != '\0';i++){
      		length++;
			Data[i] = WorkData.DeviceID[i];
	  }
      interte_array(Data,i);
      Send_Cmd(0xC4,Data,length);
      DBG_LOG("send device id");
      break;
      /*药品总剂量*/
    case CMD_TOTAL_DOSE:
      Data[0] = WorkData.Total_dose;
      Data[1] = WorkData.Total_dose >> 8;
      Send_Cmd(0xC5,&Data[0],2);
      DBG_LOG("send total dose");
      break;
      /*单次剂量*/
    case CMD_SINGLE_DOSE:
      Data[0] = WorkData.single_dose;
      Send_Cmd(0xC6,&Data[0],1);
      DBG_LOG("send single dose");
      break;
  case CMD_DATA:
        SendData_Start();
      SendData_flag = 0xAB;
      DBG_LOG("send data.......");
      break;
  case CMD_INTERRUPT:
        SendData_Stop();
        Data[0] = 0X01;
        Send_Cmd(0x45,&Data[0],1);
        DBG_LOG("interrupt send data");
      break;
            /*清除初始化数据*/
    case CMD_CLEAR_DATA:
      WorkData.StoreLog_In = 0;
      Clear_LogData();
      break;
    default:
      break;
  }
}


static void funProtocol(int argc, char* argv[]){
    argv++;
    argc--;
    if (ARGV_EQUAL("intertearray")){
      uint8_t array[4] = {4,5,1,2};
      for(uint8_t i=0;i<4;i++)  DBG_LOG(" array[%d] = %d ",i,array[i]);
      interte_array(array,4);
      for(uint8_t i=0;i<4;i++)  DBG_LOG(" array[%d] = %d ",i,array[i]);
    }
    else if (ARGV_EQUAL("stringToarray")){
      uint8_t array[16] = {0},len,i=0;
      len = stringToarray(argv[1],array);
      for(i=0;i<len;i++)  DBG_LOG(" array[%d] = 0x%x ",i,array[i]);
    }
    else if(ARGV_EQUAL("stringTo16data")){
        DBG_LOG("0x%x",charTo16data(argv[1]));
    }
    else if(ARGV_EQUAL("fun")){
      uint8_t data[16],len;
      len = stringToarray(argv[1],data);
      Protocol_Cmd_Analy(data,len);
    }
}

/************************ (C) COPYRIGHT  *****END OF FILE****/
