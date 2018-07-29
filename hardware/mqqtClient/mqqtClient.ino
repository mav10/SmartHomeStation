/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "WiFi";
const char* password = "";

const char* mqtt_server = "m23.cloudmqtt.com";
const int mqtt_port = 17455; 
const char *mqtt_user = "";
const char *mqtt_pass = "";

const char *topicPrefix = "main_";
enum topicNames {LED, CLIMAT, CONTROL, SYSTEM}; 

typedef struct {
  char *topicName;
  bool isIncomingTopic;
} Topic;

typedef struct {
  int pin;
  int value;
  Topic topic;
} Sensor;

Topic topics[] = {
    {"LED", true},
    {"CLIMAT", false},
    {"CONTROL", true},
    {"SYSTEM", true}
};

Sensor sensors[] = {
  {A0, 0, topics[CLIMAT]},
  {D4, 0, topics[LED]}
};

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  Serial.begin(115200);
  ConfigureGpio();
  ConfigureWifi();
  ConfigureMqtt();
}

void ConfigureGpio() {
  pinMode(A0, INPUT);
  pinMode(D4, OUTPUT);
}

void ConfigureWifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void ConfigureMqtt(){
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();
  if (strcmp(topic, topics[LED].topicName)==0){
      Serial.println("LED mesags!");
      sensors[1].value = message.toInt();
  } else if (strcmp(topic,  topics[CLIMAT].topicName)==0){
      Serial.println("CLIMAT mesags!");
  } else if (strcmp(topic,  topics[CONTROL].topicName)==0){
      Serial.println("CONTROL mesags!");
  } else if (strcmp(topic,  topics[SYSTEM].topicName)==0){
      Serial.println("SYSTEM mesags!");
  } else {
      Serial.println("Unknown mesages!");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("esp", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      char msgsys[] = "starting new IoT node";
      client.publish(topics[SYSTEM].topicName, msgsys);

      subscribe();
      publish();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void subscribe() {
  int length = sizeof(topics)/sizeof(Topic);
  for(int i=0; i < length; i++) {
    if(topics[i].isIncomingTopic){
      client.subscribe(topics[i].topicName);
      Serial.println("Topic was subscribed " + String(topics[i].topicName));
    }
  }
  char sysmsg[50];
  snprintf (sysmsg, 75, "sensor #%ld", value);
  Serial.println("All topics were subscribed");
}

void publish() {
  int length = sizeof(sensors)/sizeof(Sensor);
  for(int i=0; i < length; i++) {
    if(!sensors[i].topic.isIncomingTopic){
      snprintf (msg, 75, "sensor #%ld", sensors[i].value);
      client.publish(sensors[i].topic.topicName, msg);
    }
  }
}

void ApplyValues(){
  int length = sizeof(sensors)/sizeof(Sensor);
  for(int i=0; i < length; i++) {
    if(!sensors[i].topic.isIncomingTopic){
       sensors[i].value = analogRead(sensors[i].pin);
    }else{
      analogWrite(sensors[i].pin, sensors[i].value);
    }
  }
}

void loop() {
  ApplyValues();
  if (!client.connected()) {
    reconnect();
  }
  publish();
  client.loop();

  delay(100);
}
