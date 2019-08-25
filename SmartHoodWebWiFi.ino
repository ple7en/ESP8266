#include <FS.h>               //this needs to be first, or it all crashes and burns...
#define BLYNK_PRINT Serial//Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>      //https://github.com/bblanchon/ArduinoJson
#include <SPI.h>
#include <SimpleTimer.h>
#include <DHT.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>


bool shouldSaveConfig = false;//флаг для сохранения данных
char blynk_token[34] = "";

SimpleTimer timer;


#define RelePin 4
#define DHTPIN 5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WidgetLED LedIndicatorDHT(V4);//индикатор влажности, срабатывание зависит от переменной MaxHumidity
WidgetLED LedIndicatorRele(V3);//индикатор работы вытяжки

int CheckBoxRele; // включение/отключение вытяжки
int MaxHumidity = 80; // задаем начальный максимальный показетель влажности


void saveConfigCallback () {//обратный вызов, уведомляющий нас о необходимости сохранить конфигурацию
  Serial.println("Данные требуется сохранить, т.к. внесены изменения");//Следует сохранить конфигурацию
  shouldSaveConfig = true;//Меняем флаг shouldSaveConfig на true
}




void setup()
{

  Serial.begin(115200);
  Serial.println();

//чистая ФС, для тестирования
//SPIFFS.format();

  Serial.println("Монтируем файловую систему...");////читаем конфигурацию из FS json

  if (SPIFFS.begin()) {
    Serial.println("Файловая система смонтирована");
    if (SPIFFS.exists("/config.json")) {
  //файл существует, чтение и загрузка
      Serial.println("Чтение конфигурационного файла");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Конфигурационный файл открыт");
        size_t size = configFile.size();
    //Выделить буфер для хранения содержимого файла
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");          
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("Не удалось загрузить конфигурацию json");
        }
      }
    }
  } else {
    Serial.println("Неудалось смонтировать файловую систему");
  }
//конец чтения

//Дополнительные параметры для настройки (могут быть глобальными или просто в настройках)
//После подключения параметр.getValue () вернет вам настроенное значение
//идентификатор / имя заполнителя / длина подсказки по умолчанию

  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 33);//was 32 length
  
  

//WiFiManager
//Локальная инициализация. Как только его бизнес закончен, нет необходимости держать его вокруг
  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);//установить конфигурацию сохранить уведомить обратный вызов

//установить статический ip
//это для подключения к маршрутизатору Office, а не к GargoyleTest, но его можно изменить в режиме AP на 192.168.4.1
//wifiManager.setSTAStaticIPConfig(IPAddress(192,168,10,111), IPAddress (192,168,10,90), IPAddress (255,255,255,0));
  
  wifiManager.addParameter(&custom_blynk_token);//добавьте все свои параметры здесь

//проверяем, сохранились ли настройки токена от блинка, если нет, то очищаем также настройки подключения к wifi
  strcpy(blynk_token, custom_blynk_token.getValue());//прочтем обновленные параметры
  String S = blynk_token;
   
    if (S.length() == 0)
    {
      wifiManager.resetSettings();      
    }


//wifiManager.resetSettings ();//сброс настроек - для тестирования

//устанавливаем минимальное качество сигнала, чтобы он игнорировал точки доступа под этим качеством
//по умолчанию 8%
//wifiManager.setMinimumSignalQuality ();
//  wifiManager.setTimeout(600);//10 минут для ввода данных, а затем ESP сбрасывает, чтобы повторить попытку.

//выбирает ssid и pass и пытается подключиться, если он не подключается, запускает точку доступа с указанным именем и переходит в цикл блокировки в ожидании конфигурации
      
    if (!wifiManager.autoConnect("SmartHood", "12345678")) {//Задайте здесь параметры точки доступа (SSID, password)
    Serial.println("Не удалось подключиться и истекло время ожидания");
    delay(3000);//перезагрузите и попробуйте снова, или, возможно, положить его в глубокий сон
    ESP.reset();
    delay(5000);
   }

  Serial.println("Подключение... :)");//if you get here you have connected to the WiFi

  strcpy(blynk_token, custom_blynk_token.getValue());//прочтем обновленные параметры
  
//сохранить пользовательские параметры в FS
  if (shouldSaveConfig) {  //прочтем обновленные параметры
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Не удалось открыть файл конфигурации для записи");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
//конец сохранения
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  
  if (WiFi.status() == WL_CONNECTED) {

  Blynk.config(blynk_token);
  Blynk.connect();
  Serial.println("соединение с WiFi-сетью успешно установлено"); 
  Serial.print("Подключение к Blynk ");
  Serial.println(Blynk.connected());
  }
  else
  {
    Serial.println("Нет соединения с WiFi"); 
  }
    
  
  
pinMode (RelePin, OUTPUT);//Указываем, что реле на выходе
dht.begin();
digitalWrite(RelePin, 1);  



}

BLYNK_WRITE(V5)
{
  CheckBoxRele = param.asInt();
}


//считываем максимальный показатель влажности с регулятора slider из моб.приложения
  BLYNK_WRITE(V2)
  {
    if (MaxHumidity != param.asInt()) {
    MaxHumidity = param.asInt();  
    Serial.print("Значение регулятора = ");
    Serial.println(MaxHumidity);
    }   
  }

  void loop()
{
  float h = dht.readHumidity();//передаем показатели влажности переменной
  float t = dht.readTemperature();//передаем показатели температуры переменной

  if (isnan(h) || isnan(t)) 
  {
    Serial.println("Невозможно считать датчик DHT");
    return;
  }

  Blynk.virtualWrite(V6, h);
  Blynk.virtualWrite(V7, t);//погрешность температуры (в корпусе нагрев идет от источника питания, если dht близко к корпусу)
  Blynk.virtualWrite(V3, digitalRead(RelePin));
  Serial.print("Влажность   ");
  Serial.println(h);
  Serial.print("Tемпература   ");
  Serial.println(t);
  Serial.print("CheckBoxRele    ");
  Serial.println(CheckBoxRele);
  Serial.print("RelePin   ");
  Serial.println(digitalRead(RelePin));
  
  

  if (CheckBoxRele == 0)//включить принудительно?
  {
    LedIndicatorRele.off();//отключаем индикатор 
    
    if(h > MaxHumidity)
    {  LedIndicatorDHT.on();//включаем индикатор 
       LedIndicatorRele.on();//включаем индикатор  
       digitalWrite(RelePin, LOW);//замыкаем реле (включаем вытяжку)     
       Serial.println("Вытяжка включена");
    }
    else
    {  LedIndicatorDHT.off();//отключаем индикатор 
       LedIndicatorRele.off();//отключаем индикатор 
       digitalWrite(RelePin, HIGH);//размыкаем реле (отключаем вытяжку)      
       Serial.println("Вытяжка отключена");
    }
    
  }
  else
  {
    digitalWrite(RelePin, 0);
    LedIndicatorRele.on();//включаем индикатор  
  }
  
  
  Blynk.run();//Initiates Blynk
  timer.run();//Initiates SimpleTimer  
}
