// Copyright IBM Corp. 2015
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//    http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <SPI.h>
#include <Time.h>
#include <WiFi.h>
#include <WifiIPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>
#include <aJSON.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "Adafruit_TMP006.h"
#include <BMA222.h>



// system board LED 
int redLedPin    = RED_LED;
int greenLedPin  = GREEN_LED;
int yellowLedPin = YELLOW_LED;  // message arrived - blink

#define   IDON                "ON"
#define   IDOFF               "IDOFF"

// alarm's and event strings
int arrivedcount = 0;
#define EVENTSTRINGSIZE  400
char eventString[EVENTSTRINGSIZE] = "";
int  timer = 0;
// on board temp sensor   has both an obj sensor and a die sensor we will go with the obj sensor below
//Adafruit_TMP006 tmp006;
Adafruit_TMP006 tmp006(0x41);  // start with a diferent i2c address!
bool tempSensorFound = FALSE;

// your network name also called SSID
char ssid[] = "YOURSSID";
// your network password
char password[] = "YOURNETPASSWORD";
int keyIndex = 0;

// IBM IoT Foundation Cloud Settings
// When adding a device on internetofthings.ibmcloud.com the following
// information will be generated:
// org=<org>
// type=iotsample-ti-energia
// id=<mac>
// auth-method=token
// auth-token=<password>

#define MQTT_MAX_PACKET_SIZE 4096
#define IBMSERVERURLLEN  64
#define IBMIOTFSERVERSUFFIX "messaging.internetofthings.ibmcloud.com"

char organization[] = "YOUR_IOT_FOUNDATON_ORG";

char typeId[]   = "YOUR_IOT_FOUNDATION_DEVICEID";
char pubtopic[] = "iot-2/evt/status/fmt/json";
char subTopic[] = "iot-2/cmd/+/fmt/json";


char deviceId[] = "YOUR_IOT_FOUNDATION_DEVICEID";
char clientId[64];

char mqttAddr[IBMSERVERURLLEN];
int  mqttPortNum = 1883;

// Authentication method. Should be use-toke-auth
// When using authenticated mode
char authMethod[] = "use-token-auth";
// The auth-token from the information above
char authToken[] = "YOUR_IOT_FOUNDATION_AUTHTOKEN_FOR THIS DEVICE";

MACAddress mac;
  
WifiIPStack ipstack;  
MQTT::Client<WifiIPStack, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);

// ****** stuff for trying network time (although this will not work for embeded device which do not know their time
// zone without a gps,  so I want to replace this with a radio frequency atomic time or MWTT setting the time
// based on the customer address
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov


//const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA) or central daylight savings time i think
const int timeZone = -6;  // US central time I think (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // pacific Daylight Time (USA)  ithink
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
// define pool controler stuff

BMA222 mySensor;   //accel sensor

// The function to call when a message arrives
void callback(char* topic, byte* payload, unsigned int length);
void messageArrived(MQTT::MessageData& md);

//  ***************************   connect **************************
void connect()
{
   int retryCount = 0;
   while ( WiFi.status() != WL_CONNECTED) {
      // print dots while we wait to connect
      Serial.print(".");
      WiFi.begin(ssid, password);
      //WiFi.begin(ssid, keyIndex, key);
      retryCount++;
      delay(1000);
      if (retryCount >= 20) {
         listNetworks();
         retryCount = 0;
      }  
    }
    
    retryCount = 0;
    while ( (WiFi.localIP() == INADDR_NONE) && retryCount < 5) {
      // print x while we wait for an ip addresss
      Serial.print("x");
      delay(300);
      retryCount++;
    }
    Serial.println(" ");
    Serial.print("Connecting to ");
    Serial.print(mqttAddr);
    Serial.print(":");
    Serial.println(mqttPortNum);
    Serial.print("With client id: ");
    Serial.println(clientId);
    
    int rc = -1;
    retryCount = 0;
    while (rc != 0 && retryCount < 5) {
      rc = ipstack.connect(mqttAddr, mqttPortNum);
      if (rc != 0) {
        Serial.print("ipstack did not connect rc ");
        Serial.println(rc);
        retryCount++;
      }  
    }
    Serial.println("ipstack connected !");
    delay(1000);
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    connectData.MQTTVersion = 3.1;
    connectData.clientID.cstring = clientId;
    connectData.username.cstring = authMethod;
    connectData.password.cstring = authToken;
    connectData.keepAliveInterval = 10;
    
    rc = -1;
   retryCount = 0;
    while ( ((rc = client.connect(connectData)) != 0) && (retryCount < 10) ){
       Serial.print("Connect failed ");
       Serial.println(rc);
       retryCount++;
       delay(1000);  
    }  
    Serial.println("Connected\n");
    client.yield(1000);
    
    Serial.print("Subscribing to topic: ");
    Serial.println(subTopic);
    
    // Unsubscribe the topic, if it had subscribed it before.
    client.unsubscribe(subTopic);
    // Try to subscribe for commands
    if ((rc = client.subscribe(subTopic, MQTT::QOS0, messageArrived)) != 0) {
      Serial.print("Subscribe failed with return code : ");
      Serial.println(rc);
    } else {
      Serial.println("Subscribe success\n");
    }
}  

// *****************************    1 time SETUP  ***********************
void setup() {
  
  Serial.begin(115200);
  
   // setup on board leds
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, LOW);
  pinMode(greenLedPin, OUTPUT);
  digitalWrite(greenLedPin, LOW);
  pinMode(yellowLedPin, OUTPUT);
  digitalWrite(yellowLedPin, LOW);
  
  listNetworks();
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to Network named: ");
  // print the network name (SSID);
  Serial.println(ssid); 
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(ssid, password);
  //WiFi.begin(ssid, keyIndex, key);
  while ( WiFi.status() != WL_CONNECTED) {
    // print dots while we wait to connect
    Serial.print(".");
    delay(300);
  }
  
  Serial.println("\nYou're connected to the network");
  Serial.println("Waiting for an ip address");
  
  while (WiFi.localIP() == INADDR_NONE) {
    // print x while we wait for an ip addresss
    Serial.print("x");
    delay(300);
  }

  // We are connected and have an IP address.
  printWifiStatus();

  
  // Use MAC Address as deviceId
  //sprintf(deviceId, "%02x%02x%02x%02x%02x%02x", macOctets[0], macOctets[1], macOctets[2], macOctets[3], macOctets[4], macOctets[5]);
  Serial.print("deviceId: ");
  Serial.println(deviceId);

  sprintf(clientId, "d:%s:%s:%s", organization, typeId, deviceId);
  sprintf(mqttAddr, "%s.%s", organization, IBMIOTFSERVERSUFFIX);
  Udp.begin(localPort);
  Serial.println("waiting for date sync");
  setSyncProvider(getNtpTime);
  digitalClockDisplay(); 

  
  // you can also use tmp006.begin(TMP006_CFG_1SAMPLE) or 2SAMPLE/4SAMPLE/8SAMPLE to have
  // lower precision, higher rate sampling. default is TMP006_CFG_16SAMPLE which takes
  // 4 seconds per reading (16 samples)
  if (! tmp006.begin()) {
  //if (TRUE) {
    Serial.println("NO!! on board temp sensor found");
    tempSensorFound = FALSE;
  } else {
    Serial.println("temp sensor found, good");
    tempSensorFound = TRUE;
  }  
  // start the accel sensor
  mySensor.begin();
  uint8_t chipID = mySensor.chipID();
  Serial.print("hipID: ");
  Serial.println(chipID);
  
}

//  ************************************  loop  ********************************
void loop() {
  long lSignalStrength;
  int rc = -1;
  if (!client.isConnected()) {
    connect();
  }

  // Send and receive QoS 0 message
  aJsonObject *json,*data;
  json = aJson.createObject();
  aJson.addItemToObject(json, "d", data = aJson.createObject());
  aJson.addItemToObject(data, "myName", aJson.createItem(deviceId));
  float temperature = getTemp();
  aJson.addItemToObject(data, "temp", aJson.createItem(temperature));
  int8_t sensorData = mySensor.readXData();
  aJson.addItemToObject(data, "X", aJson.createItem(sensorData));

  sensorData = mySensor.readYData();
   aJson.addItemToObject(data, "Y", aJson.createItem(sensorData));

  sensorData = mySensor.readZData();
  aJson.addItemToObject(data, "Z", aJson.createItem(sensorData));
  
  // get the network received signal strength:
  lSignalStrength = WiFi.RSSI();
  float RSSI = lSignalStrength;
  aJson.addItemToObject(data, "RSSI", aJson.createItem(RSSI));

  Serial.print("Publishing: ");
  char* string = aJson.print(json);
  if (string != NULL) {
    Serial.println(string);
  } 
  else {
    Serial.print( "error getting sting from json");
  }
  MQTT::Message message;
  message.qos = MQTT::QOS0; 
  message.retained = false;
  message.payload = string; 
  message.payloadlen = strlen(string);
  rc = client.publish(pubtopic, message);
  if (rc != 0) {
    Serial.print("Message publish failed with return code : ");
    Serial.println(rc);
  } else {
    if (strlen(eventString)) {
      eventString[0] = 0;   //remove the event string after reporting the event
    }
    timer = 0;
  }  
  //freeMem("with object");
  aJson.deleteItem(json);
  //freeMem("after deletion");
  free(string);
  
  // Wait for one second before publishing again
  // This will also service any incoming messages
  client.yield(1000);
}

//  ************************   subscrbe  Call backs  ***********************
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message has arrived on topic");
  Serial.println(topic);
  char * msg = (char *)malloc(length * sizeof(char));
  int count = 0;
  for(count = 0 ; count < length ; count++) {
    msg[count] = payload[count];
  }
  msg[count] = '\0';
  Serial.println(msg);
  
  if(length > 0) {
    digitalWrite(redLedPin, HIGH);
    delay(1000);
    digitalWrite(redLedPin, LOW);  
  }

  free(msg);
}

char printbuf[200];
void messageArrived(MQTT::MessageData& md) {
    Serial.print("Message Received\t");
    digitalClockDisplay();  
    digitalWrite(redLedPin, HIGH);
    MQTT::Message &message = md.message;
    sprintf(printbuf, "Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
		++arrivedcount, message.qos, message.retained, message.dup, message.id);
    Serial.print(printbuf);
    int topicLen = strlen(md.topicName.lenstring.data) + 1;
//    char* topic = new char[topicLen];
    char * topic = (char *)malloc(topicLen * sizeof(char));
    topic = md.topicName.lenstring.data;
    topic[topicLen] = '\0';
    
    int payloadLen = message.payloadlen + 1;
//    char* payload = new char[payloadLen];
    char * payload = (char*)message.payload;
    payload[payloadLen] = '\0';
    
    String topicStr = topic;
    Serial.print(topicStr);
    String payloadStr = payload;
    Serial.print("Command IS Supported : ");
    Serial.print(payload);
    Serial.println("\t....."); 
    
    aJsonObject* ajsonData = aJson.createObject();
    ajsonData = aJson.parse(payload);
    aJsonObject* ajsonPayload = aJson.getObjectItem(ajsonData, "d");
    
    
    //Command topic: iot-2/cmd/blink/fmt/json
    if(strstr(topic, "/cmd/blink") != NULL) {
       
      //Blink twice
      for(int i = 0 ; i < 2 ; i++ ) {
        digitalWrite(redLedPin, HIGH);
        delay(250);
        digitalWrite(redLedPin, LOW);
        delay(250);
      }
    }  
    
 
  
  if(strstr(topic, "/cmd/GPIO") != NULL) {
    Serial.println("Command GPIO-not finished");
  }
  
  
  
   //Serial.print(*topic);
  //free(topic); 
  aJson.deleteItem(ajsonData);
  digitalWrite(redLedPin, LOW);
  //delay(1000);
  
  //free topic here 
  
    
}
/////  ---------------------------- worker functions -------------

//*********************   clock functions  ***************
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
  //tmElements_t tm;
  //RTC.read(tm);
  //setTime(hour(),minute(),second(),day(),month(),year()); 
}

void printDigits(int digits){
 // utility function for digital clock display: prints preceding colon and leading 0
   Serial.print(":");
   if(digits < 10)
   Serial.print('0');
   Serial.print(digits);
}
/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// ****************************************************** end udp time


//        ************************ list networks
void listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    Serial.println(WiFi.encryptionType(thisNet));
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("Network Name: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  uint8_t macOctets[6];
  mac = WiFi.macAddress(macOctets);
  Serial.print("MAC Address: ");
  Serial.println(mac);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


//          ******************************** get on board temp sensor cc3200 has 2 cpu and object, we do object
float getTemp() {
  // Check for sleep/wake command.
  /*  to do check engery consumption later and modify
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'w') {
      Serial.println("Waking up!");
      tmp006.wake();
    }
    else if (c == 's') {
      Serial.println("Going to sleep!");
      tmp006.sleep();
    }
  }
  */
  if (tempSensorFound) {
    // Grab temperature measurements and print them.
    float objt = tmp006.readObjTempC();
    float fobjt = objt * 1.8 + 32.0;
    //Serial.print("Object Temperature: "); Serial.print(objt); Serial.print("*C  "); Serial.print(fobjt); Serial.println("*f");
    //float diet = tmp006.readDieTempC();
    //float fdiet = diet * 1.8 + 32.0;
    //Serial.print("Die Temperature: "); Serial.print(diet); Serial.print("*C  ");Serial.print(fdiet); Serial.println("*f");
   
    //delay(4000); // 4 seconds per reading for 16 samples per reading
    return fobjt;
  } else {
    return 0;
  }  
}

