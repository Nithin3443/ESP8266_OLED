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

// ====== Weather API ======
// Get free API key from https://openweathermap.org/api
const String apiKey = "YOUR_API_KEY";
const String city = "Brisbane";
const String units = "metric"; // "imperial" for Fahrenheit
String weather = "--", description = "--";
float temp = 0;
int humidity = 0;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 1 minute

// ====== Function to fetch weather data ======
void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=-33.7501&longitude=150.9354&hourly=temperature_2m,rain&timezone=Australia%2FSydney";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      weather = doc["weather"][0]["main"].as<String>();
      description = doc["weather"][0]["description"].as<String>();
      temp = doc["main"]["temp"].as<float>();
      humidity = doc["main"]["humidity"].as<int>();
    }
    http.end();
  }
}

// ====== Function to update OLED ======
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Weather in " + city);
  display.println("--------------------");
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  display.print("Cond: ");
  display.println(weather);
  display.display();
}

// ====== Webpage HTML ======
String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP8266 Weather</title>";
  html += "<style>body{font-family:Arial;text-align:center;background:#f5f5f5;color:#333;} .card{display:inline-block;padding:20px;margin-top:50px;border-radius:15px;background:#fff;box-shadow:0 0 10px rgba(0,0,0,0.1);} h1{color:#2196f3;} </style></head><body>";
  html += "<div class='card'><h1>Weather in " + city + "</h1>";
  html += "<p><b>Temperature:</b> " + String(temp) + " °C</p>";
  html += "<p><b>Humidity:</b> " + String(humidity) + " %</p>";
  html += "<p><b>Condition:</b> " + weather + "</p>";
  html += "<p><i>" + description + "</i></p>";
  html += "<p>Last Updated: " + String(millis() / 1000) + " sec ago</p></div></body></html>";
  return html;
}

// ====== Handle Web Request ======
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP8266 Weather Display + Web Server");

  // ===== Initialize Display =====
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

  // ===== WiFi Mode: AP + STA =====
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_STA, password_STA);
  WiFi.softAP(ssid_AP, password_AP);

  Serial.print("Connecting to ");
  Serial.println(ssid_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("STA IP: "); Serial.println(WiFi.localIP());
  Serial.print("AP IP: ");  Serial.println(WiFi.softAPIP());

  // ===== Start Web Server =====
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");

  // ===== Initial Weather Fetch =====
  getWeather();
  updateDisplay();
}

void loop() {
  server.handleClient();

  // Update weather every minute
  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();
    getWeather();
    updateDisplay();
  }
}
