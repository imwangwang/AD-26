/**
  ******************************************************************************
  * @file    RTC.c
  * @author  ËÎÑô
  * @version V1.0
  * @date    2015.12.29
  * @brief   Header file of RTC
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RTC_H
#define __RTC_H



/* Includes ------------------------------------------------------------------*/
#include "prjlib.h"

/** @addtogroup firmwave_WTD_LED
  * @{
  */


/** @defgroup RTC
  * @{
  */


/* Exported types ------------------------------------------------------------*/
/** @defgroup RTC_Exported_Types RTC Exported Types
  * @{
  */
typedef struct
{
    uint16_t year;
    uint8_t day;
    uint8_t month;
    uint8_t date;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} timeRTC_t;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup RTC_Exported_Constants RTC Exported Constants
  * @{
  */
#define RTC_YEAR_BASE   1970

/**
  * @}
  */



/* Exported macro ------------------------------------------------------------*/
#define IS_RTC_TIME_CALIB() (RTC_ReadCount() > (2000 - RTC_YEAR_BASE) * 365 * 24 * 60 * 60)

#define IS_LEAP(y)      ((y % 4 == 0) && (y % 100 != 0)) || ((y % 100 == 00) && (y % 400 == 0))

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RTC_Exported_Functions
  * @{
  */
void RTC_Init(void);

uint32_t RTC_ReadCount(void);
void RTC_SetCount(uint32_t cnt);
void RTC_ReadTime(timeRTC_t *time);
void RTC_SetTime(timeRTC_t *time);
void RTC_TickToTime(uint32_t tick, timeRTC_t *time);
uint32_t RTC_TimeToTick(timeRTC_t *time);

/**
  * @}
  */


/**
  * @}
  */

/**
  * @}
  */



#endif /* _RTC_H */

/************************ (C)  *****END OF FILE****/

