#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 8883;

const char* topic_led = "kai/led";
const char* topic_sensores = "kai/sensores";

#define R_pin 18
#define G_pin 19
#define B_pin 21

#define DHT_pin 15
#define PIR_pin 4

#define DHTTYPE DHT11

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT dht(DHT_pin, DHTTYPE);

unsigned long lastPublish = 0;

bool ledRemoto = false;

void LED_RGB(bool r, bool g, bool b) {
  digitalWrite(R_pin, r);
  digitalWrite(G_pin, g);
  digitalWrite(B_pin, b);
}

void conectarWiFi() {

  Serial.println("Conectando ao WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Mensagem recebida: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();

  if (length == 1) {

    if (payload[0] == '1') {

      ledRemoto = true;

      LED_RGB(0, 1, 0);

      Serial.println("LED REMOTO LIGADO");
    }

    else if (payload[0] == '0') {

      ledRemoto = false;

      LED_RGB(0, 0, 0);

      Serial.println("LED REMOTO DESLIGADO");
    }
  }
}

void conectarMQTT() {

  while (!client.connected()) {

    Serial.println("Conectando MQTT...");

    String clientId =
      "ESP32-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println("MQTT conectado!");

      client.subscribe(topic_led);

    } else {

      Serial.print("Falha MQTT: ");
      Serial.println(client.state());

      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(R_pin, OUTPUT);
  pinMode(G_pin, OUTPUT);
  pinMode(B_pin, OUTPUT);

  pinMode(PIR_pin, INPUT);

  dht.begin();

  LED_RGB(0, 0, 0);

  conectarWiFi();

  client.setServer(mqtt_server, mqtt_port);
  espClient.setInsecure();
  client.setCallback(callback);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi desconectado!");
    conectarWiFi();
  }

  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();

  if (millis() - lastPublish > 2000) {

    lastPublish = millis();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    int presenca = digitalRead(PIR_pin);

    if (isnan(temp) || isnan(hum)) {

      Serial.println("Erro no DHT11");
      return;
    }

    if (presenca && !ledRemoto) {

      LED_RGB(1, 0, 0);

    } else if (!ledRemoto) {

      LED_RGB(0, 0, 0);
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