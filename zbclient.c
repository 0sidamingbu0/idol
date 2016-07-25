/*
//gcc zbclient.c -o zbclient -lwiringPi -lcurl -lpthread -ljson-c -lmicrohttpd
sudo cp json-c/* /usr/lib/

#build libmicrohttp.so
donwload src
cp to raspi
sudo apt-get install autoconf
sudo apt-get install litool
autoreconf -fi
./configure --disable-curl
sudo make
sudo make install
sudo cp /usr/local/lib/libmicrohttp.* /lib/arm-linux-gnueabihf/
regcc .c
ok

=====
sudo rm /usr/arm-linux-gnueabihf/libmicrohttpd.so
sudo cp libmicrohttpd.so.12.37.0 /usr/arm-linux-gnueabihf/
sudo ln -s libmicrohttpd.so.12.37.0 libmicrohttpd.so.12
sudo ln -s libmicrohttpd.so.12 libmicrohttpd.so

----
ls -l |grep libmicrohttpd
sudo find / -name libmicrohttpd*
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <string.h>
#include "json.h"
#include <time.h>
#include <stddef.h>
#include <curl.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

//#include "parse_flags.h"
/* 
0xFAF8 =XIAOMIKAIGUAN1-zhuwo
0x1AD6 =XIAOMIKAIGUAN2-ciwo
0x9F68 =XIAOMIKAIGUAN3
0xFC44 =XIAOMIMENCI
0x32FE =XIAOMIRENTI


0x5C4E =KETING
0xB358 =CANTING
0xBE8A =CHUFANG
0xE6B0 =MENTING
0x5680 =GUODAO
0x7710 =ZHUWO
0x9590 =CIWO


*/
#define XMKG_ZHU_ID     0XFAF8
#define XMKG_CI_ID      0X1AD6
#define XMMENCI_ID      0xFC44
#define KETING_ID       0x5C4E
#define CANTING_ID      0XB358
#define CHUFANG_ID      0XBE8A
#define MENTING_ID      0XE6B0
#define GUODAO_ID       0X5680
#define ZHUWO_ID        0X7710
#define CIWO_ID         0X9590
#define XIAOMIRENTI_ID	0x32FE
#define XMKG_WAI_ID		0x9F68

#define XMLOUSHUI_CIWO_ID		0x0000
#define XMLOUSHUI_CHUFANG_ID    0x0000

typedef unsigned char   uint8_t;     //鏃犵鍙?浣嶆暟

#define WW_STATE      0x55
#define AA_STATE      0xaa
#define CMD_STATE1     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

#define PORT            8888
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
#define MXJ_CONFIG_GET               0x0A
#define MXJ_GET_IDXS                 0x0B
#define MXJ_SEND_IDXS                0x0C
#define MXJ_SEND_RESPONSE            0x0D 
#define MXJ_PING_REQUEST             0x0F
#define MXJ_DEVICE_ANNCE             0x0E
#define MXJ_PING_RESPONSE            0x10
#define MXJ_SEND_RESET               0x11  
#define MXJ_XIAOMI18                 0x18
#define MXJ_XIAOMI1C                 0x1C
#define MXJ_SEND_DATA                0xff

#define DEV_SIZE                200
FILE *sp;
CURL* posturl;

int permitjoin = 0;

typedef struct
{
  uint16_t id;
  uint8_t state1;
  uint8_t state2;
  uint8_t state3;
  uint8_t enable;
} MXJ_CONFIG_STRUCT;

typedef struct
{
  uint16_t id;
  uint8_t type;
  uint8_t ctrl;
  uint16_t devid;
  uint8_t idx;
  uint8_t light[3];
  uint8_t state[3];
  uint8_t registered;
} MXJ_DEVICE;
MXJ_DEVICE mxj_device[DEV_SIZE];
uint8_t devsize=0; 
int usart_fd;
uint8_t rebuf[200];    
uint8_t rx_step = 0;	
uint8_t state = WW_STATE;
uint8_t  LEN_Token;
uint8_t  FSC_Token;
uint8_t  tempDataLen = 0;
uint8_t rxbuf[200];
uint8_t len=0;
int humand = 0;
time_t now;
struct tm *tblock; 


//uint8_t humaned = 0;
//CURL* geturl;

void send_usart(uint8_t *data,uint8_t len);
void MXJ_SendRegisterMessage( uint16_t , uint8_t );
void MXJ_SendPingMessage( uint16_t id );
void MXJ_GetIdxMessage( uint16_t id );

void MXJ_SendCtrlMessage( uint16_t ,uint8_t len, uint8_t , uint8_t , uint8_t );
void MXJ_SendStateMessage( uint16_t );

void MXJ_GetStateMessage( uint16_t id );
void del_dev(void);
int find_dev(int i);

char *str = "undefine";
char strxml[500];
int post_type;

char *json_str;
struct connection_info_struct
{
  int connectiontype;
  char *answerstring;
  struct MHD_PostProcessor *postprocessor;
};

const char *askpage = "{\"devices\":[{\"id\":10552,\"type\":7,\"control\":{\"devid\":1234,\"lights\":[0,1]},\"status\":[]},{\"id\":2155,\"type\":3,\"control\":{\"devid\":6445,\"lights\":[1,2]},\"status\":[]},{\"id\":12345,\"type\":7,\"control\":{},\"status\":[1,1,0]}]}";

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";

const char *idolpage =
  "<html><body>hello idol</body></html>";
long writer(void *data, int size, int nmemb, char *content)
{
	return 1;
}

void build_json()
{
	int i=0;
  char str2[2000];
  strcpy(str2,"{\"devices\":[");
//	json_object *devices = json_object_new_array();

	for(i=0;i<devsize;i++)
		{      
			json_object *state= json_object_new_array();
			json_object_array_add(state, json_object_new_int(mxj_device[i].state[0]));
			json_object_array_add(state, json_object_new_int(mxj_device[i].state[1]));
			json_object_array_add(state, json_object_new_int(mxj_device[i].state[2]));
      //printf("state = %s\n",json_object_to_json_string(state));
            
			
      //printf("control = %s\n",json_object_to_json_string(control));
            
			json_object *dev = json_object_new_object();
			json_object_object_add(dev, "id", json_object_new_int(mxj_device[i].id));
			json_object_object_add(dev, "type", json_object_new_int(mxj_device[i].type));				
			json_object_object_add(dev, "status", state);
			if(mxj_device[i].id == KETING_ID)
				json_object_object_add(dev, "name", json_object_new_string("KETING_ID"));
			if(mxj_device[i].id == CANTING_ID)
				json_object_object_add(dev, "name", json_object_new_string("CANTING_ID"));
			if(mxj_device[i].id == CHUFANG_ID)
				json_object_object_add(dev, "name", json_object_new_string("CHUFANG_ID"));
			if(mxj_device[i].id == MENTING_ID)
				json_object_object_add(dev, "name", json_object_new_string("MENTING_ID"));
			if(mxj_device[i].id == GUODAO_ID)
				json_object_object_add(dev, "name", json_object_new_string("GUODAO_ID"));
			if(mxj_device[i].id == ZHUWO_ID)
				json_object_object_add(dev, "name", json_object_new_string("ZHUWO_ID"));
			if(mxj_device[i].id == CIWO_ID)
				json_object_object_add(dev, "name", json_object_new_string("CIWO_ID"));
			
      //printf("dev = %s\n",json_object_to_json_string(dev));
      
			//json_object_array_add(devices, dev);
      //json_object *obj = json_object_array_get_idx(devices, i);
      
      
      //snprintf (str2, MAXANSWERSIZE, json_object_to_json_string(obj));
      strcat(str2,json_object_to_json_string(dev));
      if(i<devsize-1)
        strcat(str2,",");
      else
        strcat(str2,"]}");
      
      
      //printf("------------------build-json-----------\n");
	  
			if(state!=NULL)
				json_object_put(state);
			if(dev!=NULL)
				json_object_put(dev);

				
		}
   
   

   //printf("str2 = %s\n",str2); 
   strcpy(json_str,str2);
   //printf("build json_str = %s\n",json_str); 
   //json_str=str2;
   
	
}


static int
send_page (struct MHD_Connection *connection, const char *page)
{
  int ret;
  struct MHD_Response *response;


  response =
    MHD_create_response_from_buffer (strlen (page), (void *) page,
				     MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;

  //MHD_add_response_header (response, "Content-Type", "text/html;charset=UTF-8");

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


static int
iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
              const char *filename, const char *content_type,
              const char *transfer_encoding, const char *data, uint64_t off,
              size_t size)
{
  struct connection_info_struct *con_info = coninfo_cls;
  if(size == 0) return MHD_NO;

	

  return MHD_YES;
}



static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
  struct connection_info_struct *con_info = *con_cls;
  if (NULL == con_info)
    return;

  if (con_info->connectiontype == POST)
    {
      MHD_destroy_post_processor (con_info->postprocessor);
    }
  free (con_info);
  *con_cls = NULL;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
	struct connection_info_struct *con_info = *con_cls;
int i=-1;
int len2 =0;


char *ToUserName;
char *CreateTime;
char *FromUserName;

char *re_body;
  FILE *sp;
  //printf ("====New %s request for %s using version %s\n", method, url, version);
 
  time(&now);
  tblock = localtime(&now);

  if (NULL == *con_cls)
    {		
      struct connection_info_struct *con_info;

      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info)
      {
        //printf("return MHD_NO \n");
        return MHD_NO;
      }
      con_info->answerstring = NULL;
      //printf("method2 =%s \n",method);

      *con_cls = (void *) con_info;

      return MHD_YES;
    }
  


  if (0 == strcmp (method, "GET"))
    {	
	//	printf("get json_str = %s\n",json_str);
		if(json_str!=NULL)
			return send_page (connection,json_str);
		else
			return send_page (connection, errorpage);
    }
  
  //printf("method= %s\n", method);

  if (0 == strcmp (method, "POST"))
  	{  		
	  int ret;

	  if(0 == strcmp (url, "/zbClient/API/permit"))
		{
			permitjoin = 1;
			printf("POST RECIEVE:time: %d-%d-%d %d:%d:%d len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version);
		 	if ((sp = fopen("/var/log/zbclient.txt","a+")) != NULL)
		    {
			    fprintf(sp,"POST RECIEVE:time=%d-%d-%d %d:%d:%d len=0 url=%s version=%s body=NULL\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, url, version);
			    fclose(sp);
		    }
		}


	  if (*upload_data_size != 0)
	  {

		
		
		const char* length = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH); 	
			const char* body = MHD_lookup_connection_value (connection, MHD_POSTDATA_KIND, NULL);		
			//printf("body=%s\n",body);
			
			  
			if(length != NULL)
			{		  
				len2 = atoi(length);
				re_body = (uint8_t*)calloc(len2,sizeof(uint8_t));
				strncpy(re_body,body,len2);
				printf("now datetime: %d-%d-%d %d:%d:%d - New %s request for %s using version %s - len2=%d re_body=\n%s\n End \n", tblock->tm_year, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,method, url, version,len2,re_body);
			}
			else
			{
				len2 = 0;
				post_type = 0;
				printf("now datetime: %d-%d-%d %d:%d:%d - New %s request for %s using version %s - len2=0 re_body=\nNULL\n End \n", tblock->tm_year, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,method, url, version);
			}
		
			if ((sp = fopen("/var/log/zbclient.txt","a+")) != NULL)
			{
				fprintf(sp,"POST RECIEVE:time=%d-%d-%d %d:%d:%d len=%d url=%s version=%s body=%s\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len2, url, version,body);
				fclose(sp);
			}

		
			if(len2 != 0)
			{
				json_object *my_object = json_tokener_parse(re_body);
				if(json_object_to_json_string(my_object) != NULL && (0 != strcmp (json_object_to_json_string(my_object), "null")))
				{
					  uint16_t id=0;
					  uint8_t state11=0,state22=0,state33=0;
					  post_type = 1;
					  json_object *type = NULL;
					  json_object *name = NULL;
					  
					  json_object *devid = NULL;
					  json_object_object_get_ex(my_object, "devid",&devid);
					  json_object *state1 = NULL;
					  json_object_object_get_ex(my_object, "state1",&state1);
					  json_object *state2 = NULL;
					  json_object_object_get_ex(my_object, "state2",&state2);
					  json_object *state3 = NULL;
					  json_object_object_get_ex(my_object, "state3",&state3);
					  json_object_object_get_ex(my_object, "type",&type);
					  json_object_object_get_ex(my_object, "name",&name);
				  
					  id = (uint16_t)json_object_get_int(devid);
					  state11 = (uint16_t)json_object_get_int(state1);
					  state22 = (uint16_t)json_object_get_int(state2);
					  state33 = (uint16_t)json_object_get_int(state3);
				  
				  
				  
					  if(0 == strcmp (json_object_to_json_string(type), "\"control_down\""))
						  {
							  if(0 == strcmp (json_object_to_json_string(name), "\"KETING_ID\""))
								  id = KETING_ID;
							  if(0 == strcmp (json_object_to_json_string(name), "\"CANTING_ID\""))
								  id = CANTING_ID;
							  if(0 == strcmp (json_object_to_json_string(name), "\"CHUFANG_ID\""))
								  id = CHUFANG_ID;
							  if(0 == strcmp (json_object_to_json_string(name), "\"MENTING_ID\""))
								  id = MENTING_ID;
							  if(0 == strcmp (json_object_to_json_string(name), "\"GUODAO_ID\""))
								  id = GUODAO_ID;
							  if(0 == strcmp (json_object_to_json_string(name), "\"ZHUWO_ID\""))
								  id = ZHUWO_ID;
							  if(0 == strcmp (json_object_to_json_string(name), "\"CIWO_ID\""))
								  id = CIWO_ID;
				  
							  if(id == GUODAO_ID)
							  	MXJ_SendCtrlMessage(id,3,state11,state22,state33);//2need change
							  else
							  	MXJ_SendCtrlMessage(id,3,state11,state22,state33);//1need change
							  
							  i=find_dev(id);
							if(i>=0)
							  {
								  if(state11 == 3)					  
									  if(mxj_device[i].state[0] == 1)
										  mxj_device[i].state[0]=0;
									  else if(mxj_device[i].state[0] == 0)
										  mxj_device[i].state[0]=1;
								  if(state11 == 1||state11 == 0)
									  mxj_device[i].state[0]=state11;
				  
								  if(state22 == 3)
									  if(mxj_device[i].state[1] == 1)
										  mxj_device[i].state[1]=0;
									  else if(mxj_device[i].state[1] == 0)
										  mxj_device[i].state[1]=1;
								  if(state22 == 1||state22 == 0)
									  mxj_device[i].state[1]=state22;
								  
								  build_json();
							  }
							  //MXJ_GetStateMessage(id);
						  }
					  if(0 == strcmp (json_object_to_json_string(type), "\"register_ok\""))
						  MXJ_SendRegisterMessage(id,MXJ_REGISTER_OK);
					  if(0 == strcmp (json_object_to_json_string(type), "\"register_failed\""))
						  MXJ_SendRegisterMessage(id,MXJ_REGISTER_FAILED);
					  if(0 == strcmp (json_object_to_json_string(type), "\"ask_state\""))
						  MXJ_GetStateMessage(id);
					  if(0 == strcmp (json_object_to_json_string(type), "\"any_data\""))
						  MXJ_SendCtrlMessage(id,3,state11,state22,state33);
					  if(0 == strcmp (json_object_to_json_string(type), "\"heart\""))
						  ;//MXJ_GetStateMessage(0xffff);
				  
					  //printf("\n");
					  //printf("state11 = %d\n", state11);
					  //printf("state22 = %d\n", state22);
					  //printf("state33 = %d\n", state33);
					  //printf("recieve post json= %s\n", json_object_to_json_string(my_object));
				}
				
				else
				{
					xmlDocPtr doc;
					xmlNodePtr cur;
					xmlChar *uri;
					char *str_data;
					post_type = 2;
					//printf("xml2\n");
					doc = xmlParseMemory(re_body,len2);
					if (doc == NULL ) 
					{
						printf("xmlParseMemory failed\n");
						xmlFree(doc);
					}
					else
					{
						cur = xmlDocGetRootElement(doc);
						if (cur == NULL ) 
						{
							printf("xmlDocGetRootElement failed\n");
						}
						else
						{

							cur = cur->xmlChildrenNode;
							while (cur != NULL)
							{
								if (!xmlStrcmp(cur->name, (const xmlChar *)"CreateTime"))
								{
									uri = xmlNodeGetContent(cur);
									CreateTime = uri;
									//printf("CreateTime: %s\n", uri);
								}
								else if (!xmlStrcmp(cur->name, (const xmlChar *)"FromUserName"))
								{
									uri = xmlNodeGetContent(cur);
									FromUserName = uri;
									//printf("FromUserName: %s\n", uri);
									
								}
								else if (!xmlStrcmp(cur->name, (const xmlChar *)"ToUserName"))
								{
									uri = xmlNodeGetContent(cur);
									ToUserName = uri;
									//printf("ToUserName: %s\n", uri);
									
								}
								else if (!xmlStrcmp(cur->name, (const xmlChar *)"MsgType"))
								{
									uri = xmlNodeGetContent(cur);
									//printf("MsgType: %s\n", uri);
								}
								else if (!xmlStrcmp(cur->name, (const xmlChar *)"Content"))
								{
									uri = xmlNodeGetContent(cur);
									//printf("Content: %s\n", uri);
									if(0 == strcmp (uri, "开客厅"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(KETING_ID,3,1,1,1);//1need change
									}
									else if(0 == strcmp (uri, "关客厅"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(KETING_ID,3,0,0,0);
									}
									else if(0 == strcmp (uri, "开餐厅"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(CANTING_ID,3,1,0,0);
									}
									else if(0 == strcmp (uri, "关餐厅"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(CANTING_ID,3,0,0,0);
									}
									else if(0 == strcmp (uri, "开厨房"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(CHUFANG_ID,3,1,0,0);
									}
									else if(0 == strcmp (uri, "关厨房"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(CHUFANG_ID,3,0,0,0);
									}
									else if(0 == strcmp (uri, "开门厅"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(MENTING_ID,3,1,0,0);
									}
									else if(0 == strcmp (uri, "关门厅"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(MENTING_ID,3,0,0,0);
									}
									else if(0 == strcmp (uri, "开主卧"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(ZHUWO_ID,3,1,0,0);
									}
									else if(0 == strcmp (uri, "关主卧"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(ZHUWO_ID,3,0,0,0);
									}
									else if(0 == strcmp (uri, "开次卧"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(CIWO_ID,3,1,0,0);
									}
									else if(0 == strcmp (uri, "关次卧"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(CIWO_ID,3,0,0,0);
									}
									else if(0 == strcmp (uri, "开过道"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(GUODAO_ID,3,2,1,2);//2need change
									}else if(0 == strcmp (uri, "关过道"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(GUODAO_ID,3,2,0,2);//2need change
									}
									else if(0 == strcmp (uri, "开厕所"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(GUODAO_ID,3,1,2,2);
									}
									else if(0 == strcmp (uri, "关厕所"))
									{
										str_data = uri;
										MXJ_SendCtrlMessage(GUODAO_ID,3,0,2,2);//need change
									}else
										str_data = "eg：开客厅、关客厅、开次卧、开过道、开门厅、开厨房、开餐厅、开厕所";
									
									sprintf(strxml,"<xml><ToUserName><![CDATA[%s]]></ToUserName><FromUserName><![CDATA[%s]]></FromUserName><CreateTime>%d</CreateTime><MsgType><![CDATA[text]]></MsgType><Content><![CDATA[%s]]></Content></xml>",FromUserName,ToUserName,time(NULL),str_data);
									printf("strxml = %s\n",strxml);
								}
								
								

								
								
								cur = cur->next;
							}
							xmlFree(cur);
							xmlFree(uri);
						}
					}					
				}
				
				free(re_body);
			}

		*upload_data_size = 0;
		return MHD_YES;
	  }
	  else if(post_type == 2)
	  	  return send_page (connection,strxml);
	  else if(post_type == 1&&json_str!=NULL)
		  return send_page (connection,json_str);
	  else
		  return send_page (connection, errorpage);  
  	}


  return send_page (connection, errorpage);
}

void send_usart(uint8_t *data,uint8_t len) //id,state1,state2,state3 1=寮€,0=鍏?2=淇濇寔
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
 // printf("\n");
			time(&now);
			tblock = localtime(&now);
			
			if ((sp = fopen("/var/log/zbclient.txt","a+")) != NULL)
			{
				fprintf(sp,"USART SEND:time=%d-%d-%d %d:%d:%d len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len+3);
				for(i=0;i<len;i++)
				  {
				    fprintf(sp,"%02x ",txbuf[i]);
				  }
				  fprintf(sp,"\n");
				  fclose(sp);
			}
			
			printf("USART SEND:time=%d-%d-%d %d:%d:%d len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len+3);
 // printf("send:");
  for(i=0;i<len+3;i++)
  {
    serialPutchar(usart_fd,txbuf[i]);
    printf("%02x ",txbuf[i]);
  }
  printf("\n");
}



//========thread start=========//
//AA C0 IDH IDL STATE1 STATE2 STATE3 REV CRC AB

void del_dev(void)
{
	mxj_device[devsize-1].id=0;
	mxj_device[devsize-1].type=0;
	mxj_device[devsize-1].idx=0;
	mxj_device[devsize-1].registered=0;
	mxj_device[devsize-1].ctrl=0;
	devsize--;
}


int find_dev(int id)
{
	int i = 0;
	for(i=0;i<devsize;i++)
		if(mxj_device[i].id == id)
			{
				//printf("find:%d",i);
				return i;
			}

	//printf("not find");
	return -1;
}
void recieve_usart(uint8_t *rx,uint8_t len)
{
  int i=0,j=0;
  int id=0,cid=0;
  int tempctrl[3]={2,2,2};
  time(&now);
  tblock = localtime(&now);
//  printf("\n");
//  printf("recieved:%d - at: %d-%d-%d %d:%d:%d\n",len,tblock->tm_year, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
  
//  for(i=0;i<len;i++)
//    printf("%02x ",rx[i]);
//  printf("\n");
//  printf("------------------------\n");
  printf("USART RECIEVE:time=%d-%d-%d %d:%d:%d len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);
  for(i=0;i<len;i++)
	  printf("%02x ",rx[i]);
  printf("\n");


  
  FILE *sp;
  if ((sp = fopen("/var/log/zbclient.txt","a+")) != NULL)
	{
		fprintf(sp,"USART RECIEVE:time=%d-%d-%d %d:%d:%d len=%d data=", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,len);
		for(i=0;i<len;i++)
    		fprintf(sp,"%02x ",rx[i]);
		fprintf(sp,"\n");
		fclose(sp);
	}
  
 
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
		i=find_dev(id);
		
		if(i==-1)
      	{
      		printf("control up - dev not found \n");			
      	}		
		else 
	   	{
	      printf("control up - find id = %d\n",i);
		  printf("id:%4x\n",id);
			if(mxj_device[i].type==4)
			{
				if( rx[5] == 1)
					mxj_device[i].state[0]=rx[6];
				if( rx[5] == 2)
					mxj_device[i].state[1]=rx[6];
				if( rx[5] == 3)
					mxj_device[i].state[2]=rx[6];

				if(id == CHUFANG_ID)
					humand = 0;
				build_json();
			}
	     }

		
    break;
    
    case MXJ_REGISTER_REQUEST:
		i=find_dev(id);
      if(i==-1)
      	{
      		
	  		mxj_device[devsize].id=id;
			mxj_device[devsize].type=rx[4];
			mxj_device[devsize].idx=rx[5];
			mxj_device[devsize].registered=1;
			if(devsize<DEV_SIZE)
				devsize ++;
			build_json();
      	}
      MXJ_SendRegisterMessage(id,MXJ_REGISTER_OK);
    break;

    case MXJ_SEND_STATE:
		i=find_dev(id);
      if(i>=0)
      	{
      		if(len >= 5)
	  		mxj_device[i].state[0]=rx[4];
			if(len >= 6)
	  		mxj_device[i].state[1]=rx[5];
			if(len >= 7)
			mxj_device[i].state[2]=rx[6];	
			build_json();
      	}
	  else
	  	{
	  		MXJ_SendRegisterMessage( id, MXJ_REGISTER_FAILED );
	  	}
    break;

    case MXJ_SENSOR_DATA:
      //MXJ_GetStateMessage(id);
    break;

    case MXJ_GET_STATE:
		{
			;//printf("get state\n");
			//char str[200]={0};
			///sprintf(str,"type=ask_state&dev_id=%d",id);
			//curl_easy_setopt(geturl, CURLOPT_POSTFIELDS,str);
			//curl_easy_perform(geturl);	
		}
    break;

	case MXJ_SEND_RESPONSE:
		{
;
		}
    break;	
		
    case MXJ_XIAOMI18:
		i=find_dev(id);

	 if(len>=27)
 	{
 		if(cid == 0 && rx[12] <= len - 13 && i==-1)
		{
			 uint8_t *temp_str = NULL;
		    /*鍒嗛厤鍐呭瓨绌洪棿*/
		    temp_str = (uint8_t*)calloc(rx[12],sizeof(uint8_t));
		    /*灏唄ello鍐欏叆*/
		    strncpy(temp_str, &rx[13],rx[12]);
			printf("temp_str = %s\n",temp_str);
			if (0 == strcmp (temp_str, "lumi.sensor_switch"))
			{
				mxj_device[devsize].id=id;
				mxj_device[devsize].type=0x0b;
				mxj_device[devsize].idx=1;
				mxj_device[devsize].registered=1;
				if(devsize<DEV_SIZE)
					devsize ++;			
				build_json();
			}
			else if (0 == strcmp (temp_str, "lumi.sensor_magnet"))
			{
				mxj_device[devsize].id=id;
				mxj_device[devsize].type=0x0a;
				mxj_device[devsize].idx=1;
				mxj_device[devsize].registered=1;
				if(devsize<DEV_SIZE)
					devsize ++;			
				build_json();				
			}
			else if (0 == strcmp (temp_str, "lumi.sensor_motion"))
			{
				mxj_device[devsize].id=id;
				mxj_device[devsize].type=0x03;
				mxj_device[devsize].idx=1;
				mxj_device[devsize].registered=1;
				if(devsize<DEV_SIZE)
					devsize ++;			
				build_json();				
			}
			else if (0 == strcmp (temp_str, "lumi.sensor_ht"))
			{
				mxj_device[devsize].id=id;
				mxj_device[devsize].type=0x09;
				mxj_device[devsize].idx=1;
				mxj_device[devsize].registered=1;
				if(devsize<DEV_SIZE)
					devsize ++;			
				build_json();				
			}
		}
 	}

	if(cid == 6)
	{
		if(len != 13)break;
		printf("control up - find id = %d\n",i);
		printf("id:%4x\n",id);
		if(rx[11] == 0x20)
		{
			printf("double kick\n");
			MXJ_SendCtrlMessage(ZHUWO_ID,3,0,0,0);
			MXJ_SendCtrlMessage(CIWO_ID,3,0,0,0);
			MXJ_SendCtrlMessage(CHUFANG_ID,3,0,0,0);
			MXJ_SendCtrlMessage(CANTING_ID,3,0,0,0);
			MXJ_SendCtrlMessage(KETING_ID,3,0,0,0);
			MXJ_SendCtrlMessage(MENTING_ID,3,0,0,0);
			MXJ_SendCtrlMessage(GUODAO_ID,3,0,0,0);//2need change

			//MXJ_GetStateMessage(0xffff);
		}
		else
		{
			printf("action = %d\n",rx[12]);
			if(id==XMKG_ZHU_ID&&rx[12] == 1)
			{					
				MXJ_SendCtrlMessage(ZHUWO_ID,3,3,3,3);//need change
				//MXJ_GetStateMessage(ZHUWO_ID);
			}
			if(id==XMKG_CI_ID&&rx[12] == 1)
			{
				MXJ_SendCtrlMessage(CIWO_ID,3,3,3,3);//need change
				//MXJ_GetStateMessage(CIWO_ID);
			}
			if(id==XMMENCI_ID)
			{
				MXJ_SendCtrlMessage(MENTING_ID,3,rx[12],rx[12],rx[12]);//need change
				//MXJ_GetStateMessage(MENTING_ID);
				char str[200]={0};
				if(rx[12]==1)
				{
					sprintf(str,"text=开门！");
				}
				else if(rx[12]==0)
				{
					sprintf(str,"text=关门！");
				}
								
				curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
				curl_easy_perform(posturl);
			}
			if(id==XMLOUSHUI_CIWO_ID)
			{
				if(rx[12]==0)//loushui
				{
					char str[200]={0};
					sprintf(str,"text=次卧漏水");				
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);
				}
				else if(rx[12]==1)
				{
					char str[200]={0};
					sprintf(str,"text=次卧漏水解除");
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);
				}
			}
			if(id==XMLOUSHUI_CHUFANG_ID)
			{
				if(rx[12]==0)//loushui
				{
					char str[200]={0};
					sprintf(str,"text=厨房漏水");				
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);
				}
				else if(rx[12]==1)
				{
					char str[200]={0};
					sprintf(str,"text=厨房漏水解除");
					curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
					curl_easy_perform(posturl);
				}
			}
				
		}
	}
	else if(cid == 0x406)
	{
		if(len != 13)break;
		printf("control up - find id = %d\n",i);
		printf("id:%4x\n",id);
		printf("human detected\n");
		if(id == XIAOMIRENTI_ID)
		{	
			int j = -1;
			j = find_dev(CHUFANG_ID);
			if(j != -1 && mxj_device[j].state[0] == 0)
			{				
				if(humand <= 1)
				{
					MXJ_SendCtrlMessage(CHUFANG_ID,3,1,1,1);//need change
					//MXJ_GetStateMessage(CHUFANG_ID);
					humand = 120;
				}	
								
			}
			if(humand > 1 )
				humand = 120;
		}
	}
	else if(cid == 0x402)
	{
		int16_t temp = 0;
		if(len != 14)break;
		temp = rx[13];
		temp <<= 8;
		temp |= rx[12];
		printf("temperature up - find id = %d\n",i);
		printf("id:%4x\n",id);
		printf("temperature = %02f\n",(float)temp/100);

	}
	else if(cid == 0x405)
	{
	 	uint16_t temp = 0;
		if(len != 14)break;
		temp = rx[13];
		temp <<= 8;
		temp |= rx[12];
		printf("humidity up - find id = %d\n",i);
		printf("id:%4x\n",id);
		printf("humidity = %02f\n",(float)temp/100);
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
	pthread_t id;
  struct MHD_Daemon *daemon;
  uint8_t i=0;
  for(i=0;i<DEV_SIZE;i++)
  {
    mxj_device[i].registered = 0;
	mxj_device[i].ctrl = 0;
  }

  json_str = (char*)calloc(2000,sizeof(char));
  
  
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
	
   //printf("start server ...\n");
   posturl = curl_easy_init();
   struct curl_slist *list = NULL;
   
	 if(posturl){
   
		 //list = curl_slist_append(list, "Content-Type: application/json");
		 //curl_easy_setopt(posturl, CURLOPT_HTTPHEADER, list);
		 curl_easy_setopt(posturl, CURLOPT_URL, "http://sc.ftqq.com/SCU1247T4f6d392d81bb2e29cf723e311f9bf06d5795a12c0adba.send");
		 curl_easy_setopt(posturl, CURLOPT_HTTPPOST, 1L);
		 curl_easy_setopt(posturl, CURLOPT_WRITEFUNCTION, writer);
		 //curl_easy_setopt(posturl, CURLOPT_WRITEDATA, &headerStr);
	 }
	 else
		return (1);




  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                             NULL, MHD_OPTION_END);
  if (NULL == daemon)
  {
    //printf("server creat failed!\n");
    return 1;
  }
  int tt=0;
  int j=0,k=0;

  //MXJ_SendRegisterMessage( 0x4738, MXJ_REGISTER_FAILED );
//  MXJ_SendRegisterMessage( 0x49f0, MXJ_REGISTER_FAILED );
  
  time(&now);
  tblock = localtime(&now);
  
  printf("START!\n");
  if ((sp = fopen("/var/log/zbclient.txt","a+")) != NULL)
  {
	  fprintf(sp,"START time=%d-%d-%d %d:%d:%d START!\n", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	  fclose(sp);
  }
  char str[200]={0};
  sprintf(str,"text=系统启动  AT:%d-%d-%d %d:%d:%d", tblock->tm_year+1900, tblock->tm_mon+1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
  curl_easy_setopt(posturl, CURLOPT_POSTFIELDS,str);
  curl_easy_perform(posturl);

	mxj_device[devsize].id=KETING_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=1;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
	mxj_device[devsize].id=CANTING_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=1;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
	mxj_device[devsize].id=CHUFANG_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=1;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
	mxj_device[devsize].id=MENTING_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=1;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
	mxj_device[devsize].id=GUODAO_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=2;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
	mxj_device[devsize].id=ZHUWO_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=1;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
	mxj_device[devsize].id=CIWO_ID;
	mxj_device[devsize].type=4;
	mxj_device[devsize].idx=1;
	mxj_device[devsize].registered=1;
	if(devsize<DEV_SIZE)
		devsize ++;
	
  MXJ_GetStateMessage(0xffff);
  build_json();
	while(1)
	{  
		//usleep(500000);

	 


		
		j++;
		
		if(j > 20)
		{
			j = 0;
			//MXJ_GetStateMessage(0xffff);
		}
		sleep(1);
		
		if(humand == 1)
		{
			MXJ_SendCtrlMessage(CHUFANG_ID,3,0,0,0);//need change	
			//MXJ_GetStateMessage(CHUFANG_ID);		
		}
		if(humand > 0)
		{	
			printf("humand = %d\n",humand);
			humand--;
		}
	}
	

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
void MXJ_SendCtrlMessage( uint16_t id ,uint8_t len,uint8_t msg1 , uint8_t msg2 , uint8_t msg3 )
{
//  uint8_t data[5]={MXJ_CTRL_DOWN,3,msg1,msg2,msg3};//猫?a????1鈥??掳??
  uint8_t data[7]={0,len+3,(uint8_t)(id>>8),(uint8_t)id,msg1,msg2,msg3};
  send_usart(data,4+len);
}

/*********************************************************************
 * @fn      MXJ_SendRegisterMessage
 *
 * @brief   Send the register_request message to endpoint.
 *
 * @param   none
 *
 * @return  none
 
      //uint8_t data[7]={0,6,0x17,0x1f,1,1,1};//鎺у埗鍛戒护鈫?
     //uint8_t data[4]={10,3,0x17,0x1f};//寮€鍏抽厤缃枃浠惰幏鍙栤啌
     //uint8_t data[12]={8,11,0x17,0x1f,0,0,0x7b,0x4f,1,2,2,1};//寮€鍏抽厤缃枃浠惰缃啌    
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

void MXJ_SendPingMessage( uint16_t id )
{
  uint8_t data[4]={0x0f,3,(uint8_t)(id>>8),(uint8_t)id};//猫?a????1鈥??掳??
  send_usart(data,4);
}

void MXJ_GetIdxMessage( uint16_t id )
{
  uint8_t data[4]={0x0b,3,(uint8_t)(id>>8),(uint8_t)id};//猫?a????1鈥??掳??
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
  uint8_t data[4]={5,3,(uint8_t)(id>>8),(uint8_t)id};//鑷畾涔夋暟鎹?
  send_usart(data,4);
}

