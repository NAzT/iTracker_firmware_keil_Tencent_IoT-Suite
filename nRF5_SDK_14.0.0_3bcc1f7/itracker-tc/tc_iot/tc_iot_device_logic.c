#include "tc_iot_device_config.h"
#include "tc_iot_device_logic.h"
#include "tc_iot_export.h"

int _tc_iot_shadow_property_control_callback(tc_iot_event_message *msg, void * client,  void * context);
void operate_device(tc_iot_shadow_local_data * device);

/* 影子数据 Client  */
tc_iot_shadow_client g_tc_iot_shadow_client;

tc_iot_shadow_client * tc_iot_get_shadow_client(void) {
    return &g_tc_iot_shadow_client;
}


/* 设备本地数据类型及地址、回调函数等相关定义 */
tc_iot_shadow_property_def g_tc_iot_shadow_property_defs[] = {
    { "temp", TC_IOT_PROP_temp, TC_IOT_SHADOW_TYPE_NUMBER, offsetof(tc_iot_shadow_local_data, temp) },
    { "humidity", TC_IOT_PROP_humidity, TC_IOT_SHADOW_TYPE_NUMBER, offsetof(tc_iot_shadow_local_data, humidity) },
    { "light", TC_IOT_PROP_light, TC_IOT_SHADOW_TYPE_NUMBER, offsetof(tc_iot_shadow_local_data, light) },
    { "latitude", TC_IOT_PROP_latitude, TC_IOT_SHADOW_TYPE_NUMBER, offsetof(tc_iot_shadow_local_data, latitude) },
    { "longitude", TC_IOT_PROP_longitude, TC_IOT_SHADOW_TYPE_NUMBER, offsetof(tc_iot_shadow_local_data, longitude) },
};


/* 设备当前状态数据 */
tc_iot_shadow_local_data g_tc_iot_device_local_data = {
    -100,
    0,
    0,
    0,
    0,
};

/* 设备状态控制数据 */
static tc_iot_shadow_local_data g_tc_iot_device_desired_data = {
    -100,
    0,
    0,
    0,
    0,
};

/* 设备已上报状态数据 */
tc_iot_shadow_local_data g_tc_iot_device_reported_data = {
    -100,
    0,
    0,
    0,
    0,
};

/* 设备初始配置 */
tc_iot_shadow_config g_tc_iot_shadow_config = {
    {
        {
            /* device info*/
            TC_IOT_CONFIG_DEVICE_SECRET, TC_IOT_CONFIG_DEVICE_PRODUCT_ID,
            TC_IOT_CONFIG_DEVICE_NAME, TC_IOT_CONFIG_DEVICE_CLIENT_ID,
            TC_IOT_CONFIG_DEVICE_USER_NAME, TC_IOT_CONFIG_DEVICE_PASSWORD, 0,
        },
        TC_IOT_CONFIG_SERVER_HOST,
        TC_IOT_CONFIG_SERVER_PORT,
        TC_IOT_CONFIG_COMMAND_TIMEOUT_MS,
        TC_IOT_CONFIG_TLS_HANDSHAKE_TIMEOUT_MS,
        TC_IOT_CONFIG_KEEP_ALIVE_INTERVAL_SEC,
        TC_IOT_CONFIG_CLEAN_SESSION,
        TC_IOT_CONFIG_USE_TLS,
        TC_IOT_CONFIG_AUTO_RECONNECT,
        TC_IOT_CONFIG_ROOT_CA,
        TC_IOT_CONFIG_CLIENT_CRT,
        TC_IOT_CONFIG_CLIENT_KEY,
        NULL,
        NULL,
        0,  /* send will */
        {
            {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0,
        }
    },
    TC_IOT_SUB_TOPIC_DEF,
    TC_IOT_PUB_TOPIC_DEF,
    tc_iot_device_on_message_received,
    TC_IOT_PROPTOTAL,
    &g_tc_iot_shadow_property_defs[0],
    _tc_iot_shadow_property_control_callback,
    &g_tc_iot_device_local_data,
    &g_tc_iot_device_reported_data,
    &g_tc_iot_device_desired_data,
};


static int _tc_iot_property_change( int property_id, void * data) {
    tc_iot_shadow_number temp;
    tc_iot_shadow_number humidity;
    tc_iot_shadow_number light;
    tc_iot_shadow_number latitude;
    tc_iot_shadow_number longitude;
    switch (property_id) {
        case TC_IOT_PROP_temp:
            temp = *(tc_iot_shadow_number *)data;
            g_tc_iot_device_local_data.temp = temp;
            TC_IOT_LOG_TRACE("do something for temp=%f", temp);
            break;
        case TC_IOT_PROP_humidity:
            humidity = *(tc_iot_shadow_number *)data;
            g_tc_iot_device_local_data.humidity = humidity;
            TC_IOT_LOG_TRACE("do something for humidity=%f", humidity);
            break;
        case TC_IOT_PROP_light:
            light = *(tc_iot_shadow_number *)data;
            g_tc_iot_device_local_data.light = light;
            TC_IOT_LOG_TRACE("do something for light=%f", light);
            break;
        case TC_IOT_PROP_latitude:
            latitude = *(tc_iot_shadow_number *)data;
            g_tc_iot_device_local_data.latitude = latitude;
            TC_IOT_LOG_TRACE("do something for latitude=%f", latitude);
            break;
        case TC_IOT_PROP_longitude:
            longitude = *(tc_iot_shadow_number *)data;
            g_tc_iot_device_local_data.longitude = longitude;
            TC_IOT_LOG_TRACE("do something for longitude=%f", longitude);
            break;
        default:
            TC_IOT_LOG_WARN("unkown property id = %d", property_id);
            return TC_IOT_FAILURE;
    }

    TC_IOT_LOG_TRACE("operating device");
    operate_device(&g_tc_iot_device_local_data);
    return TC_IOT_SUCCESS;

}

int _tc_iot_shadow_property_control_callback(tc_iot_event_message *msg, void * client,  void * context) {
    tc_iot_shadow_property_def * p_property = NULL;

    if (!msg) {
        TC_IOT_LOG_ERROR("msg is null.");
        return TC_IOT_FAILURE;
    }

    if (msg->event == TC_IOT_SHADOW_EVENT_SERVER_CONTROL) {
        p_property = (tc_iot_shadow_property_def *)context;
        if (!p_property) {
            TC_IOT_LOG_ERROR("p_property is null.");
            return TC_IOT_FAILURE;
        }

        return _tc_iot_property_change(p_property->id, msg->data);
    } else if (msg->event == TC_IOT_SHADOW_EVENT_REQUEST_REPORT_FIRM) {
        tc_iot_report_firm(tc_iot_get_shadow_client(),
                "product", g_tc_iot_shadow_config.mqtt_client_config.device_info.product_id,
                "device", g_tc_iot_shadow_config.mqtt_client_config.device_info.device_name,
                "sdk-ver", TC_IOT_SDK_VERSION,
                "firm-ver","1.0", NULL);
    } else {
        TC_IOT_LOG_TRACE("unkown event received, event=%d", msg->event);
    }
    return TC_IOT_SUCCESS;
}

