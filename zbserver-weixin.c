//gcc zbserver.c -o zbserver -lmicrohttpd -lwiringPi -lpthread
//json版本 返回json

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

typedef unsigned char   uint8_t;     //无符号8位数

#define START_STATE    0
#define AA_STATE       1
#define C0_STATE       2

#define PORT            80
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512
#define GET             0
#define POST            1

int usart_fd;
uint8_t rebuf[100];    
uint8_t rx_step = 0;	


void send_usart(uint16_t id,uint8_t state1,uint8_t state2,uint8_t state3);

char *str = "undefine";
struct connection_info_struct
{
  int connectiontype;
  char *answerstring;
  struct MHD_PostProcessor *postprocessor;
};

const char *askpage = "{ \"abc\": 12, \"foo\": \"%s\", \"bool0\": false, \"bool1\": true }";

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";

const char *idolpage =
  "<html><body>hello idol</body></html>";

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
  //printf("key =%s \n",key);
   //printf("size =%d \n",size);
   printf("post data =%s \n",data);
  if (0 == strcmp (key, "open"))
    {
    str = "open ";
	  printf("open\n");
     send_usart(0xb6d6,1,1,1);//send===================================
	if ((size > 0) && (size <= MAXNAMESIZE))
        {
          char *answerstring;
          answerstring = malloc (MAXANSWERSIZE);
          if (!answerstring)
            return MHD_NO;

          snprintf (answerstring, MAXANSWERSIZE, askpage, key);
          con_info->answerstring = answerstring;
		  //printf("data =%s \n",data);
        }
      else
        con_info->answerstring = NULL;
	  
      return MHD_NO;
    }
  else if (0 == strcmp (key, "close"))
    {
    str = "close";
    printf("close\n");
    send_usart(0xb6d6,0,0,0);//send===================================
      if ((size > 0) && (size <= MAXNAMESIZE))
        {
          char *answerstring;
          answerstring = malloc (MAXANSWERSIZE);
          if (!answerstring)
            return MHD_NO;

          snprintf (answerstring, MAXANSWERSIZE, askpage, key);
          con_info->answerstring = answerstring;
		  //printf("data =%s \n",data);
        }
      else
        con_info->answerstring = NULL;

      return MHD_NO;
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
      if (con_info->answerstring)
        free (con_info->answerstring);
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

  printf("method =%s \n",method);
  if (NULL == *con_cls)
    {
    	//printf("NuLl=ClS \n");
		
      struct connection_info_struct *con_info;

      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info)
        return MHD_NO;
      con_info->answerstring = NULL;

      if (0 == strcmp (method, "POST"))
        {
          con_info->postprocessor =
            MHD_create_post_processor (connection, POSTBUFFERSIZE,
                                       iterate_post, (void *) con_info);

          if (NULL == con_info->postprocessor)
            {
              free (con_info);
              return MHD_NO;
            }

          con_info->connectiontype = POST;
        }
      else
        con_info->connectiontype = GET;

      *con_cls = (void *) con_info;

      return MHD_YES;
    }

  if (0 == strcmp (method, "GET"))
    {
    	char *reply;
    	reply = malloc (strlen (askpage) + strlen (str) + 1);
	  	if (NULL == reply)
	    	return MHD_NO;
	  	snprintf (reply,strlen (askpage) + strlen (str) + 1,askpage,str);
		return send_page (connection,reply);
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
	  //printf("reply= %s \n",reply);
	  //struct connection_info_struct *con_info = *con_cls;

	  if (*upload_data_size != 0)
	  {
		MHD_post_process (con_info->postprocessor, upload_data,
						  *upload_data_size);
		*upload_data_size = 0;

		return MHD_YES;
	  }
	  else if (NULL != con_info->answerstring)
		  return send_page (connection, con_info->answerstring);

	  
  	}

  return send_page (connection, errorpage);
}

void send_usart(uint16_t id,uint8_t state1,uint8_t state2,uint8_t state3) //id,state1,state2,state3 1=开,0=关,2=保持
{
  uint8_t txbuf[10];
  uint8_t i=0;
  txbuf[0] = 0xaa;
  txbuf[1] = 0xc0;
  txbuf[2] = id>>8&0xff;
  txbuf[3] = id&0xff;
  txbuf[4] = state1;
  txbuf[5] = state2;
  txbuf[6] = state3;
  txbuf[7] = 0;
  txbuf[8] = (txbuf[2] + txbuf[3] + txbuf[4] + txbuf[5] + txbuf[6]);
  txbuf[9] = 0xab;
  for(i=0;i<10;i++)
    serialPutchar(usart_fd,txbuf[i]);
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
  struct MHD_Daemon *daemon;
  
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
	
   printf("start server ...\n");

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
		printf("This is the main process.\n");
		sleep(1);
	}
	
	//pthread_join(id,NULL);

	serialClose(usart_fd);
	return (0);
}

