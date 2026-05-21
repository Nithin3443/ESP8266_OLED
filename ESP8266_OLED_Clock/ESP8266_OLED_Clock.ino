#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "secrets.h"

// OLED Display Settings
SSD1306Wire display(0x3c, D6, D5);

// Time Settings
#define MY_NTP_SERVER "pool.ntp.org"

// Weather Data
struct WeatherData {
  float temp;
  int humidity;
  float windSpeed;
  int weatherCode;
  float rainSum;
  bool valid;
  String errorMsg;
};

WeatherData currentWeather = {0, 0, 0, 0, 0, false, ""};

// Location Management
int currentLocationIndex = 0;

// Display State Management
enum DisplayState {
  STATE_LOCATION_NAME,
  STATE_TIME,
  STATE_WEATHER_ITEM,
  STATE_ANIMATION
};

DisplayState currentState = STATE_LOCATION_NAME;
unsigned long lastStateChange = 0;
int weatherIndex = 0; // 0=Temp, 1=Hum, 2=Wind, 3=Rain

// Durations
const unsigned long locationNameDuration = 3000; // 3s
const unsigned long timeDuration = 10000;        // 10s
const unsigned long weatherItemDuration = 3000;  // 3s
const unsigned long animationDuration = 4000;    // 4s

// Eye Expressions
enum EyeExpression {
  NEUTRAL,
  HAPPY,
  SAD,
  ANGRY,
  SURPRISED,
  SHIVER,   // Cold
  SWEAT,    // Hot/Humid
  BLOWN,    // Windy
  CRY       // Rain
};

EyeExpression currentExpression = NEUTRAL;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting...");

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Connecting to WiFi...");
  display.display();

  WiFi.begin(SSID_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  display.clear();
  display.drawString(0, 0, "WiFi Connected!");
  display.drawString(0, 15, WiFi.localIP().toString());
  display.display();
  delay(2000);

  // Initialize first location
  switchLocation(0);
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - lastStateChange;

  switch (currentState) {
    case STATE_LOCATION_NAME:
      if (elapsed >= locationNameDuration) {
        currentState = STATE_TIME;
        lastStateChange = currentMillis;
      }
      break;

    case STATE_TIME:
      if (elapsed >= timeDuration) {
        currentState = STATE_WEATHER_ITEM;
        weatherIndex = 0;
        lastStateChange = currentMillis;
      }
      break;

    case STATE_WEATHER_ITEM:
      if (elapsed >= weatherItemDuration) {
        currentState = STATE_ANIMATION;
        determineExpression(weatherIndex);
        lastStateChange = currentMillis;
      }
      break;

    case STATE_ANIMATION:
      if (elapsed >= animationDuration) {
        weatherIndex++;
        if (weatherIndex > 3) {
          // End of cycle for this location
          // Switch to next location
          currentLocationIndex = (currentLocationIndex + 1) % NUM_LOCATIONS;
          switchLocation(currentLocationIndex);
          
          currentState = STATE_LOCATION_NAME;
        } else {
          currentState = STATE_WEATHER_ITEM;
        }
        lastStateChange = currentMillis;
      }
      break;
  }

  // Drawing
  display.clear();
  
  if (currentState == STATE_LOCATION_NAME) {
    drawLocationName();
  } else if (currentState == STATE_TIME) {
    drawTime();
  } else if (currentState == STATE_WEATHER_ITEM) {
    drawWeatherItem(weatherIndex);
  } else if (currentState == STATE_ANIMATION) {
    drawAnimatedEyes(currentExpression, elapsed);
  }
  
  display.display();
  delay(10);
}

void switchLocation(int index) {
  Serial.print("Switching to: ");
  Serial.println(locations[index].name);
  
  // Update Timezone
  configTime(locations[index].timezone.c_str(), MY_NTP_SERVER);
  
  // Fetch Weather for new location
  fetchWeather(index);
}

void fetchWeather(int index) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(locations[index].lat, 4) + 
                 "&longitude=" + String(locations[index].lon, 4) + 
                 "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m&daily=precipitation_sum&timezone=auto&forecast_days=1";
    
    Serial.println("Fetching: " + url);

    if (http.begin(client, url)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          currentWeather.temp = doc["current"]["temperature_2m"];
          currentWeather.humidity = doc["current"]["relative_humidity_2m"];
          currentWeather.windSpeed = doc["current"]["wind_speed_10m"];
          currentWeather.weatherCode = doc["current"]["weather_code"];
          currentWeather.rainSum = doc["daily"]["precipitation_sum"][0];
          currentWeather.valid = true;
          currentWeather.errorMsg = "";
        } else {
          currentWeather.valid = false;
          currentWeather.errorMsg = "JSON Error";
        }
      } else {
        currentWeather.valid = false;
        currentWeather.errorMsg = "HTTP " + String(httpCode);
      }
      http.end();
    } else {
      currentWeather.valid = false;
      currentWeather.errorMsg = "Conn Error";
    }
  }
}

void determineExpression(int index) {
  if (!currentWeather.valid) {
    currentExpression = NEUTRAL;
    return;
  }

  switch (index) {
    case 0: // Temp
      if (currentWeather.temp > 30.0) currentExpression = SWEAT;
      else if (currentWeather.temp < 10.0) currentExpression = SHIVER;
      else currentExpression = HAPPY;
      break;
      
    case 1: // Humidity
      if (currentWeather.humidity > 80) currentExpression = SWEAT;
      else currentExpression = NEUTRAL;
      break;
      
    case 2: // Wind
      if (currentWeather.windSpeed > 20.0) currentExpression = BLOWN;
      else currentExpression = HAPPY;
      break;
      
    case 3: // Rain
      if (currentWeather.rainSum > 2.0 || (currentWeather.weatherCode >= 51 && currentWeather.weatherCode <= 67)) 
        currentExpression = CRY;
      else currentExpression = HAPPY;
      break;
  }
}

void drawLocationName() {
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 20, "LOCATION");
  
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 35, locations[currentLocationIndex].name);
}

void drawTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  if (timeinfo->tm_year > 70) {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 0, locations[currentLocationIndex].name); // Show City in Header
    display.drawLine(0, 14, 128, 14);

    display.setFont(ArialMT_Plain_24);
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    display.drawString(64, 20, timeStr);
    
    display.setFont(ArialMT_Plain_10);
    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%a %d %b", timeinfo);
    display.drawString(64, 50, dateStr);
  } else {
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 20, "Syncing Time...");
  }
}

void drawWeatherItem(int index) {
  if (!currentWeather.valid) {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 25, "Fetching Weather...");
    if (currentWeather.errorMsg != "") display.drawString(64, 40, currentWeather.errorMsg);
    return;
  }

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  
  String header, value, unit;

  switch(index) {
    case 0: header = "TEMPERATURE"; value = String(currentWeather.temp, 1); unit = "C"; break;
    case 1: header = "HUMIDITY"; value = String(currentWeather.humidity); unit = "%"; break;
    case 2: header = "WIND SPEED"; value = String(currentWeather.windSpeed, 1); unit = "km/h"; break;
    case 3: header = "RAIN FORECAST"; value = String(currentWeather.rainSum, 1); unit = "mm"; break;
  }

  display.drawString(64, 0, header);
  display.drawLine(0, 14, 128, 14);

  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 25, value + unit);
}

void drawAnimatedEyes(EyeExpression exp, unsigned long elapsed) {
  int lx = 32;
  int rx = 96;
  int y = 32;
  int r = 20;
  
  bool blink = (elapsed % 3000) < 150;
  int offsetX = 0;
  int offsetY = 0;
  
  if (exp == SHIVER) offsetX = (elapsed % 100) < 50 ? -2 : 2;
  if (exp == BLOWN) {
    offsetX = 4;
    if ((elapsed % 200) < 100) blink = true;
  }
  
  if (blink) {
    display.fillRect(lx - r + offsetX, y - 2, r * 2, 4);
    display.fillRect(rx - r + offsetX, y - 2, r * 2, 4);
  } else {
    display.fillCircle(lx + offsetX, y + offsetY, r);
    display.fillCircle(rx + offsetX, y + offsetY, r);
    
    display.setColor(BLACK);
    int pupilX = 0;
    int pupilY = 0;
    
    if (exp == NEUTRAL || exp == HAPPY) {
      int look = (elapsed / 1000) % 4;
      if (look == 1) pupilX = -6;
      if (look == 3) pupilX = 6;
    }
    if (exp == BLOWN) pupilX = 6;
    
    display.fillCircle(lx + offsetX + pupilX, y + offsetY + pupilY, 6);
    display.fillCircle(rx + offsetX + pupilX, y + offsetY + pupilY, 6);
    display.setColor(WHITE);
    
    display.setColor(BLACK);
    switch (exp) {
      case HAPPY:
        display.fillRect(lx - r + offsetX, y + 8 + offsetY, r * 2, r);
        display.fillRect(rx - r + offsetX, y + 8 + offsetY, r * 2, r);
        break;
      case ANGRY:
        display.fillRect(lx - r + offsetX, y - r + offsetY, r * 2, 12);
        display.fillRect(rx - r + offsetX, y - r + offsetY, r * 2, 12);
        break;
      case SAD:
      case CRY:
        display.fillRect(lx - r + offsetX, y - r + offsetY, r * 2, 10);
        display.fillRect(rx - r + offsetX, y - r + offsetY, r * 2, 10);
        break;
      case SWEAT:
        display.setColor(WHITE);
        int dropY = (elapsed % 1000) / 20;
        if (dropY > 30) dropY = 0;
        display.fillCircle(120, 10 + dropY, 3);
        display.setColor(BLACK);
        break;
    }
    display.setColor(WHITE);
    
    if (exp == CRY) {
       int tearY = (elapsed % 800) / 15;
       if (tearY > 20) tearY = 0;
       display.fillCircle(lx, y + 10 + tearY, 2);
       display.fillCircle(rx, y + 10 + tearY, 2);
    }
  }
}
