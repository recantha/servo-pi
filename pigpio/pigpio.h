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
PARTIES OR A FAILURE OF THE SOFTWARE TO OPERATE WITH ANY OTHER SOFTWARE),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.
*/

#ifndef PIGPIO_H
#define PIGPIO_H

/************************************************************************** /
/                                                                           /
/ pigpio is a C library for the Raspberry Pi which allows                   /
/ control of the gpios.                                                     /
/                                                                           /
/ Its main features are:                                                    /
/                                                                           /
/ 1) provision of PWM on any number of gpios 0-31 simultaneously.           /
/ 2) provision of servo pulses on any number of gpios 0-31 simultaneously.  /
/ 3) callbacks when any of gpios 0-53 change state.                         /
/ 4) callbacks at timed intervals.                                          /
/ 5) reading/writing all of the gpios in a bank (0-31, 32-53) as a          /
/    single operation.                                                      /
/ 6) individually setting gpio modes, reading and writing.                  /
/                                                                           /
/ NOTE:                                                                     /
/                                                                           /
/ ALL gpios are identified by their Broadcom number.                        /
/                                                                           /
*************************************************************************** /
/                                                                           /
/ The PWM and servo pulses are timed using the DMA and PWM peripherals.     /
/                                                                           /
/ This use was inspired by Richard Hirst's servoblaster kernel module.      /
/ See https://github.com/richardghirst/PiBits                               /
/ Tag rgh on the Raspberry Pi forums http://www.raspberrypi.org/phpBB3/     /
/                                                                           /
*************************************************************************** /
/                                                                           /
/ Usage:                                                                    /
/                                                                           /
/ copy libpigpio.a-hard or libpigpio.a-soft to libpigpio.a                  /
/ copy libpigpio.a to /usr/local/lib                                        /
/ copy pigpio.h    to /usr/local/include                                    /
/                                                                           /
/ #include <pigpio.h> in your source files                                  /
/                                                                           /
/ Assuming your source is in example.c use the following command to build   /
/                                                                           /
/ gcc -o example example.c -lpigpio -lpthread -lrt                          /
/                                                                           /
/ For examples see checklib.c and demolib.c                                 /
/                                                                           /
****************************************************************************/

/*
This version of pigpio.h is for pigpio version 2
*/

#include <stdint.h>

#define PIGPIO_VERSION 2

typedef void (*gpioAlertFunc_t)(int gpio, int level, uint32_t tick);
typedef void (*gpioTimerFunc_t)(void);

/*-------------------------------------------------------------------------*/

/* library function prototypes */

/*
   All the functions which return an int return < 0 on error.
*/

int      gpioInitialise(void);
void     gpioTerminate(void);

int      gpioSetMode(unsigned gpio, unsigned mode);
int      gpioGetMode(unsigned gpio);

int      gpioSetPullUpDown(unsigned gpio, unsigned pud);

int      gpioRead (unsigned gpio);
int      gpioWrite(unsigned gpio, unsigned level);

int      gpioPWM(unsigned gpio, unsigned dutycycle);

int      gpioServo(unsigned gpio, unsigned pulsewidth);

int      gpioSetAlertFunc(unsigned gpio, gpioAlertFunc_t f);
int      gpioSetAlertTimeout(unsigned gpio, unsigned timeout);

int      gpioSetTimerFunc(unsigned timer, unsigned ms, gpioTimerFunc_t f);

uint32_t gpioRead_Bits_0_31(void);
uint32_t gpioRead_Bits_32_53(void);

void     gpioWrite_Bits_0_31_Clear(uint32_t levels);
void     gpioWrite_Bits_32_53_Clear(uint32_t levels);

void     gpioWrite_Bits_0_31_Set(uint32_t levels);
void     gpioWrite_Bits_32_53_Set(uint32_t levels);

int      gpioTime (unsigned timetype, int * seconds, int * micros);
int      gpioSleep(unsigned timetype, int   seconds, int   micros);

uint32_t gpioTick(void);

unsigned gpioHardwareRevision(void);

/*-------------------------------------------------------------------------*/

/* gpio: 0-31 for servos and PWM, otherwise 0-53 */

#define PI_MIN_GPIO        0
#define PI_MAX_SERVO_GPIO 31
#define PI_MAX_PWM_GPIO   31
#define PI_MAX_GPIO       53

/* dutycycle: 0-255 */

#define PI_MIN_DUTYCYCLE 0
#define PI_MAX_DUTYCYCLE 255

/* pulsewidth: 0, 500-2500 */

#define PI_SERVO_OFF 0
#define PI_MIN_SERVO_PULSEWIDTH 500
#define PI_MAX_SERVO_PULSEWIDTH 2500

/* level: 0-1 */

#define	PI_OFF 0
#define	PI_ON  1

/* level reported for gpio timeout, see gpioSetAlertTimeout */

#define PI_TIMEOUT 2

/* pud: 0-2 */

#define	PI_PUD_OFF  0
#define	PI_PUD_DOWN 1
#define	PI_PUD_UP   2

/* mode: 0-7 */

#define PI_INPUT  0
#define PI_OUTPUT 1
#define PI_ALT0   4
#define PI_ALT1   5
#define PI_ALT2   6
#define PI_ALT3   7
#define PI_ALT4   3
#define PI_ALT5   2

/* timer: 0-9 */

#define PI_MIN_TIMER 0
#define PI_MAX_TIMER 9

/* ms: 10-60000 */

#define PI_MIN_MS 10
#define PI_MAX_MS 60000

/* timeout: 0-60000 */

#define PI_MIN_TIMEOUT 0
#define PI_MAX_TIMEOUT 60000

/* timetype: 0-1 */

#define PI_TIME_RELATIVE 0
#define PI_TIME_ABSOLUTE 1

/* errors */

#define PI_INIT_FAILED     -1  /* gpioInitialise failed */
#define PI_BAD_USER_GPIO   -2  /* gpio not 0-31 */
#define PI_BAD_GPIO        -3  /* gpio not 0-53 */
#define PI_BAD_MODE        -4  /* mode not 0-7 */
#define PI_BAD_LEVEL       -5  /* level not 0-1 */
#define PI_BAD_PUD         -6  /* pud not 0-2 */
#define PI_BAD_PULSEWIDTH  -7  /* pulsewidth not 0 or 500-2500 */
#define PI_BAD_DUTYCYCLE   -8  /* dutycycle not 0-255 */
#define PI_BAD_TIMER       -9  /* timer not 0-9 */
#define PI_BAD_MS          -10 /* ms not 10-60000 */
#define PI_BAD_TIMETYPE    -11 /* timetype not 0-1 */
#define PI_BAD_SECONDS     -12 /* seconds < 0 */
#define PI_BAD_MICROS      -13 /* micros not 0-999999 */
#define PI_TIMER_FAILED    -14 /* gpioSetTimerFunc failed */
#define PI_BAD_TIMEOUT     -15 /* timeout not 0-60000 */
#define PI_NO_ALERT_FUNC   -16 /* trying to set timeout for null function */



#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------------------------------*/
int gpioInitialise(void);
/*-------------------------------------------------------------------------*/
/* Initialises the library, call before using any other library functions.

   Returns the library version.
*/



/*-------------------------------------------------------------------------*/
void gpioTerminate(void);
/*-------------------------------------------------------------------------*/
/* Terminates the library, call before program exit.

   NOTES:

   This function resets the DMA and PWM peripherals, releases memory, and
   terminates any running threads.
*/



/*-------------------------------------------------------------------------*/
int gpioSetMode(unsigned gpio, unsigned mode);
/*-------------------------------------------------------------------------*/
/* Sets the gpio mode, typically input or output.

   Arduino style: pinMode.
*/



/*-------------------------------------------------------------------------*/
int gpioGetMode(unsigned gpio);
/*-------------------------------------------------------------------------*/
/* Gets the gpio mode.
*/



/*-------------------------------------------------------------------------*/
int gpioSetPullUpDown(unsigned gpio, unsigned pud);
/*-------------------------------------------------------------------------*/
/* Sets or clears resistor pull ups or downs on the gpio.
*/



/*-------------------------------------------------------------------------*/
int gpioRead (unsigned gpio);
/*-------------------------------------------------------------------------*/
/* Reads the gpio level, on or off.

   Arduino style: digitalRead.
*/



/*-------------------------------------------------------------------------*/
int gpioWrite(unsigned gpio, unsigned level);
/*-------------------------------------------------------------------------*/
/* Sets the gpio level, on or off.

   Arduino style: digitalWrite
*/



/*-------------------------------------------------------------------------*/
int gpioPWM(unsigned gpio, unsigned dutycycle);
/*-------------------------------------------------------------------------*/
/* Starts PWM on the gpio, dutycycle between 0 (off) and 255 (fully on).

   Arduino style: analogWrite

   NOTES:

   This and the servo functionality use the DMA and PWM peripherals
   to control and schedule the pulse lengths and duty cycles.
*/



/*-------------------------------------------------------------------------*/
int gpioServo(unsigned gpio, unsigned pulsewidth);
/*-------------------------------------------------------------------------*/
/* Starts servo pulses on the gpio, 0 (off), 500 (most anti-clockwise) to
   2500 (most clockwise).

   NOTES:

   The range supported by servos varies and should probably be determined
   by experiment.  A value of 1500 should always be safe and represents
   the mid-point of rotation.  You can DAMAGE a servo if you command it
   to move beyond its limits.

   EXAMPLE:

   ...
   gpioServo(17, 1500);
   ...

   This example causes an on pulse of 1500 microseconds duration to be
   transmitted on gpio 17 at a rate of 50 times per second.

   This will command a servo connected to gpio 17 to rotate to
   its mid-point.
*/



/*-------------------------------------------------------------------------*/
int gpioSetAlertFunc(unsigned gpio, gpioAlertFunc_t f);
/*-------------------------------------------------------------------------*/
/* Registers a function to be called (a callback) when the specified
   gpio changes state.

   One function may be registered per gpio.

   The function is passed the gpio, the new level, and the tick.

   The alert may be cancelled by passing NULL as the function.

   EXAMPLE:

   void aFunction(int gpio, int level, uint32_t tick)
   {
      printf("gpio %d became %d at %d\n", gpio, level, tick);
   }
   ...
   gpioSetAlertFunc(4, aFunction);
   ...

   This example causes aFunction to be called whenever
   gpio 4 changes state.

   NOTES:

   The gpio transitions are sampled 200 thousand times per second. Level
   changes of less than 5 microseconds may be missed.

   The thread which calls the alert functions is triggered 1000 times per
   second.  The active alert functions will be called once per level change
   since the last time the thread was activated. i.e. The active alert
   functions will get all level changes but there will be a latency.

   The tick value is the time stamp of the sample in microseconds, see
   gpioTick for more details.
*/



/*-------------------------------------------------------------------------*/
int gpioSetAlertTimeout(unsigned gpio, unsigned timeout);
/*-------------------------------------------------------------------------*/
/* Sets a timeout for a previously registered alert function.
   
   The timeout is nominally in milliseconds.

   One timeout may be registered per gpio.

   It is an error if no alert function is currently registered for
   the gpio.

   if no level change has been detected for the gpio for timeout
   milliseconds the registered alert function is called with the
   level set to PI_TIMEOUT.

   The timeout may be cancelled by setting timeout to 0.

   EXAMPLE:

   void aFunction(int gpio, int level, uint32_t tick)
   {
      printf("gpio %d became %d at %d\n", gpio, level, tick);
   }
   ...
   gpioSetAlertFunc(4, aFunction);
   gpioSetAlertTimeout(4, 5);
   ...

   This example causes aFunction to be called whenever
   gpio 4 changes state or every 5 ms.
*/



/*-------------------------------------------------------------------------*/
int gpioSetTimerFunc(unsigned timer, unsigned ms, gpioTimerFunc_t f);
/*-------------------------------------------------------------------------*/
/* Registers a function to be called (a callback) every ms milliseconds.

   10 timers are supported numbered 0 to 9.

   One function may be registered per timer.

   The timer may be cancelled by passing NULL as the function.

   EXAMPLE:

   ...
   void bFunction(void)
   {
      printf("two seconds have elapsed\n");
   }
   ...
   gpioSetTimerFunc(0, 2000, bFunction);
   ...

   This example causes bFunction to be called every 2000 milliseconds.
*/




/*-------------------------------------------------------------------------*/
uint32_t gpioRead_Bits_0_31(void);
/*-------------------------------------------------------------------------*/
/* Returns the current level of gpios 0-31.
*/



/*-------------------------------------------------------------------------*/
uint32_t gpioRead_Bits_32_53(void);
/*-------------------------------------------------------------------------*/
/* Returns the current level of gpios 32-53.
*/



/*-------------------------------------------------------------------------*/
void gpioWrite_Bits_0_31_Clear(uint32_t levels);
/*-------------------------------------------------------------------------*/
/* Clears gpios 0-31 if the corresponding bit in levels is set.

   EXAMPLE:

   To clear (set to 0) gpios 4, 7, and 15.

   ...
   gpioWrite_Bits_0_31_Clear( (1<<4) | (1<<7) | (1<<15) );
   ...
*/



/*-------------------------------------------------------------------------*/
void gpioWrite_Bits_32_53_Clear(uint32_t levels);
/*-------------------------------------------------------------------------*/
/* Clears gpios 32-53 if the corresponding bit (0-21) in levels is set.
*/



/*-------------------------------------------------------------------------*/
void gpioWrite_Bits_0_31_Set(uint32_t levels);
/*-------------------------------------------------------------------------*/
/* Sets gpios 0-31 if the corresponding bit in levels is set.
*/



/*-------------------------------------------------------------------------*/
void gpioWrite_Bits_32_53_Set(uint32_t levels);
/*-------------------------------------------------------------------------*/
/* Sets gpios 32-53 if the corresponding bit (0-21) in levels is set.

   EXAMPLE:

   To set (set to 1) gpios 32, 40, and 53.

   ...
   gpioWrite_Bits_32_53_Set( (1<<(32-32)) | (1<<(40-32)) | (1<<(53-32)) );
   ...
*/



/*-------------------------------------------------------------------------*/
int gpioTime (unsigned timetype, int * seconds, int * micros);
/*-------------------------------------------------------------------------*/
/* Updates the seconds and micros variables with the current time.

   If timetype is PI_TIME_ABSOLUTE updates seconds and micros with the
   number of seconds and microseconds since the epoch (1st January 1970).

   If timetype is PI_TIME_RELATIVE updates seconds and micros with the
   number of seconds and microseconds since the library was initialised.

   EXAMPLE:

   ...
   int secs, mics;
   ...
   gpioTime(PI_TIME_RELATIVE, &secs, &mics);
   printf("library started %d.%03d seconds ago\n", secs, mics/1000);
   ...
   prints the number of seconds since the library was started.

*/



/*-------------------------------------------------------------------------*/
int gpioSleep(unsigned timetype, int   seconds, int   micros);
/*-------------------------------------------------------------------------*/
/* Sleeps for the number of seconds and microseconds specified by seconds
   and micros.
  
   If timetype is PI_TIME_ABSOLUTE the sleep ends when the number of seconds
   and microseconds since the epoch (1st January 1970) has elapsed.  System
   clock changes are taken into account.

   If timetype is PI_TIME_RELATIVE the sleep is for the specified number
   of seconds and microseconds.  System clock changes do not effect the
   sleep length.

   NOTES:

   For short relative sleeps (say, 200 microseonds) I use usleep.
   It's less to type and just as reliable.

   EXAMPLE:

   ...
   gpioSleep(PI_TIME_RELATIVE, 2, 500000); // sleep for 2.5 seconds
   ...
   gpioSleep(PI_TIME_RELATIVE, 0, 100000); // sleep for 1/10th of a second
   ...
   gpioSleep(PI_TIME_RELATIVE, 60, 0);     // sleep for one minute
   ...
*/



/*-------------------------------------------------------------------------*/
uint32_t gpioTick(void);
/*-------------------------------------------------------------------------*/
/* Returns the current system tick.

   Tick is the number of microseconds since system boot.

   NOTES:

   As tick is an unsigned 32 bit quantity it wraps around after
   2^32 microseconds, which is approximately 1 hour 12 minutes.

   You don't need to worry about the wrap around as long as you
   take a tick (uint32_t) from another tick, i.e. the following
   code will always provide the correct difference.

   EXAMPLE:

   uint32_t startTick, endTick;
   int diffTick;
   ...
   startTick = gpioTick();
   ...
   // do some processing
   ...
   endTick = gpioTick();

   diffTick = endTick - startTick;

   printf("some processing took %d microseconds\n", diffTick);
   ...
*/



/*-------------------------------------------------------------------------*/
unsigned gpioHardwareRevision(void);
/*-------------------------------------------------------------------------*/
/* Used to get the hardware revision.

   If the hardware revision can not be found or is not a valid hexadecimal
   number the function returns 0.

   NOTES:

   The hardware revision is the last 4 characters on the Revision line of
   /proc/cpuinfo.

   The revision number can be used to determine the assignment of gpios
   to pins.

   There are at least two types of board.

   Type 1 has gpio 0 on P1-3, gpio 1 on P1-5, and gpio 21 on P1-13.

   Type 2 has gpio 2 on P1-3, gpio 3 on P1-5, gpio 27 on P1-13, and
   gpios 28-31 on P5.

   Type 1 boards have hardware revision numbers of 2 and 3.

   Type 2 boards have hardware revision numbers of 4, 5, 6, and 15.

   EXAMPLES:

   for "Revision	: 0002" the function returns 2.
   for "Revision	: 000f" the function returns 15.
   for "Revision	: 000g" the function returns 0.
*/



/*-------------------------------------------------------------------------*/
void gpioInternals(unsigned what, int value);
/*-------------------------------------------------------------------------*/
/* Used to tune internal settings.  Not for general use.
*/



#ifdef __cplusplus
}
#endif

#endif
