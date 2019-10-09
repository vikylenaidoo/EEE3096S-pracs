/*IMPORTS*/
#include <stdio.h>
#include <iostream>
#include <thread>
//#include <chrono>
#include <unistd.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>


using namespace std;



/*GLOBAL CONSTANTS*/
#define SPI_CHANNEL 0
#define SPI_SPEED 500000

const int BTNS[] = {26, 27, 28, 29};
const int ALARM = 7; //PWM output on this pin













/*FUNCTION DECLARATIONS*/

//initialisation
void init_GPIO(void);
void init_RTC(void);
void init_ADC(void);

//RTC Functions
void fetchTime(int* secs, int* mins, int* hours); //fetch the time from the RTC and store int the given variables

//ADC functions
void *read_ADC(void *threadargs);

//program functions
void updateDisplayFrequency(void);

//program end functions
void cleanupGPIO(void);
void terminateHandler(int sig_num);

/*INTERRUPT HANDLERS*/
void btnHandler_Reset(void);
void btnHandler_StartStop(void);
void btnHandler_ChangeInterval(void);
void btnHandler_DismissAlarm(void);