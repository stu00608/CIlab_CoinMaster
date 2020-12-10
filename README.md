# 自製Wifi存錢箱

* 搭載街機常用之投幣孔模組以及mp3播放模組。

## 硬體零件

| 材料 | 示意圖 |
| -------- | -------- |
| WeMos D1 WiFi Arduino UNO 開發板ESP8266| ![](https://i.imgur.com/r71nrkS.png)|
| DFPlayer Mini Player Module | ![](https://i.imgur.com/JvQxQYf.png)|
|SD記憶卡| ![](https://i.imgur.com/mUzLACQ.jpg)|
| 多币值出口款投币器 | ![](https://i.imgur.com/83i6y9I.png )|
| 8Ω 1W 單體 | ![](https://i.imgur.com/x1hzGQb.png) |
| DC5V 3W+3W數位功率音效放大器 | ![](https://i.imgur.com/MFPGKNp.png)|
| AC ADAPTER 交換式電子變壓器 12V/1A | ![](https://i.imgur.com/GFOjBQ4.png) |


## 外觀設計

![](https://i.imgur.com/exv1kwQ.png)

* 使用CorelDraw設計，3mm木板雷射切割。

## 軟體設計

* 先在Arduino內新增ESP8266之開發板資訊，參見[教學](https://sites.google.com/site/wenyumaker/10-esp8266/02memos-d1?tmpl=%2Fsystem%2Fapp%2Ftemplates%2Fprint%2F&showPrintDialog=1)。
* 若找不到函式庫，請自行搜尋下載。
* 修改想要連線的網路ssid與password。
* 程式執行後若有連接到網路則會在Serial Port顯示IP位址，可以根據上面的IP位址去連線，記得要在同一個網域底下。

#### Main Program

```c=
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
```

#### DFRobotDFPlayerMini

* 參考投幣音效：https://www.youtube.com/watch?v=iA6XpqaZvCU

```c=
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

void MP3_setup(){
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  
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
```

#### ESP8266WiFi

* 對網頁程式不熟的我這邊真的是個難關，花了一段時間才理解裡面的原理。Wifi+網頁UI的部分共有3個函式負責。

```c=
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
```

* `WifiSetup()`函式用來在Arduino初始化時連接到指定Wifi網路，其中的`WiFi.begin(ssid, password)`就是連線的函式，輸入值分別是Wifi名稱和密碼。最後會印出這個Wifi的IP位址，在Client端就依靠這個IP進入底下的網頁。

```c=
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
```

* `ClientHandler()`函式負責處理有Client連線到這個位址時的操作，我們用`server.available()`來偵測，在確認沒有Timeout後就會進入到`showUI()`來顯示網頁內容。

```c=
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
```

* `showUI()`函式負責處理網頁內容顯示。8~11行是html的描述，接下來的判斷式則是判斷有沒有進到網頁的某個index，而我們在底下會用按鈕超連結導向到指定index執行重設和重新整理，再來就跟一般html的寫法一樣，這裡的樣式採用css。

#### EEPROM 寫入

```c=
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
```

* 我們預期金額會是一筆相當大的數字(雖然不太可能)，所以我們將金額以`long`的形式儲存，在寫入記憶體時就得要把每個byte拆分開來儲存，值得注意的地方是Wemos這一塊若要將資料寫進硬體(斷電保存)，只有4K的空間，而且必須執行`EEPROM.commit()`才會真正存進去。

## 照片

[影片Demo](https://youtu.be/Ekf3MOymUUg)

![](https://i.imgur.com/jNEKNJb.jpg)

![](https://i.imgur.com/ysVI4uo.jpg)

![](https://i.imgur.com/DX2rPAO.jpg)




## 總結

* Wemos D1這塊板子的腳位非常不好找，不知道是不是我買的版本問題，腳位上的位置和實際取用的位置不同，有的還有重複(暈)，建議想要實作的人可以用一般Arduino外接ESP8266就好。
* 投幣機的訊號是用脈衝的方式傳遞，在Arduino內可以用`pulsein()`來抓取，但是如果連續投幣，會抓不到投的硬幣種類，所以我直接在指示燈上外接一條線input到Arduino的腳位來解決這個問題，就不使用投幣機本身的訊號線了。

## 參考

* https://randomnerdtutorials.com/esp8266-web-server-with-arduino-ide/
* https://www.w3schools.com/css/default.asp
* https://zhuanlan.zhihu.com/p/103756212

:pancakes: Author : 沈奕辰

##### tags: `Arduino` 