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
#include <ArduinoJson.h>

// Update these with values suitable for your network.

const char* ssid = "WiFi-DOM.ru-2463";
const char* password = "89502657277";

const char* mqtt_server = "m23.cloudmqtt.com";
const int mqtt_port = 17455; 
const char *mqtt_user = "orclnzbz";
const char *mqtt_pass = "iGYujXAXAtqp";

const char *topicPrefix = "main_";
enum topicNames {LED, CLIMAT, CONTROL, SYSTEM}; 

typedef struct {
  char *topicName;
  bool isIncomingTopic;
} Topic;

typedef struct {
  int id;
  int pin;
  int value;
  String sensorName;
  Topic topic;
} Sensor;

Topic topics[] = {
    {"LED", true},
    {"CLIMAT", false},
    {"CONTROL", true},
    {"SYSTEM", true}
};

Sensor sensors[] = {
  {0, A0, 0, "temperature", topics[CLIMAT]},
  {1, D1, 0, "led1", topics[LED]},
  {2, D2, 0, "led2", topics[LED]},
  {3, D4, 0, "led3", topics[LED]}
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
  for(int i=1; i < sizeof(sensors)/sizeof(Sensor); i++){
    pinMode(sensors[i].pin, OUTPUT);
  }
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
  char JSONMessage[length];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
    JSONMessage[i] = (char)payload[i];
  }
  Serial.println();
  if (strcmp(topic, topics[LED].topicName)==0){
      Serial.println("LED mesags!");
      parseJsonMessage(JSONMessage);
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
      client.publish(
        sensors[i].topic.topicName, 
        prepareValuesForSending(sensors[i].id, sensors[i].sensorName, sensors[i].value)
      );
    }
  }
}

char *prepareValuesForSending(int id, String sensorName, int value){
  StaticJsonBuffer<200> JsonOutgoingBuffer;
  JsonObject& JSONencoder = JsonOutgoingBuffer.createObject();
  JSONencoder["device"] = "ESP8266";
  JSONencoder["id"] = id;
  JSONencoder["sensorName"] = sensorName;
  JsonArray& values = JSONencoder.createNestedArray("values");
 
  values.add(value);

  char JSONmessageBuffer[200];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("------");
  Serial.println(JSONmessageBuffer);
  Serial.println("------");

  return JSONmessageBuffer;
}

void parseJsonMessage(char *jsonMessage){
  Serial.println(jsonMessage);
  StaticJsonBuffer<200> JsonIncomingBuffer;
  JsonObject& parsed = JsonIncomingBuffer.parseObject(jsonMessage);
 
  if (!parsed.success()) {   //Check for errors in parsing
 
    Serial.println("Parsing failed");
    delay(2000);
    return;
  }

  int id = parsed["Id"];
  if(id > 0 && id < sizeof(sensors)/sizeof(Sensor) - 1){
    String sensorType = parsed["sensorName"];
    if(sensorType == sensors[id].sensorName){
      sensors[id].value = parsed["Value"];
    }else{
      Serial.println("Incorrect state of sensor");
    }
  }else{
    Serial.println("The current sensor was not presente in hardware");
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
