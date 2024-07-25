/*#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>



String ssid = "Projeto";
String password = "2022-11-07";
const String serverName = "https://8a2d-2804-388-608f-6ae1-40bc-cecd-a445-9e7e.ngrok-free.app/passaPresenca";
JsonDocument doc; 
StaticJsonDocument<200> docPresenca;
String matriculas[5] = {"132435", "123414", "978758", "587576", "788776"};
int horarios[5];
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

String hora;

String horaRTC;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", -3 * 3600, 60000);

const byte rxPin =  14;
const byte txPin = 12;

SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin);

void setup()
{
   Serial.begin(115200);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);


   pinMode(rxPin, INPUT);
   pinMode(txPin, OUTPUT);


  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Wifi connected!");


  timeClient.begin();
  timeClient.update();
  unsigned long hora_ntp = timeClient.getEpochTime();


  mySerial.begin(115200);
}

void create_nested_json()
{
  
  JsonObject obj;
   for(int i = 0; i < 5; i++){
    obj = docPresenca.createNestedObject();
    obj["matricula"] = matriculas[i];
    obj["horario"] = hora;
  }
}


void loop() {


  timeClient.update();
  time_t hora_ntp = timeClient.getEpochTime();
  Serial.println("NTP Time server: " + timeClient.getFormattedTime());
  hora = timeClient.getFormattedTime();

  if (mySerial.available()) {
    Serial.println("Arduino mandou o horário do RTC!");
    String texto = mySerial.readStringUntil('\n');
    texto.trim();

    Serial.println("texto recebido: " + texto);
  }

  if(Serial.available())
  {
    String texto = Serial.readStringUntil('\n');
    mySerial.print(hora_ntp);
    //texto.trim();
    //mySerial.print(texto);
  }

  create_nested_json();


  if(WiFi.status()==WL_CONNECTED)
  {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    // Ignore SSL certificate validation
    client->setInsecure();


    HTTPClient https;


    if(https.begin(*client, serverName))
    {
      https.addHeader("Content-Type", "application/json");
      Serial.print("[HTTPS] POST...\n");


    }

    String json;


    StaticJsonDocument<200> docTurma;
    JsonObject turma;
    turma = docTurma.createNestedObject();

    String data;
    struct tm *ptm = gmtime ((time_t *)&hora_ntp); 
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    int currentYear = ptm->tm_year+1900;
    
    String currentDate = String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear);

    turma["data"] = currentDate;
    turma["presenca"] = docPresenca;

    serializeJson(docTurma,Serial);


    serializeJson(docPresenca, json);



    int httpCode = https.POST(json);

    Serial.println(httpCode);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);


      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        const String& payload = https.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    }
    else
    {
      Serial.printf("[HTTP] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  else
  {
    Serial.println("Desconnected Wifi!");
  }

  delay(10000);
}*/