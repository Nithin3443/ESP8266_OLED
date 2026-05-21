#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====== WiFi Settings ======
const char* ssid_STA = "WiFi-A4AC";      
const char* password_STA = "56246322"; 

const char* ssid_AP = "ESP8266_AP";         
const char* password_AP = "12345678";       

// ====== OLED Settings ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== Web Server ======
ESP8266WebServer server(80);

// ====== Location (Sydney example) ======
const float latitude = -33.7501;
const float longitude = 150.9354;

// ====== Weather Data ======
float temp = 0;
float windspeed = 0;
String weatherTime = "--";
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 1 minute

// ====== Fetch Weather Data ======
void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&current=temperature_2m,weather_code,wind_speed_10m";
    Serial.println("Fetching: " + url);
    WiFiClient client;
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      temp = doc["current_weather"]["temperature"];
      windspeed = doc["current_weather"]["windspeed"];
      weatherTime = doc["current_weather"]["time"].as<String>();

      Serial.printf("Temp: %.1f°C, Wind: %.1f m/s\n", temp, windspeed);
    } else {
      Serial.printf("HTTP Error: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected for weather update!");
  }
}

// ====== Update OLED ======
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Weather (Open-Meteo)");
  display.println("--------------------");
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");
  display.print("Wind: ");
  display.print(windspeed);
  display.println(" m/s");
  display.println();
  display.println("Updated:");
  display.println(weatherTime);
  display.display();
}

// ====== Webpage HTML ======
String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='60'>";
  html += "<title>ESP8266 Weather</title>";
  html += "<style>body{font-family:Arial;text-align:center;background:#f0f0f0;color:#333;} .card{display:inline-block;margin-top:50px;padding:20px;background:#fff;border-radius:15px;box-shadow:0 0 10px rgba(0,0,0,0.2);} h1{color:#2196f3;}</style></head><body>";
  html += "<div class='card'><h1>Weather (Open-Meteo)</h1>";
  html += "<p><b>Temperature:</b> " + String(temp) + " °C</p>";
  html += "<p><b>Wind Speed:</b> " + String(windspeed) + " m/s</p>";
  html += "<p><b>Updated:</b> " + weatherTime + "</p>";
  html += "<p><small>Auto-refresh every 60s</small></p></div></body></html>";
  return html;
}

// ====== Web Request Handler ======
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP8266 Weather (Open-Meteo) + Web Server");

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Connecting WiFi...");
  display.display();

  // Wi-Fi setup
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_STA, password_STA);
  WiFi.softAP(ssid_AP, password_AP);

  Serial.print("Connecting to ");
  Serial.println(ssid_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("STA IP: "); Serial.println(WiFi.localIP());
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Start web server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");

  // Initial weather fetch
  getWeather();
  updateDisplay();
}

// ====== Loop ======
void loop() {
  server.handleClient();

  // Update weather every minute
  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();
    getWeather();
    updateDisplay();
  }
}
