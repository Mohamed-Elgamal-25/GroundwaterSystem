/*
 * Water Quality Monitoring System
 * 
 * This program reads ORP (Oxidation-Reduction Potential) and TDS (Total Dissolved Solids)
 * sensor values, processes them, and sends the data to a server via WiFi.
 * 
 * Features:
 * - ORP measurement with smoothing
 * - TDS measurement
 * - Water level detection
 * - WiFi connectivity
 * - NTP time synchronization
 * - JSON data packaging
 * - Server communication
 * 
 * Hardware Connections:
 * - ORP sensor: Analog pin A2
 * - TDS sensor: Analog pin A0
 * - Water level sensor: Digital pin 3
 * - Status LED: Digital pin 13
 */

#include <WiFiNINA.h>      // For WiFi connectivity
#include <ArduinoJson.h>   // For JSON data formatting
#include <NTPClient.h>     // For network time protocol
#include <WiFiUdp.h>       // For UDP communication with NTP server



// WiFi credentials
const char* ssid = "ssid";     // WiFi network name
const char* password = "password"; // WiFi password

// NTP Client setup for time synchronization
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Update every 60 seconds

// WiFi client for sending data
WiFiClient client;

/**
 * Setup function - runs once at startup
 */
void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    
    // Configure pins
   
    // Connect to WiFi
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        WiFi.begin(ssid, password);
    }
    Serial.println("\nConnected to WiFi");

    // Initialize NTP client for time synchronization
    timeClient.begin();
    timeClient.setTimeOffset(0); // UTC time
    timeClient.update(); // Fetch the time immediately
}

/**
 * Gets the current timestamp in ISO 8601 format
 * 
 * @return String containing the formatted timestamp (YYYY-MM-DDTHH:MM:SSZ)
 */
String getTimestamp() {
    timeClient.update(); // Ensure we have fresh time data
    
    // Get the current epoch time (seconds since Jan 1, 1970)
    unsigned long epochTime = timeClient.getEpochTime();
    
    // Calculate time components
    int hours = ((epochTime / 3600) % 24);
    int minutes = (epochTime / 60) % 60;
    int seconds = epochTime % 60;
    
    // Calculate date components (simplified method)
    int daysSinceEpoch = epochTime / 86400;
    int year = 1970;
    int month = 1;
    int day = 1;
    
    // Year calculation accounting for leap years
    while (daysSinceEpoch > 365) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            if (daysSinceEpoch > 366) {
                daysSinceEpoch -= 366;
                year++;
            }
        } else {
            daysSinceEpoch -= 365;
            year++;
        }
    }
    
    // Month calculation
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29; // February has 29 days in leap years
    }
    
    for (int i = 0; i < 12; i++) {
        if (daysSinceEpoch <= daysInMonth[i]) {
            month = i + 1;
            day = daysSinceEpoch;
            break;
        }
        daysSinceEpoch -= daysInMonth[i];
    }
    
    // Adjust for local timezone (UTC+3 in this case)
    hours = hours + 3;
    if (hours >= 24) {
        hours -= 24;
        day += 1;
    }
    
    // Handle month overflow from day adjustment
    if (day > daysInMonth[month-1]) {
        day = 1;
        month += 1;
        if (month > 12) {
            month = 1;
            year += 1;
        }
    }

    // Format as ISO 8601 string (YYYY-MM-DDTHH:MM:SSZ)
    char timestamp[25];
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ", 
            year, month, day, hours, minutes, seconds);
    return String(timestamp);
}

/**
 * Sends sensor data to the server
 * 
 * @param location The location identifier
 * @param tdsValue The TDS measurement value
 * @param orpValue The ORP measurement value
 * @param waterLevel The water level reading
 */
void sendData(int location, float tdsValue, float orpValue, int waterLevel) {
    if (WiFi.status() == WL_CONNECTED) {
        // Create JSON document
        StaticJsonDocument<200> doc;
        doc["timestamp"] = getTimestamp();
        doc["location"] = location;
        doc["TDS"] = tdsValue;
        doc["ORP"] = orpValue;
        doc["waterLevel"] = waterLevel;

        // Serialize JSON to string
        String jsonData;
        serializeJson(doc, jsonData);

        // Connect to server and send data
        if (client.connect("192.168.10.6", 3000)) {
            client.println("POST /api/mongodb HTTP/1.1");
            client.println("Host: 192.168.10.6");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.print("Content-Length: ");
            client.println(jsonData.length());
            client.println();
            client.println(jsonData);

            Serial.println("Data sent: " + jsonData);
            client.stop();
        } else {
            Serial.println("Failed to connect to server!");
        }
    }
}

/**
 * Main program loop - runs continuously
 */
void loop() {
    
    // Send data to server
    sendData(0, tdsValue, orpValue, waterLevel);
    
    // Delay between readings
    delay(500);
}