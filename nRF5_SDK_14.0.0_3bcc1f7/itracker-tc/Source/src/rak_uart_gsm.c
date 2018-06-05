/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include <ctype.h>

#include "rak_uart_gsm.h"

#include "bsp_itracker.h"

//#define  GSM_RXBUF_MAXSIZE           512//0

static uint16_t rxReadIndex  = 0;
static uint16_t rxWriteIndex = 0;
static uint16_t rxCount      = 0;
uint8_t Gsm_RxBuf[GSM_RXBUF_MAXSIZE];

//extern uint8_t uart2_rx_data;
extern int R485_UART_TxBuf(uint8_t *buffer, int nbytes);
/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/



static char get_printf_ascii(int c)
{
    if ( isprint(c))
        return c;

    return '.';
}

void dump_mem(const void* buf, int size)
{
        int line = size/16;

        int i, left ;
        char line_buf[64];
        char char_buf[20];
        char xxx[8];
        const unsigned char *p = buf;

        for (i = 0 ; i<line ; i++)
        {
                sprintf( line_buf , "%02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x",
                        p[0],p[1],p[2],p[3],
                        p[4],p[5],p[6],p[7],
                        p[8],p[9],p[10],p[11],
                        p[12],p[13],p[14],p[15]);

                sprintf( char_buf , "%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
                        get_printf_ascii(p[0]),
                        get_printf_ascii(p[1]),
                        get_printf_ascii(p[2]),
                        get_printf_ascii(p[3]),
                        get_printf_ascii(p[4]),
                        get_printf_ascii(p[5]),
                        get_printf_ascii(p[6]),
                        get_printf_ascii(p[7]),
                        get_printf_ascii(p[8]),
                        get_printf_ascii(p[9]),
                        get_printf_ascii(p[10]),
                        get_printf_ascii(p[11]),
                        get_printf_ascii(p[12]),
                        get_printf_ascii(p[13]),
                        get_printf_ascii(p[14]),
                        get_printf_ascii(p[15]) );              

                IOT_TRACE("%03x %s | %s\n", i , line_buf, char_buf);
                p += 16;
        }

        left = size %16;
        line_buf[0] = 0;
        char_buf[0] = 0;

        for(i=0; i< left ; i++)
        {
                sprintf( xxx, "%02x ", p[i]);
                strcat(line_buf, xxx);
        }
        for(i=0; i< left ; i++)
        {
                sprintf( xxx, "%c", get_printf_ascii(p[i]));
                strcat(char_buf, xxx);
        }       
        IOT_TRACE("endx %s | %s\n", line_buf, char_buf);

}


int gsm_rxbuf_print(const char* prefix)
{
    int  count = 0;
    if (rxCount>0)
    {
        IOT_TRACE("%s BANG!! rxReadIndex %d rxWriteIndex %d rxCount %d ascii %d\n", prefix, 
            rxReadIndex,rxWriteIndex,rxCount, Gsm_RxBuf[rxReadIndex]);
        count = rxCount;
        if (count>32)count = 32;
        dump_mem(Gsm_RxBuf + rxReadIndex, count);
    }  

    return 0;
}




void  AppTaskGPRS ( void const * argument );

void delay_ms(uint32_t ms)
{
		nrf_delay_ms(ms);
}

int GSM_UART_TxBuf(uint8_t *buffer, int nbytes)
{
		uint32_t err_code;
		for (uint32_t i = 0; i < nbytes; i++)
		{
				do
				{
						err_code = app_uart_put(buffer[i]);
						if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
						{
								//NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
								APP_ERROR_CHECK(err_code);
						}
				} while (err_code == NRF_ERROR_BUSY);
		}
		return err_code;
}

void Gsm_RingBuf(uint8_t in_data)
{
	Gsm_RxBuf[rxWriteIndex]=in_data;
    rxWriteIndex++;
    rxCount++;
                      
    if (rxWriteIndex == GSM_RXBUF_MAXSIZE)
    {
        rxWriteIndex = 0;
    }
            
     /* Check for overflow */
    if (rxCount == GSM_RXBUF_MAXSIZE)
    {
        rxWriteIndex = 0;
        rxCount      = 0;
        rxReadIndex  = 0;
    }  
}

		
void Gsm_PowerUp(void)
{
    //DPRINTF(LOG_DEBUG,"GMS_PowerUp\r\n");
	#if defined (M35)
    GSM_PWR_OFF;
    delay_ms(200);
    /*Pwr en wait at least 30ms*/
    GSM_PWR_ON;
    delay_ms(100);
//		GSM_RESET_LOW;
//		delay_ms(200);
//		GSM_RESET_HIGH;
    /*Pwr key low to high at least 2S*/
    GSM_PWRKEY_LOW;
    delay_ms(1000); //2s
    GSM_PWRKEY_HIGH;
    delay_ms(1000);
	#elif defined (BC95)
		GSM_RESET_LOW;
		GSM_PWR_OFF;
    delay_ms(200);
    /*Pwr en wait at least 30ms*/
    GSM_PWR_ON;
    delay_ms(500);
		GSM_RESET_HIGH;
	#endif
}

void Gsm_PowerDown(void)
{
	#if 0
    DPRINTF(LOG_DEBUG,"GMS_PowerDown\r\n");
    GSM_PWD_LOW;
    delay_ms(800); //800ms     600ms > t >1000ms  
    GSM_PWD_HIGH;
    delay_ms(12000); //12s
	  GSM_PWR_EN_DISABLE;
    delay_ms(2000);
	#endif
}

int Gsm_RxByte(void)
{
    int c = -1;

    __disable_irq();
    if (rxCount > 0)
    {
        c = Gsm_RxBuf[rxReadIndex];
				
        rxReadIndex++;
        if (rxReadIndex == GSM_RXBUF_MAXSIZE)
        {
            rxReadIndex = 0;
        }
        rxCount--;
    }
    __enable_irq();

    return c;
}

int Gsm_WaitRspOK(char *rsp_value,uint16_t timeout_ms,uint8_t is_rf)
{
    int retavl=-1,wait_len=0;
    uint16_t time_count=timeout_ms;
    char str_tmp[GSM_GENER_CMD_LEN];
    uint8_t i=0;
    
    memset(str_tmp,0,GSM_GENER_CMD_LEN);
    wait_len=is_rf?strlen(GSM_CMD_RSP_OK_RF):strlen(GSM_CMD_RSP_OK);
       
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);
				//SEGGER_RTT_printf(0, "%c", c);
        str_tmp[i++]=(char)c;
        if(i>=wait_len)
        {
            char *cmp_p=NULL;
            if(is_rf)
                cmp_p=strstr(str_tmp,GSM_CMD_RSP_OK_RF);
            else
                cmp_p=strstr(str_tmp,GSM_CMD_RSP_OK);
            if(cmp_p)
            {
                if(i>wait_len&&rsp_value!=NULL)
                {
                    //printf("--%s  len=%d\r\n", str_tmp, (cmp_p-str_tmp)); 
                    memcpy(rsp_value,str_tmp,(cmp_p-str_tmp));
                }               
                retavl=0;
                break;
            }
                   
        }
        if (i>=GSM_GENER_CMD_LEN)
        {
            IOT_TRACE("AT_COMMAND RESP_overflow %d\r\n", i);
            dump_mem(str_tmp, sizeof(str_tmp) );
            memcpy(str_tmp , str_tmp + GSM_GENER_CMD_LEN - wait_len, wait_len);
            i = wait_len;
        }

    }while(time_count>0);
    IOT_TRACE("AT_COMMAND ret %d RESP %s\r\n", retavl, str_tmp);
    //DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

int Gsm_WaitSendAck(uint16_t timeout_ms)
{
    int retavl=-1;
    uint16_t time_count=timeout_ms;    
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);  
           
        if((char)c=='>')
        {
            //IOT_TRACE("Gsm_WaitSendAck %d-%c tc %d %d\n", c, c, time_count, timeout_ms);  
            retavl=0;
            //break; 
        }
        if(retavl==0 && (char)c==' ')
        {
            break;//wait for space
        }
    }while(time_count>0);
    
    //DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

//return the resp_value buffer len
//return < 0 , error
int Gsm_WaitNewLine(char *rsp_value,uint16_t timeout_ms)
{
    int retavl=-1;
	int i=0;
    uint16_t time_count=timeout_ms;   
	//char new_line[3]={0};
	char str_tmp[GSM_GENER_CMD_LEN];
	//char *cmp_p=NULL;
	memset(str_tmp, 0,GSM_GENER_CMD_LEN);
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);
		//SEGGER_RTT_printf(0, "read %c %d\n", c, c);
		str_tmp[i++]=c;
        if (i>=GSM_GENER_CMD_LEN)
        {
            IOT_TRACE("Gsm_WaitNewLine buffer overflow, %d\r\n", i);
            return -2;
        }
		if(i>=strlen("\r\n"))
		{
				if( str_tmp[i-2]=='\r' && str_tmp[i-1]=='\n')
				{
						memcpy(rsp_value, str_tmp, i);
						retavl=i;
						break;
				}   				
		}

    }while(time_count>0);
    
    //DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

int Gsm_AutoBaud(void)
{
    int retavl=-1,rety_cunt=GSM_AUTO_CMD_NUM;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_AUTO_CMD_STR);
        do
        {
//            DPRINTF(LOG_DEBUG,"\r\n auto baud rety\r\n");
            GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
            retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,NULL);
            delay_ms(500);
            rety_cunt--;
        }while(retavl!=0&& rety_cunt>0);
        
        free(cmd);
    }
    return retavl;
}

int Gsm_FixBaudCmd(int baud)
{
    int retavl=-1;
    char *cmd;
    
    cmd=(char*)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%d%s\r\n",GSM_FIXBAUD_CMD_STR,baud,"&W");
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
        
        free(cmd);
    }
    return retavl;
}

//close cmd echo
int Gsm_SetEchoCmd(int flag)
{
    int retavl=-1;
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%d\r\n",GSM_SETECHO_CMD_STR,flag);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
        
        free(cmd);
    }
    return retavl;
}
//Check SIM Card Status
int Gsm_CheckSimCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKSIM_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        IOT_TRACE("AT_COMMAND REQ %s\r\n", cmd);

        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            if(NULL!=strstr(cmd,GSM_CHECKSIM_RSP_OK))
            {
                retavl=0;
            }  
            else
            {
                retavl=-1;
            }
        }
        
        
        free(cmd);
    }
    
    return retavl;
}
//Check Network register Status
int Gsm_CheckNetworkCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKNETWORK_CMD_STR);
        //DPRINTF(LOG_INFO, "%s", cmd);
		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            
            if (strstr(cmd,GSM_CHECKNETWORK_RSP_OK))
            {
                retavl = 0;
            } 
            else if (strstr(cmd,GSM_CHECKNETWORK_RSP_OK_5)) 
            {
               retavl = 0;
            } 
            else {
               retavl = -1;
            }
        }
        
        
        free(cmd);
    }
    return retavl;
}


//Check GPRS  Status
int Gsm_CheckGPRSCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKGPRS_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            if(!strstr(cmd,GSM_CHECKGPRS_RSP_OK))
            {
                retavl=-1;
            }  
        }
       
        free(cmd);
    }
    return retavl;
}

int Gsm_SetGPRSCmd(void)
{
    int retavl=-1;
    //
    char *cmd = "AT+CGATT?";
    

    uint8_t cmd_len = strlen(cmd);
    GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);
    
    memset(cmd,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT*10,true);

    return retavl;
}


//Set context 0
int Gsm_SetContextCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_SETCONTEXT_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}

//Set tcp buffer mode
int Gsm_SetQNDI(int mode)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%d\r\n","AT+QINDI=" , mode);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        IOT_TRACE("Gsm_SetQNDI %s\n", cmd);
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}

//Set tcp buffer mode
int Gsm_getQLTS()
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n","AT+QLTS" );
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        IOT_TRACE("Gsm_getQLTS %s\n", cmd);
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}

//Set dns mode, if 1 domain, 
int Gsm_SetDnsModeCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_SETDNSMODE_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}
//Check  ATS=1
int Gsm_SetAtsEnCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_ATS_ENABLE_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}

//Check  Rssi
int Gsm_GetRssiCmd()
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_RSSI_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            
            if(!strstr(cmd,GSM_GETRSSI_RSP_OK))
            {
                retavl=-1;
            }  
            else
            {
                
                retavl=atoi(cmd+2+strlen(GSM_GETRSSI_RSP_OK));
                if (retavl == 0) 
                {
                   retavl =53; //113
                }
                else if (retavl ==1) 
                {
                     retavl =111;
                }
                else if (retavl >=2 && retavl <= 30)
                {                   
                    retavl -=2;
                    retavl = 109- retavl*2;                  
                }
                else if (retavl >= 31)
                {
                   retavl =51;
                }         
            }
        }
       
        free(cmd);
    }
    return retavl;
}

void Gsm_CheckAutoBaud(void)
{
    uint8_t  is_auto = true, i=0;
    uint16_t time_count =0;
    uint8_t  str_tmp[64];
	
    delay_ms(800); 
    //check is AutoBaud 
    memset(str_tmp, 0, 64);
	
    do
    {
        int       c;
        c = Gsm_RxByte();
        if(c<=0)
        {
            time_count++;
            delay_ms(2);
            continue;
        }
				
        //R485_UART_TxBuf((uint8_t *)&c,1);		
		if(i<64) {
           str_tmp[i++]=(char)c;
				}
	
        if (i>3 && is_auto == true)
        {
            if(strstr((const char*)str_tmp, FIX_BAUD_URC))
            {
                is_auto = false;
                time_count = 800;  //Delay 400ms          
            }  
        }
    }while(time_count<1000);  //time out 2000ms
    
    if(is_auto==true)
    {
        Gsm_AutoBaud();
       
//        DPRINTF(LOG_DEBUG,"\r\n  Fix baud\r\n");
        Gsm_FixBaudCmd(GSM_FIX_BAUD);
    }  
}


int Gsm_WaitRspTcpConnt(char *rsp_value,uint16_t timeout_ms)
{
    int retavl=-1,wait_len=0;
    uint16_t time_count=timeout_ms;
    char str_tmp[GSM_GENER_CMD_LEN];
    uint8_t i=0;
    
    memset(str_tmp,0,GSM_GENER_CMD_LEN);
    
    wait_len=strlen(GSM_OPENSOCKET_OK_STR);
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);
        str_tmp[i++]=(char)c;
        if(i>=wait_len)
        {
            char *cmp_ok_p=NULL,*cmp_fail_p=NULL;
            
            cmp_ok_p=strstr(str_tmp,GSM_OPENSOCKET_OK_STR);
            cmp_fail_p=strstr(str_tmp,GSM_OPENSOCKET_FAIL_STR);
            if(cmp_ok_p)
            {
                if(i>wait_len&&rsp_value!=NULL)
                {
                     memcpy(rsp_value,str_tmp,(cmp_ok_p-str_tmp));
                }               
                retavl=0;
                break;
            }
            else if(cmp_fail_p)
            {
                retavl=GSM_SOCKET_CONNECT_ERR;
                break;
            }
                      
        }
    }while(time_count>0);
    
//    DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

//Open socket 
int Gsm_OpenSocketCmd(uint8_t SocketType,char *ip,uint16_t DestPort)
{
    int retavl=-1;
    //
    char *cmd;
      
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%s,\"%s\",\"%d\"\r\n",GSM_OPENSOCKET_CMD_STR,
                        (SocketType==GSM_TCP_TYPE)?GSM_TCP_STR:GSM_UDP_STR,
                         ip,DestPort);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        IOT_TRACE( "Gsm_OpenSocketCmd REQ %s\r\n", cmd);

        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT*2,true);       
       
        if(retavl>=0)
        {   
            //recv rsp msg...
            retavl=Gsm_WaitRspTcpConnt(NULL,GSM_OPENSOCKET_CMD_TIMEOUT);           
        }
        
        free(cmd);
    }
    return retavl;
}




//  send data 
int Gsm_SendDataCmd(char *data,uint16_t len)
{
    int retavl=-1;
    //
    char *cmd;
    gsm_rxbuf_print("Gsm_SendDataCmd_start");
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%d\r\n",GSM_SENDDATA_CMD_STR,len);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        gsm_rxbuf_print("Gsm_SendDataCmd_mid1");
        //delay_ms(100);
        retavl = Gsm_WaitSendAck(GSM_GENER_CMD_TIMEOUT*4);//500*4=2000ms
        gsm_rxbuf_print("Gsm_SendDataCmd_mid1b");
        if(retavl ==0)
        {
            GSM_UART_TxBuf((uint8_t *)data,len);
            gsm_rxbuf_print("Gsm_SendDataCmd_mid2");
            //delay_ms(800);
            gsm_rxbuf_print("Gsm_SendDataCmd_mid3");
            retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true); 
            gsm_rxbuf_print("Gsm_SendDataCmd_mid3b");
            if(retavl==0)
            {
                retavl=len;
            } 
			else
			{
					//DPRINTF(LOG_INFO, "ret2=%d", retavl);
			}
        }  
		else
		{
				//DPRINTF(LOG_INFO, "ret1=%d", retavl);
		}
        free(cmd);
    }
    gsm_rxbuf_print("Gsm_SendDataCmd_end");
    return retavl;
}


uint16_t Gsm_RecvData(char *recv_buf,uint16_t block_ms)
{
      int         c=0, i=0;      
      //OS_ERR      os_err; 
      //uint32_t    LocalTime=0, StartTime=0;
            
      int time_left = block_ms;
      
      do
      {
          c= Gsm_RxByte();
          if (c < 0) 
          {                 
               time_left--;
               delay_ms(1); 			 
               continue;
          }
          //R485_UART_TxBuf((uint8_t *)&c,1);
		  SEGGER_RTT_printf(0, "%d-%c ", time_left, c);
          recv_buf[i++] =(char)c;
          
      }while(time_left>0);
      
      IOT_TRACE("Gsm_RecvData %d ms ret %d\n", block_ms, i);
      return i;
}

int Gsm_Wait4Cmd(char *rsp_value, const char* cmd, uint16_t timeout_ms,uint8_t is_rf)
{
    int retavl=-1;
    uint16_t time_count=timeout_ms;
    char str_tmp[GSM_GENER_CMD_LEN];
    uint8_t i=0;
    char *cmp_p=NULL;
    
    memset(str_tmp,0,sizeof(str_tmp));
    //wait_len=is_rf?strlen(GSM_CMD_RSP_OK_RF):strlen(GSM_CMD_RSP_OK);
       
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);
                //SEGGER_RTT_printf(0, "%c", c);
        str_tmp[i++]=(char)c;
        if (i>=GSM_GENER_CMD_LEN)
        {
            IOT_TRACE("Gsm_Wait4Cmd buffer overflow, %d\r\n", i);
            return -2;
        }
        //if(i>=wait_len)
        if (cmp_p==NULL)
        {
            //found cmd
            cmp_p=strstr(str_tmp,cmd);

            if(cmp_p)
            {
                IOT_TRACE("Gsm_Wait4Cmd found cmd %s\r\n", str_tmp);
                retavl=-2;
                if (!is_rf)
                {
                    //need return 
                    if(rsp_value!=NULL && !is_rf)
                    {
                        //don't need rf end, so we can return
                        //printf("--%s  len=%d\r\n", str_tmp, (cmp_p-str_tmp)); 
                        memcpy(rsp_value,str_tmp,(cmp_p-str_tmp));
                        rsp_value[cmp_p-str_tmp] = 0;//add \0
                    }  
                    retavl=0;
                    break;                  
                }    
                 
            }                   
        }
        else  
        {
            //cmd_p not NULL , we have found cmd
            //but we not return yet , so we need found rf end
            const char* rf_p = strstr(cmp_p, "\r\n");
            if (rf_p)
            {
                if(rsp_value!=NULL)
                {
                    memcpy(rsp_value,str_tmp,(rf_p-str_tmp));
                    rsp_value[rf_p-str_tmp] = 0;//add \0
                }
                retavl=0;
                break;
            }

        }
    }while(time_count>0);
    DPRINTF(LOG_INFO,"Gsm_Wait4Cmd ret %d : %s\r\n", retavl,str_tmp);
    //DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

int Gsm_QIRD(char *recv_buf, int len)
{
    int retavl=-1;
    //
    char strbuf[GSM_GENER_CMD_LEN];
    uint8_t cmd_len;
    int read_len = 0;
    int ttl = 100;
    int read_cnt = 0;

    cmd_len=sprintf(strbuf,"%s%d\r\n","AT+QIRD=0,1,0," ,len);
    
    gsm_rxbuf_print("Gsm_QIRD_start");
    GSM_UART_TxBuf((uint8_t *)strbuf,cmd_len);
    IOT_TRACE("RECV_CMD:%s\n", strbuf);
    
    //wait for QIRD resposne
    do {
        memset(strbuf,0,sizeof(strbuf));
        retavl=Gsm_WaitNewLine(strbuf,GSM_GENER_CMD_TIMEOUT); 

        if (retavl<0)
            return -3;//?

        IOT_TRACE("CMD resp line %d:%s", retavl , strbuf);


        char* p1 = strstr(strbuf, "+QIRD:");
        char* p2 = strstr(strbuf, "OK\r\n");
        if (p1==NULL && p2==NULL)
            continue; //read for new line
        
        if (p2!=NULL)
            return -1; //nothing can be readed
        //so p1!=NULL    
        p2 = strstr(p1, "TCP,");

        if (p2==NULL)
            return -2; //response not like "+QIRD: 122.152.224.121:6666,TCP,103"

        p2+=4; //skip "TCP,"
        read_len = atoi(p2);

        break;
    }  while(ttl>0);

    //read tcp data to recv buffer
    for ( ;read_cnt<read_len && read_cnt < len; ) {

        int       c;
        c=Gsm_RxByte();

        if (c<0){
            delay_ms(1);
            ttl--;
            continue;
        }

        recv_buf[read_cnt] = c ;
        read_cnt++;
    }
    retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true); 
            
    gsm_rxbuf_print("Gsm_QIRD_end");
    IOT_TRACE("Gsm_QIRD ret %d\n", read_cnt);

    dump_mem(recv_buf, read_cnt);
    return read_cnt;
}

//return >0 , 读取成功
//return -1 , 什么都没有读取到
//return <-1, 各种错误
int Gsm_RecvData2(char *recv_buf, int len, uint16_t block_ms)
{
    int ret = Gsm_QIRD(recv_buf, len);

    if (ret == -1)
    {
        //noting can read ,  so we wait for +QIRDI:....
        ret = Gsm_Wait4Cmd(NULL, "+QIRDI:", block_ms, true);

        if (ret==0) //+QIRDI:xxx arrived , so we send QIRD
        {
            ret = Gsm_QIRD(recv_buf, len);
            return ret;
        } 
        else 
            return -1; //nothing to read   
    }
    return ret; //just retrun


}

#if 0
//  send data 
int Gsm_SendCmd(const char *command , uint16_t len)
{
    int retavl=-1;
    //
    char *cmd;
      
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%d\r\n","GSM_SENDDATA_CMD_STR",len);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        retavl = Gsm_WaitSendAck(GSM_GENER_CMD_TIMEOUT*4);//500*4=2000ms
        if(retavl ==0)
        {
            GSM_UART_TxBuf((uint8_t *)data,len);
            retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true); 
            if(retavl==0)
            {
                retavl=len;
            } 
                        else
                        {
                                //DPRINTF(LOG_INFO, "ret2=%d", retavl);
                        }
        }  
                else
                {
                        //DPRINTF(LOG_INFO, "ret1=%d", retavl);
                }
        free(cmd);
    }
    return retavl;
}
#endif

//  close socket 
int Gsm_CloseSocketCmd(void)
{
    int retavl=-1;
    char *cmd;
      
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CLOSESOCKET_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
        
        free(cmd);
    }
    return retavl;
}

#if defined (BC95)

int Gsm_CheckNbandCmd(int *band)
{
    int retavl=-1;
    char *cmd;
		
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKNBAND_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        if(retavl>=0)
        {
            
            if(!strstr(cmd,GSM_CHECKNBAND_RSP_OK))
            {
                retavl=-1;
            }  
            else
            {
                *band=atoi(cmd+2+strlen(GSM_GETRSSI_RSP_OK));   
            }
        }
        free(cmd);
    }
    return retavl;
}

int Gsm_CheckConStatusCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKCONSTATUS_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            
            if (strstr(cmd,GSM_CHECKCONSTATUS_RSP_OK))
            {
                retavl = 0;
            }
            else {
               retavl = -1;
            }
        }
        
        
        free(cmd);
    }
    return retavl;
}

#endif
/**
* @}
*/





