/*
//gcc zbclient.c -o zbclient -lwiringPi -lcurl -lpthread -ljson-c
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <curl.h>
#include <string.h>
#include "json.h"
//#include "parse_flags.h"


typedef unsigned char   uint8_t;     //无符号8位数

#define WW_STATE      0x55
#define AA_STATE      0xaa
#define CMD_STATE1     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

#define PORT            6868
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     3000
#define MAXANSWERSIZE   512
#define GET             0
#define POST            1

#define MXJ_GATEWAY_CLUSTERID        0x01
  
#define MXJ_PM25_CLUSTERID           0x02
#define MXJ_HUMAN_CLUSTERID          0x03
#define MXJ_SWITCH_CLUSTERID         0x04
#define MXJ_LIGHT_CLUSTERID          0x05
#define MXJ_POWER_CLUSTERID          0x06
#define MXJ_REMOTE_CLUSTERID         0x07
#define MXJ_LIGHT_SENSOR_CLUSTERID   0x08
#define MXJ_TEMPERATURE_CLUSTERID    0x09
#define MXJ_DOOR_CLUSTERID           0x0A
#define MXJ_REV1_CLUSTERID           0x0B
#define MXJ_REV2_CLUSTERID           0x0C
#define MXJ_REV3_CLUSTERID           0x0D
#define MXJ_REV4_CLUSTERID           0x0E
#define MXJ_REV5_CLUSTERID           0x0F
  
#define MXJ_CTRL_DOWN                0x00
#define MXJ_CTRL_UP                  0x01
#define MXJ_REGISTER_REQUEST         0x02
#define MXJ_REGISTER_OK              0x03
#define MXJ_REGISTER_FAILED          0x04
#define MXJ_GET_STATE                0x05
#define MXJ_SEND_STATE               0x06
#define MXJ_SENSOR_DATA              0x07
#define MXJ_CONFIG_WRITE             0x08
#define MXJ_CONFIG_RETURN            0x09
#define MXJ_CONFIG_GET               0x0a
#define MXJ_SEND_IDXS                0x0B
#define MXJ_SEND_RESPONSE            0x0D



#define MXJ_XIAOMI18                 0x18
#define MXJ_XIAOMI1C                 0x1C
#define MXJ_SEND_DATA                0xff
int usart_fd;
uint8_t rebuf[100];    
uint8_t rx_step = 0;	
uint8_t state = WW_STATE;
uint8_t  LEN_Token;
uint8_t  FSC_Token;
uint8_t  tempDataLen = 0;
uint8_t rxbuf[100];
uint8_t len=0;
uint8_t humaned = 0;
CURL* geturl;

void send_usart(uint8_t *data,uint8_t len);
void MXJ_SendRegisterMessage( uint16_t , uint8_t );
void MXJ_SendCtrlMessage( uint16_t , uint8_t , uint8_t , uint8_t );
void MXJ_SendStateMessage( uint16_t );
void MXJ_GetConfigMessage( uint16_t id );
void MXJ_GetStateMessage( uint16_t id );

char headerStr[100];

//json_object *my_object;
//json_object *val = NULL;

typedef struct
{
  uint16_t id;
  uint8_t state1;
  uint8_t state2;
  uint8_t state3;
  uint8_t enable;
} MXJ_CONFIG_STRUCT;


void send_usart(uint8_t *data, uint8_t len);

long writer(void *data, int size, int nmemb, char *content)
{
	uint16_t id=0;
	uint8_t state11=0,state22=0,state33=0;
	json_object *type = NULL;
	json_object *my_object = json_tokener_parse(data);
	json_object *devid = NULL;
	json_object_object_get_ex(my_object, "devid",&devid);
	json_object *state1 = NULL;
	json_object_object_get_ex(my_object, "state1",&state1);
	json_object *state2 = NULL;
	json_object_object_get_ex(my_object, "state2",&state2);
	json_object *state3 = NULL;
	json_object_object_get_ex(my_object, "state3",&state3);
	json_object_object_get_ex(my_object, "type",&type);

	id = (uint16_t)json_object_get_int(devid);
	state11 = (uint16_t)json_object_get_int(state1);
	state22 = (uint16_t)json_object_get_int(state2);
	state33 = (uint16_t)json_object_get_int(state3);

	if(0 == strcmp (json_object_to_json_string(type), "\"control_down\""))
		MXJ_SendCtrlMessage(id,state11,state22,state33);
	if(0 == strcmp (json_object_to_json_string(type), "\"register_ok\""))
		MXJ_SendRegisterMessage(id,MXJ_REGISTER_OK);
	if(0 == strcmp (json_object_to_json_string(type), "\"register_failed\""))
		MXJ_SendRegisterMessage(id,MXJ_REGISTER_FAILED);
	if(0 == strcmp (json_object_to_json_string(type), "\"ask_state\""))
		MXJ_GetStateMessage(id);
	if(0 == strcmp (json_object_to_json_string(type), "\"any_data\""))
		MXJ_SendCtrlMessage(id,state11,state22,state33);
	
	printf("id = %d\n", id);
	printf("state11 = %d\n", state11);
	printf("state22 = %d\n", state22);
	printf("state33 = %d\n", state33);
	printf("data= %s\n", data);
	printf("json= %s\n", json_object_to_json_string(my_object));
	
	return 1; 
}



void send_usart(uint8_t *data,uint8_t len) //id,state1,state2,state3 1=开,0=关,2=保持
{
  uint8_t txbuf[100];
  uint8_t i=0;
  uint8_t crc = 0;
  txbuf[0] = 0x55;
  txbuf[1] = 0xaa;
  
  for(i=0;i<len;i++)
  {
    txbuf[i+2] = data[i];
    crc += data[i];
  }
  txbuf[len+2] = crc;
  printf("\n");
  printf("send:");
  for(i=0;i<len+3;i++)
  {
    serialPutchar(usart_fd,txbuf[i]);
    printf("%02x ",txbuf[i]);
  }
  printf("\n");
}


void recieve_usart(uint8_t *rx,uint8_t len)
{
  int i=0,j=0;
  int id=0,cid=0;
  int tempctrl[3]={2,2,2};
  printf("\n");
  printf("recieved:%d\n",len);
  
  for(i=0;i<len;i++)
    printf("%02x ",rx[i]);
  printf("\n");
  printf("------------------------\n");
  
 
  if(len>=4)
	{
		id=rx[2];
		id <<= 8;
		id |= rx[3];
	}

  if(len>=6)
	{
		cid=rx[4];
		cid <<= 8;
		cid |= rx[5];
	}
  
  switch(rx[0])
  {
    case MXJ_CTRL_UP:
		{
			char str[200]={0};
			sprintf(str,"type=control_up&dev_id=%d&idx=%d&action=%d&action_string=normal",id,rx[5],rx[6]);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);		
    	}
    break;
    
    case MXJ_REGISTER_REQUEST:
		{
			char str[200]={0};
			sprintf(str,"type=register_request&dev_id=%d&dev_type=%d&dev_string=NULL&idxs=%d",id,rx[4],rx[5]);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);	
    	}
    break;

    case MXJ_SEND_STATE:
		{
			char str[200]={0};
			sprintf(str,"type=response_state&dev_id=%d&state1=%d&state2=%d&state3=%d",id,rx[4],rx[5],rx[6]);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);	
    	}
    break;

    case MXJ_SENSOR_DATA:
      //MXJ_GetStateMessage(id);
    break;

    case MXJ_GET_STATE:
		{
			printf("get state\n");
			char str[200]={0};
			sprintf(str,"type=ask_state&dev_id=%d",id);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);	
		}
    break;

	case MXJ_SEND_RESPONSE:
		{
			printf("send response\n");
			char str[200]={0};
			sprintf(str,"type=send_response&dev_id=%d&seq_num=%d&state=%d",id,rx[2],rx[3]);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);	
		}
    break;	
		
    case MXJ_XIAOMI18:
	

	 if(len>=27)
 	{
 		if(cid == 0 && rx[12] <= len - 13)
		{
			 uint8_t *temp_str = NULL;
		    /*分配内存空间*/
		    temp_str = (uint8_t*)calloc(rx[12],sizeof(uint8_t));
		    /*将hello写入*/
		    strncpy(temp_str, &rx[13],rx[12]);
			printf("temp_str = %s\n",temp_str);
			if (0 == strcmp (temp_str, "lumi.sensor_switch"))
			{
				char str[200]={0};
				sprintf(str,"type=register_request&dev_id=%d&dev_type=11&dev_string=xiaomikaiguan&idxs=1",id);
				curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(geturl);

			}
			else if (0 == strcmp (temp_str, "lumi.sensor_magnet"))
			{
				char str[200]={0};
				sprintf(str,"type=register_request&dev_id=%d&dev_type=10&dev_string=xiaomimenci&idxs=1",id);
				curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);

				curl_easy_perform(geturl);			
			}
			else if (0 == strcmp (temp_str, "lumi.sensor_motion"))
			{
				char str[200]={0};
				sprintf(str,"type=register_request&dev_id=%d&dev_type=3&dev_string=xiaomihuman&idxs=1",id);
				curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);

				curl_easy_perform(geturl);			
			}
			else if (0 == strcmp (temp_str, "lumi.sensor_ht"))
			{
				char str[200]={0};
				sprintf(str,"type=register_request&dev_id=%d&dev_type=9&dev_string=xiaomiwenshidu&idxs=1",id);
				curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(geturl);			
			}
		}
 	}

 		if(cid == 6)
 		{
 			if(len != 13)break;
			printf("control up\n");
			printf("id:%4x\n",id);
			if(rx[11] == 0x20)
			{
				printf("double tick\n");
				char str[200]={0};
				sprintf(str,"type=control_up&dev_id=%d&idx=1&action=%d&action_string=double_tick",id,2);
				curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(geturl);							
			}
			else
			{
				printf("action = %d\n",rx[12]);
				char str[200]={0};
				sprintf(str,"type=control_up&dev_id=%d&idx=1&action=%d&action_string=normal",id,rx[12]);
				curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(geturl);
			}
 		}
		else if(cid == 0x406)
 		{
 			if(len != 13)break;
			printf("control up\n");
			printf("id:%4x\n",id);
			printf("human detected\n");
			printf("action = %d\n",rx[12]);
			char str[200]={0};
			sprintf(str,"type=control_up&dev_id=%d&idx=1&action=%d&action_string=human_detected",id,1);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);		
 		}
		else if(cid == 0x402)
 		{
 			int16_t temp = 0;
			if(len != 14)break;
			temp = rx[13];
			temp <<= 8;
			temp |= rx[12];
			printf("temperature up\n");
			printf("id:%4x\n",id);
			printf("temperature = %02f\n",(float)temp/100);
			printf("action = %d\n",rx[12]);
			char str[200]={0};
			sprintf(str,"type=temperature_up&dev_id=%d&idx=1&value=%d",id,temp);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);			
 		}
		else if(cid == 0x405)
 		{
 		 	uint16_t temp = 0;
			if(len != 14)break;
			temp = rx[13];
			temp <<= 8;
			temp |= rx[12];
			printf("humidity up\n");
			printf("id:%4x\n",id);
			printf("humidity = %02f\n",(float)temp/100);
			char str[200]={0};
			sprintf(str,"type=humidity_up&dev_id=%d&idx=1&value=%d",id,temp);
			curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			curl_easy_perform(geturl);	

		}




		
 	
    break;

	case MXJ_XIAOMI1C:

	break;

    
  }
}

void thread(void)
{
	uint8_t ch = 0;
	uint8_t i = 0;
  uint8_t temp2=0;
  uint16_t id=0;
  uint8_t datax[7]={0,6,0x5e,0xe0,1,1,1};
	while (1)
	{
		ch = serialGetchar(usart_fd);
   //printf("recieve ch=%02x\n",ch);
		 switch (state)
    {
      case WW_STATE:
        if (ch == 0x55)
          state = AA_STATE;
          //printf("55");
        break;
        
      case AA_STATE:
        if (ch == 0xaa)
          state = CMD_STATE1;
        else
          state = WW_STATE;
        break;
        
      case CMD_STATE1:
        rxbuf[0] = ch;
        state = LEN_STATE;
        break;
        
      case LEN_STATE:
        LEN_Token = ch;

        tempDataLen = 0;     
          rxbuf[1] = LEN_Token;
          state = DATA_STATE;

        break;


      case DATA_STATE:

        // Fill in the buffer the first byte of the data 
        rxbuf[2 + tempDataLen++] = ch;

        // If number of bytes read is equal to data length, time to move on to FCS 
        if ( tempDataLen == LEN_Token - 1 )
            state = FCS_STATE;

        break;

      case FCS_STATE:

        FSC_Token = ch;
        uint8_t ii;
        uint8_t crcout;
        // Make sure it's correct 
        crcout = 0;
        for(ii=0;ii<1 + LEN_Token;ii++)
          crcout += rxbuf[ii];

        rxbuf[LEN_Token + 1] = crcout;
        if (crcout == FSC_Token)
        {
          //printf("recieve:");
          len = LEN_Token+1;
          for(ii=0;ii<LEN_Token+1;ii++)
            //printf("%02x",rxbuf[ii]);
            
          //printf("\n");
          id=rxbuf[2];
          id<<=8;
          id|=rxbuf[3];
          
          if(id==0x6eab)
          {
             datax[4]=rxbuf[6];            
             send_usart(datax,7);
          }
          recieve_usart(rxbuf,LEN_Token+1);
          
          
          
        }
        else
        {
          // deallocate the msg 
          printf("crc wrong\n");
        }

        // Reset the state, send or discard the buffers at this point 
        state = WW_STATE;

        break;

      default:
        state = WW_STATE;
       break;
    }
	}

}

int main(void)
{
	
	static uint8_t i=1;

	geturl = curl_easy_init();
	

	if(geturl){

		curl_easy_setopt(geturl, CURLOPT_URL, "127.0.0.1:6868");
		/* Now specify the POST data */
	//curl_easy_setopt(geturl, CURLOPT_TIMEOUT, 2L);
	curl_easy_setopt(geturl, CURLOPT_HTTPPOST, 1L);
		curl_easy_setopt(geturl, CURLOPT_WRITEFUNCTION, writer);
		curl_easy_setopt(geturl, CURLOPT_WRITEDATA, &headerStr);

		//curl_easy_cleanup(geturl);

	}
	else
	   return (1);


	pthread_t id;
  
	if(wiringPiSetup() < 0)
	{
		printf("wiringpi setup failed!\n");
		return 1;
	}
	if((usart_fd = serialOpen("/dev/ttyAMA0",115200)) < 0)
	{
		printf("serial open failed!\n");
		return 1;
	}
	printf("serial test start ...\n");

	//ret=pthread_create(&id,NULL,(void *) thread,NULL);
	if(pthread_create(&id,NULL,(void *) thread,NULL)!=0)
	{
		printf ("Create pthread error!\n");
		return (1);
	}
	
 
	 MXJ_SendRegisterMessage( 0x9b02, MXJ_REGISTER_FAILED );
	 MXJ_SendRegisterMessage( 0x49f0, MXJ_REGISTER_FAILED );


	while(1)
	{
		static uint8_t i=0;
		//printf("This is the zbclient process.\n");
		sleep(1);
   //usleep(50000);

		
	}
	
	//pthread_join(id,NULL);

	serialClose(usart_fd);
	return (0);
}
/*********************************************************************
 * @fn      MXJ_SendCtrlMessage
 *
 * @brief   Send the ctrl message to endpoint.
 *
* @param   ID
 *
 * @return  none
 */
void MXJ_SendCtrlMessage( uint16_t id ,uint8_t msg1 , uint8_t msg2 , uint8_t msg3 )
{
//  uint8_t data[5]={MXJ_CTRL_DOWN,3,msg1,msg2,msg3};//自定义数据
  uint8_t data[7]={0,6,(uint8_t)(id>>8),(uint8_t)id,msg1,msg2,msg3};
  send_usart(data,7);
}

/*********************************************************************
 * @fn      MXJ_SendRegisterMessage
 *
 * @brief   Send the register_request message to endpoint.
 *
 * @param   none
 *
 * @return  none
 
      //uint8_t data[7]={0,6,0x17,0x1f,1,1,1};//控制命令↓
     //uint8_t data[4]={10,3,0x17,0x1f};//开关配置文件获取↓
     //uint8_t data[12]={8,11,0x17,0x1f,0,0,0x7b,0x4f,1,2,2,1};//开关配置文件设置↓    
     //send_usart(data,12);
 */
void MXJ_SendRegisterMessage( uint16_t id, uint8_t state )
{
  uint8_t data[5]={0,4,(uint8_t)(id>>8),(uint8_t)id,1};
  if(state == MXJ_REGISTER_OK)
  {
    data[0]=3;
    data[4]=1;
    send_usart(data,5);
  }
  else if(state == MXJ_REGISTER_FAILED)
  {
    data[0]=4;
    data[4]=0;
    send_usart(data,5);
  }
}

/*********************************************************************
 * @fn      MXJ_SendConfigMessage
 *
 * @brief   Send the config message to endpoint.
 *
 * @param   id des id; idx=0,1,2; 
 *
 * @return  none
 */
void MXJ_SendConfigMessage( uint16_t id,uint8_t idx,uint8_t state, MXJ_CONFIG_STRUCT mxjconfig )
{
  uint8_t data[12]={8,11,(uint8_t)(id>>8),(uint8_t)id,idx,state,(mxjconfig.id>>8&0xff),(mxjconfig.id&0xff),mxjconfig.state1,mxjconfig.state2,mxjconfig.state3,mxjconfig.enable};//开关配置文件设置↓ 
  send_usart(data,12);
}


void MXJ_GetConfigMessage( uint16_t id )
{
  uint8_t data[4]={10,3,(uint8_t)(id>>8),(uint8_t)id};//开关配置文件获取↓
  send_usart(data,4);
}

/*********************************************************************
 * @fn      MXJ_GetStateMessage
 *
 * @brief   Get the state message from gateway.
 *
 * @param   none
 *
 * @return  none
 */
void MXJ_GetStateMessage( uint16_t id )
{
  uint8_t data[4]={5,3,(uint8_t)(id>>8),(uint8_t)id};//自定义数据
  send_usart(data,4);
}

