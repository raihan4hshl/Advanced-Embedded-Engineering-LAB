#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "DHT.h"

// ——— CONFIGURATION —————————————————————————————————————————
// Your Wi-Fi network:
char ssid[] = "Pixel_5277";             // ← EDIT
char pass[] = "Donotaccess.";         // ← EDIT

// Local Mosquitto broker:
const char* local_mqtt_server = "10.60.189.180"; // ← EDIT if needed
const int   local_mqtt_port   = 1883;

// Adafruit IO:
const char* aio_server    = "io.adafruit.com";
const int   aio_port      = 1883;
const char* aio_username  = "ismail343"; // ← EDIT
const char* aio_key       = "aio_lRmM99iaw1rnSnl74HY6WWbCY9VV";      // ← EDIT
// ———————————————————————————————————————————————————————————————

const int moisturePin = A0;
#define DHTPIN   2
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient   wifiClient;
PubSubClient clientLocal(wifiClient);
PubSubClient clientAIO(wifiClient);

void reconnectLocal() {
  while (!clientLocal.connected()) {
    if (clientLocal.connect("pubLocal")) {
      clientLocal.publish("farm/status", "Publisher Local Connected");
    } else {
      delay(5000);
    }
  }
}

void reconnectAIO() {
  while (!clientAIO.connected()) {
    if (clientAIO.connect("pubAIO", aio_username, aio_key)) {
      clientAIO.publish((String(aio_username)+"/feeds/status").c_str(), "Publisher AIO Connected");
    } else {
      delay(5000);
    }
  }
}

void publishSensorData() {
  // Read sensors
  int moisture = analogRead(moisturePin);
  float hum    = dht.readHumidity();
  float temp   = dht.readTemperature();

  // Convert to strings
  String sMoist = String(moisture);
  String sHum   = isnan(hum)   ? "nan" : String(hum, 2);
  String sTemp  = isnan(temp)  ? "nan" : String(temp, 2);

  // — Local Mosquitto —
  if (!clientLocal.connected()) reconnectLocal();
  clientLocal.loop();
  clientLocal.publish("farm/moisture",    sMoist.c_str());
  clientLocal.publish("farm/humidity",    sHum.c_str());
  clientLocal.publish("farm/temperature", sTemp.c_str());
  Serial.print("Local → M:"); Serial.print(moisture);
  Serial.print(" H:"); Serial.print(hum);
  Serial.print(" T:"); Serial.println(temp);

  // — Adafruit IO —
  if (!clientAIO.connected()) reconnectAIO();
  clientAIO.loop();
  clientAIO.publish((String(aio_username)+"/feeds/soil-moisture").c_str(), sMoist.c_str());
  clientAIO.publish((String(aio_username)+"/feeds/humidity").c_str(),      sHum.c_str());
  clientAIO.publish((String(aio_username)+"/feeds/temperature").c_str(),   sTemp.c_str());
  Serial.println(" → Published to AIO");
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Connect Wi-Fi
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) { delay(500); }
  Serial.print("Wi-Fi IP: "); Serial.println(WiFi.localIP());

  // Init DHT
  dht.begin();

  // Configure brokers
  clientLocal.setServer(local_mqtt_server, local_mqtt_port);
  clientAIO.setServer(aio_server, aio_port);
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 2000) {
    last = millis();
    publishSensorData();
  }
}
