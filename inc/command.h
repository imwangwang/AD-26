/**
  ******************************************************************************
  * @file    COMMAND.h
  * @author  ËÎÑô
  * @version V1.0
  * @date    2015.12.31
  * @brief   Header file of command
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _COMMAND_H
#define _COMMAND_H



/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"

/** @addtogroup firmwave_F1_MCU
  * @{
  */


/** @defgroup COMMAND
  * @{
  */


/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

/** @defgroup COMMAND_Exported_Constants COMMAND Exported Constants
  * @{
  */
#define COMMAND_MAX        UART_RX_BUF_SIZE

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMMAND_Exported_Functions
  * @{
  */
void Command_Init(void);

void CommandReceive_Poll(void);


/**
  * @}
  */



/**
  * @}
  */

/**
  * @}
  */



#endif /* _COMMAND_H */

/************************ (C)  *****END OF FILE****/

