#ifndef BINCLOCK_H
#define BINCLOCK_H

//Some reading (if you want)
//https://stackoverflow.com/questions/1674032/static-const-vs-define-vs-enum

// Function definitions
int hFormat(int hours);
void lightHours(int units);
void lightMins(int units);
int hexCompensation(int units);
int decCompensation(int units);
void initGPIO(void);
void secPWM(int units);
void hourInc(void);
void minInc(void);
void toggleTime(void);
bool *decToBin();
void updateTime(void);

// define constants
const char RTCAddr = 0x6f; //hardware address of slave
const char RTCSEC = 0x00; // see register table in datasheet
const char RTCMIN = 0x01;
const char RTCHOUR = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)

// define pins
const int LEDS_HOURS[] = {0,2,3,25};
const int LEDS_MINS[] = {7,22,21,27,4,6}; //H0-H4, M0-M5
const int SECS = 1;
const int BTNS[] = {5,30}; // B0, B1


#endif
