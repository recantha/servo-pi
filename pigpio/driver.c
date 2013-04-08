/*
for pigpio version 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "pigpio.h"

#define GREENLED 16
#define SDDET    47
#define SDCLK    48

int test  =1;
int passes=0;
int errors=0;

struct timeval libInitTime;

int GPIO=4;

unsigned inited, count, onMicros, offMicros;

int __nsleep(const struct timespec *req, struct timespec *rem)
{
    struct timespec temp_rem;
    if(nanosleep(req,rem)==-1)
        __nsleep(rem,&temp_rem);
    else
        return 1;
}
 
int msleep(unsigned long milisec)
{
    struct timespec req={0},rem={0};
    time_t sec=(int)(milisec/1000);
    milisec=milisec-(sec*1000);
    req.tv_sec=sec;
    req.tv_nsec=milisec*1000000L;
    __nsleep(&req,&rem);
    return 1;
}

static void servoTest(unsigned waitfor, unsigned pulsewidth)
{
   int ticks, on, off;
   unsigned expectedPulses, ep1, ep2;
   float    expectedRatio,  er1, er2;
   float ratio;
   float delay=400;
 
/*   expectedPulses=(492*waitfor)/10; ep1=(482*waitfor)/10; ep2=(502*waitfor)/10;
   expectedRatio = (float)(20000-pulsewidth)/(float)pulsewidth;
   er1=expectedRatio*0.9; er2=expectedRatio*1.1;
*/

/*   printf("Servo pulse test (%d micros), wait %d seconds\n",
      pulsewidth, waitfor);
   printf("Expect %d pulses and an off/on ratio of %.1f\n",
      expectedPulses, expectedRatio);
*/

   gpioServo(GPIO, pulsewidth); 

/*   inited = 0;
   gpioSetAlertFunc(GPIO, alert);
*/
   msleep(delay);
/*   gpioSetAlertFunc(GPIO, NULL); */

   gpioServo(GPIO, 0);

/*
    ticks = count/2; on = onMicros/1000; off = offMicros/1000;
   ratio = (float)off/(float)on;
*/

/*   printf("servo pulses=%d on ms=%d off ms=%d ratio=%.1f\n", ticks, on, off, ratio);

   if ( ((ticks>ep1) && (ticks<ep2)) && ((ratio>er1) && (ratio<er2)) )
   {
     printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else
   {
      printf("TEST %d: FAILED\n\n", test);
   }
*/
}

int main(int argc, char *argv[])
{

   int i;
   int position=1000;

   if (argc > 1) GPIO = atoi(argv[1]);
   if (argc > 2) position = atoi(argv[2]);

   printf("Initialisation test\n");

   gettimeofday(&libInitTime, NULL);

   /* Initialise pigpio */
   i = gpioInitialise();

   if (i<0) printf("Initialisation failed");
   else
   {
      printf("Initialisating successful. pigpio version is %d\n\n", i);
      ++passes;
   }

   printf("Hardware revision is %d\n\n", gpioHardwareRevision());

   servoTest(1, position);

   gpioTerminate(); /* stop DMA and free memory */

   return (1);
}

