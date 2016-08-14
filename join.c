#include <stdio.h>
#include <wiringPi.h>

int main(void)
{
	if(wiringPiSetup() < 0)
	{
		printf("wiringpi setup failed!\n");
		return 1;
	}
	pinMode (22, OUTPUT);
	while(1)
	{
		printf("permit join\n");
		digitalWrite(22,LOW);
		usleep(500000);
		digitalWrite(22,HIGH);
		sleep(30);
	}
	return 0;
}
