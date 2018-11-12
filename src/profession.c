/**
    ******************************************************************************
    * @file    profession.c
    * @author  宋阳
    * @version V1.0
    * @date    2017.12.15
    * @brief   业务逻辑相关函数.
    *
    ******************************************************************************
    */


/* Includes ------------------------------------------------------------------*/
#include "includes.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static void button_handler(uint8_t pin_no, uint8_t button_action);
/*
** 按键配置
**/
static const app_button_cfg_t app_buttons[1] =
{
  { KEY, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_handler },
};


StoreLog_t key_log;
uint8_t    key_status = 0,launch_status = 0,sendflag = 0;


/* Private function prototypes -----------------------------------------------*/

static void prof_Console(int argc, char* argv[]);

/* Exported functions --------------------------------------------------------*/

/**
 * 业务逻辑初始化
 */
void Profession_init(void) {


  CMD_ENT_DEF(prof, prof_Console);
  Cmd_AddEntrance(CMD_ENT(prof));

  app_button_init((app_button_cfg_t*)app_buttons, 1,
                  APP_TIMER_TICKS(5, APP_TIMER_PRESCALER));
  app_button_enable();


  DBG_LOG("Procession init.");
}




/**
 * 按键处理轮询
 */
void button_polling(void) {
		uint8_t Int_value,buf[16] ={0};
		uint32_t value;
		float point_value;
	   if(key_status == 2){
		 	DBG_LOG("store key num log");
	   		Write_StoreLog(&key_log);
			key_status = 0;
	   }
	   if(Connect != FALSE && sendflag != 0){
              Int_value = (uint8_t)key_log.temperature;
              point_value = key_log.temperature - (uint32_t)Int_value;
              value = (uint32_t)(point_value * 1000000);

              buf[0] = key_log.key_num;
              buf[1] = key_log.key_num >> 8;
              buf[2] = key_log.time;
              buf[3] = key_log.time >>8;
              buf[4] = key_log.time >> 16;
              buf[5] = key_log.time >>24;
              buf[6] = Int_value ;
              buf[7] = value;
              buf[8] = value >> 8;
              buf[9] = value>> 16;

              //Send_Cmd(0xC7,(uint8_t *)buf,10);
			  sendflag = 0;
			  DBG_LOG("press key %u",key_log.key_num);
        }
	   if(launch_status == 1){
		  	launch_status = 0;
			//Eq_launch();
			WorkData.sleep_state = 0x01;
			DBG_LOG("equipment is launch");
	   }
	   if(Connect != TRUE){
		    if(WorkData.sleep_state != 0xAA && WorkData.sleep_state != 0x55){
                Sleep_Enter_start();
                WorkData.sleep_state = 0xAA;
            }
	   }

}

/**
 * 按键中断回调函数
 * 
 * @param pin_no 中断的引脚
 * @param button_action
 *               按键的类型，上升沿或者下降沿
 */
static void button_handler(uint8_t pin_no, uint8_t button_action) {

  if (pin_no == KEY && button_action == 0x00) {
		StoreLog_t * log;
		
		LED_Flash_Start();
		if(key_log.key_num ==0 && WorkData.StoreLog_In != 0) 
		{
			log = STROE_LOG_POINT(WorkData.StoreLog_In);
			key_log.key_num = log->key_num;
		}
		key_log.key_num += 1;
		key_log.temperature = Get_Temperature();
		key_log.time = RTC_ReadCount();
		DBG_LOG("press key %u",key_log.key_num);
		if(Connect ==TRUE)	sendflag  = 1;
		key_status = 2;
		if(WorkData.sleep_state == 0x55){
			launch_status = 1;
		}	
  }
  if (pin_no == KEY && button_action == 0x01){
		LED_Flash_Stop();
		LED_ON(G);
  }
}
/**
 *  指纹传感器调试接口
 */
static void prof_Console(int argc, char* argv[]) {

	argv++;
	argc--;
	
	
	StoreLog_t* log;
  	timeRTC_t time;
  	
  if (ARGV_EQUAL("button")) {
	for(uint8_t i=1;i<=WorkData.StoreLog_In;i++){
		log = STROE_LOG_POINT(i);
		RTC_TickToTime(log->time,&time);
		DBG_LOG("time: %04d-%d-%d %02d:%02d:%02d day-%d ", time.year,
					time.month, time.date, time.hours, time.minutes,
					time.seconds, time.day);
		DBG_LOG("readlog key_num :%u.",log->key_num);
		DBG_LOG("readlog temperature :%f.",log->temperature);
		nrf_delay_ms(500);
	}
  }	else if (ARGV_EQUAL("setfpg")) {

  }
}

/************************ (C) COPYRIGHT  *****END OF FILE****/
