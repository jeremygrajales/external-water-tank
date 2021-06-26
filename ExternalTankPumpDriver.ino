#include <ESP8266WiFi.h>
#include <PubSubClient.h>

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean pressureRecovering = false;
boolean pumpCoolingDown = false;
boolean pumpRunning = false;
boolean emergencyShutoff = false;
long pumpRuntime= 0;
long pumpCooldownTime = 0;

#define PUMP 4
#define EMERGENCY_SHUTOFF 5
#define PUMP_MAX_RUNTIME 30
#define PUMP_COOLDOWN_TIME 60

const char* ssid = "MemoryAlpha";     // replace with your wifi ssid and wpa2 key
const char* password = "Bevel111";
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
    if(client.connect("ESP8266Client_ExternalTankPump")) {
      Serial.println("connected");
      client.subscribe("pressureRecovering");
      client.subscribe("emergencyShutoff");
    }
    else {
      Serial.println("failed to connect");
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, HIGH);
  pinMode(EMERGENCY_SHUTOFF, OUTPUT);
  digitalWrite(EMERGENCY_SHUTOFF, HIGH);
  //digitalWrite(LED_BUILTIN, LOW);
  //pinMode(LED_BUILTIN, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(readMessageStream);
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
    // Runs every 1 seconds
    pumpLoop(); 
  }
}

void readMessageStream(String topic, byte* _message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String message;
  for (int i = 0; i < length; i++) {
    Serial.print((char)_message[i]);
    message += (char)_message[i];
  }
  Serial.println();
  if(topic=="pressureRecovering"){
      if(message == "true"){
        pressureRecovering = true;
      }
      else if(message == "false"){
        pressureRecovering = false;
      }
  }
  if(topic=="emergencyShutoff") {
    emergencyShutoff = true;
    digitalWrite(EMERGENCY_SHUTOFF, LOW);
  }
  Serial.println();
}

void pumpProtector() {
  if(pumpRunning) {
    pumpRuntime++;
    char buff[100];
    sprintf(buff,"Pump running time: %lu seconds", pumpRuntime);
    logger(buff);
  }
  if(pumpCoolingDown) {
    pumpCooldownTime++;
    pressureRecovering = false;
    char buff[100];
    sprintf(buff,"Pump cooldown time: %lu seconds", pumpCooldownTime);
    logger(buff);
    
  }
  if(pumpRuntime > PUMP_MAX_RUNTIME) {
    pumpCoolingDown = true;
    pressureRecovering = false;
    pumpRuntime = 0;
  }
  if(pumpCooldownTime  > PUMP_COOLDOWN_TIME) {
    pumpCoolingDown = false;
    pumpCooldownTime = 0;
  }    
}

void pumpLoop() {
  if(emergencyShutoff) {
    return;
  }
  pumpProtector();
  if(pressureRecovering && !pumpCoolingDown) {
    logger("Pump ON.");
    client.publish("pump_state", "on");
    //digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(PUMP, LOW);
    pumpRunning = true;
  }
  else {
    logger("Pump OFF.");
    client.publish("pump_state", "off");
    //digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(PUMP, HIGH);
    pumpRunning = false;
  }
}

void logger(char* message) {
  client.publish("pump_log", message);
  Serial.println(message);
}
