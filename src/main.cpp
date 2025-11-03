#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <cmath>
#include <WiFi.h>
#include "time.h"

#define USING_ENV4

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

#if defined(USING_ENV4)
m5::unit::UnitENV4 unitENV4;
#else
m5::unit::UnitSHT40 unitSHT40;
m5::unit::UnitBMP280 unitBMP280;
#endif

#if defined(USING_ENV4)
auto& sht40  = unitENV4.sht40;
auto& bmp280 = unitENV4.bmp280;
#else
auto& sht40  = unitSHT40;
auto& bmp280 = unitBMP280;
#endif

// スプライト（オフスクリーン描画用）
M5Canvas canvas(&M5.Display);

// センサー値を保持する変数
float temp_sht40 = 0;
float humidity = 0;
float pressure = 0;

// 前回表示した値（変化がある時だけ更新するため）
float prev_temp_sht40 = -999;
float prev_humidity = -999;
float prev_pressure = -999;

// NTP設定
const char* ntpServer = "ntp.nict.jp";
const long gmtOffset_sec = 9 * 3600;    // JST (UTC+9)
const int daylightOffset_sec = 0;

// 時刻同期フラグ
bool timeSynced = false;

unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 3600000; // 1時間（ミリ秒）

// バッテリー表示
bool showBattery = false;

unsigned long batteryShowTime = 0;
const unsigned long batteryDisplayDuration = 3000; // 3sec

// 曜日の配列
static constexpr const char* const wd[7] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

}

void setup()
{
    M5.begin();

    // ディスプレイの初期設定
    lcd.setRotation(1);
    lcd.setTextDatum(TL_DATUM);
    lcd.fillScreen(BLACK);
    
    // タイトル表示
    lcd.setTextSize(2);
    lcd.setTextColor(WHITE, BLACK);
    lcd.setCursor(10, 10);
    lcd.println("ENV IV Sensor");
    lcd.drawLine(0, 40, 320, 40, BLUE);

    // WiFi接続
    lcd.setCursor(10, 60);
    lcd.print("Connecting WiFi...");
    
    WiFi.begin();
    
    int wifi_retry = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retry < 20) {
      delay(500);
      lcd.print(".");
      wifi_retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      lcd.println("\nWiFi Connected!");
      lcd.print("IP: ");
      lcd.println(WiFi.localIP());
      
      // NTPで時刻同期
      lcd.setCursor(10, 120);
      lcd.print("Syncing time...");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      
      // 時刻取得待機
      struct tm timeInfo;
      if (getLocalTime(&timeInfo, 10000)) {
        lcd.println(" Done!");
        lcd.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
          timeInfo.tm_year + 1900,
          timeInfo.tm_mon + 1,
          timeInfo.tm_mday,
          timeInfo.tm_hour,
          timeInfo.tm_min,
          timeInfo.tm_sec
        );
        timeSynced = true;
        lastSyncTime = millis();
      } else {
        lcd.println(" Failed!");
      }
      delay(2000);
    } else {
      lcd.println("\nWiFi Failed!");
      lcd.println("Check SSID/Password");
      delay(2000);
    }

    // デバッグ情報表示
    lcd.fillScreen(BLACK);
    lcd.setCursor(10, 10);
    lcd.println("Initializing...");

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    
    lcd.setCursor(10, 30);
    lcd.printf("SDA:%d SCL:%d", pin_num_sda, pin_num_scl);

    // BMP280設定
    {
        using namespace m5::unit::bmp280;
        auto cfg             = bmp280.config();
        cfg.osrs_pressure    = Oversampling::X16;
        cfg.osrs_temperature = Oversampling::X2;
        cfg.filter           = Filter::Coeff16;
        cfg.standby          = Standby::Time500ms;
        bmp280.config(cfg);
    }

#if defined(USING_ENV4)
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    
    lcd.setCursor(10, 60);
    lcd.println("Adding ENV4...");
    delay(500);

    bool addResult = Units.add(unitENV4, Wire);
    M5_LOGI("Units.add result: %d", addResult);
    
    lcd.setCursor(10, 90);
    lcd.printf("Add result: %d", addResult);
    delay(500);

    if (!addResult) {
        M5_LOGE("Failed to add unitENV4");
        lcd.fillScreen(RED);
        lcd.setCursor(10, 100);
        lcd.setTextColor(WHITE, RED);
        lcd.println("Failed to add ENV4!");
        while (true) {
            delay(10000);
        }
    }

    bool beginResult = Units.begin();
    M5_LOGI("Units.begin result: %d", beginResult);
    
    lcd.setCursor(10, 120);
    lcd.printf("Begin result: %d", beginResult);
    delay(500);

    if (!beginResult) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(RED);
        lcd.setCursor(10, 100);
        lcd.setTextColor(WHITE, RED);
        lcd.println("Failed to begin!");
        lcd.setCursor(10, 130);
        lcd.println("Check connection!");
        while (true) {
            delay(10000);
        }
    }
#else
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 400000U);
    if (!Units.add(unitSHT40, Wire) || !Units.add(unitBMP280, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(RED);
        lcd.setCursor(10, 100);
        lcd.setTextColor(WHITE, RED);
        lcd.println("Sensor Error!");
        while (true) {
            delay(10000);
        }
    }
#endif

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    
    lcd.fillScreen(BLACK);
    lcd.setTextColor(GREEN, BLACK);
    lcd.setCursor(10, 100);
    lcd.println("Sensor Ready!");
    delay(1000);
    
    lcd.fillScreen(BLACK);
    
    // スプライト作成（画面サイズと同じ）
    canvas.createSprite(lcd.width(), lcd.height());
    canvas.setTextDatum(TL_DATUM);
    M5_LOGI("Canvas created: %d x %d", lcd.width(), lcd.height());
}

void loop()
{
    M5.update();
    Units.update();

    if (M5.BtnA.wasPressed()) {
      showBattery = true;
      batteryShowTime = millis();
    }

    if (showBattery && (millis() - batteryShowTime > batteryDisplayDuration)) {
      showBattery = false;
    }

    // 電源ボタン長押しで電源オフ
    if (M5.BtnPWR.wasHold()) {
        lcd.fillScreen(BLACK);
        lcd.setTextColor(WHITE, BLACK);
        lcd.setTextSize(3);
        lcd.setCursor(60, 100);
        lcd.println("Power Off...");
        delay(1000);
        M5.Power.powerOff();
    }

    // 定期的にNTP時刻同期
    unsigned long currentMillis = millis();
    if (WiFi.status() == WL_CONNECTED && 
        (currentMillis - lastSyncTime >= syncInterval)) {
        struct tm timeInfo;
        if (getLocalTime(&timeInfo, 10000)) {
            lastSyncTime = currentMillis;
            timeSynced = true;
        }
    }

    // SHT40のデータ更新
    if (sht40.updated()) {
      temp_sht40 = sht40.temperature();
        humidity = sht40.humidity();
        M5.Log.printf(">SHT40Temp:%.1f\n", ">Humidity:%.1f\n", temp_sht40, humidity);
    }
    
    // BMP280のデータ更新
    if (bmp280.updated()) {
        auto p = bmp280.pressure();
        pressure = p * 0.01f;
        
        M5.Log.printf(
            ">Pressure:%.1f\n", pressure);
    }

    // 画面更新（500msごと）
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    
    if (now - lastUpdate > 500) {
        lastUpdate = now;
        
        // スプライトに描画（オフスクリーン）
        canvas.fillScreen(BLACK);

        if (showBattery) {
          canvas.setTextSize(2);
          canvas.setTextColor(WHITE);
          canvas.setCursor(10, 20);
          canvas.println("=== Battery Info ===");
  
          int y = 80;
          canvas.setTextSize(2);

          int batteryLevel = M5.Power.getBatteryLevel();
          canvas.setTextColor(GREEN);
          canvas.setCursor(10, y);
          canvas.printf("Level: %3d%%", batteryLevel);
          
          // 充電状態
          y =+ 40;
          canvas.setTextColor(LIGHTGREY);
          canvas.setCursor(10, y);
          if (M5.Power.isCharging()) {
            canvas.setTextColor(YELLOW);
            canvas.print("Status: Charging");
          } else {
            canvas.println("Status: Discharging");
          }
        } else {
          // === センサー情報 ===
        canvas.setTextSize(2);
        canvas.setTextColor(WHITE);
        canvas.setCursor(10, 5);
        canvas.println("=== Sensor Data ===");
        
        int y = 35;
        
        // Humidity
        canvas.setTextColor(CYAN);
        canvas.setCursor(10, y);
        canvas.printf("Humidity:    %.1f %%", humidity);
        
        y += 25;
        // Temperature
        canvas.setTextColor(ORANGE);
        canvas.setCursor(10, y);
        canvas.printf("Temperature: %.1f C", temp_sht40);
        
        y += 25;
        // Pressure
        canvas.setTextColor(GREENYELLOW);
        canvas.setCursor(10, y);
        canvas.printf("Pressure:  %.1f hPa", pressure);
        
        // === 時刻情報 ===
        y += 35;
        canvas.setTextColor(WHITE);
        canvas.setCursor(10, y);
        canvas.println("=== Time (NTP) ===");
        
        y += 25;
        struct tm timeInfo;
        if (getLocalTime(&timeInfo, 100)) {
            canvas.setTextSize(2);
            canvas.setTextColor(YELLOW);
            canvas.setCursor(10, y);
            canvas.printf("%04d/%02d/%02d (%s)", 
                timeInfo.tm_year + 1900,
                timeInfo.tm_mon + 1,
                timeInfo.tm_mday,
                wd[timeInfo.tm_wday]);
            
            y += 25;
            canvas.setCursor(10, y);
            canvas.printf("%02d:%02d:%02d", 
                timeInfo.tm_hour,
                timeInfo.tm_min,
                timeInfo.tm_sec);
        } else {
            canvas.setTextColor(RED);
            canvas.setCursor(10, y);
            canvas.println("Time not available");
        }
        
        // === 接続状態 ===
        canvas.setTextSize(1);
        canvas.setTextColor(LIGHTGREY);
        canvas.setCursor(10, lcd.height() - 15);
        if (WiFi.status() == WL_CONNECTED) {
            canvas.print("WiFi:OK");
        } else {
            canvas.print("WiFi:--");
        }
        
        canvas.setCursor(100, lcd.height() - 15);
        if (timeSynced) {
            canvas.print("NTP:OK");
        } else {
            canvas.print("NTP:--");
        }
        }
        
        // スプライトを画面に一括転送（ちらつき防止）
        canvas.pushSprite(0, 0);
    }
    
}