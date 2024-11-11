
/*  /////////////// пояснения по прошивке 1.20////////////////////
// пример с настройкой логина-пароля
// если при запуске нажата кнопка на D2 (к GND)
// открывается точка WiFi Config с формой ввода
// храним настройки в EEPROM

*/

//#define STATIC_IP // закомментировать если подключаетесь к мобильной точке доступа на телефоне

//const char* ssid = "INTERNET";
//const char* password = "Vladilen";

#define RELE1 33
#define RELE2 25
#define RELE3 26
#define RELE4 27
#define ON 1
#define OFF 0
#define SENS1 36                       // настройка аналог pin
#include <WiFi.h>                      // esp32 WiFi поддержка
#include <LittleFS.h>                  // Файловая система
#include <GyverPortal.h>               // Библиотека Веб морды
GyverPortal ui(&LittleFS);             // для проверки файлов
#include <RTClib.h>                    // Часы реального времени
RTC_DS3231 rtc;                        // Инициализация модуля реального времени
#include <SPI.h>                       // для I2C
#include <GyverHTU21D.h>               //Для датчика HTU21
GyverHTU21D htu;                       // Инициализация датчика температуры и влажности по I2C
#include <EEPROM.h>
#include <EEManager.h>                 // Менеджер памяти
struct Settings {                      //настройки, хранятся в памяти EEPROM
  GPtime startTime;                    // таймер света
  GPtime stopTime;                     // таймер света
  GPtime startTime2;                   // таймер полива
  GPtime stopTime2;                    // таймер полива
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
  uint32_t startSeconds = 0;
  uint32_t stopSeconds = 0;
  uint32_t startSeconds1 = 0;
  uint32_t stopSeconds1 = 0;
  };
Settings setting;
EEManager memory(setting);   // передаём нашу переменную (фактически её адрес)

struct LoginPass {           // Структура в памяти для логина и пароля WiFi
  char ssid[20];
  char pass[20];
};
LoginPass lp;

GPdate nowDate;
GPtime nowTime;
uint32_t startSeconds = 0;
uint32_t stopSeconds = 0;
uint32_t startSeconds1 = 0;
uint32_t stopSeconds1 = 0;
bool dependByTime1 = 1;         // флаг разрешения включения полив по времени
bool dependByLight = 1;
bool dependByHeating = 1;
bool dependByHumidity = 1;
bool dependByWatering = 1;
bool dependByOnOff = 1;
float temperature = 0.0;
float humidity = 0.0;
int humiditySoil = 0;
int summTemp;
int summHum; 
int summHumSoil;
String dayWeek;
int namWeek = 0;
const char *plot_1[] = {     //============Переменные для графика Ajax===========
  "temp", "humidity", "humiditySoil"
};
void htuRead() {
  htu.readTick();                      //Запускаем датчик
  temperature = htu.getTemperature();  //переменная для температуры
  humidity = htu.getHumidity();        // переменная для влажности
  summTemp = temperature * 100 / 100 ; //переменная для графика int
  summHum = humidity * 100 / 100 ;     //переменная для графика int
}
void sensRead() {
  humiditySoil = analogRead(SENS1);
  summHumSoil = humiditySoil / 100 ;
  Serial.print("Влажность почвы = ");
  Serial.print(humiditySoil);
  Serial.println(" %");
}
void dayWeekRead() {
  String daysOfTheWeek[] = {"Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
  DateTime now = rtc.now();
  dayWeek = daysOfTheWeek[now.dayOfTheWeek() ];
  namWeek = now.dayOfTheWeek();
 /* Serial.print("День недели : ");
  Serial.println(dayWeek);
  Serial.print("Лампа реле 4 :");
  Serial.println(setting.rele_4_isOn);
  Serial.print("Состояние реле 4 :");
  Serial.println(digitalRead(RELE4));
  Serial.print("Состояние переключателя датчика вл :");
  Serial.println(setting.dependByOnOff);
  Serial.print("МинВлажн :");
  Serial.println(setting.minHumiSoil);
  Serial.print("МахВлажн :");
  Serial.println(setting.maxHumiSoil);
  */
}
void build() {
  GP.ONLINE_CHECK();                   // Проверка системы на On-Line
  GP.BUILD_BEGIN(400);
  GP.THEME(GP_DARK);
  GP.PAGE_TITLE("Rosti-Shishka");
  //все обновляющиеся параметры на WEB странице надо указать тут
  GP.UPDATE("dayWeek,namWeek,nowDate,nowTime,nowDay,startTime2,stopTime2,startTime,stopTime,tempr,humid,humidsoil,releIndikator1,releIndikator_1_1,releIndikator2,releIndikator3,releIndikator_3_3,releIndikator4,releIndikator_4_4,releIndikator_2_2,sw_light,sw_1,sw_2,sw_3,sw");
  GP_MAKE_BLOCK_TAB(
    "Рости-Шишка",
    GP_MAKE_BOX(GP.DATE("nowDate", nowDate, false); GP.TIME("nowTime", nowTime, false); );
    GP_MAKE_BOX(GP.LABEL("NAN", "dayWeek"); GP.LABEL("NAN", "namWeek"); ); // День недели
    GP_MAKE_BOX(GP.NAV_TABS("Домой,Свет,Нагрев,Влажность,Полив, WiFi"); );       // Верхнее меню блоков навигации
  );  
   GP.NAV_BLOCK_BEGIN();                 // начало блока
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
    GP_MAKE_BOX(GP.LABEL("Влажность почвы: "); GP.LABEL("humidsoil", "humidsoil"); GP.LABEL("%"); );
    GP_MAKE_BOX(GP.LABEL("Вкл  в: "); GP.TIME("startTime2", setting.startTime2); );
    GP_MAKE_BOX(GP.LABEL("Выкл в: "); GP.TIME("stopTime2", setting.stopTime2);  );
    GP_MAKE_BOX(GP.LABEL("По датчику "); GP.SWITCH("sw", setting.dependByOnOff); );  // изменить на влажность
    GP_MAKE_BOX(GP.LABEL("Вкл  при: "); GP.SPINNER("minHumiSoil", setting.minHumiSoil, 10, 100, 5); GP.LABEL("%"); );
    GP_MAKE_BOX(GP.LABEL("Выкл при: "); GP.SPINNER("maxHumiSoil", setting.maxHumiSoil, 10, 100, 5); GP.LABEL("%"); );
    GP_MAKE_BOX(GP.LABEL("Реле 4:"); GP.LED_RED("releIndikator_4_4", setting.rele_4_isOn); GP.SWITCH("sw_3", setting.dependByWatering); GP.LABEL("On/Off"); );
   );
   //==========================Блок WiFi
  GP.NAV_BLOCK_BEGIN();                  // начало блока
  GP_MAKE_BLOCK_TAB( 
    "Настройка света",
    GP_MAKE_BOX(GP.FORM_BEGIN("/login"); GP.FORM_BEGIN("/login"); GP.TEXT("lg", "Login", lp.ssid); );
    GP_MAKE_BOX(GP.TEXT("ps", "Password", lp.pass); );
    GP_MAKE_BOX(GP.SUBMIT("Submit"););
  );
  GP.FORM_END();
  GP.NAV_BLOCK_END();
  GP.BUILD_END();
}
void setup() { 
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  htu.begin();
    if (! htu.begin()) Serial.println(F("HTU21D error"));        // Проверка подключения датчика темп и влажности
  rtc.begin();
    if (! rtc.begin()) Serial.println(F("Couldn't find RTC"));   // Проверка модуля реального времени

  EEPROM.begin(100); //                           читаем логин пароль из памяти
  EEPROM.get(0, lp);

  pinMode(2, INPUT_PULLUP);  // если кнопка нажата - открываем портал
  if (!digitalRead(2)) loginPortal();

  // пытаемся подключиться
  Serial.print("Connect to: ");
  Serial.println(lp.ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(lp.ssid, lp.pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Local IP: ");
  Serial.println(WiFi.localIP());

  // подключаем конструктор и запускаем
  ui.attachBuild(build);
  ui.attach(action);
  ui.start();
  ui.enableOTA();  // без пароля
  //ui.enableOTA("admin", "pass");  // с паролем
  if (!LittleFS.begin()) Serial.println("FS Error");
  ui.downloadAuto(true);

  EEPROM.begin(100);     // выделить память (больше или равно размеру структуры данных)
  memory.begin(0, 's');  // изменить букву в скобках на другую, чтобы восстановить настройки по умолчанию
  byte stat = memory.begin(0, 's');
  Serial.print(stat);

  pinMode(RELE1, OUTPUT); // определяем состояние пина //свет
  pinMode(RELE2, OUTPUT); // определяем состояние пина //нагрев
  pinMode(RELE3, OUTPUT); // определяем состояние пина //влажность
  pinMode(RELE4, OUTPUT); // определяем состояние пина //полив
  pinMode(SENS1, INPUT);  // определяем состояние пина //влажность почвы
  digitalWrite(RELE1, OFF);
  digitalWrite(RELE2, OFF);
  digitalWrite(RELE3, OFF);
  digitalWrite(RELE4, OFF);
}  //setup()
void loginPortal() {
    Serial.println("Portal start");

    // запускаем точку доступа
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WiFi Config");

    // запускаем портал
    //GyverPortal ui(&LittleFS);
    // ui.attachBuild(build);
    // ui.start();
    // ui.attach(action);
    // ui.enableOTA();
    // ui.downloadAuto(true);
    // работа портала
    while (ui.tick());
}
void action(GyverPortal& p) {
  if (p.form("/login")) {            // кнопка нажата
    p.copyStr("lg", lp.ssid);        // копируем себе
    p.copyStr("ps", lp.pass);
    EEPROM.put(0, lp);               // сохраняем
    EEPROM.commit();                 // записываем
    WiFi.softAPdisconnect();         // отключаем AP
  }
  // если было обновление 
  if (ui.update()) {
    ui.updateDate("nowDate", nowDate);
    ui.updateTime("nowTime", nowTime);
    ui.updateTime("startTime", setting.startTime);   // старт свет
    ui.updateTime("stopTime", setting.stopTime);     // стоп свет
    ui.updateTime("startTime2", setting.startTime2); // старт полив
    ui.updateTime("stopTime2", setting.stopTime2);   // стоп полив
    ui.updateFloat("tempr", temperature, 1 );
    ui.updateFloat("humid", humidity, 1);
    ui.updateInt("humidsoil", humiditySoil);
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
    ui.updateString("dayWeek", dayWeek);              // день недели
    ui.updateInt("namWeek", namWeek);                 // номер дня недели начинаеться с 0
  if (ui.update("plot")) {                                 // Обновление данных для графика
    int answ[] = {summTemp, summHum, summHumSoil};         // Обновление данных для графика
    ui.answer(answ, 3);                                    // Обновление данных для графика
    }
  } //ui.update()

  if (ui.click()) {
    if (ui.clickDate("nowDate", nowDate)) {
        rtc.adjust (DateTime(nowDate.year, nowDate.month, nowDate.day, nowTime.hour, nowTime.minute, nowTime.second)); // записываем Время (и Дату! отдельно нельзя!) в RTC
    }
    if (ui.clickTime("nowTime", nowTime)) {
        rtc.adjust (DateTime(nowDate.year, nowDate.month, nowDate.day, nowTime.hour, nowTime.minute, nowTime.second)); // записываем Время (и Дату! отдельно нельзя!) в RTC
    }
    
    // ====обновление времени запуска и отключения света
    if (ui.clickTime("startTime", setting.startTime)) {
      setting.startSeconds = setting.startTime.hour * 60 * 60 + setting.startTime.minute * 60 + setting.startTime.second;
      memory.updateNow();
    }
    if (ui.clickTime("stopTime", setting.stopTime)) {
      setting.stopSeconds = setting.stopTime.hour * 60 * 60 + setting.stopTime.minute * 60 + setting.stopTime.second;
      memory.updateNow();
    }
    if (ui.clickBool("sw_light", setting.dependByLight)) {
      memory.updateNow();
    }
    // ====Обновление нагрева switch/min/max 
    if (ui.clickBool("sw_1", setting.dependByHeating)) {
      memory.updateNow();
    }
    if (ui.clickInt("maxTempr", setting.maxTempr)) {
      memory.updateNow();
    }
    if (ui.clickInt("minTempr", setting.minTempr)) {
      memory.updateNow();
    }
       //====Обновление влажности switch/min/max 
    if (ui.clickBool("sw_2", setting.dependByHumidity)) {
      memory.updateNow();
    }
    if (ui.clickInt("minHumi", setting.minHumi)) {
      memory.updateNow();
    }
    if (ui.clickInt("maxHumi", setting.maxHumi)) {
      memory.updateNow();
    }   
      // ====Обновление времени запуска реле Полив
    if (ui.clickTime("startTime2", setting.startTime2)) {
      setting.startSeconds1 = setting.startTime2.hour * 60 * 60 + setting.startTime2.minute * 60 + setting.startTime2.second;
      memory.updateNow();
    }
    if (ui.clickTime("stopTime2", setting.stopTime2)) {
      setting.stopSeconds1 = setting.stopTime2.hour * 60 * 60 + setting.stopTime2.minute * 60 + setting.stopTime2.second;
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
  }  //ui.click()
}  //action()
void loop() {
  ui.tick();
  memory.tick();
  DateTime now = rtc.now();

  // раз в 10 сек проверим стабильность сети
  
  // раз в 1 сек проверяем показание с датчика
  static uint32_t ms2 = 0;
  if (millis() - ms2 >= 1000) {
    ms2 = millis();
    htuRead();   
  }
  
  // раз в 5 сек проверяем показание с датчика sens1
  static uint32_t ms3 = 0;
  if (millis() - ms3 >= 5000) {
    ms3 = millis();
    sensRead();
  }
    // раз в 5 сек проверяем показание с датчика sens1
  static uint32_t ms4 = 0;
  if (millis() - ms4 >= 5000) {
   ms4 = millis();
   dayWeekRead();

  }
  // раз в секунду проверяем реле и включаем если надо
  static uint32_t ms5 = 0;
  if (millis() - ms5 >= 1000) {
    ms5 = millis();

  nowTime.set(now.hour(), now.minute(), now.second());    
  nowDate.set(now.year(), now.month(), now.day());
  // определяем текущее количество секунд от начала суток
  uint32_t nowSeconds = nowTime.hour * 3600 + nowTime.minute * 60 + nowTime.second;
  
  //================================логика==============================
 
  //====================== Включаем по времени Свет ====================
  if (!setting.dependByLight)
    {
     setting.rele_1_isOn = 0;
     digitalWrite(RELE1, OFF);
    }
  else
  {
    
    // если нет перехода через полночь
    if (setting.startSeconds < setting.stopSeconds)
    {
      if ((setting.startSeconds <= nowSeconds) && (nowSeconds <= setting.stopSeconds))
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
    else if (setting.startSeconds > setting.stopSeconds)
    {
      if (((setting.stopSeconds <= nowSeconds) && (nowSeconds >= setting.startSeconds)) || ((setting.stopSeconds >= nowSeconds) && (nowSeconds <= setting.startSeconds)))
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
  else if (!setting.dependByOnOff)
    // если нет перехода через полночь
  {
    if (setting.startSeconds1 < setting.stopSeconds1)
    {
      if ((setting.startSeconds1 <= nowSeconds) && (nowSeconds <= setting.stopSeconds1))
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
    else if (setting.startSeconds1 > setting.stopSeconds1)
    {
      if (((setting.stopSeconds1 <= nowSeconds) && (nowSeconds >= setting.startSeconds1)) || ((setting.stopSeconds1 >= nowSeconds) && (nowSeconds <= setting.startSeconds1)))
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
  }
  if (setting.dependByOnOff) // Проверка 
    {
      if ((humiditySoil <= setting.minHumiSoil) || (humiditySoil <= setting.maxHumiSoil))
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
 } // ms5
} // loop()