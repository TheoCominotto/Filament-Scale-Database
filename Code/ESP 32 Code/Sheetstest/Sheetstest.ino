
// -------------------- Wifi

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid     = "your_ssid";
const char* password = "your_password";

String GOOGLE_SCRIPT_ID = "your_GAS"; // Replace by your GAS service id

//updated 04.12.2019
const char * root_ca = \
                       "-----BEGIN CERTIFICATE-----\n" \
                       "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n" \
                       "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n" \
                       "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n" \
                       "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n" \
                       "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
                       "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n" \
                       "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n" \
                       "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n" \
                       "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n" \
                       "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n" \
                       "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n" \
                       "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n" \
                       "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n" \
                       "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n" \
                       "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n" \
                       "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n" \
                       "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n" \
                       "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n" \
                       "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n" \
                       "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n" \
                       "-----END CERTIFICATE-----\n";

WiFiClientSecure client;

// -------------------- RFID
#include <SPI.h>
#include <MFRC522.h>


/*
* RST to D17 (TX2)
* MISO to D19
* MOSI to D23
* SCK to D18
* SDA to D16 (RX2)
*/

#define SS_PIN 16  
#define RST_PIN 17
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

char UID[32] = "";
// --------------------

// -------------------- Load cell
#include <RunningAverage.h>
#include "HX711.h"
// HX711 circuit wiring
//const int LOADCELL_DOUT_PIN = 25;
//const int LOADCELL_SCK_PIN = 26;

const int LOADCELL_DOUT_PIN = 33;
const int LOADCELL_SCK_PIN = 32;

HX711 scale;

float W;

const int avgCount = 50; //Size of the running average array
const int sendTime = 30 * 1000; //Time between database updates in milliseconds
const int readinterval = 120; //Time between two reads of the scale when loaded

RunningAverage rAvg(avgCount);

long lastSend;
long lastAvg;

// -------------------- OLED
#include <Arduino.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int maxSize = 70; //loading bar size
int minSize = 0;
int period = maxSize - minSize;
int loading[] = {1, 4, 8}; //Array to track the loading bar progress; currently only the first element is used.

// -------------------- State machine variables

#define btnPin 15
bool lastBtn = true;

bool loaded = false;
int wThres = 100; //If the weight is above this value, the program assumes a spool is loaded
int wTimeOut = 5000; //the amount of millis the program waits for weight after an RFID detection

//These variables will store the minimum and maximum values of the rAvg to check validity of the data
float minW = 5000.00; //The min value starts off at an arbitrarily large value. I know it's ugly, but it works.
float maxW = 0.00;

//The maximum allowable difference between the largest and the smallest weight value within one send cycle.
//If the difference exceeds this number, the data is deemed unreliable and it will not be sent.
float rangeThresh = 50;


//=================================================== Setup

void setup() {
  Serial.begin(115200);
  delay(10);

  //----------------- OLED

  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  //----------------- Wifi connection

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Started");
  Serial.print("Connecting");

  long startT = millis();

  while (WiFi.status() != WL_CONNECTED) {
    OLED_LoadingBar(&loading[0]);
    delay(50);
    Serial.print(".");

    if (millis() - startT > 5000) //Reset the ESP32 if connection hasn't been established after 5 seconds.
    {
      Serial.println("Wifi timeout");
      ESP.restart();
    }
  }

  //----------------- RFID reader


  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

  //----------------- Load Cell

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  pinMode(btnPin, INPUT_PULLUP);

}


void loop() {

  if (!loaded) //======================== Spool not loaded
  {
    Serial.println("No spool");
    W = 0;
    if (RFIDpresent()) //If true, an RFID tag was detected
    {
      Serial.println("Tag detected! waiting for W");

      long currentT = millis();

      while (millis() - currentT <= wTimeOut) //next, wait until the weight of the spool is detected (or when a timeout occurs)
      {
        if (scale.is_ready()) {
          W = ReadLoadCell(); //read the weight
          Serial.print("W wait = ");
          Serial.println(W);

          u8g2.clearBuffer();
          OLED_print("Tag detected!", (u8g2.getDisplayHeight() / 2) - 4);
          String msg = "Weight = ";
          msg += W;
          msg += "g";
          OLED_print(msg, (u8g2.getDisplayHeight() / 2) + 10);
          u8g2.sendBuffer();
        }

        if (W > wThres) //If the weight is above the threshold, the spool if fully detected! the 'loaded' state is set to true and the loop is stopped.
        {
          Serial.println("W detected! Loaded = true!");
          loaded = true;
          rAvg.fillValue(W, avgCount); //prefill the running average array with the most recent weight value.

          long currentT = millis(); //Reset the timers so that they start from this point in time, as opposed to when the program started
          lastSend = currentT;
          lastAvg = currentT;

          u8g2.clearBuffer();
          OLED_print("Spool detected!", (u8g2.getDisplayHeight() / 2) - 4);
          u8g2.sendBuffer();

          break;
        }
        delay(150); //this delay gives the load cell amplifier enough time to respond
      }
      if (!loaded) {
        Serial.println("timeout!");
        u8g2.clearBuffer();
        OLED_print("Timeout!", (u8g2.getDisplayHeight() / 2) - 4);
        u8g2.sendBuffer();
      }
    }
    else if (scale.is_ready())
    {
      W = ReadLoadCell();

      if (W > wThres) //If weight has been detected, wait until an RFID card is scanned (to confirm a loaded spool) or until the weight is removed
      {
        Serial.println("Weight detected! waiting for RFID");
        while (W > wThres)
        {
          if (scale.is_ready()) //read the load cell to ensure there is stil weight on the holder
          {
            W = ReadLoadCell();
            Serial.println(W);

            u8g2.clearBuffer();
            OLED_print("Weight detected!", (u8g2.getDisplayHeight() / 2) - 14);
            OLED_print("Waiting for RFID...", (u8g2.getDisplayHeight() / 2));
            String msg = "Weight = ";
            msg += W;
            msg += "g";
            OLED_print(msg, (u8g2.getDisplayHeight() / 2) + 14);
            u8g2.sendBuffer();

          }
          if (RFIDpresent()) //if an RFID tag is found, the spool is fully detected! the 'loaded' state is set to true and the loop is stopped.
          {
            Serial.println("RFID detected! Loaded = true!");
            loaded = true;
            rAvg.fillValue(W, avgCount); //prefill the running average array with the most recent weight value.

            long currentT = millis(); //Reset the timers so that they start from this point in time, as opposed to when the program started
            lastSend = currentT;
            lastAvg = currentT;

            u8g2.clearBuffer();
            OLED_print("Spool detected!", (u8g2.getDisplayHeight() / 2) - 4);
            u8g2.sendBuffer();

            break;
          }
          delay(150); //this delay gives the load cell amplifier enough time to respond
        }
        if (!loaded) {
          Serial.println("Weight lifted!");
        };

      }

    }
    else
    {
      u8g2.clearBuffer();
      OLED_print("No spool loaded", (u8g2.getDisplayHeight() / 2) - 4);
      u8g2.sendBuffer();
    }
  }

  else //======================== Spool loaded
  {
    // Serial.println("Spool loaded!");
    long currentT = millis();

    if ((currentT - lastAvg > - readinterval) && (scale.is_ready())) //if yes, it's time to read the scale and update the rAvg
    {
      W = ReadLoadCell();
      rAvg.addValue(W);
      lastAvg = currentT;

      //This keeps track of the maximum and minimum weight over the course of one send cycle
      //This is later used to check the validity of the data
      maxW = max(maxW, W);
      minW = min(minW, W);

      RFIDpresent(); //Checks and updates the UID if a new UID is present. 

      bool btnState = digitalRead(btnPin); //If the button is pressed, a data transmission is forced with the current load cell value
      if(lastBtn == true && btnState == false)
      {
        sendData(UID,W);
      }
      lastBtn = btnState;

      u8g2.clearBuffer();
      String msg = "Spool UID: ";
      msg += UID;
      OLED_print(msg, (u8g2.getDisplayHeight() / 2) - 14);
      msg = "Total W = ";
      msg += W;
      msg += "g";
      OLED_print(msg, (u8g2.getDisplayHeight() / 2));
      msg = "Fil left = ";
      msg += (W - 220);
      msg += "g";
      OLED_print(msg, (u8g2.getDisplayHeight() / 2) + 14);
      u8g2.sendBuffer();
    }

    if (rAvg.getAverage() < wThres) //Check whether the spool if still loaded
    {
      loaded = false;

      //Reset the max and min values for the next spool.
      maxW = 0.00;
      minW = 5000.00;

      Serial.println("Spool unloaded!");
      u8g2.clearBuffer();
      OLED_print("Spool unloaded!", (u8g2.getDisplayHeight() / 2) - 4);
      u8g2.sendBuffer();
      delay(500);
    }
    else
    {
      if (currentT - lastSend >= sendTime) // if yes, it's time to send the data to the database.
      {
        if (dataValid()) //Check if the data is valid before sending
        {
          Serial.print("data=valid    ");
          Serial.println(rAvg.getAverage());


          sendData(UID, rAvg.getAverage());
        }
        else
        {
          Serial.println("Data invalid! Send cycle skipped");
          u8g2.clearBuffer();
          OLED_print("Data invalid!", (u8g2.getDisplayHeight() / 2) - 14);
          OLED_print("Send cycle skipped", (u8g2.getDisplayHeight() / 2));
          u8g2.sendBuffer();
          Serial.println();
          delay(500);
        }
        lastSend = millis();
      }
    }
  }
  delay(2); //for stability
}



//===================================================

void sendData(String UID, int weight) {

  u8g2.clearBuffer();
  String msg = "Sending data!";
  OLED_print(msg, (u8g2.getDisplayHeight() / 2) - 14);
  msg = "Weight =  ";
  msg += weight;
  msg += "g";
  OLED_print(msg, (u8g2.getDisplayHeight() / 2));
  msg = "Fil left = ";
  msg += (weight - 220);
  msg += "g";
  OLED_print(msg, (u8g2.getDisplayHeight() / 2) + 14);
  u8g2.sendBuffer();

  HTTPClient http;
  String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "UID=" + UID + "&W=" + weight;
  //  Serial.print(url);
  //  Serial.print("Making a request");
  http.begin(url, root_ca); //Specify the URL and certificate
  int httpCode = http.GET();
  http.end();
  // Serial.println(": done " + httpCode);


  u8g2.clearBuffer();
  OLED_print("Data sent!", (u8g2.getDisplayHeight() / 2) - 4);
  u8g2.sendBuffer();

  Serial.println("Data sent.");
  Serial.println();
  delay(500);
}

float ReadLoadCell() //Please check if the scale is ready with "if (scale.is_ready())" before calling this function.
{
  //These are used for calibration. Please manually change these values to match up with your setup.
  float zeroPoint = 122899.000;
  float knownW = 404.000;
  float knownWAt = 289446.000;

  float rawW = scale.read();
  rawW -= zeroPoint;
  return (rawW / (knownWAt - zeroPoint)) * knownW;


}

bool dataValid()
{
  if (maxW - minW > rangeThresh) //check difference between max and min value
  {
    //Reset the max and min values for the next cycle.
    maxW = 0.00;
    minW = 5000.00;
    return false;
  }
  else
  {
    //Reset the max and min values for the next cycle.
    maxW = 0.00;
    minW = 5000.00;
    return true;
  }
}
