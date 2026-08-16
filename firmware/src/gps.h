#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

struct GPSData {
  float latitude;
  float longitude;
  float altitude;
  int satellites;
  bool valid;
};

class GPSModule {
private:
  HardwareSerial gpsSerial;
  GPSData currentData;
  String nmeaBuffer;
  unsigned long lastUpdate;
  bool useSerial1; // Flag to determine which serial port to use
  
  void parseNMEA(String sentence) {
    // Parse GPGGA sentence
    if (sentence.startsWith("$GPGGA") || sentence.startsWith("$GNGGA")) {
      int commaPos[15];
      int commaCount = 0;
      
      for (int i = 0; i < sentence.length() && commaCount < 15; i++) {
        if (sentence.charAt(i) == ',') {
          commaPos[commaCount++] = i;
        }
      }
      
      if (commaCount >= 9) {
        // Latitude
        String lat = sentence.substring(commaPos[1] + 1, commaPos[2]);
        String latDir = sentence.substring(commaPos[2] + 1, commaPos[3]);
        
        // Longitude
        String lon = sentence.substring(commaPos[3] + 1, commaPos[4]);
        String lonDir = sentence.substring(commaPos[4] + 1, commaPos[5]);
        
        // Altitude
        String alt = sentence.substring(commaPos[8] + 1, commaPos[9]);
        
        // Satellites
        String sats = sentence.substring(commaPos[6] + 1, commaPos[7]);
        
        if (lat.length() > 0 && lon.length() > 0) {
          currentData.latitude = convertToDecimalDegrees(lat, latDir);
          currentData.longitude = convertToDecimalDegrees(lon, lonDir);
          currentData.altitude = alt.toFloat();
          currentData.satellites = sats.toInt();
          currentData.valid = true;
        }
      }
    }
  }
  
  float convertToDecimalDegrees(String coord, String direction) {
    if (coord.length() < 4) return 0;
    
    int dotPos = coord.indexOf('.');
    if (dotPos < 0) return 0;
    
    // Extract degrees and minutes
    float degrees = coord.substring(0, dotPos - 2).toFloat();
    float minutes = coord.substring(dotPos - 2).toFloat();
    
    float decimal = degrees + (minutes / 60.0);
    
    if (direction == "S" || direction == "W") {
      decimal = -decimal;
    }
    
    return decimal;
  }
  
public:
  GPSModule() : gpsSerial(1) {  // Using Serial1 for A9G
    currentData.valid = false;
    currentData.latitude = 0;
    currentData.longitude = 0;
    currentData.altitude = 0;
    currentData.satellites = 0;
    useSerial1 = true;
  }
  
  void begin() {
    pinMode(A9G_POWER, OUTPUT);
    digitalWrite(A9G_POWER, HIGH);
    
    // A9G on Serial1 using GPIO 32 (TX) and 33 (RX)
    gpsSerial.begin(115200, SERIAL_8N1, A9G_RX, A9G_TX);
    delay(1000);
    
    // Initialize A9G module
    sendCommand("AT");
    delay(500);
    sendCommand("AT+GPS=1"); // Enable GPS
    delay(500);
    sendCommand("AT+GPSLP=2"); // GPS low power mode off
    
    Serial.println("A9G GPS module initialized");
  }
  
  void sendCommand(String cmd) {
    gpsSerial.println(cmd);
    Serial.print("GPS CMD: ");
    Serial.println(cmd);
  }
  
  void update() {
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      
      if (c == '\n') {
        if (nmeaBuffer.length() > 0) {
          parseNMEA(nmeaBuffer);
          nmeaBuffer = "";
        }
      } else if (c != '\r') {
        nmeaBuffer += c;
      }
    }
  }
  
  GPSData getLocation() {
    return currentData;
  }
  
  String getLocationString() {
    if (!currentData.valid) {
      return "GPS: No Fix";
    }
    
    return "Lat: " + String(currentData.latitude, 6) + 
           ", Lon: " + String(currentData.longitude, 6) +
           ", Sats: " + String(currentData.satellites);
  }
};

#endif
