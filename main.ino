#include <WiFi.h>
#include <IRremote.h>
#include "time.h"

const char* ntpServer =  "ntp.jst.mfeed.ad.jp";
const long  gmtOffset_sec = 9 * 3600;
const int   daylightOffset_sec = 0;

const char* ssid = "SPWH_H33_6A7748";
const char* password = "qt09932h17qy2ba";
const int pin_led = 18;

bool set_time_flag=false;
unsigned int count = 0;

const char html[]=
"<!DOCTYPE html>\
<html>\
<head>\
<meta name='viewport' content='initial-scale=1.5' charset='utf-8'>\
</head>\
<body>\
<form method='get'>\
<font size='4'>部屋の電気タイマー<br>\
</font><br>\
<br>\
<input type='submit' name=0 value='ON' style='background-color:#88ff88; color:red;'>\
<input type='submit' name=1 value='設定' style='background-color:#88ff88; color:red;'>\
<input type='time' name='timer' value='00:00'>\
</form>\
</body>\
</html>";

IRsend irsend(2);
 
WiFiServer server(80);

class alerm
{
private:
    struct tm alert_time_;
    bool switch_=false;
public:
    void setAlertTime_(struct tm time);
    void setAlertTimeHourMin(int h,int m);
    struct tm getAlertTime_();
    void setON_();
    void setOFF_();
    bool checkAlerm_();
};

void alerm::setAlertTime_(struct tm time){
    this->alert_time_=time;
};

void alerm::setAlertTimeHourMin(int h,int m){
    this->alert_time_.tm_hour = h;
    this->alert_time_.tm_min = m;
}

struct tm alerm::getAlertTime_(){
    return this->alert_time_;
}

void alerm::setON_(){
    this->switch_=true;
}

void alerm::setOFF_(){
    this->switch_=false;
}

bool alerm::checkAlerm_(){
    if (this->switch_){
        return 0;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return 0;
    }

    if (timeinfo.tm_hour == this->alert_time_.tm_hour && timeinfo.tm_min == this->alert_time_.tm_min){
        return true;
    }

    return 0;
}

alerm alm;


void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%Y %m %d %a %H:%M:%S");//日本人にわかりやすい表記へ変更
}

void setup()
{
    delay(1000);
    Serial.begin(115200);
    pinMode(pin_led, OUTPUT);      // set the LED pin mode
 
    delay(10);
 
    Serial.println();
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
     
    server.begin();
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
}
 
void loop(){
  WiFiClient client = server.available();

  if (millis()%60000 == 0){
      if(alm.checkAlerm_()){
          Serial.println("ALERT!");
          irsend.sendNEC(0x41B658A7, 32);
      }
  }
 
  if (client) {
    Serial.println("new client");
    String currentLine = "";
    String set_time;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            client.print(html);
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        if (set_time_flag == true){
            set_time+=c;
            count += 1;
            if (count == 7){
                set_time_flag = false;
                alm.setAlertTimeHourMin((set_time[0]-48)*10+(set_time[1]-48),(set_time[5]-48)*10+(set_time[6]-48));
            }
        }

        Serial.println(currentLine);
 
        if (currentLine.endsWith("GET /?0=ON")) {
            irsend.sendNEC(0x41B658A7, 32);
        }

        if (currentLine.endsWith("GET /?1=%E8%A8%AD%E5%AE%9A&timer=")){
            set_time_flag = true;
            count = 0;
            set_time = "";
        }
      }
    }
     
    client.stop();
    Serial.println("client disconnected");
    Serial.println(alm.getAlertTime_().tm_hour);
    Serial.println(alm.getAlertTime_().tm_min);
    printLocalTime();
  }
}