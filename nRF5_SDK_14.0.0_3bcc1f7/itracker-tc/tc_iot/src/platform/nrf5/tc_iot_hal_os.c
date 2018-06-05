#ifdef __cplusplus
extern "C" {
#endif

#include "tc_iot_inc.h"

#include "rak_uart_gsm.h"

#include "app_timer.h"

long tc_iot_hal_timestamp(void* zone) {
    TC_IOT_LOG_ERROR("not implemented");  
    return TC_IOT_FUCTION_NOT_IMPLEMENTED;
}

int tc_iot_hal_sleep_ms(long sleep_ms) { 
    nrf_delay_ms(sleep_ms);
    return 0;
}

 
static unsigned long DJB_hash(const unsigned char* key , int len , unsigned long org_hash)
{
    unsigned long nHash = org_hash;

    while (len-->0)
        nHash = (nHash << 5) + nHash + *key++;

    return nHash;

}

static unsigned int _rand_seed = 0;

//使用buffer, 时间tick和最初的tick计算一个随机数
unsigned long tc_iot_random(void* buf,  int len)
{
	unsigned long hash = 0;
	unsigned long tick = rtc1_get_sys_tick(); //一个移植函数
	//unsigned long tick = app_timer_cnt_get() * 30;
	unsigned char *strbuf = (unsigned char *) buf;
	_rand_seed ++;

	if (_rand_seed >= len )
		_rand_seed = _rand_seed % len;

	hash = DJB_hash(strbuf + _rand_seed ,  len - _rand_seed , hash);
	if (_rand_seed > 0)
	{
		hash = DJB_hash(strbuf ,  _rand_seed  , hash);
	}

	hash = DJB_hash( (const unsigned char*)&tick, sizeof(tick) , hash  );

	return hash;

}

long tc_iot_hal_random() {
    
    return tc_iot_random(Gsm_RxBuf, 512);
}

void tc_iot_hal_srandom(unsigned int seed) { 
    _rand_seed = seed ;
}

#ifdef __cplusplus
}
#endif
