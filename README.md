# MOTU_AVB_ESP32_Controller
Building a simple volume controller with a ESP32 for MOTU AVB audio interfaces. Part of the [Ottosonics](https://tamlab.kunstuni-linz.at/projects/ottosonics/) project . 

<img src="https://github.com/ultranoise/MOTU_AVB_ESP32_Controller/blob/main/IMG_0427.jpg" width=60% height=60%>

MOTU AVB interfaces have a particular method for receiving HTTP POST messages. First read the MOTU API Reference: 

https://cdn-data.motu.com/downloads/audio/AVB/docs/OSC%20Quick%20Reference.pdf

The principle to GET and POST with MOTU AVB interfaces is rather simple with cURL:

* get the datastore: ``` curl http://192.168.0.5/datastore ``` (use the IP of your interface here)

* read value of mic1 trim: ``` curl http://192.168.0.5/datastore/ext/ibank/0/ch/0/trim ```

* set value of mic1 trim: ``` curl --data 'json={"value":"45"}' http://192.168.0.5/datastore/ext/ibank/0/ch/0/trim ```

* set mixer fader 0 (ch1) value: ``` curl --data 'json={"value":"0.7"}' http://192.168.0.5/datastore/mix/chan/0/matrix/fader ```

* set mixer fader 1 (ch2) value: ``` curl --data 'json={"value":"0.7"}' http://192.168.0.5/datastore/mix/chan/1/matrix/fader ```

* set mixer fader 1 and fader 2 at the same time: ``` curl --data 'json={"0/matrix/fader":"0.2","1/matrix/fader":"0.1"}' http://192.168.0.5/datastore/mix/chan/ ```

# Hardware

We used a ESP32 with ethernet by [Olimex](https://www.olimex.com/Products/IoT/ESP32/ESP32-GATEWAY/open-source-hardware) and a [SPI TFT display](https://www.amazon.de/gp/product/B07YTWRZGR/ref=ppx_yo_dt_b_asin_title_o06_s01?ie=UTF8&psc=1) that we connected as follows:
![alt text](https://github.com/ultranoise/MOTU_AVB_ESP32_Controller/blob/main/schematic.jpg)
Please note that a WLAN version of this code could be easily implemented with the same board (call wlan functions instead of ethernet). 

# Code

* If you want to download our solution please go to the source folder of this repository

* With our ESP32, the following code can be used to read the datastore:

```
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

void testAvb(const char * host, uint16_t port)
{
  HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        http.begin("http://192.168.0.5/datastore/"); //HTTP
        
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


void setup()
{
  USE_SERIAL.begin(115200);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
}


void loop()
{
  if (eth_connected) {
    testAvb("google.com", 80);
  }
  delay(10000);
}
```

* The issues begin when you try to POST messages. The logic of posting with JSON encoding doesn't work. If you typically use the following code you will get an error:

```

void postDataToServer() {

    HTTPClient http;

    http.begin("http://192.168.0.5/datastore/ext/obank/2/ch/0/name");
    
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST("{\"value\":\"My favorite channel\"}");
    
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
```

you get a (-5) or (-1) error:

```
ETH Connected
 POSTing to Server...
-5

```
 
So clearly there is a difference in how headers or formatting are set. I used the online service httpbin.org to debug the messages. Basically httpbin.org replied me a copy of what we have to send.  
  
This is what you get if you use the cURL command which works with the MOTU:

```
curl --data 'json={"value":"My favorite channel"}' https://httpbin.org/post  
{
  "args": {}, 
  "data": "", 
  "files": {}, 
  "form": {
    "json": "{\"value\":\"My favorite channel\"}"
  }, 
  "headers": {
    "Accept": "*/*", 
    "Content-Length": "36", 
    "Content-Type": "application/x-www-form-urlencoded", 
    "Host": "httpbin.org", 
    "User-Agent": "curl/7.64.1", 
    "X-Amzn-Trace-Id": "Root=1-62a2ec58-01489dc01c52f8201f03eccb"
  }, 
  "json": null, 
  "origin": "188.23.136.201", 
  "url": "https://httpbin.org/post"
}
```

However, with the previous Arduino code (which doesn't work with the MOTU interface)

```
200
{
  "args": {}, 
  "data": "{\"value\":\"My favorite channel\"}", 
  "files": {}, 
  "form": {}, 
  "headers": {
    "Accept-Encoding": "identity;q=1,chunked;q=0.1,*;q=0", 
    "Content-Length": "31", 
    "Content-Type": "application/json", 
    "Host": "httpbin.org", 
    "User-Agent": "ESP32HTTPClient", 
    "X-Amzn-Trace-Id": "Root=1-62a2e98e-0825a5fe531b2c493b88287d"
  }, 
  "json": {
    "value": "My favorite channel"
  }, 
  "origin": "188.23.136.201", 
  "url": "https://httpbin.org/post"
}

```

Ergo-> there are quite noticeable differences!!

If we use the same arduino code with a different header ("application/x-www-form-urlencoded") instead of (application/json")

```
//http.addHeader("Content-Type", "application/json");
http.addHeader("Content-Type", "application/x-www-form-urlencoded");
```

We get 

```
200
{
  "args": {}, 
  "data": "", 
  "files": {}, 
  "form": {
    "{\"value\":\"My favorite channel\"}": ""
  }, 
  "headers": {
    "Accept-Encoding": "identity;q=1,chunked;q=0.1,*;q=0", 
    "Content-Length": "31", 
    "Content-Type": "application/x-www-form-urlencoded", 
    "Host": "httpbin.org", 
    "User-Agent": "ESP32HTTPClient", 
    "X-Amzn-Trace-Id": "Root=1-62a2ee48-42acf6c158c472cf4e3d67b8"
  }, 
  "json": null, 
  "origin": "188.23.136.201", 
  "url": "https://httpbin.org/post"
}
```

Which looks better, but to accommondate the format that the MOTU interface expects ( json={} ) this is the final code which finally works:

```
void postDataToServerTwo() {
    HTTPClient http;

    http.begin("http://192.168.0.5/datastore/mix/chan/0/matrix/fader"); 
    
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
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
```

This simple produces a "204" response and moves the fader!

You can take a look at a complete example in the src folder.
