/**
        ******************************************************************************
        * @file    Control.c
        * @author  宋阳
        * @version V1.0
        * @date    2015.12.4
        * @brief   控制操作相关函数
        ******************************************************************************
        */

/* Includes ------------------------------------------------------------------*/
#include "control.h"
#include "nrf_drv_wdt.h"
#include "workdata.h"
#include  "nrf_temp.h"
#include "math.h"
#include "bsp.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "bsp.h"
/** @addtogroup firmwaveF2_BLE
        * @{
        */



/** @defgroup Control
        * @brief 控制操作相关函数
        * @{
        */


/* Private typedef -----------------------------------------------------------*/
/** @defgroup Control_Private_Types Control Private Types
        * @{
        */
#define TIME1_PATCH_REG           (*(uint32_t *)0x40009C0C)
#define TIME2_PATCH_REG           (*(uint32_t *)0x4000AC0C)
/**
        * @}
        */

/* Private define ------------------------------------------------------------*/

/** @defgroup Control_Private_Constants Control Private Constants
        * @{
        */


/**
        * @}
        */

/* Private macros ------------------------------------------------------------*/
/** @defgroup Control_Private_Macros Control Private Macros
        * @{
        */

/**
        * @}
        */
/* Private variables ---------------------------------------------------------*/
/** @defgroup Control_Private_Variables Private Variables
        * @{
        */

APP_TIMER_DEF(TimerId_ChechState);
APP_TIMER_DEF(TimerId_Send);
APP_TIMER_DEF(TimerId_Sleep);
APP_TIMER_DEF(TimerId_LED);
/**
        * @}
        */
/* Private function prototypes -----------------------------------------------*/
/** @defgroup Control_Private_Functions Control Private Functions
        * @{
        */
static void Sleep_TimerCB(void * p_context);
static void funControl(int argc, char* argv[]);
static void LED_State_TimerCB(void *p_context);
static void SendData_TimerCB(void *p_context);
/**
        * @}
        */
BOOL isSendProductNameOK;
BOOL isSendFactoryNameOK;
BOOL isSendApprovalTotaldoseOK;
BOOL isDataOK;
/* Exported functions ---------------------------------------------------------*/

/** @defgroup Control_Exported_Functions Control Exported Functions
        * @{
        */
static uint8_t led_state = 0,log_len = 0;
uint16_t sleep_step = 0;
uint16_t     NTC_value = 0;
/*
* NTC的转化公式：T = NTC_B*log10(e)*NTC_T0 / ( NTC_T0*log10(fRegister) + NTC_B*log10(e)LOGE - NTC_T0*log10(NTC_R0) ) - 273
*/
const float  resistor_value = 10000.0;
const float  T2_value       = (273.15 + 25);
const float  B_param_value  =  3905.0;
const float  Ka_value       =  273.15;
/**
* @brief  控制轮循函数.
* @param  none.
* @retval none.
*/
void Control_Init(void) {
  app_timer_create(&TimerId_Sleep, APP_TIMER_MODE_REPEATED, Sleep_TimerCB);				 			// 创建设备睡眠定时器
  app_timer_create(&TimerId_LED, APP_TIMER_MODE_REPEATED, LED_State_TimerCB);				 			// 创建蓝色灯闪烁的定时器
  app_timer_create(&TimerId_Send, APP_TIMER_MODE_REPEATED, SendData_TimerCB);				 			// 创建发送设备存储数据的定时器
  
  app_timer_start(TimerId_Sleep,APP_TIMER_TICKS(SLEEP_ACTION_TIME,APP_TIMER_PRESCALER),NULL);				 			// 开始设备睡眠的定时器
  app_timer_start(TimerId_LED,APP_TIMER_TICKS(LED_ACTION_TIME,APP_TIMER_PRESCALER),NULL);				 			// 开始蓝色灯的定时器
  
  ADC_CHECK_EN();				 			// 创建设备睡眠定时器
  
  
  CMD_ENT_DEF(control, funControl);
  Cmd_AddEntrance(CMD_ENT(control));

  DBG_LOG("Device control init.");
}

/**
* 清看门狗.
*/
void WatchDog_Clear(void) {
#if WTD_EN == 1
  nrf_drv_wdt_feed();
#endif
}
/**
* 启动设备.
*/
void Eq_launch(void){
  	advertising_start();
	app_timer_start(TimerId_Sleep,APP_TIMER_TICKS(SLEEP_ACTION_TIME,APP_TIMER_PRESCALER),NULL);
	app_timer_start(TimerId_LED,APP_TIMER_TICKS(LED_ACTION_TIME,APP_TIMER_PRESCALER),NULL);
}
/**
* 设备睡眠定时器开始，时间到后设备就进入睡眠.
*/
void Sleep_Enter_start(void){
	app_timer_start(TimerId_Sleep,APP_TIMER_TICKS(SLEEP_ACTION_TIME,APP_TIMER_PRESCALER),NULL);
}
/**
* 停止设备睡眠的定时器.
*/
static void Sleep_Enter_stop(void){
	app_timer_stop(TimerId_Sleep);
}
/**
* 蓝色灯闪烁开始.
*/
void LED_Flash_Start(void){
	app_timer_start(TimerId_LED,APP_TIMER_TICKS(LED_ACTION_TIME,APP_TIMER_PRESCALER),NULL);
}
/**
* 蓝色灯停止闪烁.
*/
void LED_Flash_Stop(void){
	app_timer_stop(TimerId_LED);
}
/**
* 开始检测状态.
*/
void ChechState_Start(void){
	app_timer_start(TimerId_ChechState,APP_TIMER_TICKS(CHECK_ACTION_TIME,APP_TIMER_PRESCALER),NULL);
}
/**
 * 
 * 
 * @param us     
 * @param cb     
 */
/**
* 开始发送存储数据.
*/
void SendData_Start(void){
        app_timer_start(TimerId_Send,APP_TIMER_TICKS(SEND_DATA_ACTION_TIME,APP_TIMER_PRESCALER),NULL);
}
/**
* 停止发送存储数据.
*/
void SendData_Stop(void){
        app_timer_stop(TimerId_Send);
}
/**
        * 发送存储数据的定时器中断
        */
static void SendData_TimerCB(void *p_context){
        uint8_t buf[16];
        uint8_t Int_value;
        uint32_t value;
        float point_value;
        StoreLog_t * log;
        if(Connect != FALSE ){
        	if(SendData_flag == 0xAB && WorkData.StoreLog_In != 0){
                        log_len = WorkData.StoreLog_In;
                        SendData_flag = 0XAC;
			}else if(WorkData.StoreLog_In == 0){
				SendData_Stop();
			}
			if(WorkData.StoreLog_In != 0){
                log = STROE_LOG_POINT(log_len);

                Int_value = (uint8_t)log->temperature;
                point_value = log->temperature - (uint32_t)Int_value;
                value = (uint32_t)(point_value * 1000000);

                buf[0] = log->key_num;
                buf[1] = log->key_num >> 8;
                buf[2] = log->time;
                buf[3] = log->time >>8;
                buf[4] = log->time >> 16;
                buf[5] = log->time >>24;
                buf[6] = Int_value ;
                buf[7] = value;
                buf[8] = value >> 8;
                buf[9] = value>> 16;

                Send_Cmd(0xC7,(uint8_t *)buf,10);
                DBG_LOG("press key %u",log->key_num);
				
				log_len--;  
			}
           if(log_len == 0 && SendData_flag == 0xAC)        SendData_Stop();
        }
		else  SendData_Stop();
}
/** @addtogroup Control_Private_Functions
        * @蓝色灯闪烁定时器的中断
        */
static void LED_State_TimerCB(void *p_context){
  
  if(Connect)
  {
    led_state++;
    if(led_state == 3){
      LED_TOGGLE(G);
      led_state = 0;
    }
  }
  else{
	LED_TOGGLE(G);
  	
  }
}
/**
* 设备睡眠定时器的中断.
*/

static void Sleep_TimerCB(void * p_context){
  if(Connect != TRUE){
	  sleep_step++;
	  if(sleep_step >= 300){
		app_timer_stop(TimerId_LED);
		app_timer_stop(TimerId_Sleep);
		WorkData.sleep_state = 0x55;
		sleep_step = 0;
		LED_OFF(G);
		sd_ble_gap_adv_stop();   //关广播
	  }
  }else{
        WorkData.sleep_state = 0;
	sleep_step = 0;
	Sleep_Enter_stop();
  }
}


/**
 * 芯片温度检测初始化
 */

void temperature_Init(void)
{
  nrf_temp_init();
}
/**
 * 获取芯片内部的温度
 */
int32_t  get_temperature(void)
{
    int32_t  temp;
  
    NRF_TEMP->TASKS_START = 1; /** Start the temperature measurement. */
    
    /* Busy wait while temperature measurement is not finished, you can skip waiting if you enable interrupt for DATARDY event and read the result in the interrupt. */
    /*lint -e{845} // A zero has been given as right argument to operator '|'" */
    while (NRF_TEMP->EVENTS_DATARDY == 0)
    {
        // Do nothing.
    }
    NRF_TEMP->EVENTS_DATARDY = 0;
    
    /**@note Workaround for PAN_028 rev2.0A anomaly 29 - TEMP: Stop task clears the TEMP register. */
    temp = (nrf_temp_read() / 4);

    /**@note Workaround for PAN_028 rev2.0A anomaly 30 - TEMP: Temp module analog front end does not power down when DATARDY event occurs. */
    NRF_TEMP->TASKS_STOP = 1; /** Stop the temperature measurement. */
    
    return temp;

}

/**
 * 得到NTC阻值
 * 公式：T = NTC_B*log10(e)*NTC_T0 / ( NTC_T0*log10(fRegister) + NTC_B*log10(e)LOGE - NTC_T0*log10(NTC_R0) ) - 273
 */
float  Get_NTC_Value(void)
{
    return  ((resistor_value*(NTC_CONST_VALUE - NTC_value))/NTC_value);
}
/**
 * 根据NTC，获取外部温度
 */
float Get_Temperature(void)
{
  float Rt,temp;
  Rt = Get_NTC_Value();
  temp = Rt/resistor_value;
  temp = log(temp);
  temp  /= B_param_value;
  temp += (1 / T2_value);
  temp = (1/temp);
  temp -= Ka_value;
  
  return temp;
}

/**
        * @brief  设备控制调试命令.
        */
static void funControl(int argc, char* argv[]) {
        argv++;
        argc--;

        if (ARGV_EQUAL("batvol")){
        DBG_LOG("the batvol is %d V",BatvolADC);
        }else if (ARGV_EQUAL("NTC")){
                DBG_LOG("the temperature is %f ",Get_Temperature());
        }else if (ARGV_EQUAL("temperature")){
                DBG_LOG("the get_temperature is %u ",get_temperature());
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

