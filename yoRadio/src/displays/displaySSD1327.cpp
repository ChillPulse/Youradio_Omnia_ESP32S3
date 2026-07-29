#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1327

#include "displaySSD1327.h"
#include <Wire.h>
#include "fonts/bootlogo40.h"
//#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#ifndef I2CFREQ_HZ
  #define I2CFREQ_HZ   6000000UL
#endif

TwoWire tw = TwoWire(0);

DspCore::DspCore(): Adafruit_SSD1327(128, 128, &tw, I2C_RST/*, I2CFREQ_HZ*/) {}

#include "tools/utf8RusGFX.h"

////////////////////////////////////////////////////////////////////////////
#ifndef BATTERY_OFF

  #include "driver/adc.h"			// Подключение необходимого драйвера
  #include "esp_adc_cal.h"			// Подключение необходимой библиотеки

  #ifndef ADC_PIN
    #define ADC_PIN 1
  #endif
  #if (ADC_PIN == 1 || ADC_PIN == 36)
    #define USER_ADC_CHAN 	ADC1_CHANNEL_0
  #endif
  #if (ADC_PIN == 2)
    #define USER_ADC_CHAN 	ADC1_CHANNEL_1
  #endif
  #if (ADC_PIN == 39)
    #define USER_ADC_CHAN 	ADC1_CHANNEL_3
  #endif

  #ifndef R1
    #define R1 50		// Номинал резистора на плюс (+)
  #endif
  #ifndef R2
    #define R2 100		// Номинал резистора на плюс (-)
  #endif
  #ifndef DELTA_BAT
    #define DELTA_BAT 0	// Величина коррекции напряжения батареи
  #endif

  // Константы для оптимизации
  #define BAT_SAMPLES 100       // Количество замеров для точности
  #define BAT_CHECK_INTERVAL 5000  // Проверка каждые 5 секунд
  #define BAT_CHARGE_MIN_VOLTAGE 3.0f  // Минимальное напряжение для определения зарядки

  float ADC_R1 = R1;		// Номинал резистора на плюс (+)
  float ADC_R2 = R2;		// Номинал резистора на минус (-)
  float DELTA = DELTA_BAT;	// Величина коррекции напряжения батареи

  uint8_t g, d, e, t = 1;	// Счётчики для мигалок и осреднений
  bool Charging = false;		// Признак, что подключено зарядное устройство

  float Volt = 0; 			// Напряжение на батарее
  float Volt1, Volt2, Volt3, Volt4, Volt5, Volt6, Volt7, Volt8, Volt9 = 0;	 // Предыдущие замеры напряжения
  int16_t VVector, VDelta;	 // Вектор напряжения
  static esp_adc_cal_characteristics_t adc1_chars;

  uint8_t ChargeLevel;
// ==================== Массив напряжений на батарее, соответствующий проценту оставшегося заряда: 
  float vs[22] = {2.60, 3.10, 3.20, 3.26, 3.29, 3.33, 3.37, 3.41, 3.46, 3.51, 3.56, 3.61, 3.65, 3.69, 3.72, 3.75, 3.78, 3.82, 3.88, 3.95, 4.03, 4.25};

  // Структура для хранения состояния зарядки
  struct {
    float lastVoltage = 0;
    uint32_t lastCheck = 0;
    uint8_t chargeCount = 0;    // Счетчик последовательных увеличений напряжения
    uint8_t dischargeCount = 0; // Счетчик последовательных падений напряжения
  } chargingState;

  #endif
/////////////////////////////////////////////////////////////////////////////////

#define CLR_ITEM1    0xA
#define CLR_ITEM2    0x8
#define CLR_ITEM3    0x5
void DspCore::initDisplay() {
  tw.begin(I2C_SDA, I2C_SCL);
  if (!begin(SCREEN_ADDRESS)) {
    Serial.println(F("SSD1327 allocation failed"));
    for (;;);

#ifndef BATTERY_OFF		///////////////////////////////////////////////////////////////////////////
  // Инициализация ADC для батареи
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(USER_ADC_CHAN, ADC_ATTEN_DB_12);
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 0, &adc1_chars);
#endif				///////////////////////////////////////////////////////////////////////////
  }
  config.theme.background = TFT_BG;
  config.theme.meta       = TFT_BG;
  config.theme.metabg     = TFT_LOGO;
  config.theme.metafill   = TFT_LOGO;
  config.theme.title1     = TFT_LOGO;
  config.theme.title2     = SILVER;
  config.theme.clock      = TFT_LOGO;
  config.theme.clockbg    = DARK_GRAY;
  config.theme.rssi       = TFT_FG;
  config.theme.weather    = ORANGE;
  config.theme.ip         = SILVER;
  config.theme.vol        = SILVER;
  config.theme.bitrate    = TFT_LOGO;
  config.theme.digit      = TFT_LOGO;
  config.theme.buffer     = TFT_FG;
  config.theme.volbarout  = TFT_FG;
  config.theme.volbarin   = SILVER;
  config.theme.playlist[0] = CLR_ITEM1;
  config.theme.playlist[1] = CLR_ITEM2;
  config.theme.playlist[2] = CLR_ITEM3;
  config.theme.playlist[3] = CLR_ITEM3;
  config.theme.playlist[4] = CLR_ITEM3;
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
  
  plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  plTtemsCount = round((float)height()/plItemHeight);
  if(plTtemsCount%2==0) plTtemsCount++;
  plCurrentPos = plTtemsCount/2;
  plYStart = (height() / 2 - plItemHeight / 2) - plItemHeight * (plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
}

void DspCore::drawLogo(uint16_t top) {
  drawRGBBitmap((DSP_WIDTH - 62) / 2, top, bootlogo40, 62, 40);
}

void DspCore::printPLitem(uint8_t pos, const char* item, ScrollWidget& current){
  setTextSize(playlistConf.widget.textsize);
  if (pos == plCurrentPos) {
    current.setText(item);
  } else {
    uint8_t plColor = (abs(pos - plCurrentPos)-1)>4?4:abs(pos - plCurrentPos)-1;
    setTextColor(config.theme.playlist[plColor], config.theme.background);
    setCursor(TFT_FRAMEWDT, plYStart + pos * plItemHeight);
    fillRect(0, plYStart + pos * plItemHeight - 1, width(), plItemHeight - 2, config.theme.background);
    print(utf8Rus(item, true));
  }
}

void DspCore::drawPlaylist(uint16_t currentItem) {
  uint8_t lastPos = config.fillPlMenu(currentItem - plCurrentPos, plTtemsCount);
  if(lastPos<plTtemsCount){
    fillRect(0, lastPos*plItemHeight+plYStart, width(), height()/2, config.theme.background);
  }
}

void DspCore::clearDsp(bool black) {
  fillScreen(black?0:config.theme.background);
}

GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
  return gfxFont->glyph + c;
}

uint8_t DspCore::_charWidth(unsigned char c){
  GFXglyph *glyph = pgm_read_glyph_ptr(&DS_DIGI28pt7b, c - 0x20);
  return pgm_read_byte(&glyph->xAdvance);
}

uint16_t DspCore::textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  return w;
}

void DspCore::_getTimeBounds() {
  _timewidth = textWidth(_timeBuf);
  char buf[4];
  strftime(buf, 4, "%H", &network.timeinfo);
  _dotsLeft=textWidth(buf);
}

void DspCore::_clockSeconds(){
  setTextSize(1);
  setFont(&DS_DIGI28pt7b);
  setTextColor((network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : (CLOCKFONT_MONO?config.theme.clockbg:config.theme.background), config.theme.background);
  setCursor(_timeleft+_dotsLeft, clockTop);
  print(":");                                     /* print dots */
  setFont();

/////////////////////////////////////////////////////////////////////////////
  #ifndef BATTERY_OFF
  if(!config.isScreensaver) {
// ================= Отрисовка мигалок  и батарейки =====================
  char batbuf[8] = "";
  setTextSize(1);			// Установка TextSize
  if (Charging) {			// Если идёт зарядка - бегающие квадратики
//     setTextColor(config.theme.clock, config.theme.background);				// Светлый
     if (g == 1) strcpy(batbuf,"\xA0\xA2\x9E\x9F"); 				// 2 квад. в конце
     if (g == 2) strcpy(batbuf,"\xA0\x9E\x9E\xA3"); 				// 2 квад. по краям
     if (g == 3) strcpy(batbuf,"\x9D\x9E\xA2\xA3"); 				// 2 квад. в начале
     if (g >= 4) {g = 0; strcpy(batbuf,"\x9D\xA2\xA2\x9F");} 		// 2 квад. в середине
     g++;
  } else if (Volt < 2.8) { 	// Требуется зарядка - мигающие квадратики
//      setTextColor(config.theme.background, config.theme.clock); 		// (0%) установка инверсного цвета
     setTextColor(config.theme.clock, config.theme.background);			// Светлый
     if (d == 1) strcpy(batbuf,"\xA0\xA2\xA2\xA3"); 				// полная - 6 кв.
     if (d >= 2) {d = 0; strcpy(batbuf,"\x9D\x9E\x9E\x9F");} 		// пустая - 0 кв.
     d++;
  } else { 				// Статическая батарейка
//     setTextColor(config.theme.clock, config.theme.background); 
     if (Volt >= 3.82)      {strcpy(batbuf, "\xA0\xA2\xA2\xA3"); } 		//больше 85% (6 квад.)
     else if (Volt >= 3.72) {strcpy(batbuf, "\x9D\xA2\xA2\xA3"); } 		//от70 до 85% (5 квад.)
     else if (Volt >= 3.61) {strcpy(batbuf, "\x9D\xA1\xA2\xA3"); } 		//от 55 до 70% (4 квад.)
     else if (Volt >= 3.46) {strcpy(batbuf, "\x9D\x9E\xA2\xA3"); } 		//от 40 до 55% (3 квад.)
     else if (Volt >= 3.33) {strcpy(batbuf, "\x9D\x9E\xA1\xA3"); } 		//от 25 до 40% (2 квад.)
     else if (Volt >= 3.20) {strcpy(batbuf, "\x9D\x9E\x9E\xA3"); } 		//от 10 до 25% (1 квад.)
     else if (Volt >= 2.8)  {strcpy(batbuf, "\x9D\x9E\x9E\x9F"); } 		//от 0 до 10% (0 квад.)
  }
     setCursor(BatX, BatY);
     print(batbuf);

//  #ifndef HIDE_ClockDiv
//    drawFastVLine(_timeleft-2, clockTop, 8, config.theme.clock); 				/*divider vert*/
//    drawFastVLine(_timeleft-2, clockTop, clockTimeHeight, config.theme.clock); 		/*divider vert*/
//  #endif

// ======= Вывод цифровых значений напряжения  на дисплей ===================
  char procbuf[6];
  sprintf(procbuf, "%4i", ChargeLevel);		// формат вправо
  strlcat(procbuf, "%%", sizeof(procbuf));
//  setTextColor(config.theme.clock, config.theme.background);
  setCursor(ProcX, ProcY); 			// Установка координат для вывода процентов заряда
  if (e == 1) {printf(procbuf);}  		// Вывод процентов заряда батареи с форматом
  if (e > 2) {
     #ifndef HIDE_VOLT  			// Если напряжение выводится
     char voltbuf[11];
     snprintf(voltbuf, sizeof(voltbuf), "%.2fv", Volt);
//       if (display.mode()!=PLAYER) {			// и если не режим "Player"
//       if (player.status()!=PLAYING) {			// и если не режим "Play"
//          setCursor(ProcX, ProcY);			// Установка координат для вывода напряжения
          printf(voltbuf);     /* } */			// Вывод напряжения
     #endif 						// Конец вывода напряжения
                  }
  if (e > 3) {e = 0;}
  e++;
  }

  #endif			//   #ifndef BATTERY_OFF
/////////////////////////////////////////////////////////////////////////////////
}

void DspCore::_clockDate(){ }

void DspCore::_clockTime(){
  if(_oldtimeleft>0 && !CLOCKFONT_MONO) dsp.fillRect(_oldtimeleft,  clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);
  _timeleft = (width()/2 - _timewidth/2)+clockRightSpace;
  setTextSize(1);
  setFont(&DS_DIGI28pt7b);
  
  if(CLOCKFONT_MONO) {
    setCursor(_timeleft, clockTop);
    setTextColor(config.theme.clockbg, config.theme.background);
    print("88:88");
  }
  setTextColor(config.theme.clock, config.theme.background);
  setCursor(_timeleft, clockTop);
  print(_timeBuf);
  setFont();
  strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  _oldtimewidth = _timewidth;
  _oldtimeleft = _timeleft;
}

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw){
  clockTop = top;
  clockRightSpace = rightspace;
  clockTimeHeight = timeheight;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    _getTimeBounds();
    _clockTime();
  }
  //_clockSeconds();
}

void DspCore::clearClock(){
  dsp.fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth, clockTimeHeight+1, config.theme.background);
}

void DspCore::startWrite(void) { }

void DspCore::endWrite(void) { }

void DspCore::loop(bool force) {
  display();
  delay(5);
  #ifndef BATTERY_OFF			////////////////////////////////////////////////////////////////////////////
    static uint32_t lastBatteryUpdate = 0;
       if (millis() - lastBatteryUpdate >= 1000) { // Обновляем каждую секунду
          if(!config.isScreensaver) readBattery();
          lastBatteryUpdate = millis();
       }
  #endif						///////////////////////////////////////////////////////////////////////////
}

void DspCore::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}

void DspCore::setTextSize(uint8_t s){
  Adafruit_GFX::setTextSize(s);
}

void DspCore::flip(){
  if(ROTATE_90){
    setRotation(config.store.flipscreen?3:1);
  }else{
    setRotation(config.store.flipscreen?2:0);
  }
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) { oled_command(SSD1327_DISPLAYOFF); }
void DspCore::wake(void) { oled_command(SSD1327_DISPLAYON); }

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Adafruit_SSD1327::writePixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Adafruit_SSD1327::writeFillRect(x, y, w, h, color);
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setFont(&DS_DIGI28pt7b);
  setTextSize(1);
}

/////////////////////////////////////////////////////////////////////////////
#ifndef BATTERY_OFF
// =================== Расчёт напряжений и заряда ==========================
void DspCore::readBattery() {
    static uint32_t lastRead = 0;
    if (millis() - lastRead < 400) return; // Ограничиваем частоту чтений
    lastRead = millis();

    //  Читаем АЦП "BAT_SAMPLES"= раз и складываем результат в милливольтах
    float tempmVolt = 0;
    for(uint8_t i = 0; i < BAT_SAMPLES; i++) {
        tempmVolt += esp_adc_cal_raw_to_voltage(adc1_get_raw(USER_ADC_CHAN), &adc1_chars);
    }
    float mVolt = (tempmVolt / BAT_SAMPLES) / 1000;		       //  Получаем средний результат в Вольтах
    //  Коррекция результата и получение напряжения батареи в Вольтах
    float rawVolt = (mVolt + 0.0028 * mVolt * mVolt + 0.0096 * mVolt - 0.051) / (ADC_R2 / (ADC_R1 + ADC_R2)) + DELTA;
    if (rawVolt < 0) rawVolt = 0;

    // Сглаживание - скользящее среднее по 10 последним измерениям
    Volt9 = Volt8; Volt8 = Volt7; Volt7 = Volt4; Volt6 = Volt5; Volt5 = Volt4;
    Volt4 = Volt3; Volt3 = Volt2; Volt2 = Volt1; Volt1 = rawVolt;
    if (Volt9 > 0) Volt = (Volt1 + Volt2 + Volt3 + Volt4 + Volt5 + Volt6 + Volt7 + Volt8 + Volt9 + rawVolt) / 10;

// ==========  Контроль подключения зарядного устройства ======================
    if (millis() - chargingState.lastCheck >= BAT_CHECK_INTERVAL) {
        if (chargingState.lastVoltage < (Volt - 1)) chargingState.lastVoltage = Volt;
        chargingState.lastCheck = millis();
        VDelta = int((Volt - chargingState.lastVoltage) * 10000);

        if (VDelta > 300) {				// Подключение зарядки
           Charging = true;
           VVector = 0;					// Сброс вектора (и счетчиков)
           chargingState.chargeCount = 0;
           chargingState.dischargeCount = 0;
        }

        if (VDelta < -300) {				// Отключение зарядки
           Charging = false;
           VVector = 0;					// Сброс вектора (и счетчиков)
           chargingState.chargeCount = 0;
           chargingState.dischargeCount = 0;
        }

        if (VDelta > 0 && Volt >= BAT_CHARGE_MIN_VOLTAGE) { // ( > 0.00 && >= 3.0) 
           chargingState.chargeCount++;
           chargingState.dischargeCount = 0;
           if (chargingState.chargeCount > 3 && VVector > 0) {
               Charging = true;				// Зарядка подключена
           }

        } else { 								// ( <= 0.00) 
           chargingState.chargeCount = 0;
           chargingState.dischargeCount++;
           if (chargingState.dischargeCount > 2 && VVector < 0) {
               Charging = false; 				// Зарядка не подключена
           }
        }
    //	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       if (abs(VVector) > 599) VVector /= 3;
       VVector += VDelta;

        chargingState.lastVoltage = Volt;
    }

// ========= Рассчитываем остаток заряда в процентах ==========================
// Поиск индекса, соответствующего вольтажу. Индекс соответствует проценту оставшегося заряда
    uint8_t idx = 0;
    while (true) {
        if (Volt < vs[idx]) {ChargeLevel = 0; break;}
        if (Volt < vs[idx+1]) {
            mVolt = Volt - vs[idx];
            ChargeLevel = idx * 5 + round(mVolt /((vs[idx+1] - vs[idx]) / 5 ));
            if (Charging) ChargeLevel = ChargeLevel - 9;		// Коррекция при подключении зарядки
            break;
        }
        else {idx++;}
    }
    if (ChargeLevel < 0) ChargeLevel = 0;
    if (ChargeLevel > 100) ChargeLevel = 100;
}
#endif			//   #ifndef BATTERY_OFF
/////////////////////////////////////////////////////////////////////////////////
#endif
