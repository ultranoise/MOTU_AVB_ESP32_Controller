/*
    This sketch shows the Ethernet event usage

*/
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 5
#include <ETH.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ESP32Encoder.h>
#include <ArduinoJson.h>

#define USE_SERIAL Serial


static bool eth_connected = false;

#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;

const char* server = "http://10.0.0.5/datastore/mix/chan/0/matrix/fader";


//Sensor vars
ESP32Encoder encoder; //encoder variable
int encoderValue = 0;
int encoderValue_past = 0;
int encoderIncrement = 0;
int encoderIncrement_past;
int pushButton = 2; //push button in the encoder
int pushValue = 1;
int pushValue_past = 0;
bool newIncValue = false;

//fader values etc
float faderVolume;
float incVol;
bool firstConnection = true;

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

void testClient(const char * host, uint16_t port)
{
  USE_SERIAL.print("\nconnecting to ");
  USE_SERIAL.println(host);

  WiFiClient client;
  if (!client.connect(host, port)) {
    USE_SERIAL.println("connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available());
  while (client.available()) {
    USE_SERIAL.write(client.read());
  }

  USE_SERIAL.println("closing connection\n");
  client.stop();
}



float getFaderValue()
{
  HTTPClient http;

        //USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        
        http.begin("http://10.0.0.5/datastore/mix/chan/0/matrix/fader"); 

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
                  return -1.0;
                } else {
                  float f = doc["value"];
                  //USE_SERIAL.println("extracted value");
                  //USE_SERIAL.println(f);

                  return f;
                }
                
                
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
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
    http.begin("http://10.0.0.5/datastore/mix/chan/0/matrix/fader");  
    
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


void postDataToServerJSONtwo(float vol) {
   //controlling various faders
   //we have to build 'json={"0/matrix/fader":"0.2","1/matrix/fader":"0.1"}' http://192.168.0.5/datastore/mix/chan/
    
    StaticJsonDocument<200> doc;
    doc["0/matrix/fader"] = vol;
    doc["1/matrix/fader"] = vol;
    
    //Serial.println("deserialized doc" );
    //serializeJson(doc, Serial);  // Generate the minified JSON and send it to the Serial port to debug it
    
    String s;
    serializeJson(doc, s); 
    
    //Serial.println("string s" );
    //Serial.println(s);
    
    HTTPClient http;

    //various faders?
    // 'json={"0/matrix/fader":"0.2","1/matrix/fader":"0.1"}' http://192.168.0.5/datastore/mix/chan/
    http.begin("http://10.0.0.5/datastore/mix/chan/"); 
    
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


    
void setup()
{
  USE_SERIAL.begin(115200);
  //Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();

  //sensors
  pinMode(pushButton, INPUT_PULLUP);  //prepare encoder push button input
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);

  // ENCODER SETUP ///
  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder.clearCount(); // set starting count value
  USE_SERIAL.println("Encoder Start = "+String((int32_t)encoder.getCount()));
  encoder.attachFullQuad(15, 14); //set precision of four values per step

  firstConnection = true;
  encoderValue_past = encoderValue;
}


void loop()
{
  
  if (eth_connected) {

        if(firstConnection) {
          
          //retrieve fader value at the beginning
          USE_SERIAL.print("Retrieving values from interface ");
          faderVolume = getFaderValue();
          firstConnection = false;
          USE_SERIAL.println("Ready!");
    
          //initial encoder value is 0
          
        } else {
                //encoder reading
                
                encoderValue = encoder.getCount(); //encoder position absolute
                /*
                USE_SERIAL.print("encoderValue ");
                USE_SERIAL.println(encoderValue);
                USE_SERIAL.print("encoderPastValue ");
                USE_SERIAL.println(encoderValue_past);
                */
                pushValue = digitalRead(pushButton); //push button in the encoder
                
                //USE_SERIAL.print("push button ");
                //USE_SERIAL.println(pushValue);
        
                
                //check if it is different
                if(encoderValue_past != encoderValue) {
        
                  encoderIncrement = encoderValue-encoderValue_past;  //this can be positive or negative
                  
                  //USE_SERIAL.print("enc increment ");
                  //USE_SERIAL.println(encoderIncrement);
                  
                  incVol = 0.01 *  encoderIncrement/4;
                  //USE_SERIAL.print("inc vol ");
                  //USE_SERIAL.println(incVol);
                  faderVolume = faderVolume + incVol;
                  
                  encoderValue_past = encoderValue;
                  
                  //postDataToServerJSON(faderVolume);  //one fader
                  postDataToServerJSONtwo(faderVolume); //two faders
                  
                  
                }
                /*
                USE_SERIAL.println(" ");
                USE_SERIAL.print("faderVolume ");
                USE_SERIAL.println(faderVolume);
                USE_SERIAL.println(" ");
                */
                delay(100);
        } //end first connection
  } else {
      USE_SERIAL.print("Connecting... ");
      delay(1000);
    
  }  //if (eth_connected)
  
} //loop
