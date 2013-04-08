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
This version of demolib is for pigpio version 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "pigpio.h"

/*

P1  Name  gpio    used for

 3  SDA   0/2     i2c
 5  SCL   1/3     i2c
 7  ---   4       LASER
 8  TXD   14      LED1
10  RXD   15      LED2
11  ---   17      SERVO 1
12  ---   18      SERVO 2
13  ---   21/27   SERVO 3
15  ---   22      LED3
16  ---   23      TI Launchpad
18  ---   24      Sonar trigger
19  MOSI  10      Sonar echo
21  MISO  9       Motor B In 1
22  ---   25      LDR
23  SCLK  11      Motor B In 2
24  CE0   8       Motor A In1
26  CE1   7       Motor A In2

*/

#define LASER          4
#define MOTOR_A_IN2    7
#define MOTOR_A_IN1    8
#define MOTOR_B_IN1    9
#define SONAR_ECHO    10
#define MOTOR_B_IN2   11
#define LED1          14
#define LED2          15
#define SERVO1        17
#define SERVO2        18
#define SERVO3        21
#define LED3          22
#define LAUNCHPAD     23
#define SONAR_TRIGGER 24
#define LDR           25

#define SD_CARD_CLOCK 48

#define LEDS 4

short rawAcc[3];
short rawGyr[3];
short rawMag[3];

#define ROLL  0
#define PITCH 1
#define YAW   2

#define ACC_ORIENTATION(X, Y, Z) \
   {rawAcc[ROLL] = -X; rawAcc[PITCH] = -Y; rawAcc[YAW] = Z;}

#define GYRO_ORIENTATION(X, Y, Z) \
   {rawGyr[ROLL] = Y;  rawGyr[PITCH] = -X; rawGyr[YAW] = -Z;}

#define MAG_ORIENTATION(X, Y, Z) \
   {rawMag[ROLL] = X;  rawMag[PITCH] = Y;  rawMag[YAW] = -Z;}


#define CALIBRATIONS 200

#define ADXL345_I2C_ADDR  0x53
#define ITG3200_I2C_ADDR  0x68


static volatile unsigned long SDcardChange=0;
static volatile unsigned long launchpadPulses;
static volatile unsigned long launchpad5;
static volatile unsigned long launchpad10;
static volatile unsigned long launchpad15;
static volatile unsigned long launchpadOutRange;
static volatile int           launchpadErr;
static volatile uint32_t      LDRrechargeTick;


/* forward prototypes */

void LEDlaserTick  (void);
void motorTick(void);
void i2cTick  (void);
void sonarLDRtick(void);

void SDcardAlert(int gpio, int level, uint32_t tick);
void launchpadAlert(int gpio, int level, uint32_t tick);
void sonarAlert(int gpio, int level, uint32_t tick);
void LDRalert(int gpio, int level, uint32_t tick);

void putTTY(char * buf);
void putTTYstr(int row, int col, char * buf);


int main(int argc, char *argv[])
{
   unsigned long cardPrev, cardNow;

   int version;

   char str[256];

   putTTY("\033c"); /* clear console */

   /* before using the library you must call gpioInitialise */

   version = gpioInitialise();

   if (version >= 0)
   {
      /* initialise pins, only gpio numbers are supported */

      gpioSetMode(SERVO1,        PI_OUTPUT);
      gpioSetMode(SERVO2,        PI_OUTPUT);
      gpioSetMode(SERVO3,        PI_OUTPUT);
      gpioSetMode(LASER,         PI_OUTPUT);
      gpioSetMode(LED1,          PI_OUTPUT);
      gpioSetMode(LED2,          PI_OUTPUT);
      gpioSetMode(LED3,          PI_OUTPUT);
      gpioSetMode(MOTOR_A_IN1,   PI_OUTPUT);
      gpioSetMode(MOTOR_A_IN2,   PI_OUTPUT);
      gpioSetMode(MOTOR_B_IN1,   PI_OUTPUT);
      gpioSetMode(MOTOR_B_IN2,   PI_OUTPUT);

      gpioSetMode(SONAR_TRIGGER, PI_OUTPUT);
      gpioWrite  (SONAR_TRIGGER, PI_OFF);

      gpioSetMode(SONAR_ECHO,    PI_INPUT);
      gpioSetMode(LAUNCHPAD,     PI_INPUT);
      gpioSetMode(LDR,           PI_INPUT);

      /* update i2c fifty times a second, timer #0 */

      gpioSetTimerFunc(0, 20, i2cTick);

      /* update LEDs and laser once a second, timer #1 */

      gpioSetTimerFunc(1, 1000, LEDlaserTick);
      
      /* update motors every three seconds, timer #2 */

      gpioSetTimerFunc(2, 3000, motorTick);

      /* update sonar/LDR 10 times a second, timer #3 */

      gpioSetTimerFunc(3, 100, sonarLDRtick);

      /* keep an eye on the SD card clock and see how many
         transitions we can detect. */

      cardPrev = SDcardChange;

      gpioSetAlertFunc(SD_CARD_CLOCK, SDcardAlert);

      /* an attachecd TI launchpad is transmitting high pulses of
      15, 35, 55, 75, ..., 975, 995 microseconds repeating with 50
      microseconds off between each pulse */

      gpioSetAlertFunc(LAUNCHPAD, launchpadAlert);

      /* monitor sonar echos */

      gpioSetAlertFunc(SONAR_ECHO, sonarAlert);

      /* monitor LDR level changes */

      gpioSetAlertFunc(LDR, LDRalert);

      while (1)
      {
         sleep(1);

         cardNow = SDcardChange;

         sprintf(str, "SD clocks per sec = %6lu", cardNow-cardPrev);
         putTTYstr(3, 1, str);

         cardPrev = cardNow;

         sprintf(str, "TI pulses %ld", launchpadPulses);
         putTTYstr(9, 1, str);

         sprintf(str, "+/-5 %ld", launchpad5);
         putTTYstr(10, 6, str);

         sprintf(str, "+/-10 %ld", launchpad10);
         putTTYstr(11, 5, str);

         sprintf(str, "+/-15 %ld", launchpad15);
         putTTYstr(12, 5, str);

         sprintf(str, "Others %ld (last %d)",
            launchpadOutRange, launchpadErr);
         putTTYstr(13, 4, str);
      }
   }   

   gpioTerminate();

   return 0;
}

void LEDlaserTick(void)
{
   static int gpio[LEDS]={LED1, LED2, LED3, LASER};
   static int pos [LEDS]={   0,    3,    6,     9};
   static int inc [LEDS]={   1,    1,    1,     1};

   static int vals[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 255};
 
   int i;

   for (i=0; i<LEDS; i++)
   {
      gpioPWM(gpio[i], vals[pos[i]]);

      pos[i] += inc[i];

      if ( (pos[i]>=(sizeof(vals)/4)) || (pos[i]<0) )
      {
         inc[i] = -inc[i];
         pos[i] += inc[i];
      }
   }
}

void sonarLDRtick(void)
{
   /* trigger a sonar reading */

   gpioWrite(SONAR_TRIGGER, PI_ON);
   usleep(20);
   gpioWrite(SONAR_TRIGGER, PI_OFF);

   /* trigger a LDR reading */

   gpioSetMode(LDR, PI_OUTPUT); /* drain capacitor */

   gpioWrite(LDR, PI_OFF);

   usleep(200);

   LDRrechargeTick = gpioTick();

   gpioSetMode(LDR, PI_INPUT);  /* start capacitor recharge */
 }

void motorTick(void)
{
   static int gpio_in1[2]={MOTOR_A_IN1, MOTOR_B_IN1};
   static int gpio_in2[2]={MOTOR_A_IN2, MOTOR_B_IN2};
   static int speed   [2]={         80,          80};
   static int inc     [2]={         -50,         50};

   int i;
   char str[256];

   for (i=0; i<2; i++)
   {
      speed[i]+=inc[i];

      if (speed[i]<0)
      {
         gpioPWM(gpio_in1[i], -speed[i]);
         gpioPWM(gpio_in2[i], 0);
         if (speed[i] < -205) inc[i] = -inc[i];
         sprintf(str, "MOT%d IN1=%3d IN2=%3d", i+1, -speed[i], 0);
      }
      else
      {
         gpioPWM(gpio_in2[i], speed[i]);
         gpioPWM(gpio_in1[i], 0);
         if (speed[i] > 205) inc[i] = -inc[i];
         sprintf(str, "MOT%d IN1=%3d IN2=%3d", i+1, 0, speed[i]);
      }
      if (i) putTTYstr(7, 1, str); else putTTYstr(5, 1, str);
   }
}

/* loads of code to read/write i2c */

void selectDevice(int i2c, int addr, char * name)
{
   if (ioctl(i2c, I2C_SLAVE, addr) < 0)
   {
      fprintf(stderr, "%s not present\n", name);
   }
}

void writeToDevice(int i2c, char * buf, int len)
{
   if (write(i2c, buf, len) != len)
   {
      fprintf(stderr, "Can't write to device\n");
   }
}

void readADXL345(int i2c)
{
   char buf[8];

   selectDevice(i2c, ADXL345_I2C_ADDR, "ADXL345");

   writeToDevice(i2c, "\x32", 1);
   
   if (read(i2c, buf, 6) != 6)
   {
      fprintf(stderr, "Unable to read from ADXL345\n");
   }
   else
   {
      ACC_ORIENTATION ( 
         ((buf[1]<<8) | buf[0]),
         ((buf[3]<<8) | buf[2]),
         ((buf[5]<<8) | buf[4]) );
   }
}

void readITG3200(int i2c)
{
   char buf[8];

   selectDevice(i2c, ITG3200_I2C_ADDR, "ITG3200");

   writeToDevice(i2c, "\x1D", 1);
   
   if (read(i2c, buf, 6) != 6)
   {
      fprintf(stderr, "Unable to read from ITG3200\n");
   }
   else
   {
      GYRO_ORIENTATION ( 
         ((buf[0]<<8) | buf[1]),
         ((buf[2]<<8) | buf[3]),
         ((buf[4]<<8) | buf[5]) );
   }
}

int initI2Cdevices(void)
{
   int i2c;

   if ((i2c = open("/dev/i2c-0", O_RDWR)) < 0)
   {
      perror("Failed to open i2c bus");
      exit(1);
   }
   
   /* initialise ADXL345 */

   selectDevice(i2c, ADXL345_I2C_ADDR, "ADXL345");

   writeToDevice(i2c, "\x2d\x00", 2);
   writeToDevice(i2c, "\x2d\x10", 2);
   writeToDevice(i2c, "\x2d\x08", 2);
   writeToDevice(i2c, "\x31\x00", 2);
   writeToDevice(i2c, "\x31\x0b", 2);

   /* initialise ITG3200 */

   selectDevice(i2c, ITG3200_I2C_ADDR, "ITG3200");

   writeToDevice(i2c, "\x16\b00011000", 2);

   return i2c;
}

/* an attached IMU (GY-85) supplies orientation information which
   is used to position the servos */

float estimateAngle(int acc, int gyro, float oldAng, int elapsed)
{
   float angleAcc, angleInc, estAngle;
   float secs;

   secs = (float) elapsed / 1e6f;

   angleAcc = (float) acc * 90.0f / 256.0f;

   angleInc = (float) gyro * secs * 2000.0f / 32768.0f;

   estAngle = 0.75 * (oldAng + angleInc) + 0.25 * angleAcc;

   return estAngle;
}

void i2cTick(void)
{
   static int inited = 0;
   static int calibrated = 0;
   static int calibrations = 0;
   static int accCalibX  = 0, accCalibY  = 0, accCalibZ = 0;
   static int gyroCalibX = 0, gyroCalibY = 0, gyroCalibZ = 0;
   static int i2c;
   static float X=0.0, Y=0.0, Z=0.0;

   static uint32_t lastTick;

   uint32_t tick;
   int elapsed;
   int pulse;
   char str[256];

   if (inited)
   {
      tick = gpioTick();
      elapsed = tick - lastTick;
      lastTick = tick;

      readADXL345(i2c);
      readITG3200(i2c);

      if (calibrated)
      {
         X = estimateAngle(
            rawAcc[ROLL], rawGyr[ROLL] -gyroCalibX, X, elapsed);

         Y = estimateAngle(
            rawAcc[PITCH], rawGyr[PITCH] - gyroCalibY, Y, elapsed);

         Z = estimateAngle(
            rawAcc[YAW], rawGyr[YAW] - gyroCalibZ, Z, elapsed);

         pulse = 1500 + (Y * 1000 / 90);
         if (pulse < 500)  pulse = 500;
         if (pulse > 2500) pulse = 2500;
         gpioServo(SERVO1, pulse);

         pulse = 1500 - (X * 500 / 90);
         if (pulse < 1000) pulse = 1000;
         if (pulse > 2000) pulse = 2000;
         gpioServo(SERVO2, pulse);

         /* prefer Z but that doesn't change much */
         pulse = 1500 - (Y * 500 / 90);
         if (pulse < 800)  pulse = 800;
         if (pulse > 2200) pulse = 2200;
         gpioServo(SERVO3, pulse);

         sprintf(str, "X=%4.0f Y=%4.0f Z=%4.0f ", X, Y, Z);
         putTTYstr(1, 1, str);
      }
      else
      {
         accCalibX+=rawAcc[ROLL];
         accCalibY+=rawAcc[PITCH];
         accCalibZ+=rawAcc[YAW];

         gyroCalibX+=rawGyr[ROLL];
         gyroCalibY+=rawGyr[PITCH];
         gyroCalibZ+=rawGyr[YAW];

         if (++calibrations >= CALIBRATIONS)
         {
            accCalibX /= CALIBRATIONS;
            accCalibY /= CALIBRATIONS;
            accCalibZ /= CALIBRATIONS;

            gyroCalibX /= CALIBRATIONS;
            gyroCalibY /= CALIBRATIONS;
            gyroCalibZ /= CALIBRATIONS;

            calibrated = 1;
         }
      }
   }
   else
   {
      i2c = initI2Cdevices();

      gpioServo(SERVO1, 1500);
      gpioServo(SERVO2, 1500);
      gpioServo(SERVO3, 1500);

      inited = 1;
   }
}

void SDcardAlert(int gpio, int level, uint32_t tick)
{
   /* not interested in level, just count changes */

   SDcardChange++;
}

void sonarAlert(int gpio, int level, uint32_t tick)
{
   static uint32_t startTick;

   int diffTick;
   char str[256];

   if (level == PI_ON)
   {
      startTick = tick;
   }
   else if (level == PI_OFF)
   {
      diffTick = tick - startTick;

      if (diffTick < 26100)
      {
         sprintf(str, "Sonar %3d cms", (diffTick+29)/58);
         putTTYstr(15, 1, str);
      }
   }
}

void LDRalert(int pin, int level, uint32_t tick)
{
   int diffTick;
   char str[256];

   if (level == PI_ON)
   {
      diffTick = tick - LDRrechargeTick;

      sprintf(str, "LDR %4d micros", diffTick);
      putTTYstr(17, 1, str);
   }
}

void launchpadAlert(int pin, int level, uint32_t tick)
{
   static int inited = 0;
   static uint32_t lastTick=0;
   static int lastPulseLen=0;

   int pulseLen;
   int pulseDif;

   if (inited)
   {
      pulseLen = tick - lastTick;
      lastTick = tick;

      if (level==0)
      {
         if (lastPulseLen)
         {
            pulseDif = pulseLen - lastPulseLen;
            lastPulseLen = pulseLen;

            launchpadPulses++;

            if ( ((pulseDif >=   15)  && (pulseDif <=   25)) ||
                 ((pulseDif >= -990)  && (pulseDif <= -980)) )
            {
               launchpad5++;
            }
            else if ( ((pulseDif >=   10)  && (pulseDif <=   30)) ||
                      ((pulseDif >= -995)  && (pulseDif <= -975)) )
            {
               launchpad10++;
            }
            else if ( ((pulseDif >=    5)  && (pulseDif <=   35)) ||
                      ((pulseDif >= -1000)  && (pulseDif <= -970)) )
            {
               launchpad15++;
            }
            else
            {
               launchpadOutRange++;
               launchpadErr = pulseDif;
            }
         }
         else lastPulseLen = pulseLen;
      }
   }
   else
   {
      lastTick = tick;
      lastPulseLen = 0;
      launchpadPulses = 0;
      launchpad5 = 0;
      launchpad10 = 0;
      launchpad15 = 0;
      launchpadOutRange = 0;
      inited = 1;
   }
}

void putTTY(char * buf)
{
   write(1, buf, strlen(buf));
}

void putTTYstr(int row, int col, char * buf)
{
   char str[256];

   sprintf(str, "\033[%d;%dH%s", row, col, buf);

   putTTY(str);
}

