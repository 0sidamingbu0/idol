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



#define PORT 6868

#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     3000
#define MAXANSWERSIZE   512
#define GET             0
#define POST            1
int idx = 0;
typedef unsigned char   uint8_t;     //无符号8位数
const char *strrr;


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
  
  printf("key =%s \n",key);
  printf("size =%d \n",size);
  printf("data =%s \n",data);
  printf("------------------\n");
    
  

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
print_out_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
  printf ("%s: %s\n", key, value);
  return MHD_YES;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
	struct connection_info_struct *con_info = *con_cls;
	
	printf ("New %s request for %s using version %s\n", method, url, version);
	MHD_get_connection_values (connection, MHD_HEADER_KIND, print_out_key,
									  NULL);
	MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, print_out_key,
										  NULL);

  //printf("connection->read_buffer = %s \n",connection->read_buffer);
  	  const char* length = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);  		
	  const char* body = MHD_lookup_connection_value (connection, MHD_POSTDATA_KIND, NULL);  	    
	  printf("length=%s\n",length);
	  printf("body=%s\n",body);
	  

	  
  if (NULL == *con_cls)
    {
    	//printf("NuLl=ClS \n");
		
      struct connection_info_struct *con_info;


//	  MHD_get_connection_values (connection, MHD_POSTDATA_KIND, print_out_key,
//								   NULL);
//		printf ("========================\n", method, url, version);
	  
//		
//		MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, print_out_key,
//									 NULL);




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
	  	if(strrr != NULL)\
			return send_page (connection, strrr);
		else
			return send_page (connection, errorpage);
    }

  if (0 == strcmp (method, "POST"))
  	{  		
	  int ret;
	  
//	  const char* body = MHD_lookup_connection_value (connection, MHD_POSTDATA_KIND, NULL);  
//	  printf("body = %s\n",body); 

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
  printf("recieve No: %d\n",idx++);
  printf("\n\n");
  return send_page (connection, errorpage);
}


int main(void)
{

  struct MHD_Daemon *daemon;
 

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
		usleep(500000);
      
	}
	

	return (0);
}
