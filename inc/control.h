/**
  ******************************************************************************
  * @file    control.c
  * @author  宋阳
  * @version V1.0
  * @date    2015.12.4
  * @brief   led driver.
  * @brief   Header file of Control
  ******************************************************************************
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _Control_H
#define _Control_H

/* Includes ------------------------------------------------------------------*/
#include "includes.h"

/** @addtogroup firmwave_F2_BLE
  * @{
  */

/** @addtogroup Control
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup Control_Exported_Types Control Exported Types
  * @{
  */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup Control_Exported_Constants Control Exported Constants
  * @{
  */
#define APP_TIMER_PRESCALER             0 

/*电机转动的时间，单位ms*/
#define MOTOR_ACTION_TIME   100

#define CHECK_ACTION_TIME   	1000
#define SEND_DATA_ACTION_TIME       100
#define SLEEP_ACTION_TIME   	20000
#define    LED_ACTION_TIME      1000

/*电量检测的间隔时间,单位ms*/
#define BAT_CHECK_TIME      1000


#define  NTC_CONST_VALUE              846


    
    
    


extern BOOL isSendProductNameOK;
extern BOOL isSendFactoryNameOK;
extern BOOL isSendApprovalTotaldoseOK;
extern BOOL isDataOK;
extern uint16_t   NTC_value;
/**
  * @}
  */


/* Exported macro ------------------------------------------------------------*/
/** @defgroup Control_Exported_Macros Control Exported Macros
  * @{
  */



/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup Control_Exported_Functions
  * @{
  */
void Control_Init(void);
void WatchDog_Clear(void);
void Control_Polling(void);
void temperature_Init(void);
int32_t  get_temperature(void);
float  Get_NTC_Value(void);
float Get_Temperature(void);
bool  Key_Press(void);
void Eq_launch(void);
void LED_Flash_Start(void);
void LED_Flash_Stop(void);
void SendData_Start(void);
void SendData_Stop(void);
void Sleep_Enter_start(void);
/**
  * @}
  */



/**
  * @}
  */

/**
  * @}
  */



#endif /* _Control_H */

