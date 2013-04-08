#!/usr/bin/python
import sys
import os
import time

max_pwm = 249
min_pwm = 0

panStepSize = 2 # degree of change for each pan update
tiltStepSize = -2 # degree of change for each tilt update

servoPanPosition = 224 # initial pan position
servoTiltPosition = 160 # initial tilt position

panGpioPin = 6  # servoblaster pin 2 : gpio pin 18
tiltGpioPin = 7  # servoblaster pin 5 : gpio pin 23

def moveServo(servo, angle):
    move(servo, angle)
    time.sleep(0.5)
    move(servo, 0)
    time.sleep(0.5)

def move(servo, angle):
    '''Moves the specified servo to the supplied angle.

    Arguments:
        servo
          the servo number to command, an integer from 0-7
        angle
          the desired pulse width for servoblaster, an integer from 0 to 249
    '''

    command = 'echo %s=%s > /dev/servoblaster' % (str(servo), str(angle))
    os.system(command)

#    if (min_pwm <= angle <= max_pwm):
#        command = 'echo %s=%s > /dev/servoblaster' % (str(servo), str(angle))
#        os.system(command)
#        #print command
#    else:
#        print "Servo angle must be an integer between %s and %s.\n" % (str(min_pwm), str(max_pwm))

if __name__ == '__main__':
    print(sys.argv[1])
    if (sys.argv[1] == "1"):
      print(sys.argv[2])
      moveServo(panGpioPin, int(sys.argv[2]))

    elif (sys.argv[1] == "2"):
      moveServo(tiltGpioPin, int(sys.argv[2]))
