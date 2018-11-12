#ifndef USER_BLE_H
#define USER_BLE_H

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0
#define USER_TIMERS                     4

#define APP_TIMER_PRESCALER             0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         16                                          /**< Size of timer operation queues. */

#define APP_TIMER_TICK_DEF(tick)        APP_TIMER_TICKS(tick, APP_TIMER_PRESCALER)


#define DEVICE_NAME                     "ES01"

#define CENTRAL_LINK_COUNT              0                                          /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1                                          /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(30, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)            /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   1                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_ADV_INTERVAL                2000                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      0                                          /**< The advertising timeout (in units of seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */


#define	COMPANY_IDENTIFIER						0x0056



#define DEAD_BEEF                       0xDEADBEEF                                  /**< . */

extern BOOL BLE_Connect;
extern BOOL Connect;
extern int8_t BLE_RSSI;
extern  uint8_t  Save_Flag;

void user_BLE_Start(void);
void user_BLE_Disconnected(void);
void user_BLE_Connected(void);
void user_BLE_Send(uint8_t *data, uint16_t len);
void advertising_start(void);
void ble_stack_init(void);
#endif
