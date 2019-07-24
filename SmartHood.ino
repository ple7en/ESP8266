#include <SPI.h>
#include <SimpleTimer.h>
#include <DHT.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>


int CheckBoxRele; // включение/отключение вытяжки
int MaxHumidity = 80; // задаем начальный максимальный показетель влажности

//--------------------------------------------------------
//считываем максимальный показатель влажности с регулятора slider из моб.приложения
BLYNK_WRITE(V2)
{
  if (MaxHumidity != param.asInt()) {
  MaxHumidity = param.asInt();  
    Serial.print("Значение регулятора = ");
    Serial.println(MaxHumidity);
  }   
}
//--------------------------------------------------------


char auth[] = ""; //токен Blynk
char ssid[] = "TP-LINK_2g"; //название wifi
char pass[] = ""; //пароль

#define RelePin 4
#define DHTPIN 5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WidgetLED LedIndicatorDHT(V4); //индикатор влажности, срабатывание зависит от переменной MaxHumidity
WidgetLED LedIndicatorRele(V3); //индикатор работы вытяжки

void setup()
{
  Serial.begin(9600);// открываем порт для консоли+
  Blynk.begin(auth, ssid, pass); // стартуем Blynk
  pinMode (RelePin, OUTPUT); // Указываем, что реле на выходе
  dht.begin();
  digitalWrite(RelePin, 1);
}


BLYNK_WRITE(V5)
{
  CheckBoxRele = param.asInt();
}


void loop()
{
  float h = dht.readHumidity(); //передаем показатели влажности переменной
  float t = dht.readTemperature(); //передаем показатели температуры переменной

  if (isnan(h) || isnan(t)) 
  {
    Serial.println("Невозможно считать датчик DHT");
    return;
  }

  Blynk.virtualWrite(V6, h);
  Blynk.virtualWrite(V7, t);  // погрешность температуры (в корпусе нагрев идет от источника питания, если dht близко к корпусу)
  Blynk.virtualWrite(V3, digitalRead(RelePin));
  Serial.print("Влажность   ");
  Serial.println(h);
  Serial.print("Tемпература   ");
  Serial.println(t);
  Serial.print("CheckBoxRele    ");
  Serial.println(CheckBoxRele);
  Serial.print("RelePin   ");
  Serial.println(digitalRead(RelePin));
  
  

  if (CheckBoxRele == 0) //включить принудительно?
  {
    LedIndicatorRele.off(); //отключаем индикатор 
    
    if(h > MaxHumidity)
    {  LedIndicatorDHT.on(); //включаем индикатор 
       LedIndicatorRele.on(); //включаем индикатор  
       digitalWrite(RelePin, LOW); //замыкаем реле (включаем вытяжку)     
       Serial.println("Вытяжка включена");
    }
    else
    {  LedIndicatorDHT.off(); //отключаем индикатор 
       LedIndicatorRele.off(); //отключаем индикатор 
       digitalWrite(RelePin, HIGH); //размыкаем реле (отключаем вытяжку)      
       Serial.println("Вытяжка отключена");
    }
    
  }
  else
  {
    digitalWrite(RelePin, 0);
    LedIndicatorRele.on(); //включаем индикатор  
  }
  

  Blynk.run();
 
}