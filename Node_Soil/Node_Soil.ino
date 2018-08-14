#include <Wire.h>
#include <SPI.h>   // not used here, but needed to prevent a RTClib compile error
#include <SD.h>
#include <avr/sleep.h>
#include <RTClib.h>
#include <Adafruit_BME280.h>

String Node_ID = "902";
int interval_min_record = 2;
int interval_min_test = 1;

//////////////////////////////////////////////////////////
///////////                                    ///////////
///////////              I2C PINs              ///////////
///////////                                    ///////////
//////////////////////////////////////////////////////////
RTC_DS3231 RTC;
bool hasRTC = false;

int INTERRUPT_PIN = 2;
volatile int state = LOW;
bool WakeUp = false;

Adafruit_BME280 BME280;
bool hasBME = false;

//////////////////////////////////////////////////////////
///////////                                    ///////////
///////////            SD CARD PINs            ///////////
///////////                                    ///////////
//////////////////////////////////////////////////////////
//MISO  - PIN_11   MOSI - PIN_12
//SCK   - PIN_13    CS  - PIN_10
File myFile;
char file_name[] = "SoilLogs.txt";
int PIN_CHIPSELECT = 10;
bool hasSD_Card = false;

void setup (){
  Serial.begin(9600);
  pinMode(INTERRUPT_PIN, INPUT);
  //pull up the interrupt pin
  digitalWrite(INTERRUPT_PIN, HIGH);

  CheckModules();

  String logs = createDataLog();
  Serial.println(logs);
  writeSDCardFile(file_name, logs);
  
  DateTime now = RTC.now();
  int al_min = now.minute() + interval_min_test;
  int al_hr = now.hour() + (al_min / 60);
  al_min %= 60;
  al_hr %= 24;
  
  RTC.setAlarm1Simple(al_hr, al_min);
  Serial.println("\nAlarm set at " + String(al_hr) + ":" + String(al_min));
  RTC.turnOnAlarm(1);
  if (RTC.checkAlarmEnabled(1)){
    Serial.println("Alarms Enabled");
  }
  attachInterrupt(0, logData, LOW);
}

void loop (){
  DateTime now = RTC.now();
  if (RTC.checkIfAlarm(1)){
    Serial.println("Alarm Triggered");
  }

  if(WakeUp){
    String logs = createDataLog();
    Serial.println(logs);
    writeSDCardFile(file_name, logs);
    
    int al_min = now.minute() + interval_min_record;
    int al_hr = now.hour() + (al_min / 60);
    al_min %= 60;
    al_hr %= 24;
    
    RTC.setAlarm1Simple(al_hr, al_min);
    Serial.println("Alarm set at " + String(al_hr) + ":" + String(al_min));
    WakeUp = false;
  }
  
  Serial.print("Now: ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
  Serial.println("Going to Sleep\n");
  delay(600);
  sleepNow();
  Serial.println("AWAKE");
}

void sleepNow(){
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0,logData, LOW);
  sleep_mode();
  //HERE AFTER WAKING UP
  sleep_disable();
  detachInterrupt(0);
}

void logData(){
  //do something quick, flip a flag, and handle in loop();
  WakeUp = true;
}

void writeSDCardFile(char file_name[], String data){
  myFile = SD.open(file_name, FILE_WRITE);
  if(myFile){
    Serial.print("WRITING TO ");
    Serial.println(file_name);
    myFile.println(data);
    myFile.close();
    Serial.println("DONE.");
  } else {
    Serial.print("CAN'T FIND ");
    Serial.println(file_name);
  }
}

int counter = 0;

String createDataLog(){
  DateTime now = RTC.now();
  String _count_ = String(++counter);
  String _id_ = Node_ID;
  String _date_ = String(now.day()) + "-" + String(now.month()) + "-" + String(now.year());
  String _time_ = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  String _temp_ = String(BME280.readTemperature());
  String _humi_ = String(BME280.readHumidity());
  
  return _count_ + "\t" + _id_ + "\t" + _date_ + " " + _time_ + "   \t" + _temp_ + "\t" + _humi_;
}

void CheckModules(){
  Serial.println("Check Modules");

  Wire.begin();
  //Real Time Clock
  RTC.begin();
  if(RTC.now().day() == 165){
    Serial.println("RTC is NOT running!");
    hasRTC = false;
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }else{
    Serial.println("Found RealTime Clock");
    hasRTC = true;
  }
  //BME Sensor
  if(!BME280.begin()){
    Serial.println("No BME?");
    hasBME = false;
  }else{
    Serial.println("Found BME");
    hasBME = true;
  }
  //SD Card
  pinMode(SS, OUTPUT);
  if (!SD.begin(PIN_CHIPSELECT)){
    Serial.println("No SD Card?");
    hasSD_Card = false;
  }else{
    Serial.println("Found SD Card");
    hasSD_Card = true;
  }
}
