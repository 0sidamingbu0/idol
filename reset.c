#include <wiringPi.h>
#include <stdio.h>
int main()
{
  if(wiringPiSetup() < 0)
	{
		printf("wiringpi setup failed!\n");
		return 1;
	}

  pinMode (22, OUTPUT);
  pinMode (26, OUTPUT);

  digitalWrite(22,HIGH);
  usleep(200000);
  digitalWrite(22,LOW);
  
  usleep(200000);
  digitalWrite(26,HIGH);


  usleep(200000);
  digitalWrite(26,LOW);

  usleep(200000);
  digitalWrite(26,HIGH);


  usleep(200000);
  digitalWrite(26,LOW);

  usleep(200000);
  digitalWrite(26,HIGH);


  usleep(200000);
  digitalWrite(26,LOW);

  usleep(200000);
  digitalWrite(26,HIGH);


  usleep(200000);
  digitalWrite(26,LOW);


  usleep(200000);
  digitalWrite(26,HIGH);


  usleep(200000);
  digitalWrite(26,LOW);


  usleep(200000);
  digitalWrite(26,HIGH);



  usleep(200000);
  digitalWrite(26,LOW);
  
  usleep(200000);


  digitalWrite(22,HIGH);
  return 0;
}
