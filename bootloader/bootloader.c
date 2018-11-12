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
#include "bootloader.h"



/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/


/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


static pstorage_handle_t dfu_handle;

static uint32_t Write_Count;

DFU_Info_t DFUInfo;


/* Private function prototypes -----------------------------------------------*/


#if defined ( __ICCARM__ )
static inline void bootloader_reset(uint32_t start_addr);
#endif

static void pstorage_callback_handler(pstorage_handle_t* p_handle,
                                      uint8_t             op_code,
                                      uint32_t            result,
                                      uint8_t* p_data,
                                      uint32_t            data_len);
static uint32_t Wait_For_FlashOP(uint32_t op_in);



/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  初始化.
  * @param  none.
  * @retval none
  */
void Bootloader_Init(void) {
  pstorage_module_param_t param = { .cb = pstorage_callback_handler };
  pstorage_init();

  dfu_handle.block_id = APP_ADDRESS;
  pstorage_register(&param, &dfu_handle);

  DFUInfo = WDATA_DFU_INFO;
}

/**
  * @brief  跳转到APP.
  */
void Bootloader_app_jump(void) {
  bootloader_reset(APP_ADDRESS);
}

/**
 * 保存DFU信息到flash
 */
void Save_DFU_Info(void) {
  uint32_t addr = 0, pos = 0;

  addr = DFU_INFO_ADDR / 1024;
  addr *= 1024;

  pos = DFU_INFO_ADDR % 1024;

  memcpy(mulitiBuf, (uint8_t*)addr, 1024);
  memcpy(mulitiBuf + pos, &DFUInfo, sizeof(DFUInfo));

  dfu_handle.block_id = addr;
  pstorage_clear(&dfu_handle, 1024);
  Wait_For_FlashOP(PSTORAGE_CLEAR_OP_CODE);
  pstorage_store(&dfu_handle, mulitiBuf, 1024, 0);
  Wait_For_FlashOP(PSTORAGE_STORE_OP_CODE);
}

/**
  * @brief  校验APP.
  */
uint8_t Bootloader_app_status(void) {
  uint32_t crc;

  if (*((uint32_t*)APP_ADDRESS) == EMPTY_FLASH_MASK)
    return DFU_NO_IMAGE;

  if (DFUInfo.size == 0 || DFUInfo.size == EMPTY_FLASH_MASK || DFUInfo.crc == 0 || DFUInfo.crc == 0xFF)
    return DFU_FLAG_NONE;

  if (DFUInfo.size > DFU_TOTAL_SIZE)
    return DFU_FLASH_FAULT;

  crc = CRC_32(0, (uint8_t*)APP_ADDRESS, DFUInfo.size);

  if (crc != DFUInfo.crc)
    return  DFU_IMAGE_CRC_FAULT;

  return DFU_COMPLETE;
}

/**
  * @brief  擦除flash.
  */
void DFU_Clear(uint32_t size) {

  if (size == 0 || size > DFU_TOTAL_SIZE)
    size = DFU_TOTAL_SIZE;

  DBG_LOG("DFU erase start, size:%u.", size);
  dfu_handle.block_id = APP_ADDRESS;
  pstorage_clear(&dfu_handle, size);
  Wait_For_FlashOP(PSTORAGE_CLEAR_OP_CODE);

  DBG_LOG("DFU erase complited.");

  Write_Count = 0;
}

/**
  * @brief  结束DFU.
  */
void DFU_End(void) {
  uint8_t res;

  /* 延时以便flash写结束 */
  nrf_delay_ms(100);

  res = Bootloader_app_status();

  DBG_LOG("DFU end. System will reset, res:%d", res);

  user_BLE_Disconnected();
  nrf_delay_ms(100);
  NVIC_SystemReset();
}


/**
  * @brief  DFU写数据.
  */
void DFU_Write_Data(uint8_t* data, uint16_t size) {
  uint8_t rem = 0;
  uint32_t wsize;

  if (data != NULL && size > 0) {
    /*补齐字对齐*/
    wsize = size;
    rem = size % 4;
    if (rem > 0) {
      rem = 4 - rem;
      wsize += rem;
      memset(data + size, 0xFF, rem);
    }
    dfu_handle.block_id = APP_ADDRESS;
    pstorage_store(&dfu_handle, data, wsize, Write_Count);
    Wait_For_FlashOP(PSTORAGE_STORE_OP_CODE);
    Write_Count += size;

    LED_TOGGLE(R);
    LED_TP_TOGGLE();
  }
}



#if defined ( __ICCARM__ )
static inline void bootloader_reset(uint32_t start_addr) {
  asm("ldr   r5, [%0]\n"                    // Get App initial MSP for bootloader.
        "msr   msp, r5\n"                     // Set the main stack pointer to the applications MSP.
        "ldr   r0, [%0, #0x04]\n"             // Load Reset handler into R0.

        "movs  r4, #0x00\n"                   // Load zero into R4.
        "mvns  r4, r4\n"                      // Invert R4 to ensure it contain ones.

        "mrs   r5, IPSR\n"                    // Load IPSR to R5 to check for handler or thread mode
        "cmp   r5, #0x00\n"                   // Compare, if 0 then we are in thread mode and can continue to reset handler of bootloader.
        "bne   isr_abort\n"                   // If not zero we need to exit current ISR and jump to reset handler of bootloader.

        "mov   lr, r4\n"                      // Clear the link register and set to ones to ensure no return.
        "bx    r0\n"                          // Branch to reset handler of bootloader.

        "isr_abort: \n"
                                              // R4 contains ones from line above. We be popped as R12 when exiting ISR (Cleaning up the registers).
        "mov   r5, r4\n"                      // Fill with ones before jumping to reset handling. Will be popped as LR when exiting ISR. Ensures no return to application.
        "mov   r6, r0\n"                      // Move address of reset handler to R6. Will be popped as PC when exiting ISR. Ensures the reset handler will be executed when exist ISR.
        "movs  r7, #0x21\n"                   // Move MSB reset value of xPSR to R7. Will be popped as xPSR when exiting ISR. xPSR is 0x21000000 thus MSB is 0x21.
        "rev   r7, r7\n"                      // Reverse byte order to put 0x21 as MSB.
        "push  {r4-r7}\n"                     // Push everything to new stack to allow interrupt handler to fetch it on exiting the ISR.

        "movs  r4, #0x00\n"                   // Fill with zeros before jumping to reset handling. We be popped as R0 when exiting ISR (Cleaning up of the registers).
        "movs  r5, #0x00\n"                   // Fill with zeros before jumping to reset handling. We be popped as R1 when exiting ISR (Cleaning up of the registers).
        "movs  r6, #0x00\n"                   // Fill with zeros before jumping to reset handling. We be popped as R2 when exiting ISR (Cleaning up of the registers).
        "movs  r7, #0x00\n"                   // Fill with zeros before jumping to reset handling. We be popped as R3 when exiting ISR (Cleaning up of the registers).
        "push  {r4-r7}\n"                     // Push zeros (R4-R7) to stack to prepare for exiting the interrupt routine.

        "movs  r0, #0x06\n"                   // Load 0x06 into R6 to prepare for exec return command.
        "mvns  r0, r0\n"                      // Invert 0x06 to obtain EXEC_RETURN, 0xFFFFFFF9.
        "bx    r0\n"                          // No return - Handler mode will be exited. Stack will be popped and execution will continue in reset handler initializing other application.
        :: "r" (start_addr)                   // Argument list for the IAR assembly. start_addr is %0.
        :  "r0", "r4", "r5", "r6", "r7");     // List of register maintained manually.
}
#else
#error Compiler not supported.
#endif


/**@brief   Function for handling callbacks from pstorage module.
 *
 * @details Handles pstorage results for clear and storage operation. For detailed description of
 *          the parameters provided with the callback, please refer to \ref pstorage_ntf_cb_t.
 */
uint32_t volatile op = 0, res = 0;
static void pstorage_callback_handler(pstorage_handle_t* p_handle,
                                      uint8_t             op_code,
                                      uint32_t            result,
                                      uint8_t* p_data,
                                      uint32_t            data_len) {
  if (op == op_code) {
    res = result;
  }
}

static uint32_t Wait_For_FlashOP(uint32_t op_in) {
  op = op_in;
  res = 0xFFFFFFF;

  while (res == 0xFFFFFFF);

  return res;
}


/************************ (C) COPYRIGHT *****END OF FILE****/
