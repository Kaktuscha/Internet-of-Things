#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "rgb_lcd.h"
#include <RFID.h>
#include <SPI.h>
#include "rgb_lcd.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESP8266mDNS.h>

//-------Potentiometer-----
#define POTEN A0
//-------LED PIN----------
#define LEDPIN 15
//--------HTTP--------
#define TIMEOUTVAL 250
#define LINES 8
#define LEN 150
//--------RFID--------
#define SS_PIN 0
#define RST_PIN 2
//--------Task--------
#define TASK1 100
#define TASK2 2
#define TASK3 100
#define TASK4 10000

//-------Max queue----
#define MAX 10

//-------UDP_Buffer------
#define BUFFERLEN 255
#define SOCKET 8888
int packetsize;
//-------MQTT---------
#define NOPUBLISH // comment this out once publishing at less than 10 second intervals
#define ADASERVER "io.adafruit.com" // do not change this
#define ADAPORT 8883 // do not change this 
#define ADAUSERNAME "NUA" // ADD YOUR username here between the qoutation marks
#define ADAKEY "aio_LZhU92KcNC08Nmc6xtgXBIqoxq3Y" // ADD YOUR Adafruit key here between marks

//--------class instantiation------
WiFiServer SERVER(80);
WiFiClient client;
WiFiClientSecure Mclient; // create a class instance for the MQTT server
WiFiUDP UDP;
RFID myrfid(SS_PIN, RST_PIN);
rgb_lcd lcd;

const char ssid[] = "Iphone U";
const char password[] = "";
char inputbuf[LINES][LEN] = {};
const char domainname[] = "200234832";
int MAC = 0;
int IP = 0;
int SubnetMask = 0, Gateway = 0, DNS = 0;

static const char * fingerprint PROGMEM = "59 3C 48 0A B1 8B 39 4E 0D 58 50 47 9A 13 55 60 CC A0 1D AF";
Adafruit_MQTT_Client MQTT( & Mclient, ADASERVER, ADAPORT, ADAUSERNAME, ADAKEY);
Adafruit_MQTT_Subscribe Slider_high = Adafruit_MQTT_Subscribe( & MQTT, ADAUSERNAME "/feeds/covid19.slider-high");
Adafruit_MQTT_Subscribe Slider_low = Adafruit_MQTT_Subscribe( & MQTT, ADAUSERNAME "/feeds/covid19.slider-low");
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish( & MQTT, ADAUSERNAME "/feeds/covid19.temperature");

//-----------HTTP Post----
const char matchMax[] = {
  'M',
  'A',
  'X',
  '='
};
const char matchMin[] = {
  'M',
  'I',
  'N',
  '='
};

//-------UDP-SendBuffer---
char inBuffer[BUFFERLEN];
char outBuffer[BUFFERLEN];

//----------Temperature---------
double temperature = 0.0;
int matchCountMax = 0;
int matchCountMin = 0;
double maxTemp = 38.0;
double minTemp = 36.5;

//------------ID card----------------------
const int myCardd[] = {
  254,
  56,
  99,
  195
};
int cardpresent = 0, lastcardpresent = 0;

//------------Queue-----------------------
String queue_array[MAX];
int rear = -1;
int front = -1;

//----------LCD---------------------------
const int colorR = 0;
const int colorG = 0;
const int colorB = 255;

//---------Tasks-------------------------
unsigned int current_millis = 0;
unsigned int Task1c = 0, Task2c = 0, Task3c = 0, Task4c = 0;

//--------Potentiometer------------------
double analogValue = 0;

//--------Uniqe Identifier---------------
String uniqeIdentifier = "Aston_Main_Entrance";

//--------Function declaretion-----------
void servepage();
void bCastUDP();
void insertQueue();
void rfidCheck();
void http();
void MQTT_Operation(int timer);

void setup()
{

  //-------Serial.i/o----------
  Serial.begin(115200);
  while (!Serial)
  {
    ;;
  }
  //-------PinOut----------
  pinMode(LEDPIN, OUTPUT);
  //--------LCD------------------
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.clear();
  lcd.setCursor(0, 0);
  //-------WIFI-------------------
  Serial.print("Attempting to connect to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  lcd.print("WiFi Con...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Subnet masks: ");
  Serial.println(WiFi.localIP()); // In the same way, the println function prints all four IP address bytes
  // Print the subnet mask
  //--------UDP-------------------
  UDP.begin(SOCKET) ;
  //----------MQTT----------------
  Mclient.setFingerprint(fingerprint);
  MQTT.subscribe( & Slider_high);
  MQTT.subscribe( & Slider_low);
  MQTTconnect();
  //---------MDNS-----------------
  if (MDNS.begin(domainname, WiFi.localIP()))
  {
    Serial.print("Access your server at http://");
    Serial.print(domainname);
    Serial.println(".local");
  }

  //--------HTTP------------------
  SERVER.begin();
  //-------RFID-------------------
  SPI.begin();
  myrfid.init();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Present card");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
}

void loop()
{
  current_millis = millis();

  if ((current_millis - Task1c) >= TASK1)
  {
    rfidCheck();
    Task1c = current_millis;
  }
  else if ((current_millis - Task2c) >= TASK2)
  {
    http();
    Task2c = current_millis;
  }
  else if ((current_millis - Task3c) >= TASK3)
  {
    MQTT_Operation(10);
    Task3c = current_millis;
  }
  if ((current_millis - Task4c) >= TASK4)
  {
    bCastUDP();
    Task4c = current_millis;
  }


}
void http()
{
  int timecnt = 0;
  int i;

  client = SERVER.available(); // Wait for a client (a web browser) to connect to our server
  if (!client)
  {
    return;
  }

  Serial.println("There is a client, waiting for data");
  while (!client.available())
  {
    delay(1);
    timecnt++;
    if (timecnt >= TIMEOUTVAL)
    {
      return;
    }
  }

  Serial.println("Reading the request");

  for (i = 0; i < LINES; i++) //LINES
  {
    client.readStringUntil('\r').toCharArray( & inputbuf[i][0], LEN);
    //    Serial.print(inputbuf[i]);
  }

  for (i = 0; i < LEN; i++)
  {
    if (matchMax[matchCountMax] == inputbuf[6][i])
    {
      matchCountMax++;
      if (matchCountMax == 4)
      {
        matchCountMax = 0;
        if (isdigit(inputbuf[6][i + 1]))
        {
          int maxT = 10 * (inputbuf[6][i + 1] - '0') + (inputbuf[6][i + 2] - '0');
          if (maxT >= 38 && maxT <= 45) {
            maxTemp = maxT;
            Serial.print("maxTemp -> ");
            Serial.println(maxTemp);
          }
        }
      }
    }
    if (matchMin[matchCountMin] == inputbuf[6][i])
    {
      matchCountMin++;
      if (matchCountMin == 4)
      {
        matchCountMin = 0;
        if (isdigit(inputbuf[6][i + 1]))
        {
          int minT = 10 * (inputbuf[6][i + 1] - '0') + (inputbuf[6][i + 2] - '0');
          if (minT < 38 && 25 <= minT)
          {
            minTemp = minT;
            Serial.print("minTemp -> ");
            Serial.println(minTemp);
          }
        }
      }
    }

  }

  client.flush();
  servepage();
  client.stop();
}
void servepage()
{
  String reply;

  reply += "<!DOCTYPE HTML>"; // start of actual HTML file

  reply += "<html>"; // start of html
  reply += "<head>"; // the HTML header
  reply += "<title>COVID19 Temperature Monitoring Page</title>"; // page title as shown in the tab
  reply += "</head>";
  reply += "<body>"; // start of the body
  reply += "<h1>The temperature of patient:</h1>"; // heading text
  reply += "<h2>The RFID ID: ";
  reply += myCardd[0];
  reply += myCardd[1];
  reply += myCardd[2];
  reply += myCardd[3];
  reply += "</h2>";
  reply += "<h2>Temperature: ";
  reply += String(temperature);
  reply += "</h2>";
  reply += "<h2>Device uniqe ID: ";
  reply += String(uniqeIdentifier);
  reply += "</h2>";
  reply += "<form action='/post'>";
  reply += "Max Temp: <input type='text' name='MAX'>";
  reply += "<input type='submit' value='Submit'>";
  reply += "</form><br>";
  reply += "<form action='/post'>";
  reply += "Min Temp: <input type='text' name='MIN'>";
  reply += "<input type='submit' value='Submit'>";
  reply += "</form><br>";
  reply += "</form>";
  reply += "<h2>The last attempts: ";
  reply += "<br>";
  reply += queue_array[0];
  reply += "<br>";
  reply += queue_array[1];
  reply += "<br>";
  reply += queue_array[2];
  reply += "<br>";
  reply += queue_array[3];
  reply += "<br>";
  reply += queue_array[4];
  reply += "<br>";
  reply += queue_array[5];
  reply += "<br>";
  reply += queue_array[6];
  reply += "<br>";
  reply += queue_array[7];
  reply += "<br>";
  reply += queue_array[8];
  reply += "<br>";
  reply += queue_array[9];
  reply += "</h2>";
  reply += "</body>"; // end of body
  reply += "</html>"; // end of HTML

  client.printf("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %u\r\n\r\n%s", reply.length(), reply.c_str());
}

void rfidCheck()
{
  cardpresent = myrfid.isCard();
  if (cardpresent && !lastcardpresent)
  {
    if (myrfid.readCardSerial()) {
      if ((myrfid.serNum[0] == myCardd[0]) && (myrfid.serNum[1] == myCardd[1]) && (myrfid.serNum[2] == myCardd[2]) && (myrfid.serNum[3] == myCardd[3])) {
        analogValue = analogRead(POTEN);
        temperature = (analogValue * 45.0) / 1024.0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature);
        insertQueue();
        if (!MQTT.connected()) {
          MQTTconnect();
        }
        Temperature.publish(temperature);
        if ((temperature >= minTemp) && (temperature <= maxTemp )) {
          digitalWrite(LEDPIN, HIGH);
          delay(1000);
          digitalWrite(LEDPIN, LOW);
          MQTT_Operation(1000); //I did not waste CPU cycles with delay instead I used MQTT subscription check.
        } else {
          lcd.setCursor(0, 1);
          lcd.print("Go home");
          MQTT_Operation(1000);
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Present card");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
      } else {
        lcd.clear();
        lcd.print("Access Denied");
      }
    }
  }
  lastcardpresent = cardpresent;
  myrfid.halt();
}

void insertQueue()
{
  if (rear == MAX - 1)
  {
    for (int jj = front; jj < rear - 1; jj++)
    {
      queue_array[jj] = queue_array[jj + 1];
    }
    rear--;
  }
  else
  {
    if (front == -1)
      front = 0;
    rear = rear + 1;
    String str = uniqeIdentifier + String(" ") + String(temperature);
    queue_array[rear] = str;
  }

}
void MQTT_Operation(int timer) {
  // This function call ensures that we have a connection to the Adafruit MQTT server. This will make
  // the first connection and automatically tries to reconnect if disconnected. However, if there are
  // three consecutive failled attempts, the code will deliberately reset the microcontroller via the
  // watch dog timer via a forever loop.
  if (!MQTT.connected())
  {
    MQTTconnect();
  }

  // an example of subscription code
  Adafruit_MQTT_Subscribe * subscription; // create a subscriber object instance
  while (subscription = MQTT.readSubscription(timer)) // Read a subscription and wait for max of 2000 ms.
  { // will return 1 on a subscription being read.
    
    if (subscription == & Slider_low) // if the subscription we have receieved matches the one we are after
    {
      minTemp = atof((char * ) Slider_low.lastread);
      Serial.println(minTemp);
    }
    
    if (subscription == & Slider_high) // if the subscription we have receieved matches the one we are after
    {
      maxTemp = atof((char * ) Slider_high.lastread);
      Serial.println(maxTemp);
    }

  }

#ifdef NOPUBLISH
  if (!MQTT.ping())
  {
    MQTT.disconnect();
  }
#endif
}

void MQTTconnect(void)
{
  unsigned char tries = 0;

  // Stop if already connected.

  Serial.print("Connecting to MQTT... ");
  lcd.clear();
  lcd.print("MQTT Conn...");
  while (MQTT.connect() != 0) // while we are
  {
    Serial.println("Will try to connect again in five seconds"); // inform user
    lcd.clear();
    lcd.print("MQTT Problem...");
    MQTT.disconnect(); // disconnect
    delay(5000); // wait 5 seconds
    tries++;
    if (tries == 3) {
      Serial.println("problem with communication, forcing WDT for reset");
      while (1) {
        ; // forever do nothing
      }
    }
  }
  lcd.clear();
  lcd.print("MQTT Success...");
  Serial.println("MQTT succesfully connected!");
  lcd.clear();
  lcd.print("Present card");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
}
void bCastUDP()
{
  packetsize = UDP.parsePacket();
  if (packetsize) {
    UDP.read(inBuffer, BUFFERLEN);
    inBuffer[packetsize] = '\0';

    Serial.print("Received ");
    Serial.print(packetsize);
    Serial.println(" bytes");

    Serial.print("Contents: ");
    Serial.println(inBuffer);
    Serial.print("From IP ");
    Serial.println(UDP.remoteIP());
    Serial.print("From port ");
    Serial.println(UDP.remotePort());
  }
  IPAddress myIP = WiFi.localIP();
  String strIP = String(myIP[0]) + String(".") + String(myIP[1]) + String(".") + String(myIP[2]) + String(".") + String(myIP[3]) ;
  String strBuffIP = String(inBuffer);
  if (strIP.equals(strBuffIP))
  {
    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
    sprintf(outBuffer, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3] );
    UDP.write(outBuffer);
    UDP.endPacket();
  }
}
