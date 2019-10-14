/*IMPORTS*/

#include "greenhouse.h"
//#include "RTC.h"
#include "CurrentTime.c"

//#define BLYNK_DEBUG
#define BLYNK_PRINT stdout
#ifdef RASPBERRY
  #include <BlynkApiWiringPi.h>
#else
  #include <BlynkApiLinux.h>
#endif
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>
 
static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);
 
static const char *auth, *serv;
static uint16_t port;
 
#include <BlynkWidgets.h>
#define BLYNK_PRINT Serial



/*GLOBAL VARIABLES*/
//unsigned char *lightData[10];
//unsigned char *temperatureData[10];
//unsigned char *humidityData[10];
int lightData, temperatureData, humidityData;
WidgetLED LEDAlarm(V0);
WidgetTerminal terminal(V4);

                // {s,m,h}
unsigned int systemTime[3];// = {0,0,0}; //holds S:M:H of the time the system has been running
int rtcTime[3]; //stores the RTC time values after reading. {s,m,h}
unsigned int timeOfStart[3];// = {0,0,0};
unsigned int timeOfLastAlarm[3] = {0,0,0};

int RTC; //Holds the RTC instance

long lastInterruptTime=0; //usedfor debouncing

bool alarmEnable = 1; //true when alarm is allowed to be active. will only be enabled 3min after the last sound
bool alarmActive = 0; //true when alarm is sounding
bool monitorActive = 1;
int displayFrequency = 1;



/*---------------------------------------------------------------------------------------*/
/*MAIN IMPLEMENTATION*/
/*---------------------------------------------------------------------------------------*/

int main(int argc, char* argv[]){
    
    //setup and initilialisation
    wiringPiSetupGpio();
    init_GPIO();
    //init_RTC();
    thread thread_ADC = init_ADC();
    signal(SIGINT, terminateHandler);

    parse_options(argc, argv, auth, serv, port);
    Blynk.begin(auth, serv, port);
    //Blynk.connect();
    //Blynk.connectWiFi(TP-LINK, pass);
    Blynk.run();

    sleep(5);
    
    alarmEnable=1;
    alarmActive=0;
    monitorActive=1;
    int prevSec = rtcTime[0];
    updateRealTime();
    btnHandler_Reset();

   
    while(true){
        cout<<"";
        while(monitorActive){
            float humidity;
            float temp;
            float vout;
            int light = lightData;

            updateRealTime();
            updateSystemTime();            
            //std::this_thread::sleep_for(std::chrono::milliseconds(displayFrequency*1000)); //monitor frequncy
            while(1){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                humidity = calculateVoltage(humidityData, 3.3);
                //temp: Vout = Tc*Ta + V0 = 10m*Ta+0.5 ==> Ta = (Vout-V0)/Tc
                temp = (calculateVoltage(temperatureData, 3.3)-0.5)*100; //TODO: check is correct
                //calculate DAC voltage
                vout = (lightData/1023.0)*humidity;
                writeTo_DAC(vout);

                updateRealTime();
                updateSystemTime();

                
                    
                if(prevSec!=rtcTime[0]){
                    if((rtcTime[0]%displayFrequency)==0){
                        
                        break;
                    }
                    
                }
            }
            
            
            prevSec=rtcTime[0];
            updateRealTime();
            updateSystemTime();


            //display readings 
            //cout<<setw(2)<<setprecision(3);
            cout<<"\t"<<setw(2)<<setfill('0')<<rtcTime[2]<<":"<<setw(2)<<setfill('0')<<rtcTime[1]<<":"<<setw(2)<<setfill('0')<<rtcTime[0]<<"\t";
            cout<<setw(2)<<setfill('0')<<systemTime[2]<<":"<<setw(2)<<setfill('0')<<systemTime[1]<<":"<<setw(2)<<setfill('0')<<systemTime[0]<<"\t";
            cout<<setprecision(3)<< humidity<<"\t\t";
            cout<<setprecision(3)<<temp<<"\t\t";
            cout<<light<<"\t";
            cout<<setprecision(2)<<vout<<"\t";

            Blynk.virtualWrite(V1, humidity);
            Blynk.virtualWrite(V2, temp);
            Blynk.virtualWrite(V3, light);
            Blynk.virtualWrite(V5, vout);
            
            Blynk.virtualWrite(V4, "RTC Time = ");
            Blynk.virtualWrite(V4, rtcTime[2]);
            Blynk.virtualWrite(V4, ":");
            Blynk.virtualWrite(V4, rtcTime[1]);
            Blynk.virtualWrite(V4, ":");
            Blynk.virtualWrite(V4, rtcTime[0]);
            Blynk.virtualWrite(V4, "\t\t\t\t\t");

            Blynk.virtualWrite(V4, "System Time = ");
            Blynk.virtualWrite(V4, systemTime[2]);
            Blynk.virtualWrite(V4, ":");
            Blynk.virtualWrite(V4, systemTime[1]);
            Blynk.virtualWrite(V4, ":");
            Blynk.virtualWrite(V4, systemTime[0]);
            Blynk.virtualWrite(V4, "\n");

            //alarmActive = 0; //switch alarm off
            if((vout<0.65 || vout>2.65)){
                
                if(alarmEnable){ // 3mins have passed since last alarm
                    alarmActive = true;
                    //cout<<"Alarm Active\t";
                    timeOfLastAlarm[0] = rtcTime[0];
                    timeOfLastAlarm[1] = rtcTime[1];
                    timeOfLastAlarm[2] = rtcTime[2];
                    alarmEnable=0;
                }
            }
            //updateRealTime();
            
            if(!alarmEnable){
                updateRealTime();
                //total seconds since last alarm
                int alarmSecs = rtcTime[0] - timeOfLastAlarm[0]; //seconds diference
                alarmSecs = alarmSecs + 60*(rtcTime[1] - timeOfLastAlarm[1]); //mins
                alarmSecs = alarmSecs + 3600*(rtcTime[2] - timeOfLastAlarm[2]); //hours
                //cout<<"alarmsecs"<<alarmSecs<<"\n";
                //alarmEnable = false;
                if(alarmSecs>180){ //more than 3mins has passed
                    alarmEnable = true;
                    //cout<<"\nalarm enabled\n";
                }    

            }

            
            
            //writeTo_DAC(vout);
            updateAlarmOutput();
            Blynk.run();
            cout <<endl;
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


    LEDAlarm.off();
    
    cout<< "init_GPIO Done \n\n";

}

void init_RTC(){

    //RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
    //set initial System time to 00:00:00
    


}


thread init_ADC(void){
   // wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED);
    
    wiringPiSPISetup(SPI_CHANNEL_ADC, 500000);
    wiringPiSPISetup(SPI_CHANNEL_DAC, 500000);
    mcp3004Setup(ADC_BASE, SPI_CHANNEL_ADC);
    std::thread thread_ADC(readADC2); // run read_ADC() on a new thread
    cout<<"init_ADC done, Thread started\n";
    return thread_ADC;
    
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
    unsigned char outputData[2];
    //set control bits
    char control = 0b01110000;
    
    //convert vout to 10bit binary value
    uint16_t data = (uint16_t)((vout/3.3)*1023);
    outputData[0] = control; //fisrt 8 bits
    outputData[0] |= (data)>>6;
    outputData[1] = (data)<<2;

    //cout<<"\ndata="<<data;

    //printf("\n vout = %x ",outputData[0]);
    //printf("%x \n",outputData[1]);
    
    
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
	rtcTime[0] =  getSecs();//wiringPiI2CReadReg8(RTC, RTCSEC);
	rtcTime[1] =  getMins();//wiringPiI2CReadReg8(RTC, RTCMIN);
	rtcTime[2] =  getHours();//wiringPiI2CReadReg8(RTC, RTCHOUR);
}

/*update the system time based on the rtcTime and timeOfStart*/
void updateSystemTime(void){
    int second1 = rtcTime[0];
    int minute1 = rtcTime[1];
    int hour1 = rtcTime[2];
    int second2 = timeOfStart[0];
    int minute2 = timeOfStart[1];
    int hour2 = timeOfStart[2];
    
    if(second2 > second1) {
      minute1--;
      second1 += 60;
    }
   
   systemTime[0] = second1 - second2;
   
   if(minute2 > minute1) {
      hour1--;
      minute1 += 60;
   }
   systemTime[1] = minute1 - minute2;
   systemTime[2] = hour1 - hour2;




}

/*since the data from adc is value between 0-1023 we need to calulate voltage
* refernec is the value at 1023
*/
float calculateVoltage(int data, float reference){
    return (reference/1023)*data;
}

void updateAlarmOutput(void){
    //Blynk.run();
    if(alarmActive){
        digitalWrite(ALARM, HIGH);
        cout<<"*";
        LEDAlarm.on();
    }
    else
    {
        digitalWrite(ALARM, LOW);
        LEDAlarm.off();
    }
    
}



/* cleanup the gpio pins by setting them all back to input with a LOW value*/
void cleanupGPIO(void){
	
    pinMode(ALARM, INPUT);
    pullUpDnControl(ALARM, PUD_DOWN);

	printf("exiting gracefully\n");

	exit(0);

}

void terminateHandler(int sig_num){
	if(sig_num==SIGINT){
		cleanupGPIO();
	}
	printf("error");
	exit(0);
}








/*---------------------------------------------------------------------------------------*/
/*INTERRUPT HANDLERS*/
/*---------------------------------------------------------------------------------------*/
void btnHandler_Reset(void){
    long now = millis();
    
    if(now-lastInterruptTime>200){
        //sleep(1);

        updateRealTime();
        timeOfStart[0] = rtcTime[0];
        timeOfStart[1] = rtcTime[1];
        timeOfStart[2] = rtcTime[2];
        systemTime[0] = 0;
        systemTime[1] = 0;
        systemTime[2] = 0;
        updateSystemTime();
        alarmEnable = 1;
        alarmActive = 0;
        system("clear");
        cout<<"*system reset*\n";
        cout<<"\n\tRTC Time\tSystem Time\tHumidity\tTemporature\tLight\tDAC Out\tAlarm";
        cout <<endl;
        Blynk.virtualWrite(V4, "clr");
        Blynk.virtualWrite(V4, "*system reset*\n");
    
    
    
    }
    lastInterruptTime = now;
}

void btnHandler_StartStop(void){
    long now = millis();

    if(now-lastInterruptTime>200){
        monitorActive = !monitorActive;
        //cout<<"\n*monitor = "<<monitorActive<<"*\n";
        /*
        if(monitorActive){
            monitorActive = 0;
            cout<<"\n*stop*\n";  
        }
        else{
            monitorActive = 1;
            cout<<"\n*start*\n"; 
        }
        */
    }
    lastInterruptTime = now;
    

}

void btnHandler_ChangeInterval(void){
    long now = millis();
    
    if(now-lastInterruptTime>200){
        updateDisplayFrequency();
        cout<<"\n*display frequency changed to every "<< displayFrequency<< "seconds*\n"; 
    
    
    }
    lastInterruptTime = now;
    
}

void btnHandler_DismissAlarm(void){
    long now = millis();

    if(now-lastInterruptTime>200){
        alarmActive = false;
        cout<<"\n*Alarm dismissed*\n\n";
        Blynk.virtualWrite(V4, "\n*Alarm dismissed*\n");
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