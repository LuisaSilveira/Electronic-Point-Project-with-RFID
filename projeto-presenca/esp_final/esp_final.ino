#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>


String ssid = "Projeto";
String password = "2022-11-07";
const String POSTRouteServer = "https://6henp5-ip-179-242-48-91.tunnelmole.net/recebeEEnvia";
//const String serverName2 = "https://6henp5-ip-179-242-48-91.tunnelmole.net/recebeCadastro";
const String GETRouteServer = "https://6henp5-ip-179-242-48-91.tunnelmole.net/recebeEEnvia";
//const String serverName = "https://215fd557-a5b8-4fcd-aa2d-0f76fbb13625-00-4flode0l653d.kirk.replit.dev/message";
JsonDocument doc;
StaticJsonDocument<200> docPresenca;
String matriculas[5] = { "132435", "123414", "978758", "587576", "788776" };
int horarios[5];
String weekDays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

String hora;
String arduino_says;

//json server
String jsonPresenca;
String jsonCadastro;
//

//json espelho
DynamicJsonDocument docEspelho(2048);
bool send_data = false;

String horaRTC;
time_t hora_ntp;

bool inicioPrograma = false;
bool esp_tah_ativo = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", -3 * 3600, 60000);

unsigned long millisNTP = millis();
unsigned long millisAtualizaRTC = millis();

const byte rxPin = 12;
const byte txPin = 14;

SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);


void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  mySerial.setTimeout(10000);


  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);


  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Wifi connected!");


  timeClient.begin();
  timeClient.update();
  unsigned long hora_ntp = timeClient.getEpochTime();

  mySerial.println("Conexao com ESP8266 estabelecida!");
}

void create_nested_json() {

  JsonObject obj;
  for (int i = 0; i < 5; i++) {
    obj = docPresenca.createNestedObject();
    obj["matricula"] = matriculas[i];
    obj["horario"] = hora;
    obj["turma"] = "33A";
  }
}

void post_dados_server(const JsonDocument& _doc) {

  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    // Ignore SSL certificate validation
    client->setInsecure();

    HTTPClient https;


    if (https.begin(*client, POSTRouteServer)) {
      https.addHeader("Content-Type", "application/json");
      Serial.print("[HTTPS] POST...\n");
    }

    //if (https.begin(*client, serverName2)) {
    //https.addHeader("Content-Type", "application/json");
    //Serial.print("[HTTPS] POST...\n");
    // }

    String jsonDados;

    // String data;
    // struct tm *ptm = gmtime((time_t *)&hora_ntp);
    // int monthDay = ptm->tm_mday;
    // int currentMonth = ptm->tm_mon + 1;
    // int currentYear = ptm->tm_year + 1900;

    // String currentDate = String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear);

    //serializeJson(docTurma,Serial);

    //subindo dados para o servidor
    serializeJson(_doc, jsonDados);

    int httpCode = https.POST(jsonDados);

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = https.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      //Serial.printf("[HTTP] POST... failed, error: %s\n", https.errorToString(httpCode2).c_str());
    }
    https.end();
  } else {
    Serial.println("Desconnected Wifi!");
  }
}

void get_dados_server() {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    // Ignore SSL certificate validation
    client->setInsecure();

    HTTPClient https;

    if (https.begin(*client, GETRouteServer)) {
      https.addHeader("Content-Type", "application/json");
      Serial.print("[HTTPS] GET...\n");

      int httpCode = https.GET();

      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
          //ver o tamanho do documento enviado pelo server
          deserializeJson(docEspelho, https.getStream());
          send_data = true;
        }

      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
    }

  } else {
    Serial.println("Desconnected Wifi!");
  }
}

void loop() {

  if (millis() - millisNTP > 5000) {
    timeClient.update();
    hora_ntp = timeClient.getEpochTime();
    Serial.println("NTP Time server: " + timeClient.getFormattedTime());
    hora = timeClient.getFormattedTime();
    millisNTP = millis();
  }

  if (inicioPrograma == false) {
    if (mySerial.available()) {
      Serial.println("aqui");
      String arduino_says = mySerial.readStringUntil('\n');
      arduino_says.trim();
      if (arduino_says == "comecou programa") {
        Serial.println("entremos");
        inicioPrograma = true;
        mySerial.println(hora_ntp);
      }
    }
  } else {
    if (mySerial.available()) {
      Serial.println("Arduino mandou o doc de presencas e cadastros!");
      //comunicação com o arduino
      StaticJsonDocument<300> docPres;
      DeserializationError err = deserializeJson(docPres, mySerial);

      if (err == DeserializationError::Ok) {
        Serial.println("deu bom");
        serializeJson(docPres, Serial);
        post_dados_server(docPres);
        get_dados_server();
      } else {
        Serial.print("deserializeJson() returned ");
        Serial.println(err.c_str());
      }
    }

    if (millis() - millisAtualizaRTC > 20000)  //se já se passou mais de meia hora manda o horario de novo
    {
      mySerial.println(hora_ntp);
      millisAtualizaRTC = millis();
    }

    if (send_data) {
      send_data = false;
      Serial.println("Dados recebidos do servidor, enviando para o arduino....");
      mySerial.println("mandando json!");
      serializeJson(docEspelho, mySerial);
      //serializeJson(docEspelho, mySerial);
    }
  }
}