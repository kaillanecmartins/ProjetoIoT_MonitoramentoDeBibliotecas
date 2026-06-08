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

#define led1 19
#define led2 18

#define DHT_pin 15
#define IR1_pin 4
//#define IR2_pin 4
#define PIR_pin 22
//#define KY_pin 15
#define DHTTYPE DHT11

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT dht(DHT_pin, DHTTYPE);

unsigned long lastPublish = 0;

bool ledRemoto = false;

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

      digitalWrite(led1, HIGH);
      Serial.println("LED REMOTO LIGADO");
    }

    else if (payload[0] == '0') {

      ledRemoto = false;

      digitalWrite(led1, LOW);

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

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  pinMode(DHT_pin, INPUT);
  pinMode(IR1_pin, INPUT);
  //pinMode(IR2_pin, INPUT);
  pinMode(PIR_pin, INPUT);
  //pinMode(KY_pin, INPUT);

  dht.begin();

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


    int presenca2 = digitalRead(PIR_pin);
    //int som = analogRead(KY_pin);
    int presenca1 = digitalRead(IR1_pin); 
    //int presenca2 = digitalRead(IR2_pin); 

    if (isnan(temp) || isnan(hum)) {

      Serial.println("Erro no DHT11");
      return;
    }


    if (!presenca1 && !ledRemoto) {
      digitalWrite(led1, HIGH);
    } else if(!ledRemoto){
      digitalWrite(led1, LOW);
    }

    if (!presenca2 && !ledRemoto) {
      digitalWrite(led2, HIGH);
    } else if(!ledRemoto){
      digitalWrite(led2, LOW);
    }

    StaticJsonDocument<128> doc;

    doc["temperatura"] = temp;
    doc["umidade"] = hum;
    doc["movimento1"] = presenca1;
    doc["movimento2"] = presenca2;
    //doc["som"] = som;

    char buffer[128];

    serializeJson(doc, buffer);

    client.publish(topic_sensores, buffer);

    Serial.println(buffer);
  }
}