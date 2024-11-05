
/*  /////////////// пояснения по прошивке 1.10////////////////////
  // использование встроенного OTA update
  // зайди на адрес x.x.x.x/ota_update для открытия страницы обновления
  // Скетч/Экспорт бинарного файла (для получения файла прошивки)
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  // х.х.х.х адрес еспшки в сети, его ты указываешь ниже.
  // он будет что то типа staticIP(192, 168, 3, 201); но вместо числа 3 надо указать свое
  // нажимаем Win + R
  // пишем cmd жмем Enter
  // открывается черное окно, пишем ipconfig жмем Enter
  // появляется куча информации о сети, надо там найти:
  // "IPv4 адрес. . . . . . . . . . : 192.168.10.ххх "
  // вместо ххх у вас будет айпишник вашего компьютера \ ноутбука
  // вместо третьего числа "10" будет ваша подсеть ( может быть 1, может 100, какое угодно число)
  // это число подсети нужно указать ниже в настройках, вместо десятки.
  // так же важно указать в настройках все остальные наборы чисел как у вас тут вылезло.
  // не закрывайте это окно, а спуститесь ниже по скетчу, там где пять строк с IPAddress
  // и измените там все как у вас в этом черном окне
*/

//#define STATIC_IP // закомментировать если подключаетесь к мобильной точке доступа на телефоне

const char* ssid = "VideoWiFi";
const char* password = "01082011";

#ifdef STATIC_IP
//со статическим айпишничком
IPAddress staticIP(192, 168, 0, 150);  // важно правильно указать третье число - подсеть, смотри пояснения выше
IPAddress gateway(192, 168, 0, 1);    // и тут изменить тройку на свою подсеть
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 0, 1);  // изменить тройку на свою подсеть
IPAddress dns2(8, 8, 8, 8);
#endif

// настройка pin's
#define RELE1 33
#define RELE2 25
#define RELE3 26
#define RELE4 27
#define ON 0
#define OFF 1
// настройка аналог pin
#define SENS1 36


#include <WiFi.h>              // espESP8266  ESP8266WiFi.h
//#include <GyverNTP.h>
//GyverNTP ntp(3);
//Datime dt(NTP);
#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS);     // для проверки файлов

#include <RTClib.h>      // Часы реального времени
RTC_DS3231 rtc;
char daysOfTheWeek[7][23] = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"};

  


#include <SPI.h> // для I2C


//Для датчика HTU21
#include <GyverHTU21D.h>
GyverHTU21D htu;

//============Переменные для графика Ajax===========
const char *plot_1[] = {
  "temp", "humidity", "humiditySoil"
};

//настройки, хранятся в памяти EEPROM
struct Settings {
  //  char str[15] = "ello kitty!";
  GPtime startTime;             // таймер света
  GPtime stopTime;              // таймер света
  GPtime startTime2;            // таймер полива
  GPtime stopTime2;             // таймер полива
  int16_t minTempr = 0;
  int16_t maxTempr = 0;
  int16_t minHumi = 0;
  int16_t maxHumi = 0;
  int16_t minHumiSoil = 0;
  int16_t maxHumiSoil = 0;
  bool dependByLight;           // свитч Свеи on/off
  bool dependByHeating;         // свитч Нагрев on/off
  bool dependByHumidity;        // свитч Влажность on/off
  bool dependByWatering;        // свитч Полив on/off
  bool dependByOnOff;           // переключатель "Полива по времени или по датчику"
  bool rele_1_isOn = 0;
  bool rele_2_isOn = 0;
  bool rele_3_isOn = 0;
  bool rele_4_isOn = 0;
  
  };

Settings setting;

#include <EEPROM.h>
#include <EEManager.h>          // подключаем либу
EEManager memory(setting);      // передаём нашу переменную (фактически её адрес)


GPdate nowDate;
GPtime nowTime;
uint32_t startSeconds = 0;
uint32_t stopSeconds = 0;
bool dependByTime1 = 1;         // флаг разрешения включения полив по времени
bool dependByLight = 1;
bool dependByHeating = 1;
bool dependByHumidity = 1;
bool dependByWatering = 1;
bool dependByOnOff = 1;
uint32_t startSeconds1 = 0;
uint32_t stopSeconds1 = 0;
float temperature = 0.0;
float humidity = 0.0;
float humiditySoil = 0.0;



//функци для датчика температуры htu21D
void htuRead() {
  htu.readTick();                      //Запускаем датчик
  temperature = htu.getTemperature();  //переменная для температуры
  humidity = htu.getHumidity();        // переменная для влажности
 //Serial.print("Температура = ");
 // Serial.print(temperature);
 // Serial.println(" °C");
 // Serial.print("Влажность = ");
 // Serial.print(humidity);
 // Serial.println(" %"); 
}

// функця для датчика влажности почвы
void sensRead() {
  humiditySoil = analogRead(SENS1);
 // Serial.print("Влажность почвы = ");
 // Serial.print(humiditySoil);
 // Serial.println(" %");
}

// функция дня недели



// поддержка wifi связи
void wifiSupport() {
  if (WiFi.status() != WL_CONNECTED) {
    // Подключаемся к Wi-Fi
    Serial.print("try conn to ");
    Serial.print(ssid);
    Serial.print(":");
    WiFi.mode(WIFI_STA);
#ifdef STATIC_IP
    if (WiFi.config(staticIP, gateway, subnet, dns1, dns2) == false) {
      Serial.println("wifi config failed.");
      return;
    }
#endif
    WiFi.begin(ssid, password);
    uint8_t trycon = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if (trycon++ < 30) delay(500);
      else {
        Serial.print("no connection to Wifi. Esp restarts NOW!");
        delay(1000);
        ESP.restart();
      }
    }
    Serial.println("Connected. \nIP: ");
    Serial.println(WiFi.localIP());    // Выводим IP ESP32
  }
}  //wifiSupport()


/*
//проверка NTP связи
void checkNTPstauts() {
  //проверим статус обновления ntp
  byte ntpErr = ntp.status();
  if (ntpErr) {
    Serial.print("ntp err ");
  }
  */
  /* Код ошибок NTP
    // 0 - всё ок
    // 1 - не запущен UDP
    // 2 - не подключен WiFi
    // 3 - ошибка подключения к серверу
    // 4 - ошибка отправки пакета
    // 5 - таймаут ответа сервера
    // 6 - получен некорректный ответ сервера
  */
 /*
  if (!ntp.synced()) Serial.println("NTP not sync");
  Serial.println(ntpErr);

}  //checkNTPstauts()
*/

// конструктор WEB страницы
void build() {
  GP.BUILD_BEGIN(400);
  GP.THEME(GP_DARK);
  GP.PAGE_TITLE("Rosti-Shishka");
  GP.ONLINE_CHECK();                   // Проверка системы на On-Line

  //все обновляющиеся параметры на WEB странице надо указать тут
  GP.UPDATE("nowDate,nowTime,nowDay,startTime2,stopTime2,startTime,stopTime,tempr,humid,humidsoil,releIndikator1,releIndikator_1_1,releIndikator2,releIndikator3,releIndikator_3_3,releIndikator4,releIndikator_4_4,releIndikator_2_2,sw_light,sw_1,sw_2,sw_3,sw,dayWeek,daysWeek");
  
  GP_MAKE_BLOCK_TAB(
    "Рости-Шишка",
    GP_MAKE_BOX(GP.DATE("nowDate", nowDate, false); GP.TIME("nowTime", nowTime, false); );
    GP_MAKE_BOX(GP.NAV_TABS("Домой,Свет,Нагрев,Влажность,Полив"); ); // Верхнее меню блоков навигации
  );  
  GP.LABEL("daysWeek" , "dayWeek") ;



  GP.NAV_BLOCK_BEGIN();                 // начало блока
  //                    ===========Блок индикации реле============
  
  // ====================================== блок 0 "Домой"=================================
  GP.BLOCK_BEGIN(GP_TAB, "100%", "Параметры системы");
  GP.LABEL("Освещение:");
  GP.LED_RED("releIndikator1", setting.rele_1_isOn);  
  GP.LABEL("Нагрев:");
  GP.LED_RED("releIndikator2", setting.rele_2_isOn);  
  GP.BREAK();
  GP.LABEL("Влажность:");
  GP.LED_RED("releIndikator3", setting.rele_3_isOn);  
  GP.LABEL("Полив :");
  GP.LED_RED("releIndikator4", setting.rele_4_isOn); 
  GP.BREAK();
  GP.HR();
  GP.LABEL("Графики"); 
  GP.BREAK();
  GP.AJAX_PLOT_DARK("plot", plot_1, 3, 20, 3000);   // Конструктор графика AJAX_PLOT
  GP.BLOCK_END();
  GP.NAV_BLOCK_END();  // закончен весь блок

  // ================================= Блок 1 "Свет" =================================
  GP.NAV_BLOCK_BEGIN();                  // начало блока
  GP_MAKE_BLOCK_TAB( 
    "Настройка света",
    GP_MAKE_BOX(GP.LABEL("Вкл.  в: "); GP.TIME("startTime", setting.startTime);            );
    GP_MAKE_BOX(GP.LABEL("Выкл. в: "); GP.TIME("stopTime", setting.stopTime);              );
    GP_MAKE_BOX(GP.LABEL("Реле 1:"); GP.LED_RED("releIndikator_1_1", setting.rele_1_isOn); );
    GP_MAKE_BOX(GP.SWITCH("sw_light", setting.dependByLight); GP.LABEL("On/Off");          );
  );
  GP.NAV_BLOCK_END();

  //========================= Блок 2 "Нагрев" ==========================
  GP.NAV_BLOCK_BEGIN();
  GP_MAKE_BLOCK_TAB(
    "Настройка подогрева",                              
    GP_MAKE_BOX(GP.LABEL("Текущая температура: "); GP.LABEL("tempr", "tempr"); GP.LABEL("*С");                                                             );
    GP_MAKE_BOX(GP.LABEL("Вкл.  при: "); GP.SPINNER("minTempr", setting.minTempr, 0, 80, 1, 1); GP.LABEL("*С");                                            );
    GP_MAKE_BOX(GP.LABEL("Выкл. при: "); GP.SPINNER("maxTempr", setting.maxTempr, 0, 80, 1, 1); GP.LABEL("*С");                                            );
    GP_MAKE_BOX(GP.LABEL("Реле 2:"); GP.LED_RED("releIndikator_2_2", setting.rele_2_isOn); GP.SWITCH("sw_1", setting.dependByHeating); GP.LABEL("On/Off"); );
  );
  GP.NAV_BLOCK_END();

  //============================= Блок 3 "Влажность"============================
  GP.NAV_BLOCK_BEGIN();
  GP_MAKE_BLOCK_TAB( 
    "Настройка увлажнителя воздуха",
    GP_MAKE_BOX(GP.LABEL("Текщая влажность: "); GP.LABEL("humid", "humid"); GP.LABEL("%");                                                                   );
    GP_MAKE_BOX(GP.LABEL("Вкл.  при: "); GP.SPINNER("minHumi", setting.minHumi, 0, 80, 1, 1); GP.LABEL("%");                                                 );
    GP_MAKE_BOX(GP.LABEL("Выкл. при: "); GP.SPINNER("maxHumi", setting.maxHumi, 0, 80, 1, 1); GP.LABEL("%");                                                 ); // Тут дописать автоматику
    GP_MAKE_BOX( GP.LABEL("Реле 3:"); GP.LED_RED("releIndikator_3_3", setting.rele_3_isOn); GP.SWITCH("sw_2", setting.dependByHumidity); GP.LABEL("On/Off"); );
   );    
  GP.NAV_BLOCK_END();

  //============================== Блок 4 "Полив"===============================
  GP.NAV_BLOCK_BEGIN();
  GP_MAKE_BLOCK_TAB( 
  "Настройка полива",
    GP_MAKE_BOX(GP.LABEL("Текущая влажность почвы: "); GP.LABEL("humidsoil", "humidsoil"); GP.LABEL("%");                                                   );
    GP_MAKE_BOX(GP.LABEL("Вкл  в: "); GP.TIME("startTime2", setting.startTime2);                                                                            );
    GP_MAKE_BOX(GP.LABEL("Выкл в: "); GP.TIME("stopTime2", setting.stopTime2);                                                                              );
    GP_MAKE_BOX(GP.LABEL("Либо по датчику "); GP.SWITCH("sw", setting.dependByOnOff);                                                                       );  // изменить на влажность
    GP_MAKE_BOX(GP.LABEL("Вкл  при: "); GP.SPINNER("minHumiSoil", setting.minHumiSoil, 10, 100, 5); GP.LABEL("%");                                          );
    GP_MAKE_BOX(GP.LABEL("Выкл при: "); GP.SPINNER("maxHumiSoil", setting.maxHumiSoil, 10, 100, 5); GP.LABEL("%");                                          );
    GP_MAKE_BOX(GP.LABEL("Реле 4:"); GP.LED_RED("releIndikator_4_4", setting.rele_4_isOn); GP.SWITCH("sw_3", setting.dependByWatering); GP.LABEL("On/Off"); );
   );
  GP.NAV_BLOCK_END();
  GP.BUILD_END();
}


void action() {
  // было обновление
  if (ui.update()) {
    ui.updateDate("nowDate", nowDate);
    ui.updateTime("nowTime", nowTime);
    ui.updateTime("startTime", setting.startTime);   // старт свет
    ui.updateTime("stopTime", setting.stopTime);     // стоп свет
    ui.updateTime("startTime2", setting.startTime2); // старт полив
    ui.updateTime("stopTime2", setting.stopTime2);   // сьлп полив
    ui.updateFloat("tempr", temperature, 1);
    ui.updateFloat("humid", humidity, 1);
    ui.updateFloat("humidsoil", humiditySoil);
    ui.updateBool("releIndikator1", setting.rele_1_isOn);
    ui.updateBool("releIndikator_1_1", setting.rele_1_isOn);
    ui.updateBool("releIndikator2", setting.rele_2_isOn);
    ui.updateBool("releIndikator_2_2", setting.rele_2_isOn);
    ui.updateBool("releIndikator3", setting.rele_3_isOn);
    ui.updateBool("releIndikator_3_3", setting.rele_3_isOn);
    ui.updateBool("releIndikator4", setting.rele_4_isOn);
    ui.updateBool("releIndikator_4_4", setting.rele_4_isOn);
    ui.updateBool("sw_light", setting.dependByLight); // Свитч свет
    ui.updateBool("sw_1", setting.dependByHeating);   // Свич нагрева
    ui.updateBool("sw_2", setting.dependByHumidity);  // Свич влажности
    ui.updateBool("sw_3", setting.dependByWatering);  // Свич полива
    ui.updateBool("sw", setting.dependByOnOff);       // Свитч 
  
  if (ui.update("plot")) {                                 // Обновление данных для графика
    int answ[] = {temperature, humidity, humiditySoil};    // Обновление данных для графика было int
    ui.answer(answ, 3);                                    // Обновление данных для графика
    }
  } //ui.update()
  // =====================был клик по компоненту внутри веб странички
  if (ui.click()) {
    if (ui.clickDate("nowDate", nowDate)) {
        rtc.adjust (DateTime(nowDate.year, nowDate.month, nowDate.day, nowTime.hour, nowTime.minute, nowTime.second)); // записываем Время (и Дату! отдельно нельзя!) в RTC
    
      //      Serial.print("Date: ");
      //      Serial.println(valDate.encode());
    }
    if (ui.clickTime("nowTime", nowTime)) {
        rtc.adjust (DateTime(nowDate.year, nowDate.month, nowDate.day, nowTime.hour, nowTime.minute, nowTime.second)); // записываем Время (и Дату! отдельно нельзя!) в RTC
      //      Serial.print("Time: ");
      //      Serial.println(valTime.encode());
    }
    //===============================================Меняем дату и время==================================
    // ======================обновление времени запуска и отключения света
    if (ui.clickTime("startTime", setting.startTime)) {
      startSeconds = setting.startTime.hour * 60 * 60 + setting.startTime.minute * 60 + setting.startTime.second;
      memory.updateNow();
    }
    if (ui.clickTime("stopTime", setting.stopTime)) {
      stopSeconds = setting.stopTime.hour * 60 * 60 + setting.stopTime.minute * 60 + setting.stopTime.second;
      memory.updateNow();
    }
    if (ui.clickBool("sw_light", setting.dependByLight)) {
      memory.updateNow();
    }

    // =====================================Обновление нагрева switch/min/max =========================
    if (ui.clickBool("sw_1", setting.dependByHeating)) {
      memory.updateNow();
    }
    if (ui.clickInt("maxTempr", setting.maxTempr)) {
      memory.updateNow();
    }
    if (ui.clickInt("minTempr", setting.minTempr)) {
      memory.updateNow();
    }
       //=================================Обновление влажности switch/min/max ==========================
    if (ui.clickBool("sw_2", setting.dependByHumidity)) {
      memory.updateNow();
    }
    if (ui.clickInt("minHumi", setting.minHumi)) {
      memory.updateNow();
    }
    if (ui.clickInt("maxHumi", setting.maxHumi)) {
      memory.updateNow();
    }   
      // ===============Обновление времени запуска реле Полив========================
    if (ui.clickTime("startTime2", setting.startTime2)) {
      startSeconds1 = setting.startTime2.hour * 60 * 60 + setting.startTime2.minute * 60 + setting.startTime2.second;
      memory.updateNow();
    }
    if (ui.clickTime("stopTime2", setting.stopTime2)) {
      stopSeconds1 = setting.stopTime2.hour * 60 * 60 + setting.stopTime2.minute * 60 + setting.stopTime2.second;
      memory.updateNow();
    }
    if (ui.clickBool("sw_3", setting.dependByWatering)) {
      memory.updateNow();
    }
    if (ui.clickBool("sw", setting.dependByOnOff)) {
      memory.updateNow();
    }
    if (ui.clickInt("minHumiSoil", setting.minHumiSoil)) {
      memory.updateNow();
    }
    if (ui.clickInt("maxHumiSoil", setting.maxHumiSoil)) {
      memory.updateNow();
    }
     // =================Если было нажание на кнопку 1 Включаем реле RELE1 Свет
  /* if (ui.click("sw_light")) {  
      setting.rele_1_isOn = !setting.rele_1_isOn;
      if (setting.rele_1_isOn) digitalWrite(RELE1, ON);
      else digitalWrite(RELE1, OFF);
      memory.updateNow();
  */
  }  //ui.click()
}  //action()

//========================================void setup ==========================================
void setup() {
  Serial.begin(115200);
  wifiSupport();
  htu.begin();
  rtc.begin();
   if (!htu.begin()) Serial.println(F("HTU21D error"));
   
   if (! rtc.begin()) Serial.println(F("Couldn't find RTC"));  // Проверка модуля реального времени
  
  // подключаем конструктор и запускаем
  ui.attachBuild(build);
  ui.attach(action);
  ui.start();
  ui.enableOTA();  // без пароля
  //ui.enableOTA("admin", "pass");  // с паролем

  if (!LittleFS.begin()) Serial.println("FS Error");
  ui.downloadAuto(true);

 // ntp.begin();         // запускаем часы
 // ntp.setGMT(3);       // часовой пояс. Для Москвы: 3
 // ntp.setPeriod(600);  // обновлять раз в 10 минут
  
  //bme280Init();

  EEPROM.begin(100);     // выделить память (больше или равно размеру структуры данных)
  memory.begin(0, 'e');  // изменить букву в скобках на другую, чтобы восстановить настройки по умолчанию

  pinMode(RELE1, OUTPUT); // определяем состояние пина //свет
  pinMode(RELE2, OUTPUT); // определяем состояние пина //нагрев
  pinMode(RELE3, OUTPUT); // определяем состояние пина //влажность
  pinMode(RELE4, OUTPUT); // определяем состояние пина //полив
  pinMode(SENS1, INPUT);  // определяем состояние пина //влажность почвы
  
  //==============Проверка состояния реле после подачи питания========================
  if (setting.rele_1_isOn) digitalWrite(RELE1, ON);
  else digitalWrite(RELE1, OFF);
  if (setting.rele_2_isOn) digitalWrite(RELE2, ON);
  else digitalWrite(RELE2, OFF);
  if (setting.rele_3_isOn) digitalWrite(RELE3, ON);
  else digitalWrite(RELE3, OFF);
  if (setting.rele_4_isOn) digitalWrite(RELE4, ON);
  else digitalWrite(RELE4, OFF);


}  //setup()

void loop() { 
  ui.tick();
  memory.tick();
  DateTime now = rtc.now();

  // раз в 10 сек проверим стабильность сети
  static uint32_t ms1 = 0;
  if (millis() - ms1 >= 10000) {
    ms1 = millis();
    wifiSupport();   // проверять соединение с сетью
   // checkNTPstauts();  // проверять соединение с сервером времени
  }  //ms

  // раз в 1 сек проверяем показание с датчика
  static uint32_t ms2 = 0;
  if (millis() - ms2 >= 1000) {
    ms2 = millis();
    htuRead();   // или bme280Read();
  }
  
  // раз в 2 сек проверяем показание с датчика sens1
  static uint32_t ms3 = 0;
  if (millis() - ms3 >= 2000) {
    ms3 = millis();
    sensRead();
  }
   // раз в 5 сек проверяем показание с датчика sens1
  static uint32_t ms4 = 0;
  if (millis() - ms4 >= 5000) {
   ms4 = millis();
    now.dayOfTheWeek();
    String dayWeek = (daysOfTheWeek[now.dayOfTheWeek() - 1 ]);
    Serial.print("День недели: ");
    Serial.println (dayWeek);
  }
  
  
  // отдаем текущую дату и время переменным в веб интерфейс
  //nowTime.set(ntp.hour(), ntp.minute(), ntp.second());
  //nowDate.set(ntp.year(), ntp.month(), ntp.day());

  nowTime.set(now.hour(), now.minute(), now.second());    
  nowDate.set(now.year(), now.month(), now.day());


  

  //================================логика==============================
 
  //====================== Включаем по времени Свет ====================
  if (!setting.dependByLight)
  { // Если свитч отключен, то реле выключить
    setting.rele_1_isOn = 0;
    digitalWrite(RELE1, OFF);
  }
  else
  {
    uint32_t nowSeconds = nowTime.hour * 3600 + nowTime.minute * 60 + nowTime.second;
    // если нет перехода через полночь
    if (startSeconds < stopSeconds)
    {
      if ((startSeconds <= nowSeconds) && (nowSeconds <= stopSeconds))
      {
        digitalWrite(RELE1, ON);
        setting.rele_1_isOn = 1;
      }
      else
      {
        digitalWrite(RELE1, OFF);
        setting.rele_1_isOn = 0;
      }
    }
    // переход через полночь
    else if (startSeconds > stopSeconds)
    {
      if ((stopSeconds <= nowSeconds) && (nowSeconds <= startSeconds))
      {
        digitalWrite(RELE1, OFF);
        setting.rele_1_isOn = 0;
      }
      else
      {
        digitalWrite(RELE1, ON);
        setting.rele_1_isOn = 1;
      }
    }
  }
  //===============================Логика нагрев воздуга ============
  // включаем по температуре Нагрев
  if (!setting.dependByHeating)
  { // Если свитч отключен, то реле выключить
    setting.rele_2_isOn = 0;
    digitalWrite(RELE2, OFF);
  }
  else if ((temperature <= setting.minTempr) || (temperature < setting.maxTempr))
  {
    setting.rele_2_isOn = 1;
    digitalWrite(RELE2, ON);
  }
  else
  {
    setting.rele_2_isOn = 0;
    digitalWrite(RELE2, OFF);
  }
  //===============================Логика увлажнение воздуха ============
  // ==========================включаем по влажности Увлажнитель=========
  if (!setting.dependByHumidity)
  { // Если свитч отключен, то реле выключить
    setting.rele_3_isOn = 0;
    digitalWrite(RELE3, OFF);
  }
  else if ((humidity <= setting.minHumi) || (humidity < setting.maxHumi))
  {
    setting.rele_3_isOn = 1;
    digitalWrite(RELE3, ON);
  }
  else
  {
    setting.rele_3_isOn = 0;
    digitalWrite(RELE3, OFF);
  }
  //==============================Логика полива по датчику влажности почвы и по времени===========
  // =============================включаем Полив по датчику влажност почвы===========
  if (!setting.dependByWatering) // Если свитч отключен, то реле выключить
  { 
    digitalWrite(RELE4, OFF);
    setting.rele_4_isOn = 0;
  }
  else if ((setting.dependByOnOff) && (setting.dependByWatering))
  {
    uint32_t nowSeconds1 = nowTime.hour * 3600 + nowTime.minute * 60 + nowTime.second;
    // если нет перехода через полночь
    if (startSeconds1 < stopSeconds1)
    {
      if ((startSeconds1 <= nowSeconds1) && (nowSeconds1 <= stopSeconds1))
      {
        digitalWrite(RELE4, ON);
        setting.rele_4_isOn = 1;
      }
      else
      {
        digitalWrite(RELE4, OFF);
        setting.rele_4_isOn = 0;
      }
    }
    // переход через полночь
    else if (startSeconds1 > stopSeconds1)
    {
      if ((stopSeconds1 <= nowSeconds1) && (nowSeconds1 <= startSeconds1))
      {
        digitalWrite(RELE4, OFF);
        setting.rele_4_isOn = 0;
      }
      else
      {
        digitalWrite(RELE4, ON);
        setting.rele_4_isOn = 1;
      }
    }

     if (setting.dependByOnOff) // Проверка 
    {
      if ((humiditySoil <= setting.minHumiSoil) || (humiditySoil <= setting.maxHumiSoil))
      {
        setting.rele_4_isOn = 1;
        digitalWrite(RELE4, ON);
      }
      else
      {
        setting.rele_4_isOn = 0;
        digitalWrite(RELE4, OFF);
      }
    }
  }

} // loop()