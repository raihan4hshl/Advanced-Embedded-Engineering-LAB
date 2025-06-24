#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Servo.h>

// ——— EDIT THESE LINES —————————————————————————————————————
char ssid[] = "Pixel_5277";
char pass[] = "Donotaccess.";

const char* local_mqtt_server = "10.60.189.180"; // your Pi’s Mosquitto IP
const int   local_mqtt_port   = 1883;

const char* aio_server    = "io.adafruit.com";
const int   aio_port      = 1883;
const char* aio_username  = "ismail343";
const char* aio_key       = "aio_lRmM99iaw1rnSnl74HY6WWbCY9VV";
// —————————————————————————————————————————————————————————————

const int servoPin   = 9;
const int buzzerPin  = 8;
const int DRY_THRESHOLD = 100;
bool valveOpen = false;

Servo waterValveServo;
WiFiClient   wifiClient;
PubSubClient clientLocal(wifiClient);
PubSubClient clientAIO(wifiClient);

// Central handler:
void handleMessage(const String& topic, const String& msg) {
  Serial.print("▶ MSG ["); Serial.print(topic); Serial.print("] → "); Serial.println(msg);

  // Valve control
  String lValve = "farm/actuator";
  String aValve = String(aio_username)+"/feeds/valve-control";
  if (topic==lValve || topic==aValve) {
    if (msg=="open") {
      waterValveServo.write(90);
      valveOpen = true;
      digitalWrite(buzzerPin, LOW);
      Serial.println("  Servo OPENED + buzzer OFF");
    } else if (msg=="close") {
      waterValveServo.write(0);
      valveOpen = false;
      Serial.println("  Servo CLOSED");
    }
    return;
  }

  // Moisture → buzzer
  String lMoist = "farm/moisture";
  String aMoist = String(aio_username)+"/feeds/soil-moisture";
  if (topic==lMoist || topic==aMoist) {
    int m = msg.toInt();
    Serial.print("  Moisture="); Serial.println(m);
    if (!valveOpen && m < DRY_THRESHOLD) {
      digitalWrite(buzzerPin, HIGH);
      Serial.println("  Buzzer ON (dry)");
    } else {
      digitalWrite(buzzerPin, LOW);
      Serial.println("  Buzzer OFF");
    }
  }
}

void callbackLocal(char* topic, byte* payload, unsigned int len) {
  String msg; for (unsigned i=0;i<len;i++) msg += (char)payload[i];
  msg.trim();
  handleMessage(String(topic), msg);
}

void callbackAIO(char* topic, byte* payload, unsigned int len) {
  String msg; for (unsigned i=0;i<len;i++) msg += (char)payload[i];
  msg.trim();
  handleMessage(String(topic), msg);
}

void reconnectLocal() {
  Serial.println("Connecting to LOCAL Mosquitto...");
  while (!clientLocal.connected()) {
    if (clientLocal.connect("subLocal")) {
      Serial.println("  ✓ Connected LOCAL");
      clientLocal.subscribe("farm/actuator");
      clientLocal.subscribe("farm/moisture");
      Serial.println("  Subscribed LOCAL: farm/actuator, farm/moisture");
    } else {
      Serial.print("  ✗ LOCAL connect failed, rc=");
      Serial.print(clientLocal.state());
      Serial.println(" → retry in 5s");
      delay(5000);
    }
  }
}

void reconnectAIO() {
  Serial.println("Connecting to ADAFRUIT IO...");
  while (!clientAIO.connected()) {
    if (clientAIO.connect("subAIO", aio_username, aio_key)) {
      Serial.println("  ✓ Connected AIO");
      String vt = String(aio_username)+"/feeds/valve-control";
      String mt = String(aio_username)+"/feeds/soil-moisture";
      clientAIO.subscribe(vt.c_str());
      clientAIO.subscribe(mt.c_str());
      Serial.print("  Subscribed AIO: "); Serial.print(vt); Serial.print(", "); Serial.println(mt);
    } else {
      Serial.print("  ✗ AIO connect failed, rc=");
      Serial.print(clientAIO.state());
      Serial.println(" → retry in 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.print("Wi-Fi connecting to "); Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  // Servo & buzzer setup
  waterValveServo.attach(servoPin);
  waterValveServo.write(0);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // MQTT clients
  clientLocal.setServer(local_mqtt_server, local_mqtt_port);
  clientLocal.setCallback(callbackLocal);
  clientAIO.setServer(aio_server, aio_port);
  clientAIO.setCallback(callbackAIO);
}

void loop() {
  if (!clientLocal.connected()) reconnectLocal();
  clientLocal.loop();

  if (!clientAIO.connected()) reconnectAIO();
  clientAIO.loop();
}
