
/**
 * 
 * ESP8266_ThingWorx_REST_Demo.ino
 *
 *  
 *  (c) PTC, Inc. 2016-2020
 *
 */


#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>


//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "Jube"; // WiFi access point SSID
const char WiFiPSK[] = "jube@simiane"; // WiFi password - empty string for open access points

//////////////////////////////////////////////
// ThingWorx server definitions            //
//  modify for a specific platform instance //
//////////////////////////////////////////////
const char TWPlatformBaseURL[] = "https://pp-2007011431nt.devportal.ptc.io";
const char appKey[] = "2d4e9440-3e51-452f-a057-b55d45289264";

////////////////////////////////////////////////////////
// Pin Definitions - board specific for Adafruit board//
////////////////////////////////////////////////////////
const int RED_LED = 0; // Thing's onboard, red LED - 
const int BLUE_LED = 2; // Thing's onboard, blue LED
const int ANALOG_PIN = 0; // The only analog pin on the Thing

const int OFF = HIGH;
const int ON = LOW;

// this will set as the Accept header for all the HTTP requests to the ThingWorx server
// valid values are: application/json, text/xml, text/csv, text/html (default)
#define ACCEPT_TYPE "text/csv"  

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

    String uniqueName;
    uniqueName = "Sensor001";
    Serial.println("DeviceID>" + uniqueName);
    return uniqueName;
}

///////////////////////////////
// make HTTP GET to a specific Thing and Propertry on a ThingWorx server
// thingName - Name of Thing on server to make GET from
// property - Property of thingName to make GET from
// returns HTTP response code from server and prints full response
///////////////////////////////
int httpGetPropertry(String thingName, String property){
    std::unique_ptr<WiFiClientSecure>client(new WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
        int httpCode = -1;
        String response = "";
        Serial.print("[httpsGetPropertry] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ thingName +"/Properties/"+ property +"?appKey=" + String(appKey);

        https.begin(*client,fullRequestURL);
        https.addHeader("Accept",ACCEPT_TYPE,false,false);
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
int httpPutPropertry(String thingName, String property){
    std::unique_ptr<WiFiClientSecure>client(new WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
        int httpCode = -1;
        String response = "";
        Serial.print("[httpsGetPropertry] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ thingName +"/Properties/"+ property +"?appKey=" + String(appKey);

        https.begin(*client,fullRequestURL);
        https.addHeader("Accept",ACCEPT_TYPE,false,false);
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
// make HTTP POST to ThingWorx server Thing service
// nameOfThing - Name of Thing to POST to
// endPoint - Services URL to invoke
// postBody - Body of POST to send to ThingWorx platform
// returns HTTP response code from server
///////////////////////////////
int postToThing(String nameOfThing, String endPoint, String postBody){
  
        std::unique_ptr<WiFiClientSecure>client(new WiFiClientSecure);
      client->setInsecure();
    HTTPClient https;
        int httpCode = -1;
        String response = "";
        Serial.print("[postToThing] begin...");
        String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/"+ nameOfThing +"/Services/"+ endPoint +"?appKey=" + String(appKey);
        Serial.println("URL>" + fullRequestURL + "<");
        https.begin(*client,fullRequestURL);
        https.addHeader("Accept",ACCEPT_TYPE,false,false);
        https.addHeader("Content-Type","application/json",false,false);
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

}

void loop() {

    String thingName = getUniqueDeviceName(); //unique name for this Thing so many work on one ThingWorx server

    while (WiFi.status() == WL_CONNECTED) { //confirm WiFi is connected before looping as long as WiFi is connected

      int getResponseCode = httpGetPropertry(thingName, "SomeNumber");

      if (getResponseCode == 404){ // a 404 means connected, but either no Thing or no property
        
        // Service invoke : 
        //postResp = postToThing(thingName,"RestartThing",""); //POST to RestartThing endpoint with no body
        // Property put : 
        
      }

      delay(2000);

    }// end WiFi connected while loop
    Serial.printf("****Wifi connection dropped****\n");
    WiFi.disconnect(true);
    delay(10000);
    connectToWiFi(10);
}