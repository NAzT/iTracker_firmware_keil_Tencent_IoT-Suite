#ifdef __cplusplus
extern "C" {
#endif

#include "tc_iot_inc.h"
#include "rak_uart_gsm.h"
	
/**
 * @brief tc_iot_hal_net_read 接收网络对端发送的数据
 *
 * @param network 网络连接对象
 * @param buffer 接收缓存区
 * @param len 接收缓存区大小
 * @param timeout_ms 最大等待时延，单位ms
 *
 * @return 结果返回码或成功读取字节数, 
 *  假如timeout_ms超时读取了0字节, 返回 TC_IOT_NET_NOTHING_READ
 *  假如timeout_ms超时读取字节数没有达到 len , 返回TC_IOT_NET_READ_TIMEOUT
 *  假如timeout_ms超时对端关闭连接, 返回 实际读取字节数
 * @see tc_iot_sys_code_e
 */
int tc_iot_hal_net_read(tc_iot_network_t* network, unsigned char* buffer,
                        int len, int timeout_ms) {
	tc_iot_timer expired_ts;
    int ret_sum = 0;
    int timeout = timeout_ms;
    
	IOT_DEBUG("tc_iot_hal_net_read len  %d timeout %d\r\n", len , timeout_ms);
													
    IF_NULL_RETURN(network, TC_IOT_NULL_POINTER);

    tc_iot_hal_timer_init(&expired_ts);
    tc_iot_hal_timer_countdown_ms(&expired_ts, timeout_ms);
		

    while (ret_sum < len && !tc_iot_hal_timer_is_expired(&expired_ts))
    {
        int ret = Gsm_RecvData2(buffer + ret_sum , len - ret_sum , timeout);

        if (ret == -1)
        {
            return TC_IOT_NET_NOTHING_READ;
        }
        if (ret < -1)
        {
            return TC_IOT_SUCCESS;
        }

        ret_sum+=ret ;

    }

    if (ret_sum < len )
        return TC_IOT_NET_READ_TIMEOUT;

    return ret_sum ;
	
}

int tc_iot_hal_net_write(tc_iot_network_t* network, const unsigned char* buffer,
                         int len, int timeout_ms) {
    int ret = 0;
    ret = Gsm_SendDataCmd((char*)buffer, len);

    IOT_DEBUG("tc_iot_hal_net_write len  %d ret %d\r\n", len , ret);
    return ret;
}

static int tcp_stm32_connect( void* ctx , const char *host,  int port)
{
    int  ret = -1;
    ret = Gsm_OpenSocketCmd(GSM_TCP_TYPE, (char*)host, port); 
    if(ret != 0)
    {
        Gsm_CloseSocketCmd(); 
        return -1; 
    }
    return ret;
}

static int tcp_stm32_disconn(void* ctx)
{
    return Gsm_CloseSocketCmd(); 
}

int _net_set_block( int fd )
{
    
    return 0;
}

int tc_iot_hal_net_connect(tc_iot_network_t* network, const char* host,
                           uint16_t port) {

    int ret = TC_IOT_SUCCESS;
     
    tc_iot_net_context_t * pNetContext = &(network->net_context);
    int set_ret;
    
    //FUNC_TRACE;
     
    IOT_DEBUG("tc_iot_hal_net_connect host %s:%d\r\n", host , port);

    if(NULL == network) {
        return TC_IOT_NULL_POINTER;
    }
    
    if (host==NULL)
        host = pNetContext->host;
    if (port==0)
        port = pNetContext->port;
     
    IOT_DEBUG("tcp_stm32_connect to %s/%d...\r\n", host , port);
     

    if((ret = tcp_stm32_connect(pNetContext->extra_context , host, port)) != 0) {
         IOT_ERROR(" failed\n  ! tcp_stm32_connect returned -0x%x\r\n", -ret);
         return ret;
    }   
     set_ret = _net_set_block(pNetContext->fd);
     if(set_ret != 0) {
         IOT_ERROR(" failed\n  ! net_set_(non)block() returned -0x%x\r\n", -set_ret);
         return TC_IOT_NET_CONNECT_FAILED;
     } IOT_DEBUG(" ok\n");
     

     pNetContext->is_connected = 1;
     return  ret;
}

int tc_iot_hal_net_is_connected(tc_iot_network_t* network) {
    return network->net_context.is_connected; 

}

int tc_iot_hal_net_disconnect(tc_iot_network_t* network) {
    tc_iot_net_context_t * pNetContext = &(network->net_context);
    int ret ;

    pNetContext->is_connected = 0;
    ret = tcp_stm32_disconn(pNetContext);
    return ret;
}

int tc_iot_hal_net_destroy(tc_iot_network_t* network) {
    //LOG_ERROR("not implemented");  
    //return TC_IOT_FUCTION_NOT_IMPLEMENTED;
    return 0;
}

int tc_iot_copy_net_context(tc_iot_net_context_t * net_context, tc_iot_net_context_init_t * init) {
    //IF_NULL_RETURN(net_context, TC_IOT_NULL_POINTER);
    //IF_NULL_RETURN(init, TC_IOT_NULL_POINTER);

    net_context->use_tls           = init->use_tls      ;
    net_context->host              = init->host         ;
    net_context->port              = init->port         ;
    net_context->fd                = init->fd           ;
    net_context->is_connected      = init->is_connected ;
    net_context->extra_context     = init->extra_context;

#ifdef ENABLE_TLS
    net_context->tls_config = init->tls_config;
#endif
	  return TC_IOT_SUCCESS;
}

int tc_iot_hal_net_init(tc_iot_network_t* network,
                        tc_iot_net_context_init_t* net_context) {
    if (NULL == network) {
        return TC_IOT_NETWORK_PTR_NULL;
    }

    network->do_read = tc_iot_hal_net_read;
    network->do_write = tc_iot_hal_net_write;
    network->do_connect = tc_iot_hal_net_connect;
    network->do_disconnect = tc_iot_hal_net_disconnect;
    network->is_connected = tc_iot_hal_net_is_connected;
    network->do_destroy = tc_iot_hal_net_destroy;
    tc_iot_copy_net_context(&(network->net_context), net_context);

    return TC_IOT_SUCCESS;
}

#ifdef __cplusplus
}
#endif
