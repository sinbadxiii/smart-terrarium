#include "DHT.h"
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

#define pinUV A1
#define PinRelayDayLamp 8
#define PinRelayUVLamp 10
#define PinRelayNightLamp 12

#define PinRelayFans 13

#define LOWER_NIGHT_TEMPERATURE 24 
#define HIGHEST_NIGHT_TEMPERATURE 28 

RTC_DS3231 rtc;
DHT dhtDay(2, DHT22); 
DHT dhtMiddle(4, DHT22);
DHT dhtNight(6, DHT22);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

float uvIndex;
int uvIndexValue [12] = {50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170};

struct Time {
  int Hour;
  int Minute;  
};

bool ForceDayLamp   = false;
bool ForceUVLamp    = false;
bool ForceNightLamp = false;

Time turtleTimeFrom = {};
Time turtleTimeTo   = {};
Time fansTime       = {};

LiquidCrystal_I2C lcd(0x27,20,4); 

void setup() {
  Serial.begin(9600);

  pinMode(PinRelayDayLamp, OUTPUT);
  digitalWrite(PinRelayDayLamp, LOW);

  pinMode(PinRelayUVLamp, OUTPUT);
  digitalWrite(PinRelayUVLamp, LOW);

  pinMode(PinRelayNightLamp, OUTPUT);
  digitalWrite(PinRelayNightLamp, LOW);

  pinMode(PinRelayFans, OUTPUT);
  digitalWrite(PinRelayFans, LOW);

  dhtDay.begin();
  dhtMiddle.begin();
  dhtNight.begin();

  Wire.begin();
   
  turtleTimeFrom = {7, 00};
  turtleTimeTo   = {20, 00};
  fansTime       = {19, 30};

  
 if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //  rtc.adjust(DateTime(2020, 11, 7, 20, 29, 40));
  }

  lcd.init();                     
  lcd.backlight();
// lcd.noBacklight();
  analogWrite(pinUV, 0);
  
}
void loop() {
  temperature();
  time();  
  uvi();
  lamps();  
  delay(1000);
}

bool diffTime(int from, int To)
{
  DateTime now = rtc.now(); 

  int currentHour = now.hour();
  int currentMinute = now.minute();

  int currentDifferent = currentHour * 60 + currentMinute;
  
  if (currentDifferent >= from && currentDifferent <= To) {
    return true;
  } else {
    return false;
  }   
}

void time()
{
  DateTime now = rtc.now();
  char dateBuffer[20];
  sprintf(dateBuffer,"%02u:%02u:%02u ", now.hour(),now.minute(),now.second());
  lcd.setCursor(0, 0);
  lcd.print(dateBuffer);
  
}

void uvi()
{
  float indexUV = getUVIndex();
  lcd.setCursor(15, 0);
  if (indexUV > 0.1) {
    String indexString = String(indexUV);
    char uvi[5];
    sprintf(uvi,"%s", indexString.c_str());
    lcd.print(uvi);
  } else {
    lcd.setCursor(15, 0);
    lcd.print("*UV* ");  
  }

}

void temperature()
{
  float tDay = dhtDay.readTemperature(); 
  float tMiddle = dhtMiddle.readTemperature(); 
  float tNight = dhtNight.readTemperature(); 

  float middleHumidity = dhtMiddle.readHumidity();

 if (isnan(tDay)) {
  tDay = 0.0; 
 }
 if (isnan(tMiddle)) {
  tMiddle = 0.0; 
  middleHumidity = 0;
 }
 if (isnan(tNight)) {
  tNight = 0.0; 
 }
 
  lcd.setCursor(0, 2);

  String temperatureBuffer;
  char tempBuffer[3];

  temperatureBuffer += dtostrf(tDay,3,1,tempBuffer);
  temperatureBuffer += (char)223;

  temperatureBuffer += dtostrf(tMiddle,3,1,tempBuffer);
  temperatureBuffer += (char)223;

  temperatureBuffer += dtostrf(tNight,3,1,tempBuffer);
  temperatureBuffer += (char)223;
  temperatureBuffer += " ";
  temperatureBuffer += dtostrf(middleHumidity,2,0,tempBuffer);
  temperatureBuffer += "%";
  
  lcd.print(temperatureBuffer);
  
  if (tNight < LOWER_NIGHT_TEMPERATURE || ForceNightLamp) {
     onNightLamp();
  }

  if (tNight >= LOWER_NIGHT_TEMPERATURE && !ForceNightLamp) {
     offNightLamp();
  }

  DateTime now = rtc.now(); 

  int currentHour = now.hour();
  int currentMinute = now.minute();

  if (tNight > HIGHEST_NIGHT_TEMPERATURE) {
    offNightLamp();
  }  

  if ((currentHour == fansTime.Hour && currentMinute == fansTime.Minute) || tNight > HIGHEST_NIGHT_TEMPERATURE) {
   fans();
  } else {
   offFans();    
  }
}

void fans()
{
  digitalWrite(PinRelayFans, HIGH);
  lcd.setCursor(13, 3);  
  lcd.print("Fans");
}

void offFans()
{
  digitalWrite(PinRelayFans, LOW);
  lcd.setCursor(13, 3);  
  lcd.print("    ");
}

void lamps()
{
  int fromDifferent =  turtleTimeFrom.Hour * 60 + turtleTimeFrom.Minute;
  int toDifferent   =  turtleTimeTo.Hour * 60 + turtleTimeTo.Minute;
  
  dayLamp(fromDifferent, toDifferent);
  uvLamp(fromDifferent, toDifferent);
}

void dayLamp(int fromDifferent, int toDifferent)
{  
  if (diffTime(fromDifferent, toDifferent) || ForceDayLamp) {
    digitalWrite(PinRelayDayLamp, HIGH);
    lcd.setCursor(0, 3);  
    lcd.print("Day");
  } else {
    digitalWrite(PinRelayDayLamp, LOW);
    lcd.setCursor(0, 3);  
    lcd.print("   ");
  }
}

void uvLamp(int fromDifferent, int toDifferent)
{  
  if (diffTime(fromDifferent, toDifferent) || ForceUVLamp) {
    digitalWrite(PinRelayUVLamp, HIGH);
    lcd.setCursor(4, 3);  
    lcd.print("UV");
  } else {
    digitalWrite(PinRelayUVLamp, LOW);
    lcd.setCursor(4, 3);  
    lcd.print("  ");
  }
}

void onNightLamp()
{   
  digitalWrite(PinRelayNightLamp, HIGH);
   lcd.setCursor(7, 3);  
   lcd.print("Night");
}

void offNightLamp()
{
   digitalWrite(PinRelayNightLamp, LOW);
   lcd.setCursor(7, 3);  
   lcd.print("     ");
}

float getUVIndex()
{
  int uvAnalog = analogRead(pinUV);
  Serial.println(uvAnalog);
  float uvNapeti = uvAnalog * (5000.0 / 1023.0);
  if (uvNapeti > 1170) {
    uvNapeti = 1170;
  }
  int i;
  for (i = 0; i < 12; i++)
  {  
    if (uvNapeti <= uvIndexValue[i]) 
    {
      uvIndex = i;
      break;
    }
  }
  if (i>0) {   
    float vRange=uvIndexValue[i]-uvIndexValue[i-1];
    float vCalc=uvNapeti-uvIndexValue[i-1]; 
    uvIndex+=(1.0/vRange)*vCalc-1.0;
  } 

  return uvIndex;
}
