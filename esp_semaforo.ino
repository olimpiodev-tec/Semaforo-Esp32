/**
- PubSub: https://pubsubclient.knolleary.net/api
- LiquidCrystal_I2C: https://github.com/locple/LiquidCrystal_I2C_UTF8
- DHT: https://github.com/adafruit/DHT-sensor-library
- ArduinoJson: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
*/

#include <PubSubClient.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* SSID = "";
const char* PASSWORD = "";

const char* MQTT_HOST = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_TOPIC_SEMAFORO_LIGAR = "semaforo/ligar";
const char* MQTT_TOPIC_SEMAFORO_DESLIGAR = "semaforo/desligar";
const char* MQTT_TOPIC_LER_TEMPERATURA = "temperatura/ler";
const char* MQTT_TOPIC_ENVIAR_TEMPERATURA = "temperatura/enviar";

const int PINO_LED_VERMELHO = 5;
const int PINO_LED_AMARELO = 18;
const int PINO_LED_VERDE = 23;

#define DHTPIN 4
#define DHTTYPE DHT11

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

void showText(const char* linha1, const char* linha2, bool isDelay) {
  lcd.clear();
  lcd.print(linha1);
  lcd.setCursor(0, 1);
  lcd.print(linha2);

  if (isDelay) {
    delay(1000);
  }
}

void conectarWifi() {
  WiFi.begin(SSID, PASSWORD);
  showText("Conectando WiFi", "", true);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  String ip = " " + WiFi.localIP().toString();
  showText(" Conectado WiFi", ip.c_str(), true);
}

void conectarMQTT() {
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);

  while (!client.connected()) {
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      showText("Conectado broker", "   MQTT HiveMQ  ", true);
    } else {
      String erroMQTT = "Codigo erro: " + String(client.state());
      showText("Broker MQTT falhou", erroMQTT.c_str(), true);
    }
  }

  client.subscribe(MQTT_TOPIC_SEMAFORO_LIGAR);
  client.subscribe(MQTT_TOPIC_SEMAFORO_DESLIGAR);
  client.subscribe(MQTT_TOPIC_LER_TEMPERATURA);
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "semaforo/ligar") == 0) {
    showText("    Semaforo    ", "     Ligado     ", false);
    toggleSemaforo(1);
  } else if (strcmp(topic, "semaforo/desligar") == 0) {
    showText("    Semaforo    ", "   Desligado    ", false);
    toggleSemaforo(0);
  } else if (strcmp(topic, "temperatura/ler") == 0) {
    readTemperatureHumidity();
  }
}

void toggleSemaforo(int statusLed) {
  digitalWrite(PINO_LED_VERMELHO, statusLed);
  digitalWrite(PINO_LED_AMARELO, statusLed);
  digitalWrite(PINO_LED_VERDE, statusLed);
}

void readTemperatureHumidity() {
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(temp) || isnan(hum)) {
    showText("  Falha ao ler ", " dados do sensor", true);
  } else {
    String tempDisplay = "Temp: " + String(temp) + " C";
    String humDisplay = "Umid: " + String(hum) + " %";
    showText(tempDisplay.c_str(), humDisplay.c_str(), false);

    publicTemperatureHumidity(temp, hum);
  }
}

void publicTemperatureHumidity(float temp, float hum) {
  StaticJsonDocument<200> doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;

  // Serializa o documento JSON para um buffer
  char buffer[200];
  size_t n = serializeJson(doc, buffer);

  client.publish(MQTT_TOPIC_ENVIAR_TEMPERATURA, buffer, n);
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_AMARELO, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.setBacklight(HIGH);
  showText(" Inicializando ", "    Aguarde    ", true);

  conectarWifi();
  conectarMQTT();
}

void loop() {
  client.loop();
}
