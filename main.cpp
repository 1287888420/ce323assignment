#include "mbed.h"
#include "TextLCD.h"
#include "BusOut.h"
#include "BusIn.h"

#define STATE_UNSET     0
#define STATE_EXIT      1
#define STATE_SET       2
#define STATE_ENTRY     3
#define STATE_ALARM     4
#define STATE_REPORT    5

#define CODE_LEN 4 // length of user's code
#define MAX_ATTEMPTS 3 // maximum attempts for entering code

int state =  STATE_UNSET;

int index = 0;
char inputChar;
char errorCode[16];
const char* Key_stored = "1234";
char Key_entered[CODE_LEN];

char Keytable[] = { 'F', 'E', 'D', 'C',   
                    '3', '6', '9', 'B',   
                    '2', '5', '8', '0',   
                    '1', '4', '7', 'A'    
                   };


Timeout flipper;
TextLCD lcd(p15, p16, p17, p18, p19, p20); 
SPI sw(p5, p6, p7);
DigitalOut cs(p8);
DigitalOut alarmLED(LED1);
DigitalOut countdownLED(LED3);

BusOut rows(p26,p25,p24);
BusIn cols(p14,p13,p12,p11);

void LEDOff();
void LEDBlink();
void LEDOn();
void setKey();
void initialize();
bool checkKey();
char getKey();
int readSwitch();
int readSwitch_exitEntry();
int readSwitch_fullSet();

void unset();
void exit();
void set();
void entry();
void alarm();
void report();

Timer timer;
int attempts = 0;
const float exit_period = 60.0;
const float entry_period = 120.0;
const float alarm_period = 120.0;

int main() {
    while (1) {
        switch (state) {
            case STATE_UNSET :
                unset();
                break;
            case STATE_EXIT :
                exit();
                break;
            case STATE_SET :
                set();
                break;
            case STATE_ENTRY :
                entry();
                break;
            case STATE_ALARM :
                alarm();
                break;
            case STATE_REPORT :
                report();
                break;
        }
    }
}

void unset() {
    LEDOff();
    initialize();
    attempts = 0;
    while (1) {
        lcd.locate(0, 0);
        lcd.printf("STATE:UNSET");
        setKey();
        inputChar = getKey();
        wait_ms(100);
        if (index ==  CODE_LEN && inputChar == 'B') {
            if (checkKey()) {
                state = STATE_EXIT;
                break;
            }
            else {
                attempts = attempts + 1;
                initialize();
            }
            if (attempts == MAX_ATTEMPTS) {
                strcpy(errorCode, "WRONG KEY");
                state = STATE_ALARM;
                break;
            }
        }
    }
}

void exit() {
    LEDBlink();
    initialize();
    attempts = 0;
    int switches_fullSet;
    timer.start();
    while (1) {
        lcd.locate(0, 0);
        lcd.printf("STATE:EXIT");
        if (timer.read() >= exit_period) {
            state = STATE_SET;
            break;
        }

        switches_fullSet = readSwitch_fullSet();
        if (switches_fullSet != 0) {
            float pos = log((float)switches_fullSet) / log(2.0);
            sprintf(errorCode, "FULLSET %d", (int)pos);
            state = STATE_ALARM;
            break;
        }

        setKey();
        inputChar = getKey();
        wait_ms(100);
        if (index ==  CODE_LEN && inputChar == 'B') {
            if (checkKey()) {
                state = STATE_UNSET;
                break;
            }
            else {
                attempts = attempts + 1;
                initialize();
            }
            if (attempts == MAX_ATTEMPTS) {
                strcpy(errorCode, "WRONG KEY");
                state = STATE_ALARM;
            }
        }
    }
}

void set() {
    LEDOff();
    initialize();
    attempts = 0;
    int switches_fullSet;
    int switch_exitEntry;
    while(1) {
        lcd.locate(0, 0);
        lcd.printf("STATE:EXIT");
        switches_fullSet = readSwitch_fullSet();
        switch_exitEntry = readSwitch_exitEntry();
        if (switches_fullSet != 0) {
            float pos = log((float)switches_fullSet) / log(2.0);
            sprintf(errorCode, "FULLSET %d", (int)pos);
            state = STATE_ALARM;
            break;
        }

        if (switch_exitEntry != 0) {
            state = STATE_ENTRY;
            break;
        }
    }
}

void entry() {
    LEDBlink();
    initialize();
    attempts = 0;
    int switches_fullSet;
    timer.start();
    while(1) {
        lcd.locate(0, 0);
        lcd.printf("STATE:ENTRY");
        if (timer.read() >= entry_period) {
            strcpy(errorCode, "EXIT/ENTRY SET");
            state = STATE_ALARM;
            break;
        }

        switches_fullSet = readSwitch_fullSet();
        if (switches_fullSet != 0) {
            float pos = log((float)switches_fullSet) / log(2.0);
            sprintf(errorCode, "FULLSET %d", (int)pos);
            state = STATE_ALARM;
            break;
        }

        setKey();
        inputChar = getKey();
        wait_ms(100);
        if (index ==  CODE_LEN && inputChar == 'B') {
            if (checkKey()) {
                state = STATE_UNSET;
                break;
            }
            else {
                initialize();
            }
        }
    }
}

void alarm() {
    LEDOn();
    initialize();
    attempts = 0;
    timer.start();
    while(1) {
        lcd.locate(0, 0);
        lcd.printf("STATE:ALARM");
        if (timer.read() >= alarm_period) {
            LEDOff();
        }

        setKey();
        inputChar = getKey();
        wait_ms(100);
        if (index ==  CODE_LEN && inputChar == 'B') {
            if (checkKey()) {
                state = STATE_REPORT;
                break;
            }
            else {
                initialize();
            }
        }
    }
}

void report() {
    LEDOff();
    initialize();
    attempts = 0;
    lcd.locate(0, 0);
    lcd.printf(errorCode);
    lcd.locate(0, 1);
    lcd.printf("C key to clear");
    while (1) {
        setKey();
        inputChar = getKey();
        wait_ms(100);
        if (inputChar == 'C') {
            memset(errorCode, 0, sizeof(errorCode));
            state = STATE_UNSET;
        }
    }
    
}

char getKey() {
    int i, j;
    char ch=' ';
    
    for (i = 0; i <= 3; i++) {
        rows = i; 
        for (j = 0; j <= 3; j++) {           
            if (((cols ^ 0x00FF)  & (0x0001<<j)) != 0) {
                ch = Keytable[(i * 4) + j];
            }            
        }        
    }
    return ch;
}

void setKey() {
    lcd.locate(0, 1);
    switch (index) {
        case 0:
            lcd.printf("Code:____");
            break;
        case 1:
            lcd.printf("Code:*___");
            break;
        case 2:
            lcd.printf("Code:**__");
            break;
        case 3:
            lcd.printf("Code:***_");
            break;
        case 4:
            lcd.printf("Press B to set");
            break;
    }
    
    inputChar = getKey();
    wait_ms(100);
    if (index < CODE_LEN) {
        switch (inputChar) {
        case 'A':
        case 'B':
        case 'D':
        case 'E':
        case 'F':
        case ' ':
            break;
        case 'C':
            if (index > 0) {
                lcd.locate(5 + index, 1);
                Key_entered[index] = '\0';
                index = index - 1;
            }
            break;
        default:
            lcd.locate(6 + index, 1);
            Key_entered[index] = inputChar;
            index = index + 1;
        }
    }
}

bool checkKey() {
    if (strcmp(Key_stored, Key_entered) == 0) {
        memset(Key_entered, 0, sizeof(Key_entered));
        return true;
    }
    else {
        return false;   
    }
}

void initialize() {
    memset(Key_entered, 0, sizeof(Key_entered));
    index = 0;
    timer.reset();
    lcd.cls();
}

int readSwitch() {
    rows = 4;
    int switches = cols;
    rows = 5;
    switches = switches * 16 + cols;
    return switches;
}

int readSwitch_exitEntry() {
    int switches = readSwitch();
    int switch_exitEntry = switches % 10;
    return switch_exitEntry;
}

int readSwitch_fullSet() {
    int switches = readSwitch();
    int switches_exitEntry = readSwitch_exitEntry();
    int switches_fullSet = switches - switches_exitEntry;
    return switches_fullSet;
}

void flip() {
    alarmLED = !alarmLED;
}

void LEDOff() {
    alarmLED = 0;
}

void LEDBlink() {
    alarmLED = 1;
    flipper.attach(&flip, 0.5);
}

void LEDOn() {
    alarmLED = 1;
}
