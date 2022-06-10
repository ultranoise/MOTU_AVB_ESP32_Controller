/*
    This sketch shows the Ethernet event usage

*/
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 5
#include <ETH.h>
#include <HTTPClient.h>
#include <Arduino.h>

#define USE_SERIAL Serial

static bool eth_connected = false;

#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;

const char* server = "http://10.0.0.5/datastore/mix/chan/0/matrix/fader";

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


void testAvbField()
{
  HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        
        http.begin("http://10.0.0.5/datastore/mix/chan/0/matrix/fader"); 

        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                USE_SERIAL.println(payload);
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
}


void postDataToServerTwo() {
    HTTPClient http;

    http.begin("http://10.0.0.5/datastore/mix/chan/0/matrix/fader");  //curlコマンドと同じURi『http://192.168.0.100/datastore/ext/obank/2/ch/0/name 』
    //http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    //int httpResponseCode = http.POST("{\"value\":\"My favorite channel\"}"); 
    //int httpResponseCode = http.POST("json={\"value\":\"My favorite channel\"}"); (this is ok)
    int httpResponseCode = http.POST("json={\"value\":\"0.7\"}"); 
    
    
    if (httpResponseCode > 0) {

      String response = http.getString();

      Serial.println(httpResponseCode);
      Serial.println(response);

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
}


void loop()
{
  if (eth_connected) {
    postDataToServerTwo();
    delay(2000);
    testAvbField();
  }
  delay(2000);
}
