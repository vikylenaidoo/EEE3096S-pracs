/*IMPORTS*/

#include "greenhouse.h"
#include "RTC.h"

/*GLOBAL VARIABLES*/
unsigned char lightData[10];
unsigned char temperatureData[10];
unsigned char humidityData[10];

unsigned char systemTime[] = {0,0,0}; //holds H:M:S

int RTC; //Holds the RTC instance
pthread_t thread_id; //the adc thread

bool alarmEnable; //true when alarm is allowed to be active. will only be enabled 3min after the last sound
bool alarmActive; //true when alarm is sounding
bool monitorActive;
int displayFrequency;




/*MAIN IMPLEMENTATION*/
int main(void){

    //setup and initilialisation
    wiringPiSetup();
    init_GPIO();
    init_ADC();
    
    while(true){
        while(monitorActive){


        }

    }






    return 0; 
}












/*FUNCTION IMPLEMENTATIONS*/

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


void init_ADC(void){
    wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED);
    
    std::thread thread_ADC(read_ADC); // run read_ADC() on a new thread
    /*
    pthread_attr_t tattr;
    
    int ret;
    //int newprio = 99;
    sched_param param;

    // initialized with default attributes
    ret = pthread_attr_init (&tattr);

    // safe to get existing scheduling param 
    ret = pthread_attr_getschedparam (&tattr, &param);

    // set the priority; others are unchanged 
    //param.sched_priority = newprio;

    // setting the new scheduling param 
    ret = pthread_attr_setschedparam (&tattr, &param);
    // with new priority specified 
    pthread_create (&thread_id, &tattr, read_ADC, (void*)1);
    */
        
}

/* must run on a seperate thread
 * constantly updates the data arrays based on the adc values
 * updates occur evry 100ms
 */
void *read_ADC(void *threadargs){
    unsigned char spiData[16]; //to hold data in/out from the spi comm
    
    
    while(true){
        //read light sensor
        spiData[0] = 1;
        spiData[1] = 0;
        spiData[2] = 0;
        spiData[3] = 0;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2);
        for(int i=0; i<10; i++){
            lightData[i] = spiData[i+6];
        }

        //read temp
        spiData[0] = 1;
        spiData[1] = 0;
        spiData[2] = 0;
        spiData[3] = 1;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2);
        for(int i=0; i<10; i++){
            temperatureData[i] = spiData[i+6];
        }


        //read humidty
        spiData[0] = 1;
        spiData[1] = 0;
        spiData[2] = 1;
        spiData[3] = 0;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2);
        for(int i=0; i<10; i++){
            humidityData[i] = spiData[i+6];
        }


        //pause for 100ms
        usleep(100000);
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

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


/*INTERRUPT HANDLERS*/
void btnHandler_Reset(void){


}

void btnHandler_StartStop(void){
    monitorActive = !monitorActive;

}

void btnHandler_ChangeInterval(void){
    updateDisplayFrequency;
    cout<<"\n display frequency changed to: ", displayFrequency, " Hz\n";
}

void btnHandler_DismissAlarm(void){


}