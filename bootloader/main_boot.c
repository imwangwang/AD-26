/**
  ******************************************************************************
  * @file    main.c
  * @author  宋阳
  * @version V1.0
  * @date    2015.12.2
  * @brief   智能狗碗main函数.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "nordic_common.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_delay.h"
#include "nrf_drv_wdt.h"
#include "nrf_sdm.h"
#include "nrf_nvmc.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_trace.h"
#include "app_timer.h"
#include "app_error.h"
#include "includes.h"
#include "bootloader.h"


/** @addtogroup firmwave_F2_BLE
  * @{
  */



/** @defgroup MAIN
  * @brief 主函数文件
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/** @defgroup main_Private_Variables Private Variables
  * @{
  */
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup main_Private_Functions main Private Functions
  * @{
  */
static void wdt_init(uint32_t value);
static void FlashWRPProcess(void);

#if WTD_EN == 1
static void wdt_event_handler(void);
#endif

/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/** @defgroup main_Exported_Functions main Exported Functions
  *  @brief   main 外部函数
  * @{
  */

#include "nrf_uart.h"

/**
  * @brief  main函数.
  * @param  none.
  * @retval none.
  */
int main(void) {
  uint8_t res;

  Bootloader_Init();
  res = Bootloader_app_status();
  if (res == DFU_FLAG_NONE || res == DFU_COMPLETE) {
    sd_softdevice_vector_table_base_set(APP_ADDRESS);
    Bootloader_app_jump();
    NVIC_SystemReset();
  }

  wdt_init(100000);
  FlashWRPProcess();
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
  BSP_Init();
#if DEBUG == 1
  user_uart_init();
#endif
  RTC_Init();
  user_BLE_Start();
  user_BLE_Connected();
  nrf_drv_rng_init(NULL);

  sd_ble_gap_tx_power_set(4);
  sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
  Protocol_Init();

  LED_ON(R);
  LED_OFF(G);
  LED_TP_ON();
  DBG_LOG("Bootloader Start, res:%#x, version:%u, size:%u, crc:%#x.", res,
          WDATA_DFU_INFO.newver, WDATA_DFU_INFO.size, WDATA_DFU_INFO.crc);
  for (;;) {
    WatchDog_Clear();
    /* 进入休眠 */
    sd_app_evt_wait();

    Protocol_DateProcPoll();
  }
}



/**
  * @brief  错误抛出函数.
  * @param  error_codr: 错误代码.
  * @param  line_num: 错误发生的行数.
  * @param  p_file_name: 错误发生的文件名.
  * @retval none.
  */
uint32_t m_error_code;
uint32_t m_line_num;
const uint8_t* m_p_file_name;

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
  // On assert, the system can only recover with a reset.


  m_error_code = ((error_info_t*)info)->err_code;
  m_line_num = ((error_info_t*)info)->line_num;
  m_p_file_name = ((error_info_t*)info)->p_file_name;

  UNUSED_VARIABLE(m_error_code);
  UNUSED_VARIABLE(m_line_num);
  UNUSED_VARIABLE(m_p_file_name);

  DBG_LOG("APP Error handler: Code: 0x%X  line: %d file: %s", m_error_code, m_line_num, m_p_file_name);
  nrf_delay_ms(100);
  DBG_LOG("APP Error handler: Code: 0x%X  line: %d file: %s", m_error_code, m_line_num, m_p_file_name);
  nrf_delay_ms(100);

  NVIC_SystemReset();

  __disable_irq();
  while (1);
}

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t* p_file_name) {
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**
  * @}
  */

/** @addtogroup main_Private_Functions
  * @{
  */
/**
  * @brief  看门狗初始化.
  * @param  value: 看门狗溢出时间.
  * @retval none.
  */
static void wdt_init(uint32_t value) {
#if WTD_EN == 1
  static nrf_drv_wdt_channel_id m_channel_id;

  nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;

  if (value > 0) config.reload_value = value;

  nrf_drv_wdt_init(&config, wdt_event_handler);
  nrf_drv_wdt_channel_alloc(&m_channel_id);
  nrf_drv_wdt_enable();

  nrf_drv_wdt_feed();
#endif
}

/**
  * @brief  FLASH读写保护初始化.
  * @param  none.
  * @retval none.
  */
static void FlashWRPProcess(void) {
#if FLASH_WRP == 1
  if (NRF_UICR->RBPCONF != 0) {
    nrf_nvmc_write_word((uint32_t)(&NRF_UICR->RBPCONF), 0);
    user_BLE_Disconnected();

    NVIC_SystemReset();
  }
#endif
}

/**
  * @brief  看门狗溢出中断处理.
  * @param  none.
  * @retval none.
  */
#if WTD_EN == 1

static void wdt_event_handler(void) {

}
#endif

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


