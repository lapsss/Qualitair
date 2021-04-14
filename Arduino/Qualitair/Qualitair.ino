
/**
* Qualit'Air Demo
* AMU - MIAGE 2021
 *
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "Adafruit_CCS811.h"
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "Jube"; // WiFi access point SSID
const char WiFiPSK[] = "jube@simiane"; // WiFi password - empty string for open access points

//////////////////////////////////////////////
// ThingWorx server definitions            //
//  modify for a specific platform instance //
//////////////////////////////////////////////
const char TWPlatformBaseURL[] = "http://ec2-35-180-37-49.eu-west-3.compute.amazonaws.com:8080";
const char APP_KEY[] = "29cd186c-7214-4739-b0c0-fb9382a761a0";
const char THING_NAME[] = "HMW.Sensor001";
const int INTERVAL = 5000; //refresh interval
////////////////////////////////////////////////////////
// Pin Definitions - board specific for Adafruit board//
////////////////////////////////////////////////////////
Adafruit_CCS811 ccs;
Adafruit_BME280 bme; // I2C
const int RED_LED = 0; // Thing's onboard, red LED - 
const int BLUE_LED = 2; // Thing's onboard, blue LED
const int ANALOG_PIN = 0; // The only analog pin on the Thing

const int OFF = HIGH;
const int ON = LOW;

// this will set as the Accept header for all the HTTP requests to the ThingWorx server
// valid values are: application/json, text/xml, text/csv, text/html (default)
#define ACCEPT_TYPE "application/json"  


//GPS SETUP


// A sample NMEA stream.
const char *gpsStream =
  "$GPRMC,045103.000,A,3014.1984,N,09749.2872,W,0.67,161.46,030913,,,A*7C\r\n"
  "$GPGGA,045104.000,3014.1985,N,09749.2873,W,1,09,1.2,211.6,M,-22.5,M,,0000*62\r\n"
  "$GPRMC,045200.000,A,3014.3820,N,09748.9514,W,36.88,65.02,030913,,,A*77\r\n"
  "$GPGGA,045201.000,3014.3864,N,09748.9411,W,1,10,1.2,200.8,M,-22.5,M,,0000*6C\r\n"
  "$GPRMC,045251.000,A,3014.4275,N,09749.0626,W,0.51,217.94,030913,,,A*7D\r\n"
  "$GPGGA,045252.000,3014.4273,N,09749.0628,W,1,09,1.3,206.9,M,-22.5,M,,0000*6F\r\n";

// The TinyGPS++ object
TinyGPSPlus gps;


/////////////////////
//Attempt to make a WiFi connection. Checks if connection has been made once per second until timeout is reached
//returns TRUE if successful or FALSE if timed out
/////////////////////
boolean connectToWiFi(int timeout){

  Serial.println("Connecting to: " + String(WiFiSSID));
  WiFi.begin(WiFiSSID, WiFiPSK);

  // loop while WiFi is not connected waiting one second between checks
  uint8_t tries = 0; // counter for how many times we have checked
  while ((WiFi.status() != WL_CONNECTED) && (tries < timeout) ){ // stop checking if connection has been made OR we have timed out
    tries++;
    Serial.printf(".");// print . for progress bar
    Serial.println(WiFi.status());
    delay(2000);
  }
  Serial.println("*"); //visual indication that board is connected or timeout

  if (WiFi.status() == WL_CONNECTED){ //check that WiFi is connected, print status and device IP address before returning
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else { //if WiFi is not connected we must have exceeded WiFi connection timeout
    return false;
  }

}

//////////////////////////
//create a name for the board that is probably unique by appending last two bytes of MAC address
//return name as a String
///////////////////////////////
String getUniqueDeviceName(){ 

    Serial.println("DeviceID>" + String(THING_NAME));
    return String(THING_NAME);
}

///////////////////////////////
// make HTTP GET to a specific Thing and Propertry on a ThingWorx server
// thingName - Name of Thing on server to make GET from
// property - Property of thingName to make GET from
// returns HTTP response code from server and prints full response
///////////////////////////////
int httpGetPropertry(String thingName, String property){
        HTTPClient https;
        int httpCode = -1;
        String response = "";
        Serial.print("[httpsGetPropertry] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ thingName +"/Properties/"+ property ;//"?appKey=" + String(appKey);
        https.begin(fullRequestURL);
        https.addHeader("Accept",ACCEPT_TYPE,false,false);
        https.addHeader("appKey",APP_KEY,false,false);
        Serial.println("GET URL>" + fullRequestURL +"<");
        // start connection and send HTTP header
        httpCode = https.GET();
        // httpCode will be negative on error
        if(httpCode > 0) {
            response = https.getString();
            Serial.printf("[httpGetPropertry] response code:%d body>",httpCode);
            Serial.println(response + "<\n");
        } else {
            Serial.printf("[httpGetPropertry] failed, error: %s\n\n", https.errorToString(httpCode).c_str());
        }
        https.end();
        return httpCode;

}


///////////////////////////////
// make HTTP PUT to a specific Thing and Propertry on a ThingWorx server
// thingName - Name of Thing on server to execute the PUT
// property - Property of thingName to make PUT request
// returns HTTP response code from server and prints full response
///////////////////////////////
int httpPutPropertry(String thingName, String property, String value){
        HTTPClient httpClient;
        int httpCode = -1;
        String response = "";
        Serial.print("[httpPutPropertry] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ thingName +"/Properties/"+ property ; //+"?appKey=" + String(appKey);
        httpClient.begin(fullRequestURL);
        httpClient.addHeader("Accept",ACCEPT_TYPE,false,false);
        httpClient.addHeader("Content-Type",ACCEPT_TYPE,false,false);
        httpClient.addHeader("appKey",APP_KEY,false,false);
        Serial.println("PUT URL>" + fullRequestURL +"<");
        // start connection and send HTTP header
        String putBody="{\""+property+"\":"+ value+"}";
        httpCode = httpClient.PUT(putBody);
        Serial.println(httpCode);
        // httpCode will be negative on error
        if(httpCode > 0) {
            response = httpClient.getString();
            Serial.printf("[httpPutPropertry] response code:%d body>",httpCode);
            Serial.println(response + "<\n");
        } else {
            Serial.printf("[httpPutPropertry] failed, error: %s\n\n", httpClient.errorToString(httpCode).c_str());
        }
        httpClient.end();
        return httpCode;

}





///////////////////////////////
// make HTTP POST to ThingWorx server Thing service
// nameOfThing - Name of Thing to POST to
// endPoint - Services URL to invoke
// postBody - Body of POST to send to ThingWorx platform
// returns HTTP response code from server
///////////////////////////////
int postToThing(String nameOfThing, String endPoint, String postBody){
        HTTPClient https;
        int httpCode = -1;
        String response = "";
        Serial.print("[postToThing] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ nameOfThing +"/Services/"+ endPoint;
        Serial.println("URL>" + fullRequestURL + "<");
        https.begin(fullRequestURL);
        https.addHeader("Accept","application/json",false,false);
        https.addHeader("Content-Type","application/json",false,false);
        https.addHeader("appKey",APP_KEY,false,false);
        Serial.println("[postToThing] POST body>" + postBody + "<");
        // start connection and send HTTP header
        httpCode = https.POST(postBody);
        // httpCode will be negative on error
        if(httpCode > 0) {
            response = https.getString();
            Serial.printf("[postToThing] response code:%d body>",httpCode);
            Serial.println(response + "<\n");

        } else {
            Serial.printf("[postToThing] POST... failed, error: %s\n\n", https.errorToString(httpCode).c_str());
        }
        https.end();
        return httpCode;
}


// GPS Display info
void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}




void setup() {

    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.printf("Starting...\n");

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("   WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    connectToWiFi(10);
 if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());
  bme.begin();  

  // Init GPS : 
   while (*gpsStream)
    if (gps.encode(*gpsStream++))
      displayInfo();
}

void loop() {

    String thingName = getUniqueDeviceName(); //unique name for this Thing so many work on one ThingWorx server
    int i = 12;
    while (WiFi.status() == WL_CONNECTED) { //confirm WiFi is connected before looping as long as WiFi is connected
        //int getResponseCode = httpGetPropertry(thingName, "CO2");
        // Get property Values from the sensors
        if(ccs.available()){
          if(!ccs.readData()){
        float co2 = ccs.geteCO2();
        float tvoc = ccs.getTVOC();
        float temp = bme.readTemperature();
        float hum = bme.readHumidity();
        float pres = bme.readPressure();
        // publish properties : 
        httpPutPropertry(thingName,"CO2",String(co2));
        httpPutPropertry(thingName,"TVOC",String(tvoc));
        httpPutPropertry(thingName,"Humidity",String(hum));
        httpPutPropertry(thingName,"Temperature",String(temp));
        httpPutPropertry(thingName,"Pressure",String(pres));
        // Refresh location : 
        if (gps.encode(*gpsStream++))
          displayInfo();
        httpPutPropertry(thingName,"Location","{\"latitude\":\""+String(gps.location.lat())+"\",\"longitude\":\""+gps.location.lng()+"\",\"units\":\"WGS84\"}");
         }
        }
      i++;
      // Sends every INTERVAL mseconds
      delay(INTERVAL);
    }// end WiFi connected while loop
    Serial.printf("****Wifi connection dropped****\n");
    WiFi.disconnect(true);
    delay(10000);
    connectToWiFi(10);
}
