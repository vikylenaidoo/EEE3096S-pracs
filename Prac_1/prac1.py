#!/usr/bin/python3
"""
Python Practical 1

Names: Vikyle Naidoo
Student Number: NDXVIK005
Prac: 1
Date: 26/07/2019
"""

# import Relevant Librares
import RPi.GPIO as GPIO
import time

#global vars
global counter
counter=0
global chan_in
chan_in=[16,18]
global chan_out
chan_out=[11,13,15]
time_stamp = time.time()

#define callbacks
def display():
    out = "{:03b}".format(counter)
    #disp=[0,0,0]
    for i in range(0,3):
        GPIO.output(chan_out[i], int(out[i]))
        print(out[i])

def countup(channel):
    print("sw0 pressed")
#    time.sleep(1)
    global time_stamp
    time_now = time.time()
    if (time_now-time_stamp)>0.3:
        global counter
        if  counter==7:
            counter=0
        else:
            counter=counter+1
        display()
    time_stamp = time_now

def countdown(channel):
    print("sw1 pressed")
 #   time.sleep(1)
    global time_stamp
    time_now = time.time()
    if (time_now-time_stamp)>0.3:
        global counter
        if counter==0:
            counter=7
        else:
            counter=counter-1
        display()
    time_stamp=time_now



# Logic that you write
def setupGPIO():
    GPIO.setmode(GPIO.BOARD)
#    chan_in = [16,18]
#    chan_out =[11,13,15]
    GPIO.setup(chan_in, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(chan_out, GPIO.OUT, initial=GPIO.LOW)

    GPIO.add_event_detect(chan_in[1], GPIO.RISING, callback=countup, bouncetime=100)
    GPIO.add_event_detect(chan_in[0], GPIO.RISING, callback=countdown, bouncetime=100)

    GPIO.add_event_callback(chan_in[1], countup)
    GPIO.add_event_callback(chan_in[0], countdown)

    print("setup complete")






# Only run the functions if
if __name__ == "__main__":
    # Make sure the GPIO is stopped correctly
    try:
        setupGPIO()
        print("starting")
        while True:
            time.sleep(100)

    except KeyboardInterrupt:
        print("Exiting gracefully")
        # Turn off your GPIOs here
        GPIO.cleanup()
    except Exception as e:
        GPIO.cleanup()
        print("Some other error occurred")
        print(e.message)
