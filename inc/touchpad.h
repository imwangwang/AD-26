/**
  ******************************************************************************
  * @file    touchpad.h
  * @author  宋阳
  * @version V1.0
  * @date    2017.12.15
  * @brief   触摸按键驱动函数头文件
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _TOUCHPAD_H
#define _TOUCHPAD_H

/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"

/* Exported define -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define KEY_1			(1 << (3 - 1))
#define KEY_2			(1 << (2 - 1))
#define KEY_3			(1 << (1 - 1))
#define KEY_4			(1 << (6 - 1))
#define KEY_5			(1 << (5 - 1))
#define KEY_6			(1 << (4 - 1))
#define KEY_7			(1 << (7 - 1))
#define KEY_8			(1 << (8 - 1))
#define KEY_9			(1 << (9 - 1))
#define KEY_0			(1 << (12 - 1))
#define KEY_2A		(1 << (10 - 1)) 		/* *键 */
#define KEY_23		(1 << (11 - 1))	 		/* #键 */


/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void TouchPad_init(void);
uint16_t TouchPad_SacnKeys(void);
char TouchPad_ReadKey(void);
void TouchPad_WriteThreshold(uint8_t thd);

#endif


/************************ (C)  *****END OF FILE****/

