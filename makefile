zbclient:zbclient.c
	gcc zbclient.c -o zbclient -lpthread -lwiringPi -ljson-c -lmicrohttpd -lxml2 -lcurl 
