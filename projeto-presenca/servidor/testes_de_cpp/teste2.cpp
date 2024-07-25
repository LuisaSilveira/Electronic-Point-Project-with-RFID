/*#include <Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <MCUFRIEND_kbv.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SdFat.h>
#include <TimeLib.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecureBearSSL.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>

String ssid = "Projeto";
String password = "2022-11-07";
const String serverName = "https://1xrlin-ip-139-82-247-136.tunnelmole.net/passaPresenca";
//const String serverName = "https://215fd557-a5b8-4fcd-aa2d-0f76fbb13625-00-4flode0l653d.kirk.replit.dev/message";
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

// Display
MCUFRIEND_kbv tela;

// Teclado
const byte qtdLinhas = 4;
const byte qtdColunas = 3;

char matriz_teclas[qtdLinhas][qtdColunas] = {
    {'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};

byte PinosqtdLinhas[qtdLinhas] = {35, 45, 43, 39};
byte PinosqtdColunas[qtdColunas] = {37, 33, 41};  
Keypad meuteclado = Keypad(makeKeymap(matriz_teclas), PinosqtdLinhas,
                           PinosqtdColunas, qtdLinhas, qtdColunas);

// RFID
#define RFID_SS_PIN 53
#define RFID_RST_PIN 48


#define SD_CS_PIN 10
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 12
#define SD_SCK_PIN 13

SdFat SD;
SoftSpiDriver<SD_MISO_PIN, SD_MOSI_PIN, SD_SCK_PIN> softSpi;

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

const char espelho[] = "espelho.json";
const char arqPresenca[] = "presenca.json";
const char inserir[] = "inserir2.json";
bool leituraAtiva = false;
bool est_tela = false;


String globalCurrentTime;

bool achado = false;
String matricula;
int estado_tela = 0;
String alunoo; // Pegar o nome e a matricula na presença pelo json espelho
String alunoo2;
String matricula2;
char tecla_anterior = "";
int num_turma;

int cadastrado = 0;
bool sensor = false;
int encontrado = 0;
unsigned long tempoAnterior = 0;
const long intervalo = 2000; // 2 segundo

String bateria_string = "35%";
int bateria_valor = 35;
String horario = "15:45";
bool wifi_bool = true;


//funcoes de tempo/horario

String pegaHora(){ //RTC

}


String pegaData(){ //NTPClient

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


void enviarArquivos() { #TODO
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;

    if (https.begin(*client, serverName)) {
      https.addHeader("Content-Type", "application/json");
      https.addHeader("ngrok-skip-browser-warning", "true");
      Serial.print("[HTTPS] POST...\n");

      // Enviar presenca.json
      File presencaFile = SD.open(arqPresenca, FILE_READ);
      if (presencaFile) {
        StaticJsonDocument<1024> docPresenca;
        DeserializationError error = deserializeJson(docPresenca, presencaFile);
        presencaFile.close();
        if (error) {
          Serial.print(F("Falha ao ler presenca.json: "));
          Serial.println(error.c_str());
          return;
        }

        String jsonPresenca;
        serializeJson(docPresenca, jsonPresenca);
        int httpCode = https.POST(jsonPresenca);
        Serial.println(httpCode);

        if (httpCode == HTTP_CODE_OK) {
          Serial.println("presenca.json enviado com sucesso!");
        } else {
          Serial.printf("[HTTP] POST... falhou, erro: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.println("Falha ao abrir presenca.json.");
      }

      // Enviar inserir.json
      File inserirFile = SD.open(inserir, FILE_READ);
      if (inserirFile) {
        StaticJsonDocument<1024> docInserir;
        DeserializationError error = deserializeJson(docInserir, inserirFile);
        inserirFile.close();
        if (error) {
          Serial.print(F("Falha ao ler inserir.json: "));
          Serial.println(error.c_str());
          return;
        }

        String jsonInserir;
        serializeJson(docInserir, jsonInserir);
        int httpCode = https.POST(jsonInserir);
        Serial.println(httpCode);

        if (httpCode == HTTP_CODE_OK) {
          Serial.println("inserir.json enviado com sucesso!");
        } else {
          Serial.printf("[HTTP] POST... falhou, erro: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.println("Falha ao abrir inserir.json.");
      }

      // Atualizar espelho.json
      const String& payload = https.getString();
      StaticJsonDocument<1024> docEspelho;
      DeserializationError error = deserializeJson(docEspelho, payload);
      if (error) {
        Serial.print(F("Falha ao deserializar resposta do servidor: "));
        Serial.println(error.c_str());
        return;
      }

      File espelhoFile = SD.open(espelho, FILE_WRITE | O_TRUNC);
      if (espelhoFile) {
        serializeJson(docEspelho, espelhoFile);
        espelhoFile.close();
        Serial.println("espelho.json atualizado com sucesso!");
      } else {
        Serial.println("Falha ao abrir espelho.json para escrita.");
      }

      https.end();
    }
  } else {
    Serial.println("Desconectado do Wifi!");
  }
}



void updateCurrentTime() {
  // Obtém a hora atual
  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();

  // Formata a hora como uma string no formato HH:MM:SS
  globalCurrentTime = formatTime(currentHour, currentMinute, currentSecond);
}

String formatTime(int hour, int minute, int second) {
  String formattedTime = "";

  // Adiciona hora
  if (hour < 10) formattedTime += "0";
  formattedTime += String(hour) + ":";

  // Adiciona minuto
  if (minute < 10) formattedTime += "0";
  formattedTime += String(minute) + ":";

  // Adiciona segundo
  if (second < 10) formattedTime += "0";
  formattedTime += String(second);

  return formattedTime;
}

//Funcoes de telas
void desenharTexto(int x, int y, String texto, int tamanho) {
  tela.setCursor(x, y);
  tela.setTextColor(TFT_WHITE);
  tela.setRotation(1);
  tela.setTextSize(tamanho);
  tela.print(texto);
}


void bateria(){

  int largura_retangulo =50; // Largura do retângulo principal da bateria
  int largura_preenchimento = (bateria_valor*largura_retangulo)/100;

  if (bateria_valor <= 5) {
    largura_preenchimento = 2.5;
  } 
  else {
    largura_preenchimento = (bateria_valor*largura_retangulo)/100;
  }

  uint16_t cor;
  if (bateria_valor > 75) {
    cor = TFT_GREEN;
  } 
  else if (bateria_valor > 50) {
    cor = TFT_YELLOW;
  } 
  else if (bateria_valor > 25) {
    cor = TFT_ORANGE;
  } 
  else {
    cor = TFT_RED;
  }
  //tela.fillRect(193, 17, 5, 10, cor);
  tela.fillRect(200, 12.5, largura_preenchimento, 20, cor);
  tela.drawRect(193, 17, 5, 10, TFT_WHITE);
  tela.drawRect(200, 12.5, 50, 20, TFT_WHITE);
  desenharTexto(265,15,bateria_string,2);
}

void wifi() {
  int x = 10;
  int y = 10;  
  int largura = 5; 
  int espaco = 3;  

  int alturas[4] = {5, 10, 15, 20};

  for (int i = 0; i < 4; i++) {
    tela.fillRect(x + i * (largura + espaco), y + (20 - alturas[i]), largura, alturas[i], TFT_WHITE);
  }

  if(wifi_bool){
    for (int i = 0; i < 4; i++) {
      tela.fillRect(x + i * (largura + espaco), y + (20 - alturas[i]), largura, alturas[i], TFT_WHITE);
    }
  }
}

void cabecalho(){
  desenharTexto(90, 15 , horario, 2);
  bateria();
  wifi();
}

void menu() {
  tela.fillScreen(TFT_BLACK);
  desenharTexto(10, 80, "Escolha a turma desejada:", 2);
  desenharTexto(10, 120, "1. Turma 1", 2);
  desenharTexto(10, 150, "2. Turma 2", 2);
  desenharTexto(10, 180, "3. Turma 3", 2);
  cabecalho();

  estado_tela =0;

}

void turma(int turma) {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Turma " + String(turma), 2);
  desenharTexto(10, 120, "1. Comecar aula", 2);
  desenharTexto(10, 150, "2. Inserir Aluno", 2);
  desenharTexto(10, 180, "3. Atualizar Aluno", 2);
  desenharTexto(10, 210, "4. Enviar Arquivos", 2);  // Nova opção
  desenharTexto(10, 240, "*. Voltar", 2);

  estado_tela = 1;
}

  estado_tela = 1;
}

void presenca() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Ola!", 3);
  desenharTexto(10, 120, "Aproxime a", 3);
  desenharTexto(10, 160, "carteirinha", 3);
  desenharTexto(10, 200, "*.Voltar", 2);
  estado_tela= 2;

  tempoAnterior = millis();
}

void alunoIdentificado() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Bem-vindo(a)", 3);
  desenharTexto(10, 130, alunoo2, 4);
  desenharTexto(10, 180, matricula2, 3);
  tempoAnterior = millis();

  estado_tela =3;
}

void alunoNaoIdentificado() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(0, 80," Carteirinha nao\n identificada.\n\n Fale com o\n professor!" , 3);
  tempoAnterior = millis();

  estado_tela=4;
}     

void inserir_atualizar_matricula() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Digite a matricula", 2);
  desenharTexto(10, 180, "#. Enviar matricula", 2);
  desenharTexto(10, 200, "*. voltar", 2);

  estado_tela = 5;
}

void inserir_carteirinha(){
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(0, 100," Aproxime a \n\n carteirinha do \n\n sensor" , 3);
  tempoAnterior = millis();

  estado_tela =6;
}

void aluno_inserido(int num_turma){
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Aluno inserido!", 3);
  tempoAnterior = millis();

  estado_tela =7;
}

void atualizar_carteirinha(){
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  tela.setCursor(0, 100);
  tela.setTextColor(TFT_WHITE);
  tela.setRotation(1);
  tela.setTextSize(3);
  tela.print(" Aproxime a \n\n carteirinha do \n\n sensor");
  desenharTexto(0, 100," Aproxime a \n\n carteirinha do \n\n sensor" , 3);
  tempoAnterior = millis();

  estado_tela =8;

}

void aluno_atualizado(int num_turma){
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Aluno atualizado!", 3);
  tempoAnterior = millis();

  estado_tela=9;
}

void aluno_ja_cadastrado(){
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Aluno ja", 3);
  desenharTexto(10, 150, "cadastrado!", 3);
  tempoAnterior = millis();

  estado_tela =10;
}


//funcoes RFID
void procuraAluno(){
  File esp = SD.open(espelho, FILE_READ);
  if (!esp) {
    Serial.println("Falha ao abrir o arquivo espelho JSON.");
    return;
  }

  StaticJsonDocument<1024> docEsp;
  DeserializationError error = deserializeJson(docEsp, esp);
  esp.close();

  if (error) {
    Serial.print(F("Falha ao ler o arquivo espelho JSON: "));
    Serial.println(error.c_str());
    return;
  }

  for (JsonObject aluno : docEsp["alunos"].as<JsonArray>()) {
    if (strcmp(aluno["matricula"].as<const char*>(), matricula.c_str()) == 0) {
      achado = true;
      Serial.println("aluno encontradoooo");
    }
  }
}

void lerInserirAluno(){
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  char uidString[9] = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", rfid.uid.uidByte[i]);
    strncat(uidString, hex, sizeof(uidString) - strlen(uidString) - 1);
  }



  File ins = SD.open(inserir, FILE_READ);
  if (!ins) {
    Serial.println("Falha ao abrir o arquivo inserir JSON.");
    return;
  }

  StaticJsonDocument<1024> docIns;
  DeserializationError error = deserializeJson(docIns, ins);
  ins.close();

  if (error) {
    Serial.print(F("Falha ao ler o arquivo inserir JSON: "));
    Serial.println(error.c_str());
    return;
  }

  String horaAgora = pegaHora();

  JsonObject insAluno = docIns["alunos"].createNestedObject();

  insAluno["matricula"] = matricula.c_str();
  insAluno["uid"] = uidString;
  insAluno["hora"] = horaAgora


  delay(1000);

  ins = SD.open(inserir, FILE_WRITE | O_TRUNC);

  delay (1000);
  if (!ins) {
    Serial.println("Falha ao abrir o arquivo inserir JSON para escrita.");
    return;
  }

  serializeJson(docIns, ins);
  ins.close();
  cadastrado = true;
}

void LerDarPresenca() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  char uidString[9] = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", rfid.uid.uidByte[i]);
    strncat(uidString, hex, sizeof(uidString) - strlen(uidString) - 1);
  }

  Serial.print("LerDarPresenca: UID detected: ");
  Serial.println(uidString);

  File esp = SD.open(espelho, FILE_READ);
  if (!esp) {
    Serial.println("Falha ao abrir o arquivo espelho JSON.");
    return;
  }

  StaticJsonDocument<1024> docEsp;
  DeserializationError error = deserializeJson(docEsp, esp);
  esp.close();

  if (error) {
    Serial.print(F("Falha ao ler o arquivo espelho JSON: "));
    Serial.println(error.c_str());
    return;
  }

  File prec = SD.open(arqPresenca, FILE_READ);
  if (!prec) {
    Serial.println("Falha ao abrir o arquivo presenca JSON.");
    return;
  }

  StaticJsonDocument<1024> docPrec;
  error = deserializeJson(docPrec, prec);
  prec.close();

  if (error) {
    Serial.print(F("Falha ao ler o arquivo presença JSON: "));
    Serial.println(error.c_str());
    return;
  }

  for (JsonObject aluno : docEsp["alunos"].as<JsonArray>()) {
    if (strcmp(aluno["uid"], uidString) == 0) {
      Serial.print("Bem vindo, ");
      Serial.println(aluno["nome"].as<const char *>());

      const char* jsonMatricula2 = aluno["matricula"].as<const char *>();
      const char* jsonAluno = aluno["nome"].as<const char*>();

      alunoo = String(jsonAluno);
      int spaceIndex = alunoo.indexOf(' ');

      alunoo2 = alunoo.substring(0, spaceIndex);
      matricula2 = String(jsonMatricula2);

      String horaAgora = pegaHora(); /////////////////////////////////////////

      JsonObject precAluno = docPrec["presencas"].createNestedObject();
      precAluno["data"] = #### /////////////////////////////////
      precAluno["uid"] = aluno["uid"];
      precAluno["matricula"] = aluno["matricula"];
      precAluno["hora"] = ####; ////////////////////////////////

      updateCurrentTime();

      precAluno["hora"] = globalCurrentTime;
      // Adicionar registro de tempo aqui
      // procAluno["time"] = ####

      File prec = SD.open(arqPresenca, FILE_WRITE | O_TRUNC);
      if (!prec) {
        Serial.println(
            "Falha ao abrir o arquivo presenca JSON para escrita.");
        return;
      }

      serializeJson(docPrec, prec);
      prec.close();

      encontrado = 1;
      break;
    }
    else{
      encontrado = 2;
      break;
    }
  }

  if (!encontrado) {
    Serial.println("LerDarPresenca: UID not found in espelho JSON");
  }

  rfid.PICC_HaltA();
  return;
}

void setup() {
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


  uint16_t ID = tela.readID();
  tela.begin(ID);

  SPI.begin();
  rfid.PCD_Init();


  if (!SD.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi))) {
    Serial.println("Falha ao inicializar o cartão SD!");

    while (1);
  }
  Serial.println("Cartão SD inicializado com sucesso.");
  menu();
}


void loop() {
  char tecla_pressionada = meuteclado.getKey(); 
  encontrado = 0;

  if (tecla_pressionada) {
    if (estado_tela == 0) { 
      if (tecla_pressionada == '1') {
        turma(1);
        num_turma = 1;
      } else if (tecla_pressionada == '2') {
        turma(2);
        num_turma = 2;
      } else if (tecla_pressionada == '3') {
        turma(3);
        num_turma = 3;
      }
    } else if (estado_tela == 1) {
      if (tecla_pressionada == '1') {
        presenca();
      } else if (tecla_pressionada == '2') {
        inserir_atualizar_matricula();
        tecla_anterior = '2';
      } else if (tecla_pressionada == '3') {
        inserir_atualizar_matricula();
        tecla_anterior = '3';
      } else if (tecla_pressionada == '4') {  // Nova lógica
        enviarArquivosTela();
      } else if (tecla_pressionada == '*') {
        menu();
      }
    } else if (estado_tela == 2) {
      if (tecla_pressionada == '*') {
        turma(num_turma);
      }
    } else if (estado_tela == 5) {
      if (tecla_pressionada == '*') {
        matricula = "";
        turma(num_turma);
      } else if (tecla_pressionada == '#') {
        Serial.println(matricula);
        procuraAluno();

        tela.fillRect(0, 100, 240, 70, TFT_BLACK);
        if (achado == true){
          aluno_ja_cadastrado();
        } else if (tecla_anterior == '2' && achado == false) {
          inserir_carteirinha();
        } else if (tecla_anterior == '3' && achado == false) {
          lerInserirAluno();
          atualizar_carteirinha();
        }
      } else {
        tela.fillRect(0, 100, 240, 70, TFT_BLACK);
        matricula += tecla_pressionada;
        desenharTexto(10, 120, matricula, 4);
        Serial.println(matricula);
      }
    }

    unsigned long tempoAtual = millis();

    if (estado_tela == 2 && tempoAtual - tempoAnterior >= intervalo) {
      LerDarPresenca();
      if (encontrado == 1) {
        alunoIdentificado();
      } else if (encontrado == 2) {
        alunoNaoIdentificado();
      }
    }
    else if (estado_tela == 11 ){
      enviarArquivos(); // função que envia os Json pro servidor e atualiza o espelho
      if (enviado == 1){
        arqEnviadoTela(); // seria uma tela temporaria que dps voltaria para o menu dnv
      }
      else if (enviado == 2){
        arqNaoEnviadoTela(); // seria uma tela temporaria que dps voltaria para o menu dnv
      }
    }
    else if (estado_tela == 3 && tempoAtual - tempoAnterior >= intervalo) {
      presenca();
    } 
    else if (estado_tela == 4 && tempoAtual - tempoAnterior >= intervalo) {
      presenca();
    } 
    else if (estado_tela == 6 && tecla_anterior == '2' && achado == false){
      lerInserirAluno();
      matricula = "";
      if (cadastrado == 1) {
        aluno_inserido(num_turma);
        cadastrado = 0;
      }
    } else if (estado_tela == 7 && tempoAtual - tempoAnterior >= intervalo) {
      turma(num_turma);
    } else if (estado_tela == 8 && tempoAtual - tempoAnterior >= intervalo) {
      if (sensor) {
        aluno_atualizado(num_turma);
      }
    } else if (estado_tela == 9 && tempoAtual - tempoAnterior >= intervalo) {
      turma(num_turma);
    } else if (estado_tela == 10 && tempoAtual - tempoAnterior >= intervalo) {
      turma(num_turma);
      achado = false;
      matricula = "";
    } else if (estado_tela == 11 && tempoAtual - tempoAnterior >= intervalo) {  // Nova lógica
      turma(num_turma);
    }
  }
}
Código inicial integrando com ESP e RTC
*/