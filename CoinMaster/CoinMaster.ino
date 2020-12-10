/////////////////////////////////////////////////
//                  wifi存錢箱                  //
//                Shen Yi Chen                 //
//              made by TKU CIlab              //
//  https://hackmd.io/p0ZqdZ3JTIezzjYqR2EEIA   //
/////////////////////////////////////////////////

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#define M_ADDRESS 0x00
#define EEPROM_SIZE 100

// Replace with your network credentials
const char* ssid     = "stu00608";
const char* password = "kuyastu5220";

WiFiServer server(80);
SoftwareSerial mySoftwareSerial(D6, D7); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Variable to store the HTTP request
String header;
String moneyString;

long money;
int coin_1,coin_5,coin_10,coin_50;
int last_coin_1,last_coin_5,last_coin_10,last_coin_50;
int resetFlag;

unsigned long currentTime = millis();
unsigned long countTime = millis();

unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  MP3_setup();
  WifiSetup();
  
  money = EEPROMReadlong(M_ADDRESS);
  Serial.print("money now is : ");
  Serial.println(String(money));
  

  pinMode(D5,INPUT);
  pinMode(D2,INPUT);
  pinMode(D4,INPUT);
  pinMode(D3,INPUT);

  countTime = millis();
}

void loop(){
  
  ClientHandler();

  if( (coin_1=digitalRead(D5)) || (coin_5=digitalRead(D2)) || (coin_10=digitalRead(D4)) || (coin_50=digitalRead(D3)) ){
    money = EEPROMReadlong(M_ADDRESS);
    if(coin_1 && !last_coin_1){
      myDFPlayer.play(1);
      Serial.println("您投入了1元");
      money += 1;
    }else if(coin_5 && !last_coin_5){
      myDFPlayer.play(1);
      Serial.println("您投入了5元");
      money += 5;
    }else if(coin_10 && !last_coin_10){
      myDFPlayer.play(1);
      Serial.println("您投入了10元");
      money += 10;
    }else if(coin_50 && !last_coin_50){
      myDFPlayer.play(1);
      Serial.println("您投入了50元");
      money += 50;
    }
    EEPROMWritelong(M_ADDRESS,money);
  }

  last_coin_1 = coin_1;
  last_coin_5 = coin_5;
  last_coin_10 = coin_10;
  last_coin_50 = coin_50;
  
}


void MP3_setup(){
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  myDFPlayer.volume(10);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.setTimeOut(500);
}

void WifiSetup(){
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void ClientHandler(){
  unsigned long startTime = millis();
  WiFiClient client = server.available();   

  if (client) {                            
    Serial.println("New Client.");          
    String currentLine = "";                
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { 
      currentTime = millis();         
      if (client.available()) {             
        char c = client.read();             
        Serial.write(c);                    
        header += c;
        if (c == '\n'){                    
          if (currentLine.length() == 0) {
            showUI(client);
            break;
          }else{ 
            currentLine = "";
          }
        }else if (c != '\r'){
          currentLine += c;      
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.print("Used Time : ");
    Serial.println(String(millis()-startTime));
    Serial.print("money now : ");
    money = EEPROMReadlong(M_ADDRESS);
    Serial.println(String(money));
    Serial.println("");
  }
}


void showUI(WiFiClient client){
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:

  moneyString = String(money);
  resetFlag = (money)? false:true;
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  
  // turns the GPIOs on and off
  if (header.indexOf("GET /RESET") >= 0) {
    Serial.println("GET Reset Button");
    resetFlag = true;
    money = 0;
    EEPROMWritelong(M_ADDRESS,money);
    money = EEPROMReadlong(M_ADDRESS);
    client.print("<HEAD>");
    client.print("<meta http-equiv=\"refresh\" content=\"0;url=/\">");
    client.print("</head>");
  }else if (header.indexOf("GET /REFRESH") >= 0) {
    Serial.println("GET Refresh Button");
    client.print("<HEAD>");
    client.print("<meta http-equiv=\"refresh\" content=\"0;url=/\">");
    client.print("</head>");
  }
  //client.println("");
  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
    client.println("<head>");
      
      client.println("<title>投幣機</title>");
      client.println("<meta charset=\"utf-8\">");
      client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
      client.println("<style>"); 
        //client.println("body {margin: 0;font-family: Arial, Helvetica, sans-serif;}");
        client.println("* {  box-sizing: border-box;  font-family: 微軟正黑體, Helvetica, sans-serif;}");
        client.println("body {  margin: 0;  font-family: 微軟正黑體, Helvetica, sans-serif;}");
        client.println(".button {  display: inline-block;  border-radius: 3px;  background-color: #f4511e;  border: none;  color: #FFFFFF;  text-align: center;  font-size: 20px;padding: 12px;  width: 200px;  transition: all 0.5s;  cursor: pointer;  margin: 4px;}");
        client.println(".content {  background-color: #FFFFFF;  padding: 10px;}");
        client.println(".footer {  background-color: #f1f1f1;  padding: 10px;}");
      client.println("</style>"); 
    client.println("</head>");
  
    client.println("<body>");
      client.println("<h1>投幣機</h1>");
      client.println("<h2>目前金額 : " + moneyString + "</h2>");
      client.println("<button class=\"button\" style=\"vertical-align:middle\"><a href=\"/RESET\">重置</a></button>");
      client.println("<button class=\"button\" style=\"vertical-align:middle\"><a href=\"/REFRESH\">重新整理</a></button>");
      if(resetFlag){
        client.println("<p>money reset!</p>");
      }
    client.println("</body></html>");
   
  
  client.println();
}

void EEPROMWritelong(int address, long value){
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      EEPROM.commit();
}
long EEPROMReadlong(long address){
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
