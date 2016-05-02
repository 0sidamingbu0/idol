/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <microhttpd.h>
#include <stdio.h>

#define PORT 443


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
  printf ("New %s request for %s using version %s\n", method, url, version);

  MHD_get_connection_values (connection, MHD_POSTDATA_KIND, print_out_key,
                             NULL);
  printf ("========================\n", method, url, version);

  
  MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, print_out_key,
							   NULL);
	

  return MHD_NO;
}


int
main ()
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED, NULL,
                             NULL, MHD_OPTION_END);
  if (NULL == daemon)
	{
		printf("return 1\n");
    	return 1;
  	}

  (void) getchar ();

  MHD_stop_daemon (daemon);
  return 0;
}
