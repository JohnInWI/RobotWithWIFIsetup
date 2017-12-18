/*
This example will open a configuration portal when the reset button is pressed twice. 
This method works well on Wemos boards which have a single reset button on board. It avoids using a pin for launching the configuration portal.

How It Works
When the ESP8266 loses power all data in RAM is lost but when it is reset the contents of a small region of RAM is preserved. So when the device starts up it checks this region of ram for a flag to see if it has been recently reset. If so it launches a configuration portal, if not it sets the reset flag. After running for a while this flag is cleared so that it will only launch the configuration portal in response to closely spaced resets.

Settings
There are two values to be set in the sketch.

DRD_TIMEOUT - Number of seconds to wait for the second reset. Set to 10 in the example.
DRD_ADDRESS - The address in RTC RAM to store the flag. This memory must not be used for other purposes in the same sketch. Set to 0 in the example.

This example, contributed by DataCute needs the Double Reset Detector library from https://github.com/datacute/DoubleResetDetector .
*/
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiClient.h>
//needed for library
#include <ESP8266WebServer.h>
//#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/kentaylor/WiFiManager
#include "DHT.h"
#include <DoubleResetDetector.h>  //https://github.com/datacute/DoubleResetDetector

// Number of seconds after reset during which a 
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#define DHTPIN 13    // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

#define TRIGGER 14
#define ECHO    12

DHT dht(DHTPIN, DHTTYPE, 11);

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);


char robotName[10] = "Robot203";

// Onboard LED I/O pin on NodeMCU board
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. Controls the onboard LED.

// Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
bool initialConfig = false;
const int numberOfPieces = 6;
String pieces[numberOfPieces];
String input = "";
int counter = 0;
int lastIndex = 0;

 long duration, distance;
 String arrived="not Arrived";
ESP8266WebServer server(80);

void handleRoot() {
  
  server.send(200, "text/plain", "hello from your robot!");
   
}

void handleNotFound(){
 
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
            

  for (uint8_t i=0; i<server.args(); i++){
    //message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    message +=  server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  
}
void setup() {
  // put your setup code here, to run once:
  // initialize the LED digital pin as an output.
  pinMode(PIN_LED, OUTPUT);
 //these are the yellow motor pins
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  //pinMode(2, OUTPUT); same as PIN_LED

pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(16, INPUT); //line sensor- this causes firmware flashing issues... make low or unplug to upload

  
  Serial.begin(115200);
  Serial.println("\n Starting");
  WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed
  if (WiFi.SSID()==""){
    Serial.println("We haven't got any access point credentials, so get them now");   
    initialConfig = true;
  }
  if (drd.detectDoubleReset()) {
    Serial.println("Double Reset Detected");
    initialConfig = true;
  }
  if (initialConfig) {

    Serial.println("Starting configuration portal.");
    digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
    
   // WiFiManagerParameter input_field("<input type='text' placeholder='Enter your own IP here' />");
    //Local intialization. Once its business is done, there is no need to keep it around

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //wifiManager.setConfigPortalTimeout(600);
    WiFiManager wifiManager;

WiFiManagerParameter custom_userIP("ip", "IP address", "", 40);
wifiManager.addParameter(&custom_userIP);

 //   wifiManager.addParameter(&input_field);

    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }

    digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up 
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);
  }

  digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
  WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
  unsigned long startedAt = millis();
  Serial.print("After waiting ");
  int connRes = WiFi.waitForConnectResult();
  float waited = (millis()- startedAt);
  Serial.print(waited/1000);
  Serial.print(" secs in setup() connection result is ");
  Serial.println(connRes);
  if (WiFi.status()!=WL_CONNECTED){
    Serial.println("failed to connect, finishing setup anyway");
  } else{
    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());
  }
  dht.begin();

   server.begin();
   if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
    
  }
  server.on("/", handleRoot);
   server.on("/motor", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    input=message; 
           parseCommand();   
            Serial.println(pieces[0]);
                  Serial.println(pieces[1]);
                  Serial.println(pieces[2]);
                  Serial.println(pieces[3]);
                  Serial.println(pieces[4]);
                  Serial.println(pieces[5]);
                  analogWrite(5, pieces[0].toInt());
                  analogWrite(4, pieces[2].toInt());
                  digitalWrite(0, pieces[1].toInt());
                  digitalWrite(2, pieces[3].toInt());
    
    
  }
    server.send(200, "text/plain",  message);

  
  });

  server.on("/timedMove", [](){ //motor 1 speed, motor 1 direction,motor 2 speed, motor 2 direction, time in milliseconds
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    input=message; 
           parseCommand();   
            Serial.println(pieces[0]);
                  Serial.println(pieces[1]);
                  Serial.println(pieces[2]);
                  Serial.println(pieces[3]);
                  Serial.println(pieces[4]);
                  Serial.println(pieces[5]);
                  analogWrite(5, pieces[0].toInt());
                  analogWrite(4, pieces[2].toInt());
                  digitalWrite(0, pieces[1].toInt());
                  digitalWrite(2, pieces[3].toInt());
                  delay(pieces[4].toInt());
                  analogWrite(5, 0);
                  analogWrite(4, 0);
    
  }
    server.send(200, "text/plain",  message);

  
  });
server.on("/digitalWrite", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    input=message; 
           parseCommand();   
                  Serial.println(pieces[0]);
                  Serial.println(pieces[1]);
                  Serial.println(pieces[2]);
                  Serial.println(pieces[3]);
                  Serial.println(pieces[4]);
                  Serial.println(pieces[5]); 
                  Serial.println("this is a digital write");  
                  pinMode(pieces[0].toInt(), OUTPUT);               
                  digitalWrite(pieces[0].toInt(), pieces[1].toInt());
                  
    
    
  }
    server.send(200, "text/plain",  message);

  
  });

  server.on("/lineFollow", [](){    //enable=1/disable=0,side 0=right 1=left, motor speed, time or distance-t or d, time value millisec or distance value cm
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    input=message; 
           parseCommand();   
                  Serial.println(pieces[0]);
                  Serial.println(pieces[1]);
                  Serial.println(pieces[2]);
                  Serial.println(pieces[3]);
                  Serial.println(pieces[4]);
                  Serial.println(pieces[5]); 
                
                
                  
    
    
  }
    server.send(200, "text/plain",  message);
    if ( pieces[1].toInt()==0){
      Serial.println("this is a line follow right");  
    lineFollowRight();
      
    }

     if ( pieces[1].toInt()==1){
    lineFollowLeft();
    }
  
  });

  server.on("/analogWrite", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    input=message; 
           parseCommand();   
                  Serial.println(pieces[0]);
                  Serial.println(pieces[1]);
                  Serial.println(pieces[2]);
                  Serial.println(pieces[3]);
                  Serial.println(pieces[4]);
                  Serial.println(pieces[5]);   
                  pinMode(pieces[0].toInt(), OUTPUT);               
                  analogWrite(pieces[0].toInt(), pieces[1].toInt());
                  
    
    
  }
  
    server.send(200, "text/plain",  message);

  
  });

 server.on("/analogRead", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    pinMode(A0, INPUT);
    
    
  }
    server.send(200, "text/plain",  String(analogRead(A0), DEC));

  
  });
  server.on("/distanceWait", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
        
    
  }
    server.send(200, "text/plain",  arrived);
    if (arrived=="arrived"){
      
    arrived="not arrived";
    }
  });

   server.on("/distance", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n"; 
    
    
  }
 distanceSense();

  Serial.print(distance);
  Serial.println("  Centimeter:");

    server.send(200, "text/plain",  String(distance)+ " cm");

  
  });

  server.on("/tempHumRead", [](){
    String message = "";
    for (uint8_t i=0; i<server.args(); i++){
    message +=  server.arg(i) + "\n";
    
    
  }
  
 
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (std::isnan(h) || std::isnan(t) || std::isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    server.send(200, "text/plain",  "Failed to read from DHT sensor!  Try Again");
    return;
  }
  

    server.send(200, "text/plain",  "temp: "+ String(f, DEC)+ "     hum: "+ String(h, DEC));

  
  });
    
  server.onNotFound(handleNotFound);
}



void loop() {
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd.loop();

 


  // put your main code here, to run repeatedly:
  server.handleClient();
  
}



void parseCommand()
{

 for (int i = 0; i < input.length()+1; i++) {
        // Loop through each character and check if it's a comma
        if (input.substring(i, i+1) == ",") {
          // Grab the piece from the last index up to the current position and store it
          pieces[counter] = input.substring(lastIndex, i);
          // Update the last position and add 1, so it starts from the next character
          lastIndex = i + 1;
          // Increase the position in the array that we store into
          counter++;
        }

        // If we're at the end of the string (no more commas to stop us)
        if (i == input.length() - 1) {
          // Grab the last part of the string from the lastIndex to the end
          pieces[counter] = input.substring(lastIndex, i);
        }
      }
      

      // Clear out string and counters to get ready for the next incoming string
      input = "";
      counter = 0;
      lastIndex = 0;
    }



void lineFollowRight(){   //protocol: enable/disable, speed, 
  //unsigned long time = millis();
   Serial.println("you arrived");
   Serial.println(pieces[0]);
  while (pieces[0].toInt()==1){
    delay(20);
        //linefollow right
        if (digitalRead(16)==LOW){
          
                         analogWrite(5,pieces[2].toInt());
                          analogWrite(4, 0);
                          digitalWrite(0,0);
                          digitalWrite(2, 1);
          }
        if (digitalRead(16)==HIGH){
                          analogWrite(5,0);
                          analogWrite(4, pieces[2].toInt());
                          digitalWrite(0,1);
                          digitalWrite(2, 0);
          }
        if (pieces[3].toInt() !=0){
   delay(20);
    distanceSense();
    delay(20);
    Serial.println("distance =");
    Serial.println(distance);
    //server.send(200, "text/plain",  String(distance)+ " cm");
    if (distance <= pieces[3].toInt()){
    arrived="arrived";
       pieces[3]='0';
      break;
     
  }
  }
          server.handleClient();
          if (pieces[0].toInt()==0) {
                
            break;
          }
        }
        analogWrite(5,0);
        analogWrite(4, 0);
}


void lineFollowLeft(){
  
 while (pieces[0].toInt()==1){
    delay(20);
         //linefollowLeft
if (digitalRead(16)==HIGH){
  analogWrite(5,pieces[2].toInt());
                  analogWrite(4, 0);
                  digitalWrite(0,0);
                  digitalWrite(2, 1);
  }
if (digitalRead(16)==LOW){
  analogWrite(5,0);
                  analogWrite(4, pieces[2].toInt());
                  digitalWrite(0,1);
                  digitalWrite(2, 0);
  } 
  if (pieces[3].toInt() !=0){
   delay(20);
    distanceSense();
    delay(20);
    Serial.println("distance =");
    Serial.println(distance);
   
    if (distance <= pieces[3].toInt()){
       arrived="arrived";
      pieces[3]='0';
      break;
  }
  }
        
          server.handleClient();
          if (pieces[0].toInt()==0){
                
            break;
          }
        }
        analogWrite(5,0);
        analogWrite(4, 0);
  
  
}


void distanceSense(){
  digitalWrite(TRIGGER, LOW);  
  delayMicroseconds(2); 
  
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10); 
  
  digitalWrite(TRIGGER, LOW);
  duration = pulseIn(ECHO, HIGH);
  distance = (duration/2) / 29.1;
}
