sudo apt-get install libmicrohttpd-dev
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libjson-c-dev

sudo cp curl/* /usr/include -r
sudo cp json/* /usr/include
sudo cp microhttpd/* /usr/include

gcc zbserver.c -o zbserver -lmicrohttpd -lwiringPi -lpthread
gcc zbclient.c -o zbclient -lwiringPi -lpthread -lcurl -ljson-c

20160405第一版调试通过