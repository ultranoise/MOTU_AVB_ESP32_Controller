///////////////////////////////////////
// AVB VOLUME KNOB
// PRESS KNOB FOR PANIC MODE (-24DB)
// enrique.tomas@kunstuni-linz.at 2022
// use this code at your own risk,
// no responsabilities granted. 
//////////////////////////////////////

#include "SPI.h"
#include "TFT_eSPI.h"
#include "Free_Fonts.h" // Include the header file attached to this sketch
// FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define LOAD_GFXFF

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 5
#include <ETH.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#define USE_SERIAL Serial
static bool eth_connected = false;

WiFiMulti wifiMulti;

const char* server = "http://192.168.0.101/datastore/mix/chan/0/matrix/fader";


//Sensor vars
ESP32Encoder encoder; //encoder variable
int encoderValue = 0;
int encoderValue_past = 0;
int encoderIncrement = 0;
int encoderIncrement_past;
int pushButton = 35; //push button in the encoder
int pushValue = 1;
int pushValue_past = 0;
bool newIncValue = false;

//fader values etc
float faderVolume;
float dbVolume;
float incVol;
bool firstConnection = true;
bool in_connection = true;
bool init_end = false;

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

unsigned long drawTime = 0;
int xpos, ypos;

String devices_ip[10];
String uids[10];
String IPdevices2control[10];
int counter_device2control = 0;
int devices_number = 0;
int selected_device = 0;

int studio = 2;  //0 -> tamlab, 1->alter bauhof, 2-> tamlab second rack

//controlling various faders
//we have to build 'json={"0/matrix/fader":"0.2","1/matrix/fader":"0.1"}' http://192.168.0.5/datastore/mix/chan/
    
StaticJsonDocument<400> doc_mul;

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      USE_SERIAL.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      USE_SERIAL.println("ETH Connected");
      //eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      USE_SERIAL.print("ETH MAC: ");
      USE_SERIAL.print(ETH.macAddress());
      USE_SERIAL.print(", IPv4: ");
      USE_SERIAL.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        USE_SERIAL.print(", FULL_DUPLEX");
      }
      USE_SERIAL.print(", ");
      USE_SERIAL.print(ETH.linkSpeed());
      USE_SERIAL.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      USE_SERIAL.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      USE_SERIAL.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}




float retrieveDevices(String ip_devices)
{
  HTTPClient http;

        USE_SERIAL.println(" ");
        USE_SERIAL.print("retrieving UUID information...\n");
        // configure traged server and url
        
        http.begin(ip_devices + "/datastore/avb/devs"); 

        //USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                
                //USE_SERIAL.println("payload");
                //USE_SERIAL.println(payload);

                //fader payload is a String like {"value":0.7}
                //from where we have to extract the value
                             
                DynamicJsonDocument doc(200);
                DeserializationError error =deserializeJson(doc, payload);

                // Test if parsing succeeds.
                if (error) {
                  
                  USE_SERIAL.print(F("deserializeJson() failed: "));
                  USE_SERIAL.println(error.f_str());
                  http.end();
                  return -1.0;
                } else {
                  String f = doc["value"];
                  USE_SERIAL.println("UID values");
                  USE_SERIAL.println(f);

                  int l = f.length();  //length of the string obtained e.g. 0001f2fffe008f93:0001f2fffe00f7c9
                  //USE_SERIAL.println(l);
                  
                  int n = l / 16; //each device is 16 characters, separated by :, n is the number of devices
                  USE_SERIAL.print("number of devices: ");
                  USE_SERIAL.println(n);

                  //necessary variables to extract each uid and ip:
                  
                  HTTPClient http2;

                  String uid_ip;
                  String uid_path;
                  
                  //RETRIEVE IPS OF EACH DEVICE
                  USE_SERIAL.println(" ");
                  USE_SERIAL.print("retrieving IP information of each device...\n");
                  
                  for(int i=0;i<n;i++) {
                    uids[i] = f.substring(0 + i*17, 16 + i*17);  

                    USE_SERIAL.print("Device: ");
                    USE_SERIAL.println(uids[i]);   
                    USE_SERIAL.println("index: " + i);
                       
                    // configure traged server and url
                    uid_path = ip_devices + "/datastore/avb/" + uids[i] + "/url";
                          
                    //USE_SERIAL.println(uid_path);
                    http2.begin(uid_path); 
                  
                    
                    // start connection and send HTTP header
                    httpCode = http2.GET();
                  
                    // httpCode will be negative on error
                    if(httpCode > 0) {
                                          
                      // file found at server
                      if(httpCode == HTTP_CODE_OK) {
                        
                          String payload2 = http2.getString();
                                  
                          //USE_SERIAL.println("payload");
                          USE_SERIAL.println(payload2);
                  
                          //fader payload is a String like {"value":0.7}
                          //from where we have to extract the value
                          DynamicJsonDocument doc2(200);
                          DeserializationError error2 = deserializeJson(doc2, payload2);
                  
                           // Test if parsing succeeds.
                           if (error2) {
                              USE_SERIAL.print(F("deserializeJson() failed: "));
                              USE_SERIAL.println(error2.f_str());
                              http2.end();
                              return -1.0;
                              
                            } else {
                              String ff = doc2["value"];
                              devices_ip[i] = ff;
                              devices_number = devices_number + 1;
                              USE_SERIAL.print("IP value: ");
                              USE_SERIAL.println(ff); 
                              USE_SERIAL.println("index: " + i);
                            }
                                  
                                  
                        }
                      } else {
                              USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                              http2.end();
                              return -1.0;
                          }

                          http2.end();
                                  
                    //END OF IP RETRIEVEMENT
                    
                  }//END OF FOR 
          
                  
                  http.end();
                  return 1.0;
                }
                
                
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            http.end();
            return -1.0;
        }
        
        
}


float getTrimValue() {
  HTTPClient http;

  //USE_SERIAL.print("[HTTP] begin...\n");
  // configure traged server and url
  String path = "http://" + IPdevices2control[0] + "/datastore/ext/obank/0/ch/0/trim";  //try with the first obank
  USE_SERIAL.println("retrieving fader values from the following path: ");
  USE_SERIAL.println(path);
  
  http.begin(path); 
  

  //USE_SERIAL.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          
          USE_SERIAL.println("payload");
          USE_SERIAL.println(payload);

          //fader payload is a String like {"value":0.7}
          //from where we have to extract the value
          
          
          DynamicJsonDocument doc(200);
          DeserializationError error =deserializeJson(doc, payload);

          // Test if parsing succeeds.
          if (error) {
            
            USE_SERIAL.print(F("deserializeJson() failed: "));
            USE_SERIAL.println(error.f_str());
            http.end();
            return -1.0;
          } else {
            float f = doc["value"];
            USE_SERIAL.println("extracted value");
            USE_SERIAL.println(f);
            http.end();
            return f;
          }
          
          
      }
  } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      return -1.0;
  }

               
}


float getFaderValue() {
  HTTPClient http;

        //USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        String path = "http://" + devices_ip[selected_device] + "/datastore/mix/chan/0/matrix/fader";
        http.begin(path); 
        //http.begin("http://192.168.0.101/datastore/mix/chan/0/matrix/fader"); 

        //USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                
                //USE_SERIAL.println("payload");
                //USE_SERIAL.println(payload);

                //fader payload is a String like {"value":0.7}
                //from where we have to extract the value
                
                
                DynamicJsonDocument doc(200);
                DeserializationError error =deserializeJson(doc, payload);

                // Test if parsing succeeds.
                if (error) {
                  
                  USE_SERIAL.print(F("deserializeJson() failed: "));
                  USE_SERIAL.println(error.f_str());
                  http.end();
                  return -1.0;
                } else {
                  float f = doc["value"];
                  //USE_SERIAL.println("extracted value");
                  //USE_SERIAL.println(f);
                  http.end();
                  return f;
                }
                
                
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            http.end();
            return -1.0;
        }

        
        
}


void postDataToServerJSON(float vol) {
    
    StaticJsonDocument<200> doc;
    doc["value"] = vol;
    
    //Serial.println("deserialized doc" );
    //serializeJson(doc, Serial);  // Generate the minified JSON and send it to the Serial port.
    // The above should print:
    // {"value":0.3}

    // Start a new line
    //Serial.println();
    
    String s;
    serializeJson(doc, s); 
    
    //Serial.println("string s" );
    //Serial.println(s);
    
    HTTPClient http;

    //path to one fader
    http.begin("http://192.168.0.101/datastore/mix/chan/0/matrix/fader");  
    
    //http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    
    //int httpResponseCode = http.POST("json={\"value\":\"My favorite channel\"}"); (this is ok)
    //int httpResponseCode = http.POST("json={\"value\":\"0.7\"}"); //(this is ok)
    int httpResponseCode = http.POST("json=" + s); 
     
    if (httpResponseCode > 0) {

      String response = http.getString();

      //Serial.println(httpResponseCode);
      //Serial.println(response);

    } else {

      Serial.print("Error on sending PUT Request: ");
      Serial.println(httpResponseCode);

    }

    http.end();

}


void postDataToServerJSONmultipleIP(float vol, int device_nr, String ipNumber) {

    //TO ACT ON THE TRIMMS : {"ext/obank/0/ch/0/trim":-18}  -> but this has to be done for each of the devices
    
    
    String s;
    serializeJson(doc_mul, s); 
    
    //Serial.println("string s" );
    //Serial.println(s);
    
    HTTPClient http;
    //http.begin("http://" + devices_ip[device_nr] + "/datastore/");  //root for adding the doc[]
    
    //http.begin("http://192.168.0.102/datastore/");  //root for adding the doc[]
    
    http.begin("http://" + ipNumber + "/datastore/");  //root for adding the doc[]
    
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
    int httpResponseCode = http.POST("json=" + s); 
     
    if (httpResponseCode > 0) {

      String response = http.getString();

      //Serial.println(httpResponseCode);
      //Serial.println(response);

    } else {

      Serial.print("Error on sending PUT Request: ");
      Serial.println(httpResponseCode);

    }

    http.end();

}



void setup(void) {

  USE_SERIAL.begin(115200);
  //Serial.begin(115200);
 

///TFT
  tft.begin();

  tft.setRotation(1);

  xpos =  0;
  ypos = 40;

 // tft.fillScreen(TFT_SKYBLUE); // Clear screen to navy background
  tft.setTextColor(TFT_GOLD, TFT_NAVY);

  tft.setTextDatum(TC_DATUM); // Centre text on x,y position

  xpos = tft.width() / 2; // Half the screen width
  ypos = 50;

  //tft.setFreeFont(FSB24);
  //tft.setTextFont(7);

  tft.fillScreen(TFT_NAVY); // Clear screen to navy background
  
  //tft.setTextFont(GLCD); 
  tft.setFreeFont(FF18);
  tft.drawString("...", xpos, ypos, GFXFF);
  //clear screen 
  //tft.fillScreen(TFT_BLACK);
  

  ///network
  WiFi.onEvent(WiFiEvent);
  ETH.begin();

  //sensors
  pinMode(pushButton, INPUT_PULLUP);  //prepare encoder push button input
  pinMode(36, INPUT_PULLUP);
  pinMode(39, INPUT_PULLUP);

  // ENCODER SETUP ///
  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder.clearCount(); // set starting count value
  USE_SERIAL.println("Encoder Start = "+String((int32_t)encoder.getCount()));
  //encoder.attachFullQuad(39, 36); //set precision of four values per step
  encoder.attachHalfQuad(39, 36); //set precision of four values per step
  

  firstConnection = true;
  encoderValue_past = encoderValue;  

  //tft.fillScreen(TFT_BLACK);
  //tft.setFreeFont(FSB24);
  //tft.setTextSize(3); 
  tft.setFreeFont(FF20);
  tft.drawString("Ottosonics", xpos, ypos, GFXFF);
  
  tft.drawString("AVB Controller", xpos, ypos + 50, GFXFF);
   tft.drawString("TAMLAB", xpos, ypos + 100, GFXFF);                 
  
  delay(2000);
  //wait for connection
  while(eth_connected == false){
    delay(1000);
  }

  if (eth_connected){
      //retrieve general information
      int resultDevices = retrieveDevices("http://192.168.0.100");
      if(resultDevices<0) {
        //there was an error mostly with the ip used to retrieve devices
        USE_SERIAL.println("error with initial ip");
  
        //try with other ips
        int resultDevices_afterError = retrieveDevices("http://192.168.0.101");
        if(resultDevices_afterError<0) {
          int resultDevices_afterError2 = retrieveDevices("http://192.168.0.102");
            //USE_SERIAL.println("error with secondary ip");
            if(resultDevices_afterError2<0) {
              int resultDevices_afterError3 = retrieveDevices("http://192.168.0.103");
              if(resultDevices_afterError3 <0) USE_SERIAL.println("error with secondary ip");       
            }
          
        }
        //show other possibilities to the user
        //clear screen 
        tft.fillScreen(TFT_NAVY);
        //tft.setFreeFont(FSB24);
        
        //tft.drawString("No devices found", xpos, ypos, GFXFF);
        //tft.drawString("choose another IP", xpos, ypos + 20, GFXFF);
       }

       //ok so here i have obtained all devices from the network
       USE_SERIAL.println("++++++++++++++++");
       for(int i=0;i<devices_number;i++) {
          USE_SERIAL.println("device: " + String(i) + " " + String(uids[i]) + " " + String(devices_ip[i]));  
       
          //check if it is the device we want to control
          //uid kike: 24AO - 0001f2fffe00f7c9 ; ultralite - 0001f2fffe008f93
          //uid manu: 24AO1 - 0001f2fffe00f7cd 192.168.0.104; 24AO2 - 0001f2fffe00f7da 192.168.0.102; 24AO3 - 0001f2fffe00f7f7 192.168.0.103; 1248 - 0001f2fffe004d13

          if(studio == 0) {  //tamlab
            USE_SERIAL.println("Using configuration for tamlab old rack");
            if(uids[i] == "0001f2fffe00f7c9") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }
          }
          if(studio == 1) {  //alter bauhof
            USE_SERIAL.println("Using configuration for Alter Bauhof");
            if(uids[i] == "0001f2fffe00f7cd") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }
            if(uids[i] == "0001f2fffe00f7da") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }
            if(uids[i] == "0001f2fffe00f7f7") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }
            if(uids[i] == "0001f2fffe004d13") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }
          }
          if(studio == 2) {  //tamlab second generation rack (nov 2022)
            USE_SERIAL.println("Using configuration for Tamlab equipment rack2");
            if(uids[i] == "0001f2fffe00fb36") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }
            if(uids[i] == "0001f2fffe008f93") {
              IPdevices2control[counter_device2control] = devices_ip[i];
              counter_device2control++;
            }          
          }
       }

       for(int i=0;i<counter_device2control;i++) {
          USE_SERIAL.println("List of devices to control: " + String(i) + " " + IPdevices2control[i]);  
       } 
       
    } 
    
  

}

void loop() {


  if (eth_connected) {
        
        if(firstConnection) {
          
          //clear screen 
          
          tft.fillScreen(TFT_NAVY);
          //tft.setFreeFont(FSB24);
          tft.drawString("Connecting...", xpos, ypos + 100, GFXFF);
          delay(2000);
          tft.fillScreen(TFT_NAVY);
          
          
          while(init_end == false){
            //clear screen 
            ypos = 20;
            //tft.fillScreen(TFT_BLACK);
            
            //tft.setFreeFont(FSB9);
            //tft.setTextSize(2); 
            tft.setFreeFont(FF10);
            tft.drawString("Available devices:", xpos, ypos + 15, GFXFF);
            for(int i=0;i<devices_number;i++){
              tft.drawString(String(i) + ": " + devices_ip[i], xpos, ypos + 15 + 20*(i+1), GFXFF);
            }
           
            //tft.setFreeFont(FSB9);
            tft.drawString("Press Encoder", xpos, 25+ ypos + 15*(devices_number+3), GFXFF);
            tft.drawString("to continue", xpos, 25+ ypos + 15*(devices_number+3) + 20, GFXFF);
            
            //tft.drawString("111", xpos, ypos + 14*(devices_number+2), GFXFF);

            tft.setFreeFont(FF18);
            encoderValue = encoder.getCount(); //encoder position absolute
            
            //let the user push the button to continue
            pushValue = digitalRead(pushButton); //push button in the encoder
            if(pushValue == 0) {
              init_end = true;   //change the flag to continue
              tft.fillScreen(TFT_NAVY);
              tft.drawString("initializing...", xpos, 25+ ypos + 15*(devices_number+3), GFXFF);
             } 
            
            //USE_SERIAL.print("push button ");
            //USE_SERIAL.println(pushValue);
    
            
            //check if it is different
            
            delay(300);
            
          } //end 

          

          //we have selected the device now, we can see the fader values and change them

          
          //retrieve fader value at the beginning
          USE_SERIAL.println(" ");
          USE_SERIAL.print("Retrieving trim values from interface... ");

         faderVolume = getTrimValue();  //uses the ip of selected device--> returns value in dB already
                           
         //dbVolume = 20*log10(faderVolume);  //transform to dB
          
          //display in tft
          tft.fillScreen(TFT_NAVY);
          //tft.setFreeFont(FSB24);
          tft.setTextSize(3); 
          tft.setFreeFont(FF18);
          tft.drawString(String(faderVolume), xpos - 50, 100, GFXFF);  //tft.drawString(String(dbVolume), xpos, 100, GFXFF);
          tft.drawString("dB", xpos + 100, 100, GFXFF);
                                  
          firstConnection = false;
          USE_SERIAL.println("Ready!");
          delay(3000);
          //initial encoder value is 0
          
        } //end of if(first connection)
        else {
                //encoder reading

                pushValue = digitalRead(pushButton); //push button in the encoder
                if(pushValue == 0){
                  faderVolume = -24.0;

                  //quiza crear aqui los doc de json y no en cada loop
                  doc_mul["ext/obank/0/ch/0/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/1/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/2/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/3/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/4/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/5/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/6/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/7/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/8/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/9/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/10/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/11/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/12/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/13/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/14/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/15/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/16/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/17/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/18/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/19/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/20/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/21/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/22/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/23/trim"] = faderVolume;
                  
                  
           

                  for(int i=0;i<counter_device2control;i++) {
                    postDataToServerJSONmultipleIP(faderVolume, 0, IPdevices2control[i]); 
                  } 
                  

                  //dbVolume = (int((20*log10(faderVolume))*10))/10;  //transform to dB
                  //dbVolume = 20*log10(faderVolume);  //transform to dB
                  
                  /*
                  USE_SERIAL.println(" ");
                  USE_SERIAL.print("faderdB ");
                  USE_SERIAL.println(dbVolume);
                  USE_SERIAL.println(" ");
                */
                  
                  //display in tft
                  tft.setTextSize(3); 
                  tft.fillScreen(TFT_NAVY);
                  tft.setFreeFont(FF18);
                  //tft.setFreeFont(FSB24);
                  //tft.drawString(String(faderVolume), xpos, ypos, GFXFF);
                  tft.drawString(String(faderVolume), xpos - 50, 100, GFXFF);
                  tft.drawString("dB", xpos + 100, 100, GFXFF);
                  //ypos += tft.fontHeight(GFXFF);
                  delay(250);
                
                } else {
                  encoderValue = encoder.getCount(); //encoder position absolute
                 /*
                 USE_SERIAL.print("encoderValue ");
                 USE_SERIAL.println(encoderValue);
                 USE_SERIAL.print("encoderPastValue ");
                 USE_SERIAL.println(encoderValue_past);
                 */
                 //check if it is different
                if(encoderValue_past != encoderValue) {
        
                  encoderIncrement = encoderValue-encoderValue_past;  //this can be positive or negative
                  
                  //USE_SERIAL.print("enc increment ");
                  //USE_SERIAL.println(encoderIncrement);
                  
                  incVol =  0.5 * encoderIncrement/4;
                  //USE_SERIAL.print("inc vol ");
                  //USE_SERIAL.println(incVol);
                  faderVolume = faderVolume + incVol*2;
                  
                  if(faderVolume > 0) faderVolume = 0.0;
                  if(faderVolume < -24) faderVolume = -24.0;
                  
                  //USE_SERIAL.print("faderVolume ");
                  //USE_SERIAL.println(faderVolume);
                  encoderValue_past = encoderValue;


                  //quiza crear aqui los doc de json y no en cada loop
                  doc_mul["ext/obank/0/ch/0/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/1/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/2/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/3/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/4/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/5/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/6/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/7/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/8/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/9/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/10/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/11/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/12/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/13/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/14/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/15/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/16/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/17/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/18/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/19/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/20/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/21/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/22/trim"] = faderVolume;
                  doc_mul["ext/obank/0/ch/23/trim"] = faderVolume;
                  

                  for(int i=0;i<counter_device2control;i++) {
                    postDataToServerJSONmultipleIP(faderVolume, 0, IPdevices2control[i]); 
                  } 
                  

                  //dbVolume = (int((20*log10(faderVolume))*10))/10;  //transform to dB
                  //dbVolume = 20*log10(faderVolume);  //transform to dB
                  
                  /*
                  USE_SERIAL.println(" ");
                  USE_SERIAL.print("faderdB ");
                  USE_SERIAL.println(dbVolume);
                  USE_SERIAL.println(" ");
                */
                  
                  //display in tft
                  tft.setTextSize(3); 
                  tft.fillScreen(TFT_NAVY);
                  tft.setFreeFont(FF18);
                  //tft.setFreeFont(FSB24);
                  //tft.drawString(String(faderVolume), xpos, ypos, GFXFF);
                  tft.drawString(String(faderVolume), xpos - 50, 100, GFXFF);
                  tft.drawString("dB", xpos + 100, 100, GFXFF);
                  //ypos += tft.fontHeight(GFXFF);
                  delay(250);
                }
                }
                
      
        } //end first connection
  } else {
      USE_SERIAL.print("Connecting... ");
      delay(1000);
    
  }  //if (eth_connected)

  

}


// Print the header for a display screen
void header(const char *string, uint16_t color)
{
  tft.fillScreen(color);
  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA, TFT_BLUE);
  tft.fillRect(0, 0, 320, 30, TFT_BLUE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(string, 160, 2, 4); // Font 4 for fast drawing with background
}


// Draw a + mark centred on x,y
void drawDatumMarker(int x, int y)
{
  tft.drawLine(x - 5, y, x + 5, y, TFT_GREEN);
  tft.drawLine(x, y - 5, x, y + 5, TFT_GREEN);
}


// There follows a crude way of flagging that this example sketch needs fonts which
// have not been enbabled in the User_Setup.h file inside the TFT_HX8357 library.
//
// These lines produce errors during compile time if settings in User_Setup are not correct
//
// The error will be "does not name a type" but ignore this and read the text between ''
// it will indicate which font or feature needs to be enabled
//
// Either delete all the following lines if you do not want warnings, or change the lines
// to suit your sketch modifications.

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_FONT2
//ERROR_Please_enable_LOAD_FONT2_in_User_Setup!
#endif

#ifndef LOAD_FONT4
//ERROR_Please_enable_LOAD_FONT4_in_User_Setup!
#endif

#ifndef LOAD_FONT6
//ERROR_Please_enable_LOAD_FONT6_in_User_Setup!
#endif

#ifndef LOAD_FONT7
//ERROR_Please_enable_LOAD_FONT7_in_User_Setup!
#endif

#ifndef LOAD_FONT8
//ERROR_Please_enable_LOAD_FONT8_in_User_Setup!
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
