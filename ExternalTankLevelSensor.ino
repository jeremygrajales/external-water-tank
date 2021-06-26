#include <ESP8266WiFi.h>
#include <PubSubClient.h>

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;

int trigPin = 5;
int echoPin = 4;
float duration, distance;

const char* ssid = "ssid";     // replace with your wifi ssid and wpa2 key
const char* password = "password";
const char* deviceID = "tank_1";
const char* mqtt_server = "c9.dev.jeremygrajales.com";

WiFiClient espClient;
PubSubClient client(espClient);

void getDistance()
{
  digitalWrite(echoPin, LOW);   // set the echo pin LOW
  digitalWrite(trigPin, LOW);   // set the trigger pin LOW
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);  // set the trigger pin HIGH for 10μs
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);  // measure the echo time (μs)
  distance = (duration/2.0)*0.0343;   // convert echo time to distance (cm)
  if(distance>400 || distance<2) Serial.println("Out of range");
  else
  {
    Serial.print("Distance: ");
    Serial.print(distance, 1); Serial.println(" cm");
    char result[100];
    sprintf(result, "%f", distance);
    client.publish("distance_tank_1", result);
  }
  delay(1000);
}

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
    if(client.connect("ESP8266Client_tank_1")) {
      Serial.println("connected");
      client.subscribe("distance_tank_1");
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
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT); // define trigger pin as output
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  previousMillis = 0;
}

void loop() {
  currentMillis = millis();
  if(!client.connected()) {
    reconnect();
  }
  if(!client.loop()) {
    client.connect("ESP8266Client_tank_1");
  }
  if (currentMillis - previousMillis > interval) {
    previousMillis = millis();
    // Runs every 1 seconds
    getDistance();
  }
}
