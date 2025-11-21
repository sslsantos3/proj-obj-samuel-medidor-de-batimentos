#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ---------- CONFIGURAÇÕES OLED ----------
#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1
Adafruit_SSD1306 tela(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

// ---------- PINOS ----------
#define SENSOR_PULSO 35
#define LED_BAIXO 2
#define LED_NORMAL 4
#define LED_ALTO 5

// ---------- LIMITES ----------
const int BPM_MIN = 50;
const int BPM_MAX = 120;

// ---------- WIFI & MQTT ----------
const char* WIFI_NOME = "Wokwi-GUEST";
const char* WIFI_SENHA = "";
const char* MQTT_SERVIDOR = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- FUNÇÕES AUXILIARES ----------
void apagarLeds() {
  digitalWrite(LED_BAIXO, LOW);
  digitalWrite(LED_NORMAL, LOW);
  digitalWrite(LED_ALTO, LOW);
}

void conectarWifi() {
  Serial.print("Conectando à rede: ");
  Serial.println(WIFI_NOME);
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nRede Wi-Fi estabelecida!");
}

void reconectarMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (mqtt.connect("ESP32_MonitorCardiaco")) {
      Serial.println(" Conectado!");
    } else {
      Serial.print(" Falha (cod:");
      Serial.print(mqtt.state());
      Serial.println("). Tentando novamente em 5s");
      delay(5000);
    }
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  pinMode(LED_BAIXO, OUTPUT);
  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_ALTO, OUTPUT);

  apagarLeds();

  if (!tela.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro ao inicializar OLED!");
    while(true);
  }

  tela.clearDisplay();
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(0, 28);
  tela.println("Sistema Cardio Ativo");
  tela.display();
  delay(1500);

  conectarWifi();
  mqtt.setServer(MQTT_SERVIDOR, 1883);
}

// ---------- LOOP ----------
void loop() {
  if (!mqtt.connected()) reconectarMQTT();
  mqtt.loop();

  int valorSensor = analogRead(SENSOR_PULSO);
  int bpm = map(valorSensor, 300, 600, 40, 180);
  bpm = constrain(bpm, 0, 200);

  apagarLeds();

  tela.clearDisplay();
  tela.setCursor(0, 0);

  String statusBatimento;

  if (bpm < BPM_MIN) {
    tela.println("Pulso Fraco");
    tela.setCursor(0, 20);
    tela.print("Medicao: ");
    tela.print(bpm);
    tela.println(" bpm");
    tela.display();

    digitalWrite(LED_BAIXO, HIGH);
    statusBatimento = "baixo";

    Serial.println("Aviso: batimento abaixo do esperado");

  } else if (bpm > BPM_MAX) {
    tela.println("Pulso Alto");
    tela.setCursor(0, 20);
    tela.print("Medicao: ");
    tela.print(bpm);
    tela.println(" bpm");
    tela.display();

    digitalWrite(LED_ALTO, HIGH);
    statusBatimento = "alto";

    Serial.println("Aviso: batimento acima do esperado");

  } else {
    tela.println("Pulso Normal");
    tela.setCursor(0, 20);
    tela.print("Medicao: ");
    tela.print(bpm);
    tela.println(" bpm");
    tela.display();

    digitalWrite(LED_NORMAL, HIGH);
    statusBatimento = "normal";

    Serial.println("Batimento dentro da faixa normal");
  }

  Serial.print("BPM registrado: ");
  Serial.println(bpm);

  char msgBPM[10];
  char msgStatus[20];
  sprintf(msgBPM, "%d", bpm);
  statusBatimento.toCharArray(msgStatus, 20);

  mqtt.publish("cardio/valor", msgBPM);
  mqtt.publish("cardio/status", msgStatus);

  delay(1500);
}