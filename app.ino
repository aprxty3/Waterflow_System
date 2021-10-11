#define BLYNK_PRINT Serial

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
//#include <SimpleTimer.h>
#include <LiquidCrystal_I2C.h>


// pin nodemcu
#define PIN_PUMP      0   //pin 0 pada nodemcupin 3
#define PIN_LED       2   //pin 2 pada nodemcupin 4
#define PIN_SENSOR    D5
#define PIN_POT       A0


// pin / variabel di blynk
#define BLYNK_POT     V0
#define BLYNK_LCD     V1
#define BLYNK_FLOW    V2
 

char auth[] = "81QthQDDwXAIeY5wIKRYs1PsRQfpZdXi"; 	  //ISI TOKEN PADA APLIKASI BLYNX ANDROID
char ssid[] = "POCONG";                             //NAMA HOTSPOT 
char pass[] = "12345678";                           //PASSWORD HOTSPOT


LiquidCrystal_I2C LCD(0x27,16,2);  //lcd board

SimpleTimer timer;
WidgetLCD lcd(BLYNK_LCD); //lcd android


const float calibrationFactor = 6.5;

long int interval = 1000;
boolean ledState = LOW;
volatile byte pulseCount = 0;
float flowRate = 0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;

volatile int hz = 0;


void IRAM_ATTR pulseCounter(){
    pulseCount++;
}

// proses kalkulasi setiap 1 detik (akurat)
void ICACHE_RAM_ATTR Calculate(){

    // catat hz
    hz = pulseCount;

    // segera reset counter
    pulseCount = 0;
    
    // hitung flowrate (Hz) -> L/min
    flowRate = hz / calibrationFactor;
    
    // konversi ke mL/s (60 detik)
    flowMilliLitres = (flowRate*1000)/60;

    // akumulasi
    totalMilliLitres += flowMilliLitres;
}


void sendSensor(){

    Serial.print(hz);
    Serial.print(" Hz, ");
    Serial.print(" -> ");
    Serial.print(flowRate);
    Serial.print(" L/m -> ");
    Serial.print(flowMilliLitres);
    Serial.print(" mL/s -> ");
    Serial.print(totalMilliLitres);
    Serial.print(" mL");
    Serial.println();

  
    int POT = analogRead(PIN_POT); 
    
    Serial.print(POT);
    Serial.print(" ");

    // tampilkan Hz di BLYNK_POT
    Blynk.virtualWrite(BLYNK_POT, hz);
    
    // tampilkan flow akumulasi
    Blynk.virtualWrite(BLYNK_FLOW, totalMilliLitres);

    //lcd to android && LCD to board lcd 
    
    lcd.print(0,0,"KEADAAN");         LCD.setCursor(0,0); LCD.print("ADC"); LCD.setCursor(4,0); LCD.print(POT); LCD.print(" ");
    lcd.print(0,1,"PUMP");            LCD.setCursor(0,1); LCD.print("PUMP");
    
    if (POT>500){
        Serial.println("KERING");
        lcd.print(8,0,"KERING");      LCD.setCursor(9,0); LCD.print("KERING");
        lcd.print(5,1,"ON ");         LCD.setCursor(5,1); LCD.print("ON ");
    
        digitalWrite(PIN_PUMP,LOW);
        for(int x=0; x<=10; x++){
            lcd.print(9,1,x);         LCD.setCursor(9,1); LCD.print(x);
            delay(500);
        }
        
        lcd.clear();                  LCD.clear();
        
        digitalWrite(PIN_PUMP,HIGH);
        lcd.print(0,0,"AIR MERESAP"); LCD.setCursor(0,0); LCD.print("AIR MERESAP");
        lcd.print(0,1,"    WAIT");    LCD.setCursor(0,1); LCD.print("    WAIT");
        
        for(int x=9; x>0; x--){
            lcd.print(9,1,x);         LCD.setCursor(9,1); LCD.print(x);
            delay(500);
        }
        
        lcd.clear();                  LCD.clear();
    }
  
    else if (POT>400&&POT<500){
        Serial.println("LEMBAB");
        lcd.print(8,0,"LEMBAB");      LCD.setCursor(9,0); LCD.print("LEMBAB");
        lcd.print(5,1,"OFF");         LCD.setCursor(5,1); LCD.print("OFF");
        digitalWrite(PIN_PUMP,HIGH);
    }
  
    else if (POT<400){
        Serial.println("BASAH");
        lcd.print(8,0,"BASAH ");      LCD.setCursor(9,0); LCD.print("BASAH ");
        lcd.print(5,1,"OFF");         LCD.setCursor(5,1); LCD.print("OFF");
        digitalWrite(PIN_PUMP,HIGH);
    }
}



void setup(){
    Serial.begin(9600);
    Blynk.begin(auth, ssid, pass);
    timer.setInterval(interval, sendSensor);

    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_SENSOR, INPUT_PULLUP);
    
    lcd.clear();                        LCD.init(); LCD.backlight();

    // interrupt p
    attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), pulseCounter, FALLING);

    /* aktifkan interrupt 1hz untuk menghitung flow sensor setiap 1 detik */
    timer1_attachInterrupt(Calculate); // method yang dieksekusi
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // timer berulang
    timer1_write(5000000L); // sistem kerja 5MHz atau 5jt Hz, jadi untuk eksekusi 1x / detik -> (5jt/1 = 5jt)
}

void loop(){
    Blynk.run();
    timer.run();
}
