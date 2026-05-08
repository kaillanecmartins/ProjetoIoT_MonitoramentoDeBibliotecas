#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* topic_led = "kai/led";
const char* topic_sensores = "kai/sensores";

#define R_pin 18
#define G_pin 19
#define B_pin 21

#define DHT_pin 15
#define PIR_pin 2
#define DHTTYPE DHT22

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_pin, DHTTYPE);

unsigned long lastReconnectAttempt = 0;
unsigned long lastPublish = 0;

void LED_RGB(int modeR, int modeG, int modeB){
  digitalWrite(R_pin, modeR);
  digitalWrite(G_pin, modeG);
  digitalWrite(B_pin, modeB);
}

void callback(char* topic, byte* payload, unsigned int length) {

  if (length == 1) {

    if (payload[0] == '1') {

      LED_RGB(0, 1, 0);

      Serial.println("LED LIGADO");

    }

    else if (payload[0] == '0') {

      LED_RGB(0, 0, 0);

      Serial.println("LED DESLIGADO");
    }
  }
}

boolean reconnect() {

  String clientId = "ESP32_" + String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {

    Serial.println("MQTT conectado");

    client.subscribe(topic_led);

  } else {

    Serial.print("Falha MQTT: ");
    Serial.println(client.state());
  }

  return client.connected();
}

void setup() {

  Serial.begin(115200);

  pinMode(R_pin, OUTPUT);
  pinMode(G_pin, OUTPUT);
  pinMode(B_pin, OUTPUT);

  pinMode(PIR_pin, INPUT);
  pinMode(MQ_PIN, INPUT);

  dht.begin();

  Serial.println("Conectando WiFi...");

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 10000) {

    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\nWiFi conectado!");

  } else {

    Serial.println("\nFalha no WiFi");
  }

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}


void loop() {

  if (!client.connected()) {

    unsigned long now = millis();

    if (now - lastReconnectAttempt > 5000) {

      lastReconnectAttempt = now;

      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }

  } else {

    client.loop();
  }

  if (millis() - lastPublish > 2000) {

    lastPublish = millis();
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int presenca = digitalRead(PIR_pin);

    if (isnan(temp) || isnan(hum)) {

      Serial.println("Erro no DHT22");
      return;
    }

    if (presenca) {
      LED_RGB(1, 0, 0);
    } else {
      LED_RGB(0, 1, 0);
    }

    StaticJsonDocument<128> doc;

    doc["temperatura"] = temp;
    doc["umidade"] = hum;
    doc["movimento"] = presenca;

    char buffer[128];

    serializeJson(doc, buffer);

    client.publish(topic_sensores, buffer);

    Serial.println(buffer);
  }
}