/**
  ******************************************************************************
  * @file    WorkData.c
  * @author  宋阳
  * @version V1.0
  * @date    2016.1.4
  * @brief   工作数据存储相关函数.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "includes.h"
/** @addtogroup firmwave_F2_BLE
  * @{
  */



/** @defgroup WorkData
  * @{
  */


/* Private typedef -----------------------------------------------------------*/

/** @defgroup WorkData_Private_Typedef WorkData Private Typedef
  * @{
  */

/**
  * @}
  */

/* Private define ------------------------------------------------------------*/
/** @defgroup WorkData_Private_Constants WorkData Private Constants
  * @{
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup WorkData_Private_Macros WorkData Private Macros
  * @{
  */


/**
  * @}
  */
/* Private variables ---------------------------------------------------------*/

/** @defgroup WorkData_Private_Variables Private Variables
  * @{
  */
pstorage_handle_t  psLog, psWorkData;

uint32_t volatile op = 0, res = 0;


WorkParam_t       WorkData;
StoreLog_t        LogData;
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup WorkData_Private_Functions WorkData Private Functions
  * @{
  */
static void pstorage_cb_handler(pstorage_handle_t* p_handle,
                                uint8_t op_code, uint32_t result, uint8_t* p_data, uint32_t data_len);

static void funWorkBack(int argc, char* argv[]);


/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/** @defgroup WorkData_Exported_Functions WorkData Exported Functions
  *  @brief   WorkData 外部接口函数
  * @{
  */

/**
  * @brief  工作数据初始化.
  * @param  none.
  * @retval none
  */
void WorkData_Init(void) {
  pstorage_module_param_t param;

  pstorage_init();

  /*注册pstorage */
  param.block_size = sizeof(StoreLog_t);
  param.block_count = LOG_STORE_MAX;
  param.cb = pstorage_cb_handler;
  pstorage_register(&param, &psLog);
  DBG_LOG("psLog id:%#x", psLog.block_id);

  param.block_size =PSTORAGE_FLASH_PAGE_SIZE;
  param.block_count = 1;
  param.cb = pstorage_cb_handler;
  pstorage_register(&param, &psWorkData);
  DBG_LOG("psWorkData id:%#x", psWorkData.block_id);

  DataBackInit(FALSE);

  CMD_ENT_DEF(workdata, funWorkBack);
  Cmd_AddEntrance(CMD_ENT(workdata));

  DBG_LOG("WorkData Init.");
}

/**
 * 工作数据存储更新
 */
BOOL WorkData_Update(void) {
  uint16_t crc = 0;

  crc = CRC_16(0, ((uint8_t*)&WorkData) + 2, sizeof(WorkData) - 2);
  WorkData.crc = crc;

  if (pstorage_update(&psWorkData, (uint8_t*)&WorkData, sizeof(WorkData), 0) == NRF_SUCCESS) {
    Wait_For_FlashOP(PSTORAGE_UPDATE_OP_CODE);
    DBG_LOG("Work Param Update OK.");
    return TRUE;
  }
  return FALSE;
}
/**
  * @brief  通用数据初始化.
  */
void DataBackInit(BOOL reset) {
  uint16_t crc = 0;
  uint16_t ver = 0;
  if (reset) {
    ver = 0;
    DBG_LOG("Work Data Factory Reset.");
  }
  if (pstorage_load((uint8_t*)&WorkData, &psWorkData, sizeof(WorkData), 0) == NRF_SUCCESS) {
    ver = WorkData.version;
    if (ver == 0xFFFF) {
      ver = 0;
      DBG_LOG("Work Data Not Init.");
    }
    crc = CRC_16(0, ((uint8_t*)&WorkData) + 2, sizeof(WorkData) - 2);
    if (crc != WorkData.crc) {
      ver = 0;
      DBG_LOG("Work Data CRC Fault.");
    }
    if (ver > WORKDATA_VERSION) {
      ver = 0;
    }
    if (ver != WORKDATA_VERSION) {
      if (ver < 1) {
        WorkData.version = WORKDATA_VERSION;
        WorkData.Total_dose = 400;
        WorkData.single_dose = 100;
        WorkData.StoreLog_In = 0;
        WorkData.sleep_state = 0;
		memcpy(WorkData.BLE_name,DEVICE_NAME, sizeof(DEVICE_NAME));
        memcpy(WorkData.DeviceID,DEVICEID_DEF, sizeof(DEVICEID_DEF));
        memcpy(WorkData.Factory_name,FATORY_NAME, sizeof(FATORY_NAME));

        WorkData_Update();
        DBG_LOG("Work version1 default.");
      }
    } else {
	  memcpy(WorkData.Product_name,WorkData.BLE_name, sizeof(WorkData.BLE_name));
      DBG_LOG("Work data load OK.");
    }
  }

}

/**
 * 记录压次、数据和温度数据
 * 
 * @param dat   存储的数据
 * @return 返回存储结果
 */
BOOL Write_StoreLog(StoreLog_t *dat) {
  BOOL ret = FALSE;
  StoreLog_t log;
  pstorage_handle_t mps;
  uint16_t logBlock = WorkData.StoreLog_In;

  if (IS_RTC_TIME_CALIB()) {
    DBG_LOG("logBlock = %u",logBlock);
    logBlock++;
    /*越界回滚*/
    if (logBlock >= LOG_STORE_MAX) {
      logBlock = 0;
    }
    log.time = dat->time;
    log.temperature = dat->temperature;
    log.key_num = dat->key_num;
    pstorage_block_identifier_get(&psLog, logBlock, &mps);
    DBG_LOG("mps = 0x%02x",mps.block_id);
    if (pstorage_update(&mps, (uint8_t*)&log, sizeof(StoreLog_t), 0) == NRF_SUCCESS) {
      Wait_For_FlashOP(PSTORAGE_UPDATE_OP_CODE);
      ret = TRUE;
      DBG_LOG("Write_StoreLog index:%u, time:%u, key_num:%u,temperature:%f",
              logBlock, log.time, log.key_num,log.temperature);
     WorkData.StoreLog_In = logBlock;
      WorkData_Update();
    }
  }
  return ret;
}

void Clear_LogData(void){
  WorkData.version = 0;
  WorkData.StoreLog_In = 0;
   WorkData_Update();
	pstorage_clear(&psWorkData,sizeof(WorkParam_t));
	pstorage_clear(&psLog,sizeof(StoreLog_t));
	nrf_delay_ms(200);
	NVIC_SystemReset();
}
/**
 * 等等flash擦写完成，用于非阻塞的方式实现flash操作，pstorage是非阻塞的
 * 
 * @param op_in  等待的动作
 * @return 返回结果
 */
uint32_t Wait_For_FlashOP(uint32_t op_in) {
  op = op_in;
  res = 0xFFFFFFF;

  while (res == 0xFFFFFFF);

  return res;
}

/** @addtogroup WorkData_Private_Functions
  * @{
  */

static void pstorage_cb_handler(pstorage_handle_t* p_handle,
  uint8_t op_code, uint32_t result, uint8_t* p_data, uint32_t data_len) {
  if (op == op_code) {
    res = result;
  }
}

/**
  * @brief  备份数据调试命令.
  */
static void funWorkBack(int argc, char* argv[]) {
  argv++;
  argc--;
    
  StoreLog_t* log;
  timeRTC_t time;
  if (ARGV_EQUAL("readlog")) {
    log = STROE_LOG_POINT(uatoi(argv[1]));
    RTC_TickToTime(log->time,&time);
    DBG_LOG("time: %04d-%d-%d %02d:%02d:%02d day-%d ", time.year,
                time.month, time.date, time.hours, time.minutes,
                time.seconds, time.day);
    DBG_LOG("readlog key_num :%u.",log->key_num);
    DBG_LOG("log->time :%u.",log->time);
    DBG_LOG("readlog temperature :%f.",log->temperature);
  }else if (ARGV_EQUAL("deviceid")) {
    if (argv[1] != NULL) {
      strcpy(WorkData.DeviceID, argv[1]);
      WorkData_Update();
    }
    DBG_LOG("Device ID:%s", WorkData.DeviceID);
  }
  else if (ARGV_EQUAL("singledose")) {
    if (argv[1] != NULL) {
      WorkData.single_dose = uatoi(argv[1]);
      WorkData_Update();
    }
    DBG_LOG("single_dose:%u", WorkData.single_dose);
  }
  else if (ARGV_EQUAL("totaldose")) {
    if (argv[1] != NULL) {
      WorkData.Total_dose = uatoi(argv[1]);
      WorkData_Update();
    }
    DBG_LOG("total dose :%u", WorkData.Total_dose);
  }
  else if (ARGV_EQUAL("productname")) {
    if (argv[1] != NULL) {
       strcpy(WorkData.Product_name, argv[1]);
      WorkData_Update();
    }
    DBG_LOG("product name:%s", WorkData.Product_name);
  }
  else if (ARGV_EQUAL("factoryname")) {
    if (argv[1] != NULL) {
       strcpy(WorkData.Factory_name, argv[1]);
      	WorkData_Update();
    }
    DBG_LOG("factory name:%s", WorkData.Factory_name);
  }
  else if (ARGV_EQUAL("clear_logdata")) {
    Clear_LogData();
  }
  else if(ARGV_EQUAL("lognumber")){
		DBG_LOG("Store log is %u",WorkData.StoreLog_In);
  }
   else if(ARGV_EQUAL("ble_name")){
	 if (argv[1] != NULL) {
       strcpy(WorkData.BLE_name, argv[1]);
      	WorkData_Update();
    }
	DBG_LOG("ble name is %s",WorkData.BLE_name);
  }
  else if(ARGV_EQUAL("version")){
	if (argv[1] != NULL) {
	  WorkData.version = uatoi(argv[1]);
      	WorkData_Update();
    }
		DBG_LOG("version is %u",WorkData.version);
  }
}


/**
  * @}
  */



/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT  *****END OF FILE****/
