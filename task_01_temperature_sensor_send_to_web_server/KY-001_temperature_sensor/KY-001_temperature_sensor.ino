#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiNINA.h>
#include <SPI.h>
#include "arduino_secrets.h"
#define KY001_Signal_PIN 4

OneWire oneWire(KY001_Signal_PIN);          
DallasTemperature sensors(&oneWire);
char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS; 
WiFiServer server(80);
void setup() 
{
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  sensors.begin(); 
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}
void loop() 
{
  WiFiClient client = server.available();
  float temperature=sensors.requestTemperatures();
  if (client) 
  {
    Serial.println("Client connected!");
    while (client.connected()) 
    {
      if (client.available()) 
      {
        client.read();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println("<!DOCTYPE HTML><html>");
        client.print("<h1>Temperature: ");
        client.print(temperature);
        client.println(" &deg;C</h1>");
        client.println("</html>");
        break;
      }
    }
    delay(10);
    //client.stop();
    //Serial.println("Client disconnected");
  }
delay(1000);
}

