#include <ESP8266WiFi.h>
#include <PubSubClient.h>

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;

#define SENSOR  2
#define LED_BUILTIN 16

const char* ssid = "ssid";     // replace with your wifi ssid and wpa2 key
const char* password = "password";
const char* mqtt_server = "c9.dev.jeremygrajales.com";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while(!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if(client.connect("ESP8266Client_HomePressureSensor2")) {
      Serial.println("connected");
    }
    else {
      Serial.println("failed to connect");
      Serial.println("retrying");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(0, INPUT_PULLUP);
  previousMillis = 0;
}

void loop() {
  currentMillis = millis();
  
  if(!client.connected()) {
    reconnect();
  }
  if(!client.loop()) {
    client.connect("ESP8266Client");
  }
  if (currentMillis - previousMillis > interval) {
    previousMillis = millis();
     
    char str[20];
    dtostrf(analogRead(0), 1, 2, str);
    client.publish("pressure", str);
  }
}
 
