/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_clock.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "bsp_itracker.h"

#include "tc_iot_export.h"
#include "tc_iot_device_logic.h"


#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define DEVICE_NAME                     "iTracker"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           1                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */


BLE_NUS_DEF(m_nus);                                                                 /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

APP_TIMER_DEF(m_sec_req_timer_id);    
#define SECURITY_REQUEST_DELAY          APP_TIMER_TICKS(4000)    

void sensor_collect_timer_handle();

static void sec_req_timeout_handler(void * p_context)
{
    ret_code_t           err_code;
    NRF_LOG_INFO("sec_req_timeout_handler.");

}

static void timers_init(void)
{
    ret_code_t err_code;

    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create Security Request timer.
    err_code = app_timer_create(&m_sec_req_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                sec_req_timeout_handler);
    APP_ERROR_CHECK(err_code);
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
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
		char custom_dev_name[30];
		ble_gap_addr_t gap_addr;
	
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

		err_code = sd_ble_gap_addr_get(&gap_addr);
    APP_ERROR_CHECK(err_code);
		sprintf(custom_dev_name, "%s-%02X:%02X:%02X", DEVICE_NAME, /*gap_addr.addr[5],
																																						 gap_addr.addr[4],
																																						 gap_addr.addr[3],*/
																																						 gap_addr.addr[2],
																																						 gap_addr.addr[1],
																																						 gap_addr.addr[0]);
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) custom_dev_name,
                                          strlen(custom_dev_name));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
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
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        uint32_t err_code;

        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
				DPRINTF(LOG_INFO, "Received data from BLE NUS %s", p_evt->params.rx_data.p_data);
        for (uint32_t i = 0; i < p_evt->params.rx_data.length; i++)
        {
            do
            {
                err_code = app_uart_put(p_evt->params.rx_data.p_data[i]);
                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                {
                    NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
        if (p_evt->params.rx_data.p_data[p_evt->params.rx_data.length-1] == '\r')
        {
            while (app_uart_put('\n') == NRF_ERROR_BUSY);
        }
    }

}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

#if defined(S132)
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
#endif

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

         case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
        {
            ble_gap_data_length_params_t dl_params;

            // Clearing the struct will effectivly set members to @ref BLE_GAP_DATA_LENGTH_AUTO
            memset(&dl_params, 0, sizeof(ble_gap_data_length_params_t));
            err_code = sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &dl_params, NULL);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gattc_evt.conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, 64);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */
//void uart_event_handle(app_uart_evt_t * p_event)
//{
//    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
//    static uint8_t index = 0;
//    uint32_t       err_code;

//    switch (p_event->evt_type)
//    {
//        case APP_UART_DATA_READY:
//            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
//            index++;

//            if ((data_array[index - 1] == '\n') || (index >= (m_ble_nus_max_data_len)))
//            {
//                NRF_LOG_DEBUG("Ready to send data over BLE NUS");
//                NRF_LOG_HEXDUMP_DEBUG(data_array, index);

//                do
//                {
//                    uint16_t length = (uint16_t)index;
//                    err_code = ble_nus_string_send(&m_nus, data_array, &length);
//                    if ( (err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_BUSY) )
//                    {
//                        APP_ERROR_CHECK(err_code);
//                    }
//                } while (err_code == NRF_ERROR_BUSY);

//                index = 0;
//            }
//            break;

//        case APP_UART_COMMUNICATION_ERROR:
//            APP_ERROR_HANDLER(p_event->data.error_communication);
//            break;

//        case APP_UART_FIFO_ERROR:
//            APP_ERROR_HANDLER(p_event->data.error_code);
//            break;

//        default:
//            break;
//    }
//}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
//static void uart_init(void)
//{
//    uint32_t                     err_code;
//    app_uart_comm_params_t const comm_params =
//    {
//        .rx_pin_no    = RX_PIN_NUMBER,
//        .tx_pin_no    = TX_PIN_NUMBER,
//        .rts_pin_no   = RTS_PIN_NUMBER,
//        .cts_pin_no   = CTS_PIN_NUMBER,
//        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
//        .use_parity   = false,
//        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200
//    };

//    APP_UART_FIFO_INIT(&comm_params,
//                       UART_RX_BUF_SIZE,
//                       UART_TX_BUF_SIZE,
//                       uart_event_handle,
//                       APP_IRQ_PRIORITY_LOWEST,
//                       err_code);
//    APP_ERROR_CHECK(err_code);
//}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

/*----------------------------iTracker application-------------------------------*/
#define LIS3DH_TEST
#define LIS2MDL_TEST
#define BEM280_TEST
#define OPT3001_TEST
#define GSM_TEST

tracker_data_t tracker_data;
char post_data[1024]={0};
uint8_t gsm_started = 0;


#ifdef GSM_TEST

#define  DEST_DOMAIN              "www.baidu.com"
#define  DEST_PORT                80
#define  DEST_URL									""
#define  POST_FORMAT 				\
				"{\r\n"							\
				"\"temp\" : %.2f,\r\n"				\
				"\"humi\" : %.2f,\r\n"				\
				"\"press\" : %.2f,\r\n"				\
				"\"light\" : %.2f,\r\n"				\
				"\"lati\" : %.2f,\r\n"\
				"\"longi\" : %.2f,\r\n"\
				"\"X\" : %d,\r\n"				\
				"\"Y\" : %d,\r\n"		\
				"\"Z\" : %d,\r\n"			\
				"\"m_x\" : %.2f,\r\n"				\
				"\"m_y\" : %.2f,\r\n"\
				"\"m_z\" : %.2f\r\n"\
				"}\r\n"


int GsmInit()
{
    //int  retavl;
    int time_count;
	
#if defined (M35)			
    DPRINTF(LOG_INFO,"check auto baud\r\n");
    /*module init ,check is auto baud,if auto,config to 115200 baud.*/ 
    Gsm_CheckAutoBaud();
    
    DPRINTF(LOG_INFO,"set echo\r\n");
    /*isable cmd echo*/
    Gsm_SetEchoCmd(0); //AT_COMMAND ATE
    
    DPRINTF(LOG_INFO,"check sim card\r\n");
    /*check SIM Card status,if not ready,retry 60s,return*/
    time_count=0;
    while((Gsm_CheckSimCmd()<0))
    {
        delay_ms(GSM_CHECKSIM_RETRY_TIME);
               
        if(++time_count>GSM_CHECKSIM_RETRY_NUM)
        {
            DPRINTF(LOG_WARN,"check sim card timeout\r\n");
            return -1;
        }
    }
#endif 
		
    DPRINTF(LOG_INFO,"check network\r\n");
    /*check Network register status,if not ready,retry 60s,return*/
    time_count=0;
    while((Gsm_CheckNetworkCmd()<0))
    {
        delay_ms(GSM_CHECKSIM_RETRY_TIME);
        //delay_ms(300);     
        if(++time_count>GSM_CHECKSIM_RETRY_NUM)
        {
            DPRINTF(LOG_WARN,"check network timeout\r\n");
            return -1;
        }
    }

    DPRINTF(LOG_INFO,"check GPRS\r\n");
    /*check GPRS  status,if not ready,retry 60s,return*/
    //Gsm_SetGPRSCmd();
    time_count=0;
    while((Gsm_CheckGPRSCmd()<0))
    {
        delay_ms(GSM_CHECKSIM_RETRY_TIME);
               
        if(++time_count>GSM_CHECKSIM_RETRY_NUM)
        {
            DPRINTF(LOG_WARN,"check GPRS timeout\r\n");
            return -1;
        }
    }
#if defined (BC95)
		DPRINTF(LOG_INFO,"check con status\r\n");
    /*check GPRS  status,if not ready,retry 60s,return*/
    time_count=0;
    while((Gsm_CheckConStatusCmd()<0))
    {
        delay_ms(GSM_CHECKSIM_RETRY_TIME);
               
        if(++time_count>GSM_CHECKSIM_RETRY_NUM)
        {
            DPRINTF(LOG_WARN,"check GPRS timeout\r\n");
            return -1;
        }
    }
		DPRINTF(LOG_INFO,"BC95 Connected\r\n");
#endif
#if defined (M35)		
    /*Set Context*/
		DPRINTF(LOG_INFO,"set context 0\r\n");
    Gsm_SetContextCmd();
   
    Gsm_SetQNDI(1);
    /*Set dns mode 1, domain*/
	DPRINTF(LOG_INFO,"set dns mode\r\n");
    Gsm_SetDnsModeCmd();
    /*Set qi send echo 0*/
    //Gsm_SetSdEchoCmd();
		/*Set auto reply*/
		//DPRINTF(LOG_INFO,"set auto reply mode\r\n");
		Gsm_SetAtsEnCmd();

    DPRINTF(LOG_INFO,"check GSM Network ok!!\r\n");
#endif
    int rssi=0;
    rssi= Gsm_GetRssiCmd();
    
    DPRINTF(LOG_INFO, "rssi=%d\r\n",rssi);
    
		return 0;
}


				
int Gsm_HttpPost(uint8_t* data, uint16_t len)
{ 
    int        ret = -1;
    uint16_t   httpLen = 0;
    uint8_t*   http_buf = NULL; 
    char       testbuf[256];

    ret = Gsm_OpenSocketCmd(GSM_TCP_TYPE, DEST_DOMAIN, DEST_PORT); 
    if(ret != 0)
    {
			  Gsm_CloseSocketCmd(); 
        return -1; 
    }
    DPRINTF(LOG_INFO, "Gsm_OpenSocketCmd OK\n");
    Gsm_RecvData2(testbuf, sizeof(testbuf), 5000);

    http_buf = malloc(256);
    if(http_buf == NULL)
    {
       DPRINTF(LOG_INFO, "\r\n-----http_buf malloc 256 fail!!!-----\r\n");
       return -1;
    }
    
    httpLen =sprintf((char *)http_buf,
                     "POST /%s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Content-Length: %d\r\n"
                     "Content-Type: application/json\r\n\r\n",
                     DEST_URL, DEST_DOMAIN, len);
    
    DPRINTF(LOG_DEBUG, "Gsm_HttpPost head %d - %s\r\n", httpLen, http_buf);
    ret =Gsm_SendDataCmd((char *)http_buf,httpLen); 
    
    //delay_ms(1000);
    DPRINTF(LOG_DEBUG, "Gsm_HttpPost body len %d\r\n", len);
    ret =Gsm_SendDataCmd((char *)data,len); 
    //DPRINTF(LOG_DEBUG, "Gsm_SendDataCmd %s", data);
    if (ret < 0) {
      DPRINTF(LOG_ERROR, "Gsm_HttpPost send fail...\r\n");
      free(http_buf);
      Gsm_CloseSocketCmd(); 
      return ret;   
    }
    
    free(http_buf);
    return ret;
}

int Gsm_HttpRsp(void)
{
    int        read_len; 
    char*      http_RecvBuf =NULL;
    int        ret = -1;
        
    http_RecvBuf = malloc(1024);
    if(http_RecvBuf == NULL)
    {
       DPRINTF(LOG_INFO, "\r\n-----http_RecvBuf malloc 1024 fail!!!-----\r\n");
       return -1;
    }
     
	//read_len = Gsm_RecvData(http_RecvBuf, 5000);//5 s 
    memset(http_RecvBuf, 0 , 1024);
    read_len = Gsm_RecvData2(http_RecvBuf, 1024, 5000);//5 s 
    
    if(read_len > 0) 
    {
        //DPRINTF(LOG_DEBUG, "Gsm_HttpRsp response len=%d\r\n", read_len);
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp ret %d - %s", read_len, http_RecvBuf);
    }
    else
    {
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp Gsm_RecvData2 len=%d\r\n", read_len);
    }
    
    delay_ms(200);
    memset(http_RecvBuf, 0 , 1024);
    read_len = Gsm_RecvData2(http_RecvBuf, 1024, 5000);//5 s 
    DPRINTF(LOG_DEBUG, "Gsm_HttpRsp Gsm_RecvData2 len=%d\r\n", read_len);
    if(read_len > 0) 
    {
        //DPRINTF(LOG_DEBUG, "Gsm_HttpRsp response len=%d\r\n", read_len);
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp ret %d - %s", read_len, http_RecvBuf);
    }
    else
    {
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp Gsm_RecvData2 len=%d\r\n", read_len);
    }

    delay_ms(200);
    memset(http_RecvBuf, 0 , 1024);
    read_len = Gsm_RecvData2(http_RecvBuf, 1024, 5000);//5 s 
    if(read_len > 0) 
    {
        //DPRINTF(LOG_DEBUG, "Gsm_HttpRsp response len=%d\r\n", read_len);
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp ret %d - %s", read_len, http_RecvBuf);
    }
    else
    {
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp Gsm_RecvData2 len=%d\r\n", read_len);
    }

    delay_ms(200);
    memset(http_RecvBuf, 0 , 1024);
    read_len = Gsm_RecvData2(http_RecvBuf, 1024, 5000);//5 s 
    if(read_len > 0) 
    {
        //DPRINTF(LOG_DEBUG, "Gsm_HttpRsp response len=%d\r\n", read_len);
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp ret %d - %s", read_len, http_RecvBuf);
    }
    else
    {
        DPRINTF(LOG_DEBUG, "Gsm_HttpRsp Gsm_RecvData2 len=%d\r\n", read_len);
    }

    free(http_RecvBuf);
    Gsm_CloseSocketCmd(); 
	
    return ret;
}
#endif

void start_collect_data()
{
		int ret;
#ifdef BEM280_TEST
		get_bme280_data(&tracker_data.temp_value, &tracker_data.humidity_value, &tracker_data.gas_value);
#endif
#ifdef LIS3DH_TEST
		ret = lis3dh_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "lis3dh_twi_init fail %d\r\n", ret);
		} else {
				get_lis3dh_data(&tracker_data.x, &tracker_data.y, &tracker_data.z);		
		}
#endif
#ifdef LIS2MDL_TEST
		ret = lis2mdl_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "lis2mdl_twi_init fail %d\r\n", ret);
		} else {
				get_lis2mdl_data(&tracker_data.magnetic_x, &tracker_data.magnetic_y, &tracker_data.magnetic_z);
		}
#endif
#ifdef OPT3001_TEST
		ret = opt3001_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "opt3001_twi_init fail %d\r\n", ret);
		} else {
				get_opt3001_data(&tracker_data.light_value);		
		}
#endif
		
}

/**@brief timers.
 */
APP_TIMER_DEF(sensor_collect_timer);

char * float2str(float val, int precision, char *buf)
{
    char *cur, *end;
    
    sprintf(buf, "%.6f", val);
    if (precision < 6) {
        cur = buf + strlen(buf) - 1;
        end = cur - 6 + precision; 
        while ((cur > end) && (*cur == '0')) {
            *cur = '\0';
            cur--;
        }
    }
    
    return buf;
}

char * double2str(double val, int precision, char *buf)
{
    char *cur, *end;
    
    sprintf(buf, "%.6f", val);
    if (precision < 6) {
        cur = buf + strlen(buf) - 1;
        end = cur - 6 + precision; 
        while ((cur > end) && (*cur == '0')) {
            *cur = '\0';
            cur--;
        }
    }
    
    return buf;
}

char lati_buf[128];
char longi_buf[128];
uint8_t collect_flag = 0;

void sensor_collect_timer_handle()
{
		int ret;
		static uint32_t swi_cnt=0;
	
//		if(collect_flag)
//		{
//			collect_flag = 0;
//		}
//		else
//		{
//			return;
//		}
	
		start_collect_data();
	
//		Gps_Uart_Init();
//		GpsGetLatestGpsPositionDouble(&tracker_data.latitude, &tracker_data.longitude);
//		Gsm_Uart_Init();

		sprintf(post_data, POST_FORMAT, tracker_data.temp_value,
																		tracker_data.humidity_value,
																		tracker_data.gas_value,
																		/*tracker_data.barometer_value,*/
																		tracker_data.light_value,
																		tracker_data.latitude,
																		tracker_data.longitude,
																		tracker_data.x,
																		tracker_data.y,
																		tracker_data.z,
																		tracker_data.magnetic_x,
																		tracker_data.magnetic_y,
																		tracker_data.magnetic_z
																		);
		DPRINTF(LOG_INFO, "%s", post_data);
		swi_cnt++;
//		if(swi_cnt==1)
//		{
//				DPRINTF(LOG_INFO, "uart switch to gps 666\n");
//				Gps_Uart_Init();
//				GpsGetLatestGpsPositionDouble(&tracker_data.latitude, &tracker_data.longitude);
//				float2str((float)tracker_data.latitude, 2, lati_buf);
//				float2str((float)tracker_data.longitude, 2, longi_buf);
//				DPRINTF(LOG_INFO, "gps latitude=%s lontitude=%s\n", lati_buf, longi_buf);
//		}
//		else if(swi_cnt>=1)
//		{
//				swi_cnt = 0;
//				if(gsm_started)
//				{
//						DPRINTF(LOG_INFO, "uart switch to gsm");
//						Gsm_Uart_Init();

						
                        //test_mqtt();
												//tc_io_main();
                       /*do {
                            IOT_TRACE( "test_mqtt\r\n" );
                            delay_ms(1000);
                        }	while(1);	*/
//#if defined (M35)
//						ret = Gsm_HttpPost(post_data, strlen(post_data));
//						if (ret >0 ) 
//						{
//							 ret = Gsm_HttpRsp();
//						}		
//#endif
//						int rssi=0;
//						rssi= Gsm_GetRssiCmd();
//						
//						DPRINTF(LOG_INFO, "rssi=%d\r\n",rssi);
//					}
					//else
					{
						DPRINTF(LOG_INFO, "gsm not started\r\n");
					}		
//		}
}

void _sensor_collect_timer_handle()
{
		collect_flag = 1;
        DPRINTF(LOG_INFO, "rtc1_get_sys_tick ret %u\r\n",  rtc1_get_sys_tick());
}

uint32_t sensor_collect_timer_init(void)
{
    app_timer_create(&sensor_collect_timer,APP_TIMER_MODE_REPEATED,_sensor_collect_timer_handle);
		//app_timer_start(sensor_collect_timer, 30000, NULL);
		app_timer_start(sensor_collect_timer, 10000, NULL);
}

void sensors_init()
{
		int ret;
#ifdef BEM280_TEST
		ret = bme280_spi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "bme280_spi_init fail %d\r\n", ret);
		} else {
				ret = _bme280_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "lis3dh_init fail1\r\n");
				}
		}
#endif
#ifdef LIS3DH_TEST
		ret = lis3dh_twi_init();
		if(ret != NRF_SUCCESS) {
				DPRINTF(LOG_ERROR, "lis3dh_twi_init fail %d\r\n", ret);
		} else {
				ret = lis3dh_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "lis3dh_init fail2\r\n");
				}
		}
#endif
#ifdef LIS2MDL_TEST
		ret = lis2mdl_twi_init();
		if(ret < 0) {
				DPRINTF(LOG_ERROR, "lis2mdl_twi_init fail %d\r\n", ret);
		} else {
				ret = lis2mdl_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "lis2mdl_init fail\r\n");
				}
		}
#endif
#ifdef OPT3001_TEST
		ret = opt3001_twi_init();
		if(ret < 0) {
				DPRINTF(LOG_ERROR, "opt3001_twi_init fail %d\r\n", ret);
		} else {
				ret = opt3001_init();
				if(ret < 0) {
						DPRINTF(LOG_ERROR, "opt3001_init fail\r\n");
				}
		}
#endif
}

void test_rtc();
/**@brief Application main function.
 */

int main(void)
{
    uint32_t err_code;
    bool     erase_bonds;
		int ret;

    // Initialize.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    log_init();
    //buttons_leds_init(&erase_bonds);
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
		
		//test_rtc();
		DPRINTF(LOG_INFO, "iTracker Start V1.0.0\r\n");
		// init gsm
		Gsm_Uart_Init();		
		Gsm_Gpio_Init();
		Gsm_PowerUp();
		ret = GsmInit();
		if(ret == 0)
		{
				gsm_started = 1;
		}
		// init gps
		GpsInit();
		// init sensors
		sensors_init();
		
		sensor_collect_timer_init();
		
    err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
		
		//err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
		//APP_ERROR_CHECK(err_code);
    // Enter main loop.

    //delay_ms(2000);
    
//    for (;;)
//    {
//        
//        power_manage();
//			 
//				//GSM post & sensors collect every 10s
//				
//				//GPS collect every 10s
//			
//				//sonsors every 10s
//				if(collect_flag)
//				{
//						collect_flag = 0;
//                        
//						sensor_collect_timer_handle();
//				}
//				
//    }
		if(gsm_started)
		{
			Gsm_Uart_Init();
			tc_io_main();
		}
		else
		{
			DPRINTF(LOG_INFO, "gsm not started\r\n");
		}
}


static const nrf_drv_rtc_t _rtc = NRF_DRV_RTC_INSTANCE(0); /**< Declaring an instance of nrf_drv_rtc for RTC0. */
#define COMPARE_COUNTERTIME  (3UL)      

volatile static int count1 = 0;
volatile static int count2 = 0;
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{

    if (int_type == NRF_DRV_RTC_INT_COMPARE0)
    {
        //nrf_gpio_pin_toggle(COMPARE_EVENT_OUTPUT);
        count1 ++ ;
        if (count1%8==3)
        {
            DPRINTF(LOG_INFO, "count1=%d\r\n",count1);
        }

    }
    else if (int_type == NRF_DRV_RTC_INT_TICK)
    {
        //nrf_gpio_pin_toggle(TICK_EVENT_OUTPUT);
        count2 ++;
        if (count2%4==3)
        {
            DPRINTF(LOG_INFO, "count2=%d\r\n",count2);
        }        
    }
}

static void _lfclk_config(void)
{
    DPRINTF(LOG_DEBUG, "_lfclk_config\r\n");
    ret_code_t err_code = nrf_drv_clock_init();
    DPRINTF(LOG_DEBUG, "nrf_drv_clock_init ret %u\r\n", err_code);
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}

static void _rtc_config(void)
{
    uint32_t err_code;

    //Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.prescaler = 4095;
    DPRINTF(LOG_DEBUG, "_rtc_config1\r\n");
    err_code = nrf_drv_rtc_init(&_rtc, &config, rtc_handler);
    DPRINTF(LOG_DEBUG, "nrf_drv_rtc_init ret %u\r\n", err_code);
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    nrf_drv_rtc_tick_enable(&_rtc,true);

    DPRINTF(LOG_DEBUG, "_rtc_config2\r\n");
    //Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds
    err_code = nrf_drv_rtc_cc_set(&_rtc,0,COMPARE_COUNTERTIME * 8,true);
    DPRINTF(LOG_DEBUG, "nrf_drv_rtc_cc_set ret %u\r\n", err_code);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    nrf_drv_rtc_enable(&_rtc);
}

void test_rtc()
{
    
    _lfclk_config();

    _rtc_config();
}
/**
 * @}
 */
