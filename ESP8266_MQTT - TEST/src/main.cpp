#include <Arduino.h>

#include<ESP8266WebServer.h>
#include<ESP8266WiFi.h>
#include<SoftwareSerial.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

// Macro
#define DEN D2
#define QUAT D1
#define NUTDEN D6
#define NUTQUAT D3
SoftwareSerial mySerial(D4,D5); // Rx Tx    

// D4 - PB10
// D5 - zPB11
// put function declarations here:
const char* ssid = "TP-Link_AD56";
const char* pass = "44446666";
// tạo ra biến để lưu thông số server



const char* mqtt_server = "b37.mqtt.one"; //
int mqtt_port = 1883;
const char* mqtt_user = "58kpuw3237";
const char* mqtt_pass = "23afijkqtv";
String topicsub = "58kpuw3237/databackend"; // nhận dữ liệu
String topicpub = "58kpuw3237/dataespsend"; // gửi dữ liệu
WiFiClient espClient;
PubSubClient client(espClient);


String DataMqttJson = "";

int bienTB1 = 0;
int bienTB2 = 0;

long last1 = 0;

int check=0;
int biengui = 0;
//data esp send
int Pre = 0;
uint16_t Next=0;
int State = 0;
uint16_t vantoc = 0;
long vatcan = 0;
uint16_t PIN = 0;
String preWay="a";

long C1 = 500;
long C2 = 1000;
long C3  = 4000;
String Data = "";
long last = 0;


void ConnectWiFi();
void ConnectMqtt();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void DuytriMQTT();

void Chuongtrinhcambien();

void ParseJson(String Data);
void SendDataMQTT();
void khoi_tao_Map();
void DjikStra(int s, int t);
void customSwap(int &a, int &b) ;

void setup()
{
  Serial.begin(9600);
  while(!Serial);
  mySerial.begin(9600);

  ConnectWiFi();
  ConnectMqtt();
}

void loop()
{

  // duy trì kết nối MQTT broker
  DuytriMQTT();
  SendDataMQTT();
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message topic: ");
  Serial.println(topic);
  for (int i = 0; i < length; i++)
  {
    // ghép Data = thành chuỗi tổng để xử lý
    Data += (char)payload[i];
  }
  Serial.print("Data nhận MQTT: ");
  Serial.println(Data);
  // xử lý tại chỗ JSON

  ParseJson(String(Data));
  

  Data = "";
}

void ParseJson(String Data)
{
  // đưa dữ liệu json vào thư viện json để kiểm tra đúng hay sai , đúng thì tách dữ liệu => điều khiển

  const size_t capacity = JSON_OBJECT_SIZE(2) + 256;
  DynamicJsonDocument JSON1(capacity);
  DeserializationError error1 = deserializeJson(JSON1, Data);
  if (error1)
  {
    Serial.println("Data JSON Error!!!\n");
    return;
  }
  else
  {
    Serial.println();
    Serial.println("Data JSON ESP: ");
    serializeJsonPretty(JSON1, Serial);
    Serial.println(" ");
    Serial.println("Chuoi gui xuong STM32:");
    
   if(JSON1["AGV"]=="1" ){
    String Path = JSON1["path"];
    //Serial.println(Path);
  //  if(check == 0){
    //  Serial.println("Da vao dc if");
      if(Path != "STOP" && Path !="COMEBACK" && Path !="CONTINUE"){
      String temp="1_00001110_2_00000101_3_00000001_4_00000000";
      Serial.println(temp);
      mySerial.println(temp);
      }else if (Path =="STOP"){
        Serial.println("STOP");
        mySerial.println("STOP");
      }else if(Path =="COMEBACK"){
        Serial.println("COMEBACK");
        mySerial.println("COMEBACK");
      }else if (Path == "CONTINUE"){
         Serial.println("CONTINUE");
        mySerial.println("CONTINUE");
      }
      
     // check=1;
  //  }
   }
//   if(JSON1["AGV"]=="100"){
 //   check=0;
 //  }
  }
}
/*    if (JSON1["TB2"] == "0")
    {
      // OFF TB
      mySerial.write("STOP\n");
      TB2 = 0;
    }
     else if (JSON1["TB2"] == "1")
    {
      // ON TB 1
    String wayValue = JSON1["Way"];
    mySerial.println(wayValue);
  
      TB2 = 1;
    }
  
    
    if (JSON1["TB3"] == "0")
    {
      

      // OFF TB
      mySerial.write("STOP\n");
      TB3 = 0;
    }
    else if (JSON1["TB3"] == "1")
    {
      // ON TB 1
    String wayValue = JSON1["Way"];
    mySerial.println(wayValue);
  
      TB3 = 1;
    }
    
    else if (JSON1.containsKey("C1") == 1)
    {
      String DataC1 = JSON1["C1"];
      C1 = DataC1.toInt();
      Serial.print("C1: ");
      Serial.println(C1);
    }
    else  if (JSON1.containsKey("C2") == 1)
    {

      String DataC2 = JSON1["C2"];
      C2 = DataC2.toInt();
      Serial.print("C2: ");
      Serial.println(C2);

    }
  }
*/

void Datajson(String DataAGV,String DataState,String DataVT,String DataVC, String DataPIN,String DataPre,String DataNext,String weight)
{
  DataMqttJson  = "{\"car_id\":\"" + String(DataAGV) + "\"," +
                  "\"carState\":\"" + String(DataState) + "\"," +
                  "\"carObstac\":\"" + String(DataVC) + "\"," +
                  "\"carSpeed\":\"" + String(DataVT) + "\"," +
                  "\"carBattery\":\"" + String(DataPIN) + "\"," +
                  "\"previousNode\":\"" + String(DataPre) + "\"," +
                  "\"weight\":\"" + String(weight) + "\","
                  "\"nextNode\":\"" + String(DataNext) + "\"}";
  Serial.println();
  Serial.print("DataMqttJson: ");
  Serial.println(DataMqttJson);

  client.publish(topicpub.c_str(), DataMqttJson.c_str());

  // publish(tocpic , data)

}

void SendDataMQTT()
{
  if (millis() - last >= 10000)
  {
    if (client.connected())
    {
      Chuongtrinhcambien();
      Datajson(String(1),String(State),String(vantoc),String(vatcan),String(PIN),String(Pre),String(Next),String(18));
    }
    last = millis();
  }
}

void DuytriMQTT()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}
// char a[]


// hàm kiểm tra và kết nối MQTT
void reconnect()
{

  while (!client.connected())
  {
    String clientId = String(random(0xffff), HEX); // các id client esp không trung nhau => không bị reset server
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("Connected MQTT ");

      client.subscribe(topicsub.c_str());
    }
    else
    {
      Serial.println("Disconnected MQTT mqtt.MQTT1.com!!!");
      delay(5000);
    }
  }
}



void  ConnectMqtt()
{
  client.setServer(mqtt_server, mqtt_port); // sét esp client kết nối MQTT broker
  Serial.println("Connected MQTT ");
  delay(10);
  client.setCallback(callback); // => đọc dữ liệu mqtt broker mà esp subscribe
  delay(10);
 
  
}
/*


  if (millis() - last >= 1000)
  {
    Chuongtrinhcambien();
    Datajson(String(nhietdo), String(doam), String(TB1), String(TB2), String(C1) , String(C2) , String(C3) , String(HenGioDen), String(HenGioQuat));
    last = millis();
  }
*/





void Chuongtrinhcambien()
{
  if (mySerial.available() >= 2) {
    uint8_t receive_bytes[2];
    mySerial.readBytes(receive_bytes, 2);
    // Chuyển đổi dữ liệu nhận được thành biến vận tốc kiểu uint16_t
    uint16_t speed = (receive_bytes[0] << 8) | receive_bytes[1];
    
    // Convert 'speed' to a string (chuoi)
    String chuoi = String(speed);
    Serial.print("Received data as string: ");
    Serial.println(chuoi);

    // Now, let's parse the string
    char char_array[50];
    chuoi.toCharArray(char_array, sizeof(char_array));

    // Sử dụng strtok để tách chuỗi
    char *token = strtok(char_array, "/");
  
    // Lấy giá trị của từng phần tử
    int vitri = atoi(token);

    token = strtok(NULL, "/");
    vantoc = atoi(token);

    token = strtok(NULL, "/");
    vatcan = atoi(token);

    token = strtok(NULL, "/");
    
      // Use atoi to convert the string to an integer
  }
  PIN++;
  vantoc+=2;
}



void ConnectWiFi()
{
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  //=============================================================
  Serial.println();
  Serial.println("Connect WiFi");
  Serial.print("Address IP esp: ");
  Serial.println(WiFi.localIP());
}

