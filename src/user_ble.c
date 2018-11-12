#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_trace.h"
#include "app_util_platform.h"
#include "pstorage.h"
#include "includes.h"
#include  "workdata.h"

static void gap_params_init(void);
static void advertising_init(void);
//static void ble_stack_init(void);
static void services_init(void);
static void ble_evt_dispatch(ble_evt_t* p_ble_evt);
static void sys_evt_dispatch(uint32_t sys_evt);
static void on_ble_evt(ble_evt_t* p_ble_evt);
static void nus_data_handler(ble_nus_t* p_nus, uint8_t* p_data, uint16_t length);

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
static ble_nus_t m_nus; /**< Structure to identify the Nordic UART Service. */
static ble_uuid_t m_adv_uuids[] = { { BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN }};

static ble_gap_adv_params_t m_adv_params;

BOOL BLE_Connect;
BOOL Connect;
int8_t BLE_RSSI;
uint8_t  Save_Flag = 0;


/**
 * 获取芯片的MAC地址.
 */
static void get_mac_addr(uint8_t *p_mac_addr){
    uint32_t error_code;
    ble_gap_addr_t mac_data;
    error_code = sd_ble_gap_address_get(&mac_data);
    APP_ERROR_CHECK(error_code);
    for(uint8_t i=0;i<6;i++)  p_mac_addr[i] = mac_data.addr[i];
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
/**
 * 蓝牙芯片的参数配置.
 */
static void gap_params_init(void) {
	uint32_t err_code;
	ble_gap_conn_params_t gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;
	ble_gap_addr_t mac_addr;
//	char* p;
	static char name[14];

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    
	//strcpy(name, WorkData.BLE_name);            //name set
    strcpy(name, "AD-26");        
	err_code = sd_ble_gap_address_get(&mac_addr);
	APP_ERROR_CHECK(err_code);

	
  /*  p = name;
	while (*p && *p++);
	sprintf(p, "%02X%02X%02X", mac_addr.addr[3], mac_addr.addr[4],mac_addr.addr[5]);
//	sprintf(p, "%02X%02X", 0xFE, 0xF8); 
*/	err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t*)name,strlen(name));
 
	DBG_LOG("Device Name:%s", name);

	APP_ERROR_CHECK(err_code);

	memset(&gap_conn_params, 0, sizeof(gap_conn_params));

	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
/**
 * 蓝牙芯片固件的主服务.
 */
static void services_init(void) {
	uint32_t err_code;
	ble_nus_init_t nus_init;

	memset(&nus_init, 0, sizeof(nus_init));

	nus_init.data_handler = nus_data_handler;

	err_code = ble_nus_init(&m_nus, &nus_init);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for the S110 SoftDevice initialization.
 *
 * @details This function initializes the S110 SoftDevice and the BLE event interrupt.
 */
/**
 * 蓝牙协议栈初始化.
 */
void ble_stack_init(void) {
	uint32_t err_code;

	// Initialize the SoftDevice handler module.
	SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_1000MS_CALIBRATION, NULL);

	ble_enable_params_t ble_enable_params;

	err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
												PERIPHERAL_LINK_COUNT, &ble_enable_params);
	APP_ERROR_CHECK(err_code);

	//Check the ram settings against the used number of links
	CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

	// Enable BLE stack.
	err_code = softdevice_enable(&ble_enable_params);
	APP_ERROR_CHECK(err_code);

	// Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
	APP_ERROR_CHECK(err_code);

	// Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
/**
 * 广播初始化.
 */
static void advertising_init(void) {
	uint32_t err_code;
	ble_advdata_t advdata;
	ble_advdata_t scanrsp;
    ble_advdata_manuf_data_t manuf_data;
    uint8_t m_addl_adv_manuf_data[BLE_GAP_ADDR_LEN];

	// Build advertising data struct to pass into @ref ble_advertising_init.
	memset(&advdata, 0, sizeof(advdata));
	advdata.name_type = BLE_ADVDATA_FULL_NAME;
	advdata.include_appearance = false;
	advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    
	memset(&scanrsp, 0, sizeof(scanrsp));
	scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids)
			/ sizeof(m_adv_uuids[0]);
	scanrsp.uuids_complete.p_uuids = m_adv_uuids;
    
    get_mac_addr(m_addl_adv_manuf_data);
    manuf_data.company_identifier = COMPANY_IDENTIFIER;
    manuf_data.data.size          = sizeof(m_addl_adv_manuf_data);
    manuf_data.data.p_data        = m_addl_adv_manuf_data;
    advdata.p_manuf_specific_data = &manuf_data;

	err_code = ble_advdata_set(&advdata, &scanrsp);
	APP_ERROR_CHECK(err_code);

	// Initialize advertising parameters (used when starting advertising).
	memset(&m_adv_params, 0, sizeof(m_adv_params));

	m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	m_adv_params.p_peer_addr = NULL;                // Undirected advertisement.
	m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
	m_adv_params.interval = APP_ADV_INTERVAL;
	m_adv_params.timeout = 0;
    
    printf("advertising init successed !");

}

void advertising_start(void){
    printf("advertising start!\n");
	sd_ble_gap_adv_start(&m_adv_params);
    Eq_launch();
}
/**@brief Function for dispatching a S110 SoftDevice event to all modules with a S110 SoftDevice
 *        event handler.
 *
 * @details This function is called from the S110 SoftDevice event interrupt handler after a S110
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  S110 SoftDevice event.
 */
static void ble_evt_dispatch(ble_evt_t* p_ble_evt) {
	ble_nus_on_ble_evt(&m_nus, p_ble_evt);
	on_ble_evt(p_ble_evt);
}

/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt) {
	pstorage_sys_event_handler(sys_evt);
}

/**@brief Function for the Application's S110 SoftDevice event handler.
 *
 * @param[in] p_ble_evt S110 SoftDevice event.
 */
/**
 * 蓝牙协议栈数据的接收处理.
 */
static void on_ble_evt(ble_evt_t* p_ble_evt) {
	int8_t rssi_value = 0;
	uint32_t err_code;

	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			isAuthOK = FALSE;
			BLE_Connect = TRUE;
                        Connect = TRUE;
			DBG_LOG("BLE Connected.");
			m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
			sd_ble_gap_rssi_start(m_conn_handle, 4, 3);
			break;
		case BLE_GAP_EVT_DISCONNECTED:
                        Connect = FALSE;
			DBG_LOG("BLE Disonnected.");
		case BLE_GAP_EVT_TIMEOUT:
			BLE_Connect = FALSE;
			m_conn_handle = BLE_CONN_HANDLE_INVALID;
			DBG_LOG("BLE advertising begin.");
			sd_ble_gap_adv_start(&m_adv_params);
            printf("user_ble.c,240 .");
			break;
		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			// Pairing not supported
			err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
																						 BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
			APP_ERROR_CHECK(err_code);
			break;
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
			// No system attributes have been stored.
			err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
			APP_ERROR_CHECK(err_code);
			break;
		case BLE_GAP_EVT_RSSI_CHANGED:
			rssi_value = p_ble_evt->evt.gap_evt.params.rssi_changed.rssi;
			BLE_RSSI = rssi_value;
			break;
		default:
			// No implementation needed.
			break;
	}
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */
/**
 * 蓝牙数据的接收中断.
 */
static void nus_data_handler(ble_nus_t* p_nus, uint8_t* p_data,uint16_t length) {
	Protocol_NewDate(p_data, length);
}

/**@snippet [Handling the data received over BLE] */
void user_BLE_Start(void) {
//	ble_stack_init();
	gap_params_init();
	services_init();
	advertising_init();

	DBG_LOG("BLE Start...");
}

void user_BLE_Disconnected(void) {
	sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

void user_BLE_Connected(void) {
	sd_ble_gap_adv_start(&m_adv_params);
    printf("connected successed !\n");
}

void user_BLE_Send(uint8_t* data, uint16_t len) {
	uint32_t err_code;

	if (len <= BLE_NUS_MAX_DATA_LEN) {
		err_code = ble_nus_string_send(&m_nus, data, len);
		if (err_code == BLE_ERROR_NO_TX_PACKETS) {
			nrf_delay_ms(200); /*延时等待发送OK*/
			err_code = ble_nus_string_send(&m_nus, data, len);
		}
		if (err_code != NRF_ERROR_INVALID_STATE) {
			if (err_code != NRF_SUCCESS) DBG_LOG("BLE send failed, error code:0x%X", err_code);
		}
	}
}

