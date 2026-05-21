#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
#include <ESP8266WiFi.h>
//#include <WiFi.h> 

SSD1306Wire display(0x3c, D6, D5);  // Initialize with I2C address, SDA, and SCL pins

//byte smile[8]=   {0x3C,0x42,0xA5,0x81,0xA5,0x99,0x42,0x3C};
String serialInput = ""; 
String lines[4];  // Array to store up to 4 lines (adjust as needed)
int currentLine = 0;  // Keep track of the current line to overwrite when the buffer is full


void setup() {
  Serial.begin(115200);
  Serial.println("Initializing Wi-Fi scan...");

  // Initialize the display
  display.init();
  display.flipScreenVertically();  // Flip the screen vertically to match orientation
  display.setFont(ArialMT_Plain_16); 
  // Clear the display initially
  display.clear();
  // Update the display
  display.display();


  // Start Wi-Fi scan
  WiFi.mode(WIFI_STA);  // Set Wi-Fi mode to Station (client)
  delay(2000);  // Wait for Wi-Fi to initialize
  Serial.println("Scanning for Wi-Fi networks...");
  int n = WiFi.scanNetworks();  // Scan for available Wi-Fi networks
  Serial.println("Scan done.");
  
  // Check if any networks are found
  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
     // Display the available networks on OLED
    display.clear();
    for (int i = 0; i < n && i < 4; i++) {  // Limit to 4 networks to fit the screen
      String network = WiFi.SSID(i);  // Get the SSID of the network
      lines[i] = "SSID: " + network;
      display.drawString(0, i * 16, lines[i]);  // Display each SSID
    }
    display.display();
  }
}
void loop() {

delay(10000);
}
