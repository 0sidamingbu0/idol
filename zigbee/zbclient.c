/*
//gcc zbclient.c -o zbclient -lwiringPi -lcurl -lpthread -ljson
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

#define START_STATE    0
#define AA_STATE       1
#define C0_STATE       2

#define MXJ_CTRL_DOWN                0x00
#define MXJ_CTRL_UP                  0x01
#define MXJ_REGISTER_REQUEST         0x02
#define MXJ_REGISTER_OK              0x03
#define MXJ_REGISTER_FAILED          0x04
#define MXJ_GET_STATE                0x05
#define MXJ_SEND_STATE               0x06
#define MXJ_SENSOR_DATA              0x07
#define MXJ_CONFIG_WRITE             0x08
#define MXJ_CONFIG_READ              0x09


int usart_fd;
uint8_t rebuf[100];    
uint8_t rx_step = 0;	
char headerStr[100];

json_object *my_object;
json_object *val = NULL;

typedef struct
{
  uint16_t id;
  uint8_t state1;
  uint8_t state2;
  uint8_t state3;
  uint8_t enable;
} MXJ_CONFIG_STRUCT;


int send_usart(uint16_t len,uint8_t *data);

long writer(void *data, int size, int nmemb, char *content)
{
	my_object = json_tokener_parse(data);
	json_object_object_get_ex(my_object, "foo", &val);
	printf("data: %s\n", data);
	//val = json_object_object_get(my_object, "foo");
	printf("foo: %s\n", json_object_to_json_string(val));
	return 1; 
}



int send_usart(uint16_t len,uint8_t *data) //id,state1,state2,state3 1=开,0=关,2=保持
{
  uint8_t i=0;
  if(len>100)
  	{
  		printf("send usart failed! len>100");
  		return 1;
  	}
   
  for(i=0;i<len;i++)
    serialPutchar(usart_fd,data[i]);
    return 0;
}

/*********************************************************************
 * @fn      MXJ_SendConfigMessage
 *
 * @brief   Send the config message to endpoint.
 *
 * @param   id des id; idx=0,1,2; state=0,1
 *
 * @return  none
 */
void MXJ_SendConfigMessage( uint16_t id,uint8_t idx,uint8_t state, MXJ_CONFIG_STRUCT mxjconfig )
{
  uint8_t data[12]={MXJ_CONFIG_WRITE,10,id>>8&0xff,id&0xff,idx,state,(mxjconfig.id>>8&0xff),(mxjconfig.id&0xff),mxjconfig.state1,mxjconfig.state2,mxjconfig.state3,mxjconfig.enable};
  send_usart(12,data);
  
}


//========thread start=========//
//AA C0 IDH IDL STATE1 STATE2 STATE3 REV CRC AB
void thread(void)
{
	uint8_t ch = 0;
	uint8_t i = 0;
  uint8_t temp2=0;
	while (1)
	{
		ch = serialGetchar(usart_fd);
		switch (rx_step)
		{
		  case START_STATE:
		    if (ch == 0xAA)
		      rx_step = AA_STATE;
		    break;

		  case AA_STATE:
		    if (ch == 0xC0)
		      rx_step = C0_STATE;
		    break;
		  case C0_STATE:
		    if(i < 7)
		    {
		      rebuf[i++] = ch;
		    }
		    else
		    {
		      rx_step = START_STATE;
		      i=0;
		      if(ch == 0xAB)
		      {
		        temp2 = (rebuf[0]+rebuf[1]+rebuf[2]+rebuf[3]+rebuf[4]+rebuf[5]);
		        //temp = temp2&0xff;
		        if(rebuf[6] == (temp2&0xff))//01=pm25 23=pm10
		        {  		            
				printf("%02X ",rebuf[0]);
                printf("%02X ",rebuf[1]);
                printf("%02X ",rebuf[2]);
                printf("%02X ",rebuf[3]);
                printf("%02X ",rebuf[4]);
                printf("%02X ",rebuf[5]);
                printf("%02X ",rebuf[6]);                                        
		        }
		      }
		    }
		    break;
			
		  default:
		   break;
		}
	}

}
int main(void)
{
	


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
	
 
 static uint8_t i=1;
	CURL* geturl;
	geturl = curl_easy_init();
	char *str = "127.0.0.1:8888";

	if(geturl){

		 curl_easy_setopt(geturl, CURLOPT_URL, "127.0.0.1:8888");
		 /* Now specify the POST data */
     //curl_easy_setopt(geturl, CURLOPT_TIMEOUT, 2L);
     curl_easy_setopt(geturl, CURLOPT_HTTPPOST, 1L);
		 curl_easy_setopt(geturl, CURLOPT_WRITEFUNCTION, writer);
		 curl_easy_setopt(geturl, CURLOPT_WRITEDATA, &headerStr);

         //curl_easy_cleanup(geturl);

	}
	else
		return (1);

	while(1)
	{
		printf("This is the zbclient process.\n");
		sleep(1);
   //usleep(50000);
		i=3-i;
		int res;
		if(i==1)
		{
			res=curl_easy_setopt(geturl, CURLOPT_POSTFIELDS, "open=submit");		 
		 	/* Perform the request, res will get the return code */
		 	res=curl_easy_perform(geturl);
			
			
		}
		else
		{
			res=curl_easy_setopt(geturl, CURLOPT_POSTFIELDS, "close=submit");		 
		 	/* Perform the request, res will get the return code */
		 	res=curl_easy_perform(geturl);
			
			
		}
	}
	
	//pthread_join(id,NULL);

	serialClose(usart_fd);
	return (0);
}

