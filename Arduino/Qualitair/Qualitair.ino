
/**
  Qualit'Air Project
  AMU - MIAGE 2021
   Draft example to be completed during class
*/

/* Dependent libraires: see install & setup document */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "Adafruit_CCS811.h"
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

//////////////////////////
// WiFi Configurations //
//  TO CHANGE         //
///////////////////////
const char WiFiSSID[] = "Jube"; // WiFi access point SSID
const char WiFiPSK[] = "jube@simiane"; // WiFi password - empty string for open access points

//////////////////////////////////////////////
// ThingWorx server definitions            //
//  TO CHANGE                             //
///////////////////////////////////////////
const char TWPlatformBaseURL[] = "http://ec2-15-237-75-188.eu-west-3.compute.amazonaws.com:8080";
const char APP_KEY[] = "29cd186c-7214-4739-b0c0-fb9382a761a0";
const char THING_NAME[] = "HMW.Sensor001";
const int INTERVAL = 5000; //refresh interval

//////////////////////////////////////////////
// Program execution settings              //
//  TO CHANGE                             //
///////////////////////////////////////////
const bool debug=false;


//////////////////////////////////////////////////////////
// Pin Definitions - board specific for Adafruit board //
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

///////////////////
//MOD GPS SETUP //
/////////////////
boolean gpsStatus[] = {false, false, false, false, false, false, false};
unsigned long start;
HardwareSerial gpsSerial(Serial1);
#define BAUDRATE 9600 // this is the default baudrate of the GPS module
TinyGPSPlus gps;

////////////////////
// GPS Functions //
//////////////////

void configureUblox(byte *settingsArrayPointer) {
  byte gpsSetSuccess = 0;
  Serial.println("Configuring u-Blox GPS initial state...");

  //Generate the configuration string for Navigation Mode
  byte setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, *settingsArrayPointer, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  calcChecksum(&setNav[2], sizeof(setNav) - 4);

  //Generate the configuration string for Data Rate
  byte setDataRate[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, settingsArrayPointer[1], settingsArrayPointer[2], 0x01, 0x00, 0x01, 0x00, 0x00, 0x00};
  calcChecksum(&setDataRate[2], sizeof(setDataRate) - 4);

  //Generate the configuration string for Baud Rate
  byte setPortRate[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, settingsArrayPointer[3], settingsArrayPointer[4], settingsArrayPointer[5], 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  calcChecksum(&setPortRate[2], sizeof(setPortRate) - 4);

  byte setGLL[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B};
  byte setGSA[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32};
  byte setGSV[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39};
  byte setRMC[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40};
  byte setVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x46};

  delay(2500);

  while (gpsSetSuccess < 3)
  {
    Serial.print("Setting Navigation Mode... ");
    sendUBX(&setNav[0], sizeof(setNav));  //Send UBX Packet
    gpsSetSuccess += getUBX_ACK(&setNav[2]); //Passes Class ID and Message ID to the ACK Receive function
    if (gpsSetSuccess == 5) {
      gpsSetSuccess -= 4;
      setBaud(settingsArrayPointer[4]);
      delay(1500);
      byte lowerPortRate[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 0xB5};
      sendUBX(lowerPortRate, sizeof(lowerPortRate));
      gpsSerial.begin(9600);
      delay(2000);
    }
    if (gpsSetSuccess == 6) gpsSetSuccess -= 4;
    if (gpsSetSuccess == 10) gpsStatus[0] = true;
  }
  if (gpsSetSuccess == 3) Serial.println("Navigation mode configuration failed.");
  gpsSetSuccess = 0;
  while (gpsSetSuccess < 3) {
    Serial.print("Setting Data Update Rate... ");
    sendUBX(&setDataRate[0], sizeof(setDataRate));  //Send UBX Packet
    gpsSetSuccess += getUBX_ACK(&setDataRate[2]); //Passes Class ID and Message ID to the ACK Receive function
    if (gpsSetSuccess == 10) gpsStatus[1] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("Data update mode configuration failed.");
  gpsSetSuccess = 0;


  while (gpsSetSuccess < 3 && settingsArrayPointer[6] == 0x00) {
    Serial.print("Deactivating NMEA GLL Messages ");
    sendUBX(setGLL, sizeof(setGLL));
    gpsSetSuccess += getUBX_ACK(&setGLL[2]);
    if (gpsSetSuccess == 10) gpsStatus[2] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA GLL Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while (gpsSetSuccess < 3 && settingsArrayPointer[7] == 0x00) {
    Serial.print("Deactivating NMEA GSA Messages ");
    sendUBX(setGSA, sizeof(setGSA));
    gpsSetSuccess += getUBX_ACK(&setGSA[2]);
    if (gpsSetSuccess == 10) gpsStatus[3] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA GSA Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while (gpsSetSuccess < 3 && settingsArrayPointer[8] == 0x00) {
    Serial.print("Deactivating NMEA GSV Messages ");
    sendUBX(setGSV, sizeof(setGSV));
    gpsSetSuccess += getUBX_ACK(&setGSV[2]);
    if (gpsSetSuccess == 10) gpsStatus[4] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA GSV Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while (gpsSetSuccess < 3 && settingsArrayPointer[9] == 0x00) {
    Serial.print("Deactivating NMEA RMC Messages ");
    sendUBX(setRMC, sizeof(setRMC));
    gpsSetSuccess += getUBX_ACK(&setRMC[2]);
    if (gpsSetSuccess == 10) gpsStatus[5] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA RMC Message Deactivation Failed!");
  gpsSetSuccess = 0;

  while (gpsSetSuccess < 3 && settingsArrayPointer[10] == 0x00) {
    Serial.print("Deactivating NMEA VTG Messages ");
    sendUBX(setVTG, sizeof(setVTG));
    gpsSetSuccess += getUBX_ACK(&setVTG[2]);
    if (gpsSetSuccess == 10) gpsStatus[6] = true;
    if (gpsSetSuccess == 5 | gpsSetSuccess == 6) gpsSetSuccess -= 4;
  }
  if (gpsSetSuccess == 3) Serial.println("NMEA VTG Message Deactivation Failed!");

  gpsSetSuccess = 0;
  if (settingsArrayPointer[4] != 0x25) {
    Serial.print("Setting Port Baud Rate... ");
    sendUBX(&setPortRate[0], sizeof(setPortRate));
    setBaud(settingsArrayPointer[4]);
    Serial.println("Success!");
    delay(500);
  }
}
void calcChecksum(byte *checksumPayload, byte payloadSize) {
  byte CK_A = 0, CK_B = 0;
  for (int i = 0; i < payloadSize ; i++) {
    CK_A = CK_A + *checksumPayload;
    CK_B = CK_B + CK_A;
    checksumPayload++;
  }
  *checksumPayload = CK_A;
  checksumPayload++;
  *checksumPayload = CK_B;
}

void sendUBX(byte *UBXmsg, byte msgLength) {
  for (int i = 0; i < msgLength; i++) {
    gpsSerial.write(UBXmsg[i]);
    gpsSerial.flush();
  }
  gpsSerial.println();
  gpsSerial.flush();
}


byte getUBX_ACK(byte *msgID) {
  byte CK_A = 0, CK_B = 0;
  byte incoming_char;
  boolean headerReceived = false;
  unsigned long ackWait = millis();
  byte ackPacket[10] = {0xB5, 0x62, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int i = 0;
  while (1) {
    if (gpsSerial.available()) {
      incoming_char = gpsSerial.read();
      if (incoming_char == ackPacket[i]) {
        i++;
      }
      else if (i > 2) {
        ackPacket[i] = incoming_char;
        i++;
      }
    }
    if (i > 9) break;
    if ((millis() - ackWait) > 1500) {
      Serial.println("ACK Timeout");
      return 5;
    }
    if (i == 4 && ackPacket[3] == 0x00) {
      Serial.println("NAK Received");
      return 1;
    }
  }

  for (i = 2; i < 8 ; i++) {
    CK_A = CK_A + ackPacket[i];
    CK_B = CK_B + CK_A;
  }
  if (msgID[0] == ackPacket[6] && msgID[1] == ackPacket[7] && CK_A == ackPacket[8] && CK_B == ackPacket[9]) {
    Serial.println("Success!");
    Serial.print("ACK Received! ");
    printHex(ackPacket, sizeof(ackPacket));
    return 10;
  }
  else {
    Serial.print("ACK Checksum Failure: ");
    printHex(ackPacket, sizeof(ackPacket));
    delay(1000);
    return 1;
  }
}
void printHex(uint8_t *data, uint8_t length) // prints 8-bit data in hex
{
  char tmp[length * 2 + 1];
  byte first ;
  int j = 0;
  for (byte i = 0; i < length; i++)
  {
    first = (data[i] >> 4) | 48;
    if (first > 57) tmp[j] = first + (byte)7;
    else tmp[j] = first ;
    j++;

    first = (data[i] & 0x0F) | 48;
    if (first > 57) tmp[j] = first + (byte)7;
    else tmp[j] = first;
    j++;
  }
  tmp[length * 2] = 0;
  for (byte i = 0, j = 0; i < sizeof(tmp); i++) {
    Serial.print(tmp[i]);
    if (j == 1) {
      Serial.print(" ");
      j = 0;
    }
    else j++;
  }
  Serial.println();
}

void setBaud(byte baudSetting) {
  if (baudSetting == 0x12) gpsSerial.begin(4800);
  if (baudSetting == 0x4B) gpsSerial.begin(19200);
  if (baudSetting == 0x96) gpsSerial.begin(38400);
  if (baudSetting == 0xE1) gpsSerial.begin(57600);
  if (baudSetting == 0xC2) gpsSerial.begin(115200);
  if (baudSetting == 0x84) gpsSerial.begin(230400);
}


/////////////////////
// WiFi connection. Checks if connection has been made once per second until timeout is reached
// returns TRUE if successful or FALSE if timed out
/////////////////////
boolean connectToWiFi(int timeout) {

  Serial.println("Connecting to: " + String(WiFiSSID));
  WiFi.begin(WiFiSSID, WiFiPSK);

  // loop while WiFi is not connected waiting one second between checks
  uint8_t tries = 0; // counter for how many times we have checked
  while ((WiFi.status() != WL_CONNECTED) && (tries < timeout) ) { // stop checking if connection has been made OR we have timed out
    tries++;
    Serial.printf(".");// print . for progress bar
    Serial.println(WiFi.status());
    delay(2000);
  }
  Serial.println("*"); //visual indication that board is connected or timeout

  if (WiFi.status() == WL_CONNECTED) { //check that WiFi is connected, print status and device IP address before returning
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else { //if WiFi is not connected we must have exceeded WiFi connection timeout
    return false;
  }

}

//////////////////////////////
//create a name for the board that is unique. Manual input
//return name as a String
///////////////////////////////
String getUniqueDeviceName() {

  Serial.println("DeviceID>" + String(THING_NAME));
  return String(THING_NAME);
}

///////////////////////////////
// make HTTP GET to a specific Thing and Propertry on a ThingWorx server
// thingName - Name of Thing on server to make GET from
// property - Property of thingName to make GET from
// returns HTTP response code from server and prints full response
///////////////////////////////
int httpGetPropertry(String thingName, String property) {
  HTTPClient https;
  int httpCode = -1;
  String response = "";
  if (debug) Serial.print("[httpsGetPropertry] begin...");
  String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/" + thingName + "/Properties/" + property ;
  https.begin(fullRequestURL);
  https.addHeader("Accept", ACCEPT_TYPE, false, false);
  https.addHeader("appKey", APP_KEY, false, false);
  if (debug) Serial.println("GET URL>" + fullRequestURL + "<");
  // start connection and send HTTP header
  httpCode = https.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    response = https.getString();
    if (debug) Serial.printf("[httpGetPropertry] response code:%d body>", httpCode);
    if (debug) Serial.println(response + "<\n");
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
int httpPutPropertry(String thingName, String property, String value) {
  HTTPClient httpClient;
  int httpCode = -1;
  String response = "";
  if (debug) Serial.print("[httpPutPropertry] begin...");
  String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/" + thingName + "/Properties/" + property ; //+"?appKey=" + String(appKey);
  httpClient.begin(fullRequestURL);
  httpClient.addHeader("Accept", ACCEPT_TYPE, false, false);
  httpClient.addHeader("Content-Type", ACCEPT_TYPE, false, false);
  httpClient.addHeader("appKey", APP_KEY, false, false);
  if (debug) Serial.println("PUT URL>" + fullRequestURL + "<");
  // start connection and send HTTP header
  String putBody = "{\"" + property + "\":" + value + "}";
  httpCode = httpClient.PUT(putBody);
  if (debug) Serial.println(httpCode);
  // httpCode will be negative on error
  if (httpCode > 0) {
    response = httpClient.getString();
    if (debug) Serial.printf("[httpPutPropertry] response code:%d body>", httpCode);
    if (debug) Serial.println(response + "<\n");
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
int postToThing(String nameOfThing, String endPoint, String postBody) {
  HTTPClient https;
  int httpCode = -1;
  String response = "";
  if (debug) Serial.print("[postToThing] begin...");
  String fullRequestURL = String(TWPlatformBaseURL) + "/Thingworx/Things/" + nameOfThing + "/Services/" + endPoint;
  if (debug) Serial.println("URL>" + fullRequestURL + "<");
  https.begin(fullRequestURL);
  https.addHeader("Accept", "application/json", false, false);
  https.addHeader("Content-Type", "application/json", false, false);
  https.addHeader("appKey", APP_KEY, false, false);
  if (debug) Serial.println("[postToThing] POST body>" + postBody + "<");
  // start connection and send HTTP header
  httpCode = https.POST(postBody);
  // httpCode will be negative on error
  if (httpCode > 0) {
    response = https.getString();
    if (debug) Serial.printf("[postToThing] response code:%d body>", httpCode);
    if (debug) Serial.println(response + "<\n");

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






/////////////////////////////////////////////////
// Board Setup function (launched at startup) //
///////////////////////////////////////////////
void setup() {

  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.printf("Starting...\n");

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("   WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  connectToWiFi(10);
  if (!ccs.begin()) {
    Serial.println("Failed to start sensor! Please check your wiring.");
    while (1);
  }

  // Wait for the sensor to be ready
  while (!ccs.available());
  bme.begin();

  // Init GPS :
  byte settingsArray[] = {0x03, 0xFA, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //
  configureUblox(settingsArray);
}



////////////////////////
// Main Program Loop //
//////////////////////
void loop() {

  String thingName = getUniqueDeviceName(); //unique name for this Thing so many work on one ThingWorx server
  int i = 12;
  while (WiFi.status() == WL_CONNECTED) { //confirm WiFi is connected before looping as long as WiFi is connected
    //int getResponseCode = httpGetPropertry(thingName, "CO2");
    // Get property Values from the sensors
    if (ccs.available()) {
      if (!ccs.readData()) {
        float co2 = ccs.geteCO2();
        float tvoc = ccs.getTVOC();
        float temp = bme.readTemperature();
        float hum = bme.readHumidity();
        float pres = bme.readPressure();
        // publish properties :
        httpPutPropertry(thingName, "CO2", String(co2));
        httpPutPropertry(thingName, "TVOC", String(tvoc));
        httpPutPropertry(thingName, "Humidity", String(hum));
        httpPutPropertry(thingName, "Temperature", String(temp));
        httpPutPropertry(thingName, "Pressure", String(pres));
        // Get Location from the GPS :
        while (gpsSerial.available() > 0)
          if (gps.encode(gpsSerial.read()))
            displayInfo();
        httpPutPropertry(thingName, "Location", "{\"latitude\":\"" + String(gps.location.lat()) + "\",\"longitude\":\"" + gps.location.lng() + "\",\"units\":\"WGS84\"}");
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
