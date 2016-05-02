//gcc zbserver.c -o zbserver -lmicrohttpd -lwiringPi -lpthread
//浏览器版本，返回html页面
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <string.h>
/* 
FAF8=XIAOMIKAIGUAN1
A584=XIAOMIKAIGUAN2
D2CB=XIAOMIMENCI

4E8A=KETING
B358=CANTING
BE8A=CHUFANG
E6B0=MENTING
5680=GUODAO
7710=ZHUWO
9590=CIWO


*/
#define PORT 6868

#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     3000
#define MAXANSWERSIZE   512
#define GET             0
#define POST            1
uint8_t idx = 0;
int old_idx = 0;
uint16_t tempid=0;
uint16_t tempid2=0;

uint16_t devsize = 0;
uint8_t humaned = 0;
char str22[200];


typedef unsigned char   uint8_t;     //无符号8位数

#define DEV_SIZE                200

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

typedef struct
{
  uint16_t id;
  char  type[20];
  uint8_t dev_type;
  uint8_t idxs;
  uint8_t idx;
  uint8_t action;
  uint16_t value;
} POST_STRUCT;

POST_STRUCT re_post[256]={0};

char *str = "undefine";

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
  //printf("file name =%s \n",filename);
  if(size == 0) return MHD_NO;
  int i=0;
  static state_post = 0;
 // printf("key =%s \n",key);
 // printf("size =%d \n",size);
  //printf("data =%s \n",data);
 // i=atoi(data);
  //printf("data16 =%04X \n",i);
  //printf("------------------\n");
  if(old_idx != idx)
  	{
  		old_idx = idx;
		state_post = 0;
		re_post[idx-1].action=100;
		re_post[idx-1].dev_type=0;
		//re_post[idx-1].type="0";
		re_post[idx-1].idxs=0;
		re_post[idx-1].idx=0;
		re_post[idx-1].id=0;
		re_post[idx-1].value=0;
		
  	}
  
	
  	if(0 == strcmp (key, "type"))
	{
		strcpy(re_post[idx].type,data);
	}
  	else if(0 == strcmp (key, "dev_id"))
	{
		re_post[idx].id = atoi(data);
		
	}  	
  	else if(0 == strcmp (key, "dev_type"))
	{
		re_post[idx].dev_type= atoi(data);
	}  	
  	else if(0 == strcmp (key, "idxs"))
	{
		re_post[idx].idxs = atoi(data);

	}  	
  	else if(0 == strcmp (key, "idx"))
	{
		re_post[idx].idx = atoi(data);

	}  	
  	else if(0 == strcmp (key, "action"))
	{
		re_post[idx].action= atoi(data);

	}  	
	else if(0 == strcmp (key, "action_string"))
	{
		//re_post[idx].action= atoi(data);

	}
 	else if(0 == strcmp (key, "value"))
	{
		re_post[idx].value= atoi(data);

	}
    
	

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
      //if (con_info->answerstring)
        //free (con_info->answerstring);
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
  memset(str22, 0, sizeof(uint8_t)*200);

  //printf("method =%s \n",method);
  if (NULL == *con_cls)
    {
    	//printf("NuLl=ClS \n");
		
      struct connection_info_struct *con_info;

      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info)
      {
        //printf("return MHD_NO \n");
        return MHD_NO;
      }
      con_info->answerstring = NULL;
      //printf("method2 =%s \n",method);
      if (0 == strcmp (method, "POST"))
        {
          //printf("(0 == strcmp (method, POST)\n");
          con_info->postprocessor =
            MHD_create_post_processor (connection, POSTBUFFERSIZE,
                                       iterate_post, (void *) con_info);

          if (NULL == con_info->postprocessor)
            {
              free (con_info);
              return MHD_NO;
            }
          //printf("con_info->connectiontype = POST \n");
          con_info->connectiontype = POST;
        }
      else
        con_info->connectiontype = GET;

      *con_cls = (void *) con_info;

      return MHD_YES;
    }

  if (0 == strcmp (method, "GET"))
    {
    	//build_json();
    	//const char *reply = json_str;
		
    	//reply = malloc (strlen (json_str) );
		//strcpy(reply,json_str);
		

		//
		//if (NULL == reply)
	    	//return MHD_NO;
	  	//snprintf (reply,strlen (json_str) + 1,reply);
			return send_page (connection, errorpage);
    }

  if (0 == strcmp (method, "POST"))
  	{  		
	  int ret;
	  //char *reply;
	  //char *str = "open";
	  //printf("PoSt \n");
	  //struct MHD_Response *response;
	  
	  //reply = malloc (strlen (askpage) + strlen (str) + 1); 
	  //if (NULL == reply)
	  //  return MHD_NO;
	  //snprintf (reply,strlen (askpage) + strlen (str) + 1,askpage,str);
	  ////printf("reply= %s \n",reply);
	  //struct connection_info_struct *con_info = *con_cls;

	  if (*upload_data_size != 0)
	  {
  		MHD_post_process (con_info->postprocessor, upload_data,
  						  *upload_data_size);
  		*upload_data_size = 0;
      //printf("%s\n",con_info->postprocessor);
  		return MHD_YES;
	  }
	  else if (NULL != con_info->answerstring)
		  return send_page (connection, con_info->answerstring);

	  
  	}
	printf("re_post[%d].type=%s\n",idx,re_post[idx].type);
  	printf("re_post[%d].id=%d\n",idx,re_post[idx].id);
  	printf("re_post[%d].dev_type=%d\n",idx,re_post[idx].dev_type);
  	printf("re_post[%d].idxs=%d\n",idx,re_post[idx].idxs);
 	printf("re_post[%d].idx=%d\n",idx,re_post[idx].idx);
  	printf("re_post[%d].action=%d\n",idx,re_post[idx].action);
	printf("re_post[%d].value=%d\n",idx,re_post[idx].value);
	printf("recieve No: %d\n",idx);
	printf("\n\n");

  if((0 == strcmp (re_post[idx].type, "register_request"))&&re_post[idx].dev_type!=0&&re_post[idx].idxs!=0)
	{
		//printf("=========register:%d\n",idx);
		i=find_dev(re_post[idx].id);
		sprintf(str22,"{\"devid\":\"%d\",\"type\":\"register_ok\"}",re_post[idx].id);
       if(i==-1)
      	{
      		printf("add_dev:%d\n",devsize);
	  		mxj_device[devsize].id=re_post[idx].id;
			mxj_device[devsize].type=re_post[idx].dev_type;
			mxj_device[devsize].idx=re_post[idx].idxs;
			if(devsize<DEV_SIZE)
				devsize ++;
			
			//build_json();
      	}
	   
	}
    if((0 == strcmp (re_post[idx].type, "control_up"))&&re_post[idx].id!=0&&re_post[idx].idx!=0&&re_post[idx].action!=100)
	{
		printf("=========control_up:%d\n",idx);

		if(re_post[idx].id==0xFAF8)//KAIGUAN1
		{
			if(re_post[idx].action==1)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"3\",\"state2\":\"3\",\"state3\":\"3\",\"type\":\"control_down\"}",0x7710);
			}
			else if(re_post[idx].action==2)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"0\",\"state2\":\"0\",\"state3\":\"0\",\"type\":\"control_down\"}",0xFFFF);
			}
		}
		
		if(re_post[idx].id==0x1AD6)//KAIGUAN2
		{
			if(re_post[idx].action==1)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"3\",\"state2\":\"3\",\"state3\":\"3\",\"type\":\"control_down\"}",0x9590);
			}
			else if(re_post[idx].action==2)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"0\",\"state2\":\"0\",\"state3\":\"0\",\"type\":\"control_down\"}",0xFFFF);
			}
		}

		if(re_post[idx].id==0xFC44)//MENCI
		{
			if(re_post[idx].action==1)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"1\",\"state2\":\"1\",\"state3\":\"1\",\"type\":\"control_down\"}",0xE6B0);
			}
			else if(re_post[idx].action==0)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"0\",\"state2\":\"0\",\"state3\":\"0\",\"type\":\"control_down\"}",0xE6B0);
			}
		}

    }
	
    if((0 == strcmp (re_post[idx].type, "temperature_up"))&&re_post[idx].id!=0&&re_post[idx].value!=0)
	{
		
	}
    if((0 == strcmp (re_post[idx].type, "humidity_up"))&&re_post[idx].id!=0&&re_post[idx].value!=0)
	{
		static uint8_t temp=1;
		if(re_post[idx].value > 8000)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"1\",\"state2\":\"1\",\"state3\":\"1\",\"type\":\"control_down\"}",tempid);
				temp = 0;
			}

		if(re_post[idx].value < 6000 && temp == 0)
			{
				sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"0\",\"state2\":\"0\",\"state3\":\"0\",\"type\":\"control_down\"}",tempid);	
				temp = 1;
			}
			
	}
	if((0 == strcmp (re_post[idx].type, "ask_state"))&&re_post[idx].id!=0)
	{
		//printf("askkkkkkkkkkkkk");
		i=find_dev(re_post[idx].id);
		if(i!=-1 && mxj_device[i].type == 6)
		{
			sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"0\",\"state2\":\"2\",\"state3\":\"2\",\"type\":\"control_down\"}",re_post[idx].id);
			tempid2 = re_post[idx].id;
		}
		else if(i==-1)
			sprintf(str22,"{\"devid\":\"%d\",\"type\":\"register_failed\"}",re_post[idx].id);
	}
	
	
	if(str22==NULL)memset(str22, 0, sizeof(uint8_t)*200);
	idx++;
	if(idx>250)idx=1;
	printf("str22=%s\n",str22);
  return send_page (connection, str22);
}


int main(void)
{

  struct MHD_Daemon *daemon;
int i=0;
for(i=0;i<256;i++)
	re_post[i].action=100;
 

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                             NULL, MHD_OPTION_END);
  if (NULL == daemon)
  {
    printf("server creat failed!\n");
    return 1;
  }
  
	while(1)
	{   
		//usleep(500000);
      	static uint8_t i=0;
		//printf("This is the zbclient process.\n");
		sleep(1);
		if(humaned == 1)
		{
			i++;
			if(i > 5)
				{
					i = 0;
					humaned = 0;
					//sprintf(str22,"{\"devid\":\"%d\",\"state1\":\"0\",\"state2\":\"0\",\"state3\":\"0\",\"type\":\"control_down\"}",tempid);
				}
		}
	}
	

	return (0);
}
