/*
Disclaimer of Warranty.

  THERE IS NO WARRANTY FOR THE SOFTWARE, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE SOFTWARE
IS WITH YOU.  SHOULD THE SOFTWARE PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

Limitation of Liability.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE SOFTWARE AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE SOFTWARE (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE SOFTWARE TO OPERATE WITH ANY OTHER SOFTWARES),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.
*/

/*
This version of checklib is for pigpio version 2
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

void tick(void)
{
   /* count ticks
   */

   static struct timeval lastTime;
   struct tm tmp;
   struct timeval nowTime;
   char buf[32];

   gettimeofday(&nowTime, NULL);

   localtime_r(&nowTime.tv_sec, &tmp);
   strftime(buf, sizeof(buf), "%F@%T", &tmp);

   printf("%s.%03d\n", buf, (int)nowTime.tv_usec/1000);

   /*timersub(&nowTime, &lastTime, &diffTime);*/

   lastTime = nowTime;

   if (inited)
   {
      count++;
   }
   else
   {
      count = 1;

      gettimeofday(&lastTime, NULL);

      inited = 1;
   }
}

void alert(int gpio, int level, uint32_t tick)
{
   /* accumulate number of level changes and average time gpio
      was on and off.  Hopefully the ratio should reflect the
      selected pulsewidth.
   */

   static uint32_t lastTick;

   uint32_t diffTick;

   if (inited)
   {
      count++;

      diffTick = tick - lastTick;
      
      if (level == 0)
      {
         /* elapsed time was on */
         onMicros = onMicros + diffTick;
      }
      else
      {
         /* elapsed time was off */
         offMicros = offMicros + diffTick;
      }
      lastTick = tick;
   }
   else
   {
      count     = 1;
      lastTick  = tick;
      onMicros  = 0;
      offMicros = 0;
      inited    = 1;
   }
}

static void timerTest(unsigned waitfor, unsigned ms)
{
   unsigned ep, ep1, ep2;
 
   ep= (waitfor*1000)/ms; ep1=ep-1; ep2=ep+1;

   printf("Timer ticktest (%d ms), wait %d seconds\n", ms, waitfor);
   printf("Expect %d to %d ticks\n", ep1, ep2);

   inited = 0;
   gpioSetTimerFunc(0, ms, tick);
   sleep(waitfor);
   gpioSetTimerFunc(0, ms, NULL);

   /* and the stats were? */

   printf("ticks=%d\n", count);

   if ((count>=ep1) && (count<=ep2))
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else
   {
      printf("TEST %d: FAILED\n\n", test);
   }

   ++test; sleep(1);
}

static void servoTest(unsigned waitfor, unsigned pulsewidth)
{
   int ticks, on, off;
   unsigned expectedPulses, ep1, ep2;
   float    expectedRatio,  er1, er2;
   float ratio;
 
   expectedPulses=(492*waitfor)/10; ep1=(482*waitfor)/10; ep2=(502*waitfor)/10;
   expectedRatio = (float)(20000-pulsewidth)/(float)pulsewidth;
   er1=expectedRatio*0.9; er2=expectedRatio*1.1;

   printf("Servo pulse test (%d micros), wait %d seconds\n",
      pulsewidth, waitfor);
   printf("Expect %d pulses and an off/on ratio of %.1f\n",
      expectedPulses, expectedRatio);

   gpioServo(GPIO, pulsewidth); 

   inited = 0;
   gpioSetAlertFunc(GPIO, alert);
   sleep(waitfor);
   gpioSetAlertFunc(GPIO, NULL);

   gpioServo(GPIO, 0);

   /* and the stats were? */

   ticks = count/2; on = onMicros/1000; off = offMicros/1000;
   ratio = (float)off/(float)on;

   printf("servo pulses=%d on ms=%d off ms=%d ratio=%.1f\n", ticks, on, off, ratio);

   if ( ((ticks>ep1) && (ticks<ep2)) && ((ratio>er1) && (ratio<er2)) )
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else
   {
      printf("TEST %d: FAILED\n\n", test);
   }

   ++test; sleep(1);
}

static void pwmTest(unsigned waitfor, unsigned pulsewidth)
{
   int on, off;
   float    expectedRatio,  er1, er2;
   float ratio;

   expectedRatio = (float)(255-pulsewidth)/255.0;
   er1=expectedRatio*0.9; er2=expectedRatio*1.1;

   printf("PWM test (%d), wait %d seconds\n", pulsewidth, waitfor);
   printf("Expect an off/on ratio of %.3f\n", expectedRatio);

   inited = 0;
   gpioSetAlertFunc(GPIO, alert);
   gpioPWM(GPIO, pulsewidth); 
   sleep(waitfor);
   gpioPWM(GPIO, 0);
   gpioSetAlertFunc(GPIO, NULL);

   /* and the stats were? */

   on = onMicros/1000; off = offMicros/1000;
   ratio = (float)off/((float)on+(float)off);

   printf("pwm on ms=%d off ms=%d ratio=%.3f\n", on, off, ratio);

   if ((ratio>er1) && (ratio<er2))
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else
   {
      printf("TEST %d: FAILED\n\n", test);
   }

   ++test; sleep(1);
}

void checkErrors(int expected)
{
   if (errors==expected)
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else printf("TEST %d: FAILED\n\n", test);

   errors = 0;

   ++test; sleep(1);
}

void checkValidation(void)
{
   int secs, micros;

   /* check function parameter validation */

   printf("Function parameter validation tests\n");
   printf("Expect error messages\n\n");
   sleep(1);


   if (gpioSetMode(54, 0) == PI_BAD_GPIO)  errors++;
   if (gpioSetMode(-1, 0) == PI_BAD_GPIO)  errors++;
   if (gpioSetMode(GPIO, 8)  == PI_BAD_MODE)  errors++;
   if (gpioSetMode(GPIO, -1) == PI_BAD_MODE)  errors++;

   checkErrors(4);

   if (gpioGetMode(54) == PI_BAD_GPIO) errors++;
   if (gpioGetMode(-1) == PI_BAD_GPIO) errors++;

   checkErrors(2);

   if (gpioSetPullUpDown(54, 0)  == PI_BAD_GPIO) errors++;
   if (gpioSetPullUpDown(-1, 0)  == PI_BAD_GPIO) errors++;
   if (gpioSetPullUpDown(50, 3)  == PI_BAD_PUD)  errors++;
   if (gpioSetPullUpDown(50, -1) == PI_BAD_PUD)  errors++;

   checkErrors(4);

   if (gpioRead(-1) == PI_BAD_GPIO) errors++;
   if (gpioRead(54) == PI_BAD_GPIO) errors++;

   checkErrors(2);

   if (gpioWrite(54, 0)  == PI_BAD_GPIO)  errors++;
   if (gpioWrite(-1, 0)  == PI_BAD_GPIO)  errors++;
   if (gpioWrite(50, 2)  == PI_BAD_LEVEL) errors++;
   if (gpioWrite(50, -1) == PI_BAD_LEVEL) errors++;

   checkErrors(4);

   if (gpioPWM(32, 0)  == PI_BAD_USER_GPIO)  errors++;
   if (gpioPWM(-1, 0)  == PI_BAD_USER_GPIO)  errors++;
   if (gpioPWM(GPIO, 256) == PI_BAD_DUTYCYCLE)  errors++;
   if (gpioPWM(GPIO, -1)  == PI_BAD_DUTYCYCLE)  errors++;

   checkErrors(4);

   if (gpioServo(32, 0)   == PI_BAD_USER_GPIO)  errors++;
   if (gpioServo(-1, 0)   == PI_BAD_USER_GPIO)  errors++;
   if (gpioServo(GPIO, 1)    == PI_BAD_PULSEWIDTH) errors++;
   if (gpioServo(GPIO,-1)    == PI_BAD_PULSEWIDTH) errors++;
   if (gpioServo(GPIO, 499)  == PI_BAD_PULSEWIDTH) errors++;
   if (gpioServo(GPIO, 2501) == PI_BAD_PULSEWIDTH) errors++;

   checkErrors(6);

   if (gpioSetAlertFunc(54, alert) == PI_BAD_GPIO) errors++;
   if (gpioSetAlertFunc(-1, alert) == PI_BAD_GPIO) errors++;

   checkErrors(2);

   if (gpioSetAlertTimeout(54, 0)       == PI_BAD_GPIO)      errors++;
   if (gpioSetAlertTimeout(-1, 0)       == PI_BAD_GPIO)      errors++;
   if (gpioSetAlertTimeout(GPIO, 60001) == PI_BAD_TIMEOUT)   errors++;
   if (gpioSetAlertTimeout(GPIO, -1)    == PI_BAD_TIMEOUT)   errors++;
   if (gpioSetAlertTimeout(GPIO, 0)     == PI_NO_ALERT_FUNC) errors++;

   checkErrors(5);

   if (gpioSetTimerFunc(10,  20, tick)  == PI_BAD_TIMER) errors++;
   if (gpioSetTimerFunc(-1, 20, tick)   == PI_BAD_TIMER) errors++;
   if (gpioSetTimerFunc(0, 9, tick)     == PI_BAD_MS)    errors++;
   if (gpioSetTimerFunc(0, 60001, tick) == PI_BAD_MS)    errors++;

   checkErrors(4);

   if (gpioTime(-1, &secs, &micros) == PI_BAD_TIMETYPE) errors++;
   if (gpioTime(2,  &secs, &micros) == PI_BAD_TIMETYPE) errors++;

   checkErrors(2);

   if (gpioSleep(-1, 1, 0)                      == PI_BAD_TIMETYPE) errors++;
   if (gpioSleep(2,  1, 0)                      == PI_BAD_TIMETYPE) errors++;
   if (gpioSleep(PI_TIME_ABSOLUTE, -1,  0)      == PI_BAD_SECONDS)  errors++;
   if (gpioSleep(PI_TIME_ABSOLUTE,  0, -1)      == PI_BAD_MICROS)   errors++;
   if (gpioSleep(PI_TIME_ABSOLUTE,  0, 1000000) == PI_BAD_MICROS)   errors++;
   if (gpioSleep(PI_TIME_RELATIVE, -1,  0)      == PI_BAD_SECONDS)  errors++;
   if (gpioSleep(PI_TIME_RELATIVE,  0, -1)      == PI_BAD_MICROS)   errors++;
   if (gpioSleep(PI_TIME_RELATIVE,  0, 1000000) == PI_BAD_MICROS)   errors++;

   checkErrors(8);
}

void checkGpioTime(void)
{
   struct timeval nowA, nowR, tvR, tvA, diffR, diffA;
   int diffMicrosA, diffMicrosR;
   int i, ok;

   int secR, micR, secA, micA;

   printf("Library timer tests.\n");

   ok = 0;

   for (i=0; i<10; i++)
   {
      gettimeofday(&nowA, NULL);                /* reference absolute time */

      gpioTime(PI_TIME_ABSOLUTE, &secA, &micA); /* absolute library time */
      gpioTime(PI_TIME_RELATIVE, &secR, &micR); /* relative library time */

      timersub(&nowA, &libInitTime, &nowR);     /* reference relative time */

      tvA.tv_sec = secA; tvA.tv_usec = micA;
      tvR.tv_sec = secR; tvR.tv_usec = micR;

      /* difference between reference and library absolute time */
      timersub(&tvA, &nowA, &diffA);

      /* difference between reference and library relative time */
      timersub(&tvR, &nowR, &diffR);


      diffMicrosA = (diffA.tv_sec*1000000)+diffA.tv_usec;

      diffMicrosR = (diffR.tv_sec*1000000)+diffR.tv_usec;

      if (diffMicrosA < 0) diffMicrosA = -diffMicrosA;
      if (diffMicrosR < 0) diffMicrosR = -diffMicrosR;

      if ((diffMicrosA < 10) && (diffMicrosR < 90)) ok++;
      
      printf("ABS time diff=%d, REL time diff=%d\n",
         diffMicrosA, diffMicrosR);

      sleep(1);
   }

   if (ok == 10)
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else printf("TEST %d: FAILED\n\n", test);

   ++test; sleep(1);
}

void checkGpioSleep(unsigned timetype)
{
   struct timeval t1, t2, tD;
   int i, ok, secs, micros, diffMicros, expMicros, errMicros;

   if (timetype == PI_TIME_ABSOLUTE)
      printf("Library gpioSleep ABSOLUTE tests.\n");
   else
      printf("Library gpioSleep RELATIVE tests.\n");

   ok = 0;

   for (i=15; i>0; i--)
   {
      expMicros = i * 100000;

      if (timetype == PI_TIME_ABSOLUTE)
      {
         gpioTime(PI_TIME_ABSOLUTE, &secs, &micros);

         secs   += (i / 10);
         micros += (i % 10) * 100000;

         if (micros > 999999) { secs++; micros -= 1000000; }
      }
      else
      {
         secs   = (i / 10);
         micros = (i % 10) * 100000;
      }

      gettimeofday(&t1, NULL);

      gpioSleep(timetype, secs, micros);

      gettimeofday(&t2, NULL);

      timersub(&t2, &t1, &tD);

      diffMicros = (tD.tv_sec*1000000)+tD.tv_usec;

      errMicros = diffMicros - expMicros;

      if (errMicros < 500) ok++;
      
      printf("secs=%d micros=%d err=%d\n", secs, micros, errMicros);
   }

   if (ok == 15)
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else printf("TEST %d: FAILED\n\n", test);

   ++test; sleep(1);
}

int countBank2PinChanges(int pin, int loops)
{
   static uint32_t old=0;

   uint32_t new, changes;
   int i, count;

   count = 0;

   for (i=0;i<loops;i++)
   {
      new = gpioRead_Bits_32_53();

      changes = new ^ old;

      if (changes & (1<<(pin-32))) count++;
   }
   return count;
}

void checkReadWriteBits(void)
{
   uint32_t bank1, bank2;

   int i, ok, count1, count2;

   printf("Library gpioRead/Write_Bits_x_x_Set/Clear Tests\n");
   printf("Expect 0 for pin 47 and >200000 for pin 48.\n");
   printf("Expect the green LED to flash.\n\n");

   ok = 0;

   for (i=0; i<20; i++)
   {
      gpioWrite_Bits_0_31_Set(1<<GREENLED);

      count1 = countBank2PinChanges(SDDET, 1000000);

      gpioWrite_Bits_0_31_Clear(1<<GREENLED);

      count2 = countBank2PinChanges(SDCLK, 1000000);

      bank1 = gpioRead_Bits_0_31();
      bank2 = gpioRead_Bits_32_53();

      printf("bank1=%08X, bank2=%08X, 47=%d 48=%d\n",
         bank1, bank2, count1, count2);

      if ((count1 == 0) && (count2 > 200000)) ok++;
   }
   
   if (ok == 20)
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else printf("TEST %d: FAILED\n\n", test);

   ++test; sleep(1);
}

void checkGpioTick(void)
{
   uint32_t startTick, endTick;
   int diffTick;

   printf("Library gpioTick Test\n");
   printf("Expect approximately 2 million ticks to have elapsed.\n\n");

   startTick = gpioTick();
   sleep(2); /* 2 seconds being 2 million ticks */
   endTick = gpioTick();

   diffTick = endTick - startTick;

   printf("%d ticks have elapsed\n", diffTick);
   
   if ((diffTick >= 1990000) && (diffTick <= 2010000))
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }
   else printf("TEST %d: FAILED\n\n", test);

   ++test; sleep(1);
}

int main(int argc, char *argv[])
{

   int i, waitfor;

   if (argc > 1) GPIO = atoi(argv[1]);

   fprintf(stderr,
"*****************************************************************\n"\
"* WARNING: This program sends pulses to gpio #%02d                *\n"\
"* Make sure that nothing which could be damaged is              *\n"\
"* connected to this gpio.  A LED or similar should be OK        *\n"\
"* although nothing needs to be connected.                       *\n"\
"*                                                               *\n"\
"* NOTE: many of the tests are statistical in nature, assuming   *\n"\
"* that events of a short nature will on average be detected     *\n"\
"* by sampling.  Don't fret if a particular test fails, try      *\n"\
"* running the tests again.                                      *\n"\
"*                                                               *\n"\
"* You may choose another gpio by specifying its number on       *\n"\
"* the command line, e.g. sudo ./checklib 17 will use gpio 17.   *\n"\
"*                                                               *\n"\
"* Press y (RETURN) to continue, any other character to cancel.  *\n"\
"*****************************************************************\n", GPIO);

   if (getchar() != 'y') return 0;   

   printf("Initialisation test\n");

   gettimeofday(&libInitTime, NULL);

   i = gpioInitialise();

   if (i<0) printf("TEST %d: FAILED\n\n", test);
   else
   {
      printf("TEST %d: PASS, pigpio version is %d\n\n", test, i);
      ++passes;
   }

   ++test; sleep(1);

   waitfor = 2;

   printf("Alert function test, wait %d seconds\n", waitfor);
   printf("No detected events on gpio 4 expected\n");

   inited = 0;
   gpioSetAlertFunc(GPIO, alert);
   sleep(waitfor);
   gpioSetAlertFunc(GPIO, NULL);

   printf("Events=%d\n", count);

   if (count) printf("TEST %d: FAILED\n\n", test);
   else       
   {
      printf("TEST %d: PASS\n\n", test);
      ++passes;
   }

   ++test; sleep(1);

   servoTest(10, 1500);
   servoTest(10, 2000);
   servoTest(10, 2500);

   pwmTest(5,  50);
   pwmTest(5, 100);
   pwmTest(5, 150);
   pwmTest(5, 200);

   timerTest(5, 250);
   timerTest(5, 333);
   timerTest(5, 1000);

   checkValidation();

   checkGpioTime();

   checkGpioSleep(PI_TIME_RELATIVE);

   checkGpioSleep(PI_TIME_ABSOLUTE);

   checkReadWriteBits();

   checkGpioTick();

   printf("Hardware revision is %d\n\n", gpioHardwareRevision());

   printf("Summary: %d tests, %d passes\n", test-1, passes);

   gpioTerminate(); /* stop DMA and free memory */

   return (passes - (test-1));
}

