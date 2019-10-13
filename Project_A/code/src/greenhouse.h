/*IMPORTS*/
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <iomanip>
#include <math.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>
#include <mcp3004.h>


using namespace std;



/*GLOBAL CONSTANTS*/
#define SPI_CHANNEL_ADC 0
#define SPI_CHANNEL_DAC 1
#define SPI_SPEED 500000
#define ADC_BASE 100

const int BTNS[] = {2, 3, 4, 5}; //{RESET, START_STOP, FREQUENCY, DISMISS_ALARM}
const int ALARM = 7; //PWM output on this pin













/*FUNCTION DECLARATIONS*/

//initialisation
void init_GPIO(void);
void init_RTC(void);
thread init_ADC(void);

//RTC Functions
//void fetchTime(int* secs, int* mins, int* hours); //fetch the time from the RTC and store int the given variables
void updateSystemTime(void);
void updateRealTime(void);

//ADC functions
void readADC2(void);
void read_ADC(void);

//program functions
void updateDisplayFrequency(void);
float calculateVoltage(int data, float ref);
void updateAlarmOutput();

//program end functions
void cleanupGPIO(void);
void terminateHandler(int sig_num);

/*INTERRUPT HANDLERS*/
void btnHandler_Reset(void);
void btnHandler_StartStop(void);
void btnHandler_ChangeInterval(void);
void btnHandler_DismissAlarm(void);