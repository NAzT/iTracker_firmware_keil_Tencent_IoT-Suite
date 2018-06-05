#ifndef TC_IOT_DEVICE_LOGIC_H
#define TC_IOT_DEVICE_LOGIC_H

#include "tc_iot_inc.h"

/* 数据模板本地存储结构定义 local data struct define */
typedef struct _tc_iot_shadow_local_data {
    tc_iot_shadow_number temp;
    tc_iot_shadow_number humidity;
    tc_iot_shadow_number light;
    tc_iot_shadow_number latitude;
    tc_iot_shadow_number longitude;
}tc_iot_shadow_local_data;


/* 数据模板字段 ID 宏定义*/
#define TC_IOT_PROP_temp 0
#define TC_IOT_PROP_humidity 1
#define TC_IOT_PROP_light 2
#define TC_IOT_PROP_latitude 3
#define TC_IOT_PROP_longitude 4

#define TC_IOT_PROPTOTAL 5


/* enum macro definitions */


tc_iot_shadow_client * tc_iot_get_shadow_client(void);
#endif /* end of include guard */
