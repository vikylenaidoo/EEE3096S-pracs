/*IMPORTS*/

#include "greenhouse.h"
#include "RTC.h"

/*GLOBAL VARIABLES*/
//unsigned char *lightData[10];
//unsigned char *temperatureData[10];
//unsigned char *humidityData[10];
int lightData, temperatureData, humidityData;

                // {s,m,h}
int systemTime[] = {0,0,0}; //holds S:M:H of the time the system has been running
int rtcTime[3]; //stores the RTC time values after reading. {s,m,h}
int timeOfStart[3] = {0,0,0};
int timeOfLastAlarm[3] = {0,0,0};

int RTC; //Holds the RTC instance

long lastInterruptTime=0; //usedfor debouncing

bool alarmEnable = 1; //true when alarm is allowed to be active. will only be enabled 3min after the last sound
bool alarmActive = 0; //true when alarm is sounding
bool monitorActive = 1;
int displayFrequency = 1;



/*---------------------------------------------------------------------------------------*/
/*MAIN IMPLEMENTATION*/
/*---------------------------------------------------------------------------------------*/

int main(void){

    //setup and initilialisation
    wiringPiSetup();
    init_GPIO();
    thread thread_ADC = init_ADC();
    
    sleep(1);
    btnHandler_Reset();

    while(true){
        while(monitorActive){
            std::this_thread::sleep_for(std::chrono::milliseconds(displayFrequency*1000)); //monitor frequncy
            
            float humidity = calculateVoltage(humidityData, 3.3);
            //temp: Vout = Tc*Ta + V0 = 10m*Ta+0.5 ==> Ta = (Vout-V0)/Tc
            float temp = (calculateVoltage(temperatureData, 3.3)-0.5)/10; //TODO: check is correct
            //calculate DAC voltage
            float vout = (lightData/1023)*humidity;

            //display readings 
            cout<<"RTC Time = "<<rtcTime[2]<<":"<<rtcTime[1]<<":"<<rtcTime[0]<<"\t";
            cout<<"System Time = "<<systemTime[2]<<":"<<systemTime[1]<<":"<<systemTime[0]<<"\t";

            alarmActive = 0; //switch alarm off
            if(vout<0.65 || vout>2.65){
                if(alarmEnable){ // 3mins have passed since last alarm
                    alarmActive = true;
                    timeOfLastAlarm[0] = rtcTime[0];
                    timeOfLastAlarm[1] = rtcTime[1];
                    timeOfLastAlarm[2] = rtcTime[2];
                }
            }

            updateAlarmOutput();

        }

    }





    thread_ADC.join();
    return 0; 
}











/*---------------------------------------------------------------------------------------*/
/*FUNCTION IMPLEMENTATIONS*/
/*---------------------------------------------------------------------------------------*/

/*setup btn inputs and alarm output*/
void init_GPIO(void){
    cout<< "init_GPIO Started \n";

    //setup button inputs
    for(int i=0; i< sizeof(BTNS)/sizeof(BTNS[0]); i++){
        pinMode(BTNS[i], INPUT);
        pullUpDnControl(BTNS[i], PUD_UP);
    }

    //assign interrupts
    wiringPiISR(BTNS[0], INT_EDGE_FALLING, btnHandler_Reset);
    wiringPiISR(BTNS[1], INT_EDGE_FALLING, btnHandler_StartStop);
    wiringPiISR(BTNS[2], INT_EDGE_FALLING, btnHandler_ChangeInterval);
    wiringPiISR(BTNS[3], INT_EDGE_FALLING, btnHandler_DismissAlarm);


    //setup alarm output
    pinMode(ALARM, OUTPUT);
    digitalWrite(ALARM, 0);

    cout<< "init_GPIO Done \n\n";

}

void init_RTC(){

    RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
    //set initial System time to 00:00:00
    


}


thread init_ADC(void){
   // wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED);
    
    wiringPiSPISetup(0, 500000);
    mcp3004Setup(ADC_BASE, SPI_CHANNEL_ADC);
    std::thread thread_ADC(readADC2); // run read_ADC() on a new thread

    return thread_ADC;
    cout<<"init_ADC done\n";
}

void readADC2(void){
    while(true){
        lightData=analogRead(ADC_BASE);
        temperatureData=analogRead(ADC_BASE+1);
        humidityData=analogRead(ADC_BASE+2);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

}

void writeTo_DAC(float vout){
    unsigned char outputData[16];
    //set control bits
    char control = 0b01110000;
    //convert vout to 10bit binary value
    int data = (int)((vout/3.3)*1023);
    
    wiringPiSPIDataRW(SPI_CHANNEL_DAC, outputData, 2);
}

/* must run on a seperate thread
 * constantly updates the data arrays based on the adc values
 * updates occur evry 100ms
 */
/*
void read_ADC(void){
    unsigned char spiData[16]; //to hold data in/out from the spi comm
    cout<<"thread running";
    
    while(true){
        //cout<<"printloop\n";
        //read light sensor
        spiData[0] = 1;
        spiData[1] = 0;
        spiData[2] = 0;
        spiData[3] = 0;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2);
        for(int i=0; i<10; i++){
            *lightData[i] = spiData[i+6];
        }

        //read temp
        spiData[0] = 1;
        spiData[1] = 0;
        spiData[2] = 0;
        spiData[3] = 1;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2);
        for(int i=0; i<10; i++){
            *temperatureData[i] = spiData[i+6];
        }


        //read humidty
        spiData[0] = 1;
        spiData[1] = 0;
        spiData[2] = 1;
        spiData[3] = 0;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2);
        for(int i=0; i<10; i++){
            *humidityData[i] = spiData[i+6];
        }


        //pause for 100ms
        //usleep(100000);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}*/

void updateDisplayFrequency(void){
    switch (displayFrequency){
        case 1:
            displayFrequency = 2;
            break;
        case 2:
            displayFrequency = 5;
            break;
        case 5:
            displayFrequency = 1;
            break;
        default:
            displayFrequency = 1;
            break;
    }
    
}

/*update the time from the RTC */
void updateRealTime(){ 
	rtcTime[0] =  wiringPiI2CReadReg8(RTC, RTCSEC);
	rtcTime[1] =  wiringPiI2CReadReg8(RTC, RTCMIN);
	rtcTime[2] =  wiringPiI2CReadReg8(RTC, RTCHOUR);
}

/*update the system time based on the rtcTime and timeOfStart*/
void updateSystemTime(void){
    systemTime[0] = rtcTime[0]-timeOfStart[0];
    systemTime[1] = rtcTime[1]-timeOfStart[1];
    systemTime[2] = rtcTime[2]-timeOfStart[2];
}

/*since the data from adc is value between 0-1023 we need to calulate voltage
* refernec is the value at 1023
*/
float calculateVoltage(int data, float reference){
    return (reference/1023)*data;
}

void updateAlarmOutput(void){
    if(alarmActive){
        digitalWrite(ALARM, HIGH);
    }
}













/*---------------------------------------------------------------------------------------*/
/*INTERRUPT HANDLERS*/
/*---------------------------------------------------------------------------------------*/
void btnHandler_Reset(void){
    long now = millis();
    
    if(now-lastInterruptTime>200){
        updateRealTime();
        timeOfStart[0] = rtcTime[0];
        timeOfStart[1] = rtcTime[1];
        timeOfStart[2] = rtcTime[2];
        updateSystemTime();
        system("clear");
        cout<<"system reset\n";
    
    
    
    }
    lastInterruptTime = now;
}

void btnHandler_StartStop(void){
    long now = millis();

    if(now-lastInterruptTime>200){
        monitorActive = !monitorActive;    
    
    }
    lastInterruptTime = now;
    

}

void btnHandler_ChangeInterval(void){
    long now = millis();
    
    if(now-lastInterruptTime>200){
        updateDisplayFrequency();
        cout<<"\n display frequency changed to: "<< displayFrequency<< " Hz\n"; 
    
    
    }
    lastInterruptTime = now;
    
}

void btnHandler_DismissAlarm(void){
    long now = millis();

    if(now-lastInterruptTime>200){
        alarmActive = false;
        updateRealTime();
        //total seconds since last alarm
        int alarmSecs = rtcTime[0] - timeOfLastAlarm[0]; //seconds diference
        alarmSecs = alarmSecs + rtcTime[1] - timeOfLastAlarm[1]; //mins
        alarmSecs = alarmSecs + rtcTime[2] - timeOfLastAlarm[2]; //hours

        alarmEnable = false;
        if(alarmSecs>180){ //more than 3mins has passed
            alarmEnable = true;
        } 
        
    }
    lastInterruptTime = now;

}