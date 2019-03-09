#include <Arduino.h>
#include <Stream.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//AWS
#include "sha256.h"
#include "Utils.h"


//WEBSockets
#include <Hash.h>
#include <WebSocketsClient.h>

//MQTT PUBSUBCLIENT LIB 
#include <PubSubClient.h>

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"

extern "C" {
  #include "user_interface.h"
}

//AWS IOT config, change these:
char wifi_ssid[]       = "xxxx";
char wifi_password[]   = "xxxx";
//char wifi_ssid[]       = "Rein8";
//char wifi_password[]   = "timothyseth";
char aws_endpoint[]    = "a132vkks0odbx1-ats.iot.us-east-2.amazonaws.com";
char aws_key[]         = "xxxx";
char aws_secret[]      = "xxxx";
char aws_region[]      = "us-east-2";
const char* aws_topic  = "COLevel";
int port = 443;

//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

//glucometer
int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor
int prevCOReading = 0;
int COReadingCount = 0; 
int start = 0;
float overall = 0;


ESP8266WiFiMulti WiFiMulti;


AWSWebSocketClient awsWSclient(1000);
WiFiClient wifiClient;
PubSubClient client(awsWSclient);

//# of connections
long connection = 0;

//generate random mqtt clientID
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1)
    cID[i]=(char)random(1, 256);
  return cID;
}

//count messages arrived
int arrivedcount = 0;

//callback to handle mqtt messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if(strcmp(topic, "button") == 0){
    start = 1;
  }
}

//connects to websocket layer and mqtt layer
bool connect () {
    if (client.connected()) {    
        client.disconnect ();
    }  
    //delay is not necessary... it just help us to get a "trustful" heap space value
    delay (1000);
    Serial.print (millis ());
    Serial.print (" - conn: ");
    Serial.print (++connection);
    Serial.print (" - (");
    Serial.print (ESP.getFreeHeap ());
    Serial.println (")");

    //creating random client id
    char* clientID = generateClientID ();
    client.setServer(aws_endpoint, port);
    if (client.connect(clientID)) {
      Serial.println("connected");     
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      return false;
    }
}

//subscribe to a mqtt topic
void subscribe() {
    client.setCallback(callback);
    client.subscribe(aws_topic);
    client.subscribe("button");
   //subscript to a topic
    Serial.println("MQTT subscribed");
}

//send a message to a mqtt topic
void sendmessage() {
    //send a message   
    char buf[100];
    strcpy(buf, "");   
    int rc = client.publish(aws_topic, buf); 
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
    wifi_set_sleep_type(NONE_SLEEP_T);
    Serial.begin (115200);
    delay (2000);
    Serial.setDebugOutput(1);

    setup_wifi();

  /*
    //fill with ssid and wifi password
    WiFiMulti.addAP(wifi_ssid, wifi_password);
    Serial.println ("connecting to wifi");
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print (".");
    }
    Serial.println ("\nconnected");
*/
    //fill AWS parameters    
    awsWSclient.setAWSRegion(aws_region);
    awsWSclient.setAWSDomain(aws_endpoint);
    awsWSclient.setAWSKeyID(aws_key);
    awsWSclient.setAWSSecretKey(aws_secret);
    awsWSclient.setUseSSL(true);

    if (connect ()){
      subscribe ();
      //sendmessage ();
    }
    
    pinMode(sensorPin, INPUT);
    prevCOReading = analogRead(sensorPin);
}

float readCO(){
  float average = 0;
  int sum = 0;
  for(int i=0; i<50; i++)
  {
    sensorValue = analogRead(sensorPin);
    sum = sum + sensorValue;
    delay(100);
    Serial.print(".");   
  }
  Serial.println("");
  average = sum / 50.0;
  overall = overall + sum;
  return average;
}

void loop() {
  int COlevel = 0;
  char msg[50];
  float sensorVal05Sec, sensorVal10Sec, sensorVal15Sec, sensorVal20Sec;  
  char current[16];
  
  //keep the mqtt up and running
  if (awsWSclient.connected ()) {    
      client.loop();
  } else {
    //handle reconnection
    if (connect ()){
      subscribe ();      
    }
  }
  sensorValue = analogRead(sensorPin);
  snprintf( msg, 75, "%d", sensorValue);

  COReadingCount++;

  if (COReadingCount > 10) {
    Serial.print("Publish message:");
    Serial.println(msg);
    client.publish("COLevel", msg);
  
    Serial.printf("Prev = %d, Current = %d Diff = %d\n", prevCOReading, sensorValue, (sensorValue - prevCOReading));
    sprintf(current, "Current: %5d", sensorValue);
    
    overall = 0;
    if(start)
    { 
      Serial.println("Reading CO value:");

      client.publish("status", "Reading CO values, breathe on device");
      /* ten second delay for stable reading */
      for(int i=0;i<10;i++) {
        Serial.print(".");
        delay(1000);
      }
      client.publish("status", "Getting stable reading, continue to breathe on device");
      Serial.println("");
      sensorVal05Sec = readCO();
      snprintf( msg, 75, "%.0f", sensorVal05Sec);
      client.publish("COLevel05", msg);
      client.publish("COLevel", msg);
      client.publish("status", "5 sec reading finished");
      
      Serial.printf("\nAverage after 5 sec = %.0f\n", sensorVal05Sec);
      sensorVal10Sec = readCO();
      snprintf( msg, 75, "%.0f", sensorVal10Sec);
      client.publish("COLevel10", msg);
      client.publish("COLevel", msg);
      client.publish("status", "10 sec reading finished");
      Serial.printf("\nAverage after 10 sec = %.0f\n", sensorVal10Sec);
      sensorVal15Sec = readCO();
      snprintf( msg, 75, "%.0f", sensorVal15Sec);
      client.publish("COLevel15", msg);
      client.publish("COLevel", msg);
      client.publish("status", "15 sec reading finished");
      Serial.printf("\nAverage after 15 sec = %.0f\n", sensorVal15Sec);
      
      sensorVal20Sec = readCO();
      snprintf( msg, 75, "%.0f", sensorVal20Sec);
      client.publish("COLevel20", msg);
      client.publish("COLevel", msg);
      client.publish("status", "20 sec reading finished");
      Serial.printf("\nAverage after 20 sec = %.0f\n", sensorVal20Sec);
      Serial.printf("Summary: %.0f %.0f %.0f %.0f\n", sensorVal05Sec, sensorVal10Sec, sensorVal15Sec, sensorVal20Sec);
      Serial.printf("Overall: %.0f\n", overall/200);
      
      snprintf( msg, 75, "%.0f", overall/200);
      Serial.print("Average CO:");
      Serial.println(msg);
      client.publish("COLevelAve", msg);
      //reconnect();

      delay(2000);
      client.publish("status", "CO reading finished");
    }
    
    prevCOReading = sensorValue;
    COReadingCount = 0;
    start = 0;
  }
  delay(500);
}
