#ifndef SECRETS_H
#define SECRETS_H

const char* SSID_NAME = "WiFi-A4AC";
const char* WIFI_PASSWORD = "56246322";

struct Location {
  String name;
  float lat;
  float lon;
  String timezone;
};

const int NUM_LOCATIONS = 2;

const Location locations[NUM_LOCATIONS] = {
  {
    "Kings Langley", 
    -33.7493, 
    150.9316, 
    "AEST-10AEDT,M10.1.0,M4.1.0/3" // Sydney Time
  },
  {
    "Bangalore", 
    12.9806, 
    77.5965, 
    "IST-5:30" // India Time
  }
};

#endif
