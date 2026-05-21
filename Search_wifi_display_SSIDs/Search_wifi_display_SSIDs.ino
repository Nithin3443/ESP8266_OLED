#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//#include <WiFi.h> 

// Set up the SSID and password for the Access Point
const char *ssid = "ESP8266-AP";          // Name of the Wi-Fi network (AP)
const char *password = "123456789";       // Password for the AP
ESP8266WebServer server(80);  // Create a web server that listens on port 80 (default HTTP port)

// HTML content for the web page
const char* htmlContent = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>ESP8266 Web Server</title>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }
      h1 { color: #4CAF50; }
      p { color: #555; font-size: 20px; }
      button { font-size: 20px; padding: 10px 20px; cursor: pointer; }
    </style>
  </head>
  <body>
    <h1>Welcome to ESP8266 Web Server</h1>
    <p>This is a simple web page hosted by the ESP8266 in AP mode.</p>
    <p><button onclick="window.location.href='/toggleLED'">Toggle LED</button></p>
  </body>
  </html>
)rawliteral";

// Pin for an onboard LED (you can change this pin if using an external LED)
const int ledPin = LED_BUILTIN;  // LED_BUILTIN is the default LED pin on most ESP8266 boards


SSD1306Wire display(0x3c, D6, D5);  // Initialize with I2C address, SDA, and SCL pins

//byte smile[8]=   {0x3C,0x42,0xA5,0x81,0xA5,0x99,0x42,0x3C};
String serialInput = ""; 
String lines[4];  // Array to store up to 4 lines (adjust as needed)
int currentLine = 0;  // Keep track of the current line to overwrite when the buffer is full


void setup() {

  Serial.begin(115200);
  
  // Set up the LED pin
  pinMode(ledPin, OUTPUT);

  // Initialize the display
  display.init();
  display.flipScreenVertically();  // Flip the screen vertically to match orientation
  display.setFont(ArialMT_Plain_16); 
  // Clear the display initially
  display.clear();
  // Update the display
  display.display();
  
  // Start the Wi-Fi AP
  WiFi.softAP(ssid, password);  // Set up ESP8266 as AP with the specified SSID and password
  IPAddress ip = WiFi.softAPIP();
  String ipString = ip.toString();
  // Print IP Address of the ESP8266
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(ip);  // Get the IP address of the AP
  lines[1]="Access Point Started";
  lines[1]="ESP8266-AP";
  lines[2]="123456789";
  lines[3]=ipString;
  for (int i = 0; i < 4; i++) {  // Limit to 4 networks to fit the screen
      display.drawString(0, i * 16, lines[i]);  // Display each line
    }
    display.display();

  
  // Define what to do when the "/toggleLED" endpoint is accessed
  server.on("/toggleLED", HTTP_GET, []() {
    digitalWrite(ledPin, !digitalRead(ledPin));  // Toggle the LED state
    server.send(200, "text/html", htmlContent);  // Respond with the same page
  });
  
  // Handle root ("/") requests to show the HTML page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlContent);  // Serve the HTML page
  });

  // Start the server
  server.begin();

    
  
}


void loop() {

server.handleClient();
}
