#include <Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <MCUFRIEND_kbv.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SdFat.h>

#include "RTClib.h"

#define PALETTEDEPTH 8
char namebuf[32] = "/";

SdFile root;
int pathlen;



RTC_PCF8563 rtc;
byte last_second, second, minute, hour, day, month;

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
SdFile prec;
SdFile ins;
SdFile esp;
SdFile insWrite;

// Display
MCUFRIEND_kbv tela;
bool json_mandou = false;
// Teclado
const byte qtdLinhas = 4;
const byte qtdColunas = 3;
StaticJsonDocument<1024> persiste;
StaticJsonDocument<1024> doEsp;
char matriz_teclas[qtdLinhas][qtdColunas] = {
  { '1', '2', '3' }, { '4', '5', '6' }, { '7', '8', '9' }, { '*', '0', '#' }
};

byte PinosqtdLinhas[qtdLinhas] = { 35, 45, 43, 39 };
byte PinosqtdColunas[qtdColunas] = { 37, 33, 41 };
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
SoftSpiDriver<SD_MISO_PIN, SD_MOSI_PIN, SD_SCK_PIN> softSpi;  //perguntar para o gabriel o que isso tá fazendo
//stdfat não tá funcionando

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

const char espelho[] = "espelho.json";
const char arqPresenca[] = "testes.json";
const char inserir[] = "inserir4.json";
bool leituraAtiva = false;
bool est_tela = false;

String globalCurrentTime;

String dataAtual;

bool achado = false;
String matricula;
int estado_tela = 0;
String alunoo;  // Pegar o nome e a matricula na presença pelo json espelho
String alunoo2;
String matricula2;
char tecla_anterior = "";
String num_turma;
const long interval = 2000;
unsigned long previousMillis = 0;
String lTurma;

int cadastrado = 0;
bool sensor = false;
int encontrado = 0;
unsigned long tempoAnterior = 0;
const long intervalo = 2000;  // 2 segundo
int enviado = 0;

String bateria_string = "35%";
int bateria_valor = 35;
String horario;
bool wifi_bool = true;
bool mandou_hora;

long pegaUnixtime() {
  DateTime now = rtc.now();
  return now.unixtime();
}

///////////// funções de tempo /////////////

String pegaData() {
  DateTime now = rtc.now();

  char buffer[11];

  snprintf(buffer, sizeof(buffer), "%02d-%02d-%04d", now.day(), now.month(), now.year());
  Serial.println(buffer);
  return String(buffer);
}

String pegaHora() {
  DateTime now = rtc.now();

  char buffer[11];

  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  Serial.println(buffer);
  return String(buffer);
}

///////////// funções de telas /////////////

void desenharTexto(int x, int y, String texto, int tamanho) {
  tela.setCursor(x, y);
  tela.setTextColor(TFT_WHITE);
  tela.setRotation(3);
  tela.setTextSize(tamanho);
  tela.print(texto);
}

void bateria() {

  int largura_retangulo = 50;  // Largura do retângulo principal da bateria
  int largura_preenchimento = (bateria_valor * largura_retangulo) / 100;

  if (bateria_valor <= 5) {
    largura_preenchimento = 2.5;
  } else {
    largura_preenchimento = (bateria_valor * largura_retangulo) / 100;
  }

  uint16_t cor;
  if (bateria_valor > 75) {
    cor = TFT_GREEN;
  } else if (bateria_valor > 50) {
    cor = TFT_YELLOW;
  } else if (bateria_valor > 25) {
    cor = TFT_ORANGE;
  } else {
    cor = TFT_RED;
  }
  //tela.fillRect(193, 17, 5, 10, cor);
  tela.fillRect(200, 12.5, largura_preenchimento, 20, cor);
  tela.drawRect(193, 17, 5, 10, TFT_WHITE);
  tela.drawRect(200, 12.5, 50, 20, TFT_WHITE);
  desenharTexto(265, 15, bateria_string, 2);
}

void wifi() {
  int x = 10;
  int y = 10;
  int largura = 5;
  int espaco = 3;

  int alturas[4] = { 5, 10, 15, 20 };

  for (int i = 0; i < 4; i++) {
    tela.fillRect(x + i * (largura + espaco), y + (20 - alturas[i]), largura, alturas[i], TFT_WHITE);
  }

  if (wifi_bool) {
    for (int i = 0; i < 4; i++) {
      tela.fillRect(x + i * (largura + espaco), y + (20 - alturas[i]), largura, alturas[i], TFT_WHITE);
    }
  }
}

void cabecalho() {
  desenharTexto(67, 15, horario, 2);
  bateria();
  wifi();
}

void menu() {
  tela.fillScreen(TFT_BLACK);
  desenharTexto(10, 80, "Escolha a turma desejada:", 2);
  desenharTexto(10, 120, "1. 33A", 2);
  desenharTexto(10, 150, "2. 33B", 2);
  desenharTexto(10, 180, "3. 33C", 2);
  cabecalho();

  estado_tela = 0;
}

void turma(String turma) {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 70, "33" + turma, 2);
  desenharTexto(10, 100, "1. Comecar aula", 2);
  desenharTexto(10, 130, "2. Inserir Aluno", 2);
  desenharTexto(10, 160, "3. Atualizar Aluno", 2);
  desenharTexto(10, 190, "4. Enviar Arquivos", 2);  // Nova opção
  desenharTexto(10, 220, "*. Voltar", 2);

  estado_tela = 1;
}

void presenca() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Ola!", 3);
  desenharTexto(10, 120, "Aproxime a", 3);
  desenharTexto(10, 160, "carteirinha", 3);
  desenharTexto(10, 200, "*.Voltar", 2);
  estado_tela = 2;

  tempoAnterior = millis();
}

void alunoIdentificado() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Bem-vindo(a)", 3);
  desenharTexto(10, 130, alunoo2, 4);
  desenharTexto(10, 180, matricula2, 3);
  tempoAnterior = millis();

  estado_tela = 3;
}

void alunoNaoIdentificado() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(0, 80, " Carteirinha nao\n identificada.\n\n Fale com o\n professor!", 3);
  tempoAnterior = millis();

  estado_tela = 4;
}

void inserir_atualizar_matricula() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 80, "Digite a matricula", 2);
  desenharTexto(10, 180, "#. Enviar matricula", 2);
  desenharTexto(10, 200, "*. voltar", 2);

  estado_tela = 5;
}

void inserir_carteirinha() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(0, 100, " Aproxime a \n\n carteirinha do \n\n sensor", 3);
  tempoAnterior = millis();

  estado_tela = 6;
}

void aluno_inserido(String num_turma) {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Aluno inserido!", 3);
  tempoAnterior = millis();

  estado_tela = 7;
}

void atualizar_carteirinha() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  tela.setCursor(0, 100);
  tela.setTextColor(TFT_WHITE);
  tela.setRotation(3);
  tela.setTextSize(3);
  tela.print(" Aproxime a \n\n carteirinha do \n\n sensor");
  desenharTexto(0, 100, " Aproxime a \n\n carteirinha do \n\n sensor", 3);
  tempoAnterior = millis();

  estado_tela = 8;
}

void aluno_atualizado(String num_turma) {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Aluno atualizado!", 3);
  tempoAnterior = millis();

  estado_tela = 9;
}

void aluno_ja_cadastrado() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Aluno ja", 3);
  desenharTexto(10, 150, "cadastrado!", 3);
  tempoAnterior = millis();

  estado_tela = 10;
}

void rtc_inicializado() {
  tela.fillScreen(TFT_BLACK);
  desenharTexto(10, 120, "RTC", 3);
  desenharTexto(10, 150, "inicializado!", 3);
}

void rtc_nao_inicializado() {
  tela.fillScreen(TFT_BLACK);
  desenharTexto(10, 120, "RTC", 3);
  desenharTexto(10, 150, "nao inicializado!", 3);
}

void sd_nao_inicializado() {
  tela.fillScreen(TFT_BLACK);
  desenharTexto(10, 120, "Cartao SD", 3);
  desenharTexto(10, 150, "nao inicializado!", 3);
}

void sd_inicializado() {
  tela.fillScreen(TFT_BLACK);
  desenharTexto(10, 120, "Cartao SD", 3);
  desenharTexto(10, 150, "inicializado!", 3);
}

void enviando_dados() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Enviando dados...", 3);
}

void dados_enviados() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Dados enviados!", 3);
  tempoAnterior = millis();
  estado_tela = 11;
}

void dados_nao_enviados() {
  tela.fillScreen(TFT_BLACK);
  cabecalho();
  desenharTexto(10, 120, "Erro ao Enviar", 3);
  desenharTexto(10, 150, "os dados!", 3);
  tempoAnterior = millis();
  estado_tela = 12;
}

///////////// funcoes RFID /////////////

void enviaArquivo() {
  if (!ins.open(inserir, FILE_READ)) {
    Serial.println("Falha ao abrir o arquivo inserir JSON.");
    enviado = 2;
    return;
  }

  StaticJsonDocument<1024> docIns;
  DeserializationError error = deserializeJson(docIns, ins);
  ins.close();

  if (error) {
    Serial.print(F("Falha ao ler o arquivo inserir JSON: "));
    Serial.println(error.c_str());
    enviado = 2;
    return;
  }
  if (!prec.open(arqPresenca, FILE_READ)) {
    Serial.println("Falha ao abrir o arquivo presenca JSON.");
    enviado = 2;
    return;
  }

  StaticJsonDocument<1024> docPrec;
  DeserializationError error2 = deserializeJson(docPrec, prec);
  prec.close();

  if (error2) {
    Serial.print(F("Falha ao ler o arquivo presenca JSON: "));
    enviado = 2;
    Serial.println(error.c_str());
    return;
  }

  StaticJsonDocument<200> docServidor;
  JsonObject precIns;
  precIns = docServidor.createNestedObject("data");

  precIns["presenca"] = docPrec;
  precIns["cadastro"] = docIns;

  serializeJson(docServidor, Serial1);
  //serializeJson(docServidor, Serial);
  Serial.print('Arquivo Enviado para o ESP!');
  enviado = 1;
  return;
}
void procuraAluno() {
  if (!esp.open(espelho, FILE_READ)) {
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


void lerInserirAluno() {
  Serial.print("entrei no lerInserirAluno");
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
    delay(1000);
    return;
  }

  JsonObject insAluno = docIns["alunos"].createNestedObject();

  insAluno["matricula"] = matricula;
  insAluno["uid"] = uidString;


  ins = SD.open(inserir, FILE_WRITE | O_TRUNC);

  if (!ins) {
    Serial.println("Falha ao abrir o arquivo inserir JSON para escrita.");
    delay(1000);
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

  encontrado = 2;

  char uidString[9] = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", rfid.uid.uidByte[i]);
    strncat(uidString, hex, sizeof(uidString) - strlen(uidString) - 1);
  }

  Serial.print("LerDarPresenca: UID detected: ");
  Serial.println(uidString);
  for (JsonObject aluno : doEsp["alunos"].as<JsonArray>()) {
    if (strcmp(aluno["uid"], uidString) == 0) {
      Serial.print("Bem vindo, ");
      Serial.println(aluno["nome"].as<const char*>());

      const char* jsonMatricula2 = aluno["matricula"].as<const char*>();
      const char* jsonAluno = aluno["nome"].as<const char*>();

      alunoo = String(jsonAluno);
      int spaceIndex = alunoo.indexOf(' ');

      alunoo2 = alunoo.substring(0, spaceIndex);
      matricula2 = String(jsonMatricula2);
      Serial.println(matricula2);
      JsonArray presencas = persiste["presencas"].as<JsonArray>();
      // Verifica se o uid já está presente
      bool duplicado = false;

      for (JsonObject presenca : presencas) {
        if (strcmp(presenca["uid"], uidString) == 0) {
          duplicado = true;
          break;
        }
      }

      if (!duplicado) {
        // Cria um novo objeto para a presença e adiciona ao array
        JsonObject precAluno = presencas.createNestedObject();
        precAluno["matricula"] = aluno["matricula"];
        precAluno["hora"] = pegaHora();
        precAluno["turma"] = lTurma;
        precAluno["uid"] = aluno["uid"];

        Serial.println("Presença registrada com sucesso.");
      } else {
        Serial.println("Presença duplicada detectada. Registro não adicionado.");
      }

      encontrado = 1;
      break;
    } else {
      encontrado = 2;
    }
  }

  Serial.println("Documento JSON Atualizado:");
  serializeJson(persiste, Serial);
  Serial.println();
  rfid.PICC_HaltA();

  return;
}

///////////// setup /////////////

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);

  uint16_t ID = tela.readID();
  tela.begin(ID);
  SPI.begin();
  rfid.PCD_Init();
  tela.fillScreen(TFT_BLACK);


  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    rtc_nao_inicializado();
    while (!rtc.begin()) {
      delay(1);
      Serial.flush();
    }
  }
  rtc_inicializado();

  delay(1000);

  if (rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(2024, 6, 10, 18, 0, 0));
  }

  rtc.start();

  Serial1.print("comecou programa");

  if (!SD.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi))) {
    Serial.println("Falha ao inicializar o cartão SD!");
    sd_nao_inicializado();
    while (!SD.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &softSpi))) {
      delay(1000);
    }
  }
  Serial.println("Cartão SD inicializado com sucesso.");
  sd_inicializado();
  delay(1000);

  horario = pegaHora();
  persiste["presencas"] = JsonArray();
  persiste["data"] = pegaData();

  menu();
}

///////////// loop /////////////

void loop() {

  if (Serial1.available() && json_mandou == false) {
    Serial.println("entrei 1");
    String esp_says = Serial1.readStringUntil('\n');
    esp_says.trim();
    if (esp_says == "mandando json!") {
      json_mandou = true;
    } else {
      Serial.println("entrei 2");
      String horaAtual = esp_says;
      unsigned long horario_server = horaAtual.toInt();
      Serial.println(horario_server);
      rtc.adjust(DateTime(horario_server));
      Serial.println("recebi a hora");
    }
  } else if (Serial1.available() && json_mandou == true) {
    Serial.println("entrei aq");
    DeserializationError err = deserializeJson(doEsp, Serial1);
    serializeJson(doEsp, Serial);
    delay(1000);
    json_mandou = false;
  }



  char tecla_pressionada = meuteclado.getKey();
  encontrado = 0;
  enviado = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    horario = pegaHora();
    Serial.println(horario);
  }

  if (tecla_pressionada) {
    if (estado_tela == 0) {
      if (tecla_pressionada == '1') {
        turma("A");
        num_turma = "A";
        lTurma = "33A";
      } else if (tecla_pressionada == '2') {
        turma("B");
        lTurma = "33B";
        num_turma = "B";
      } else if (tecla_pressionada == '3') {
        turma("C");
        num_turma = "C";
        lTurma = "33C";
      }
    } else if (estado_tela == 1) {
      if (tecla_pressionada == '1') {
        dataAtual = pegaData();
        presenca();
      } else if (tecla_pressionada == '2') {
        inserir_atualizar_matricula();
        tecla_anterior = '2';
      } else if (tecla_pressionada == '3') {
        inserir_atualizar_matricula();
        tecla_anterior = '3';
      } else if (tecla_pressionada == '4') {
        tecla_anterior = '4';
        Serial.println("apertei pra enviar");
        enviaArquivo();
        enviando_dados();
        if (enviado == 1) {
          dados_enviados();
        } else if (enviado == 2) {
          dados_nao_enviados();
        }

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
        if (achado == true) {
          aluno_ja_cadastrado();
          achado = false;
        }

        else if (tecla_anterior == '2' && achado == false) {
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
  }

  unsigned long tempoAtual = millis();

  if (estado_tela == 2 && tempoAtual - tempoAnterior >= intervalo) {
    LerDarPresenca();
    Serial.println(encontrado);
    if (encontrado == 1) {
      alunoIdentificado();
    } else if (encontrado == 2) {
      alunoNaoIdentificado();
    }
  } else if (estado_tela == 3 && tempoAtual - tempoAnterior >= intervalo) {
    presenca();
  } else if (estado_tela == 4 && tempoAtual - tempoAnterior >= intervalo) {
    presenca();
  } else if (estado_tela == 6 && tecla_anterior == '2' && achado == false) {
    lerInserirAluno();
    if (cadastrado == 1) {
      aluno_inserido(num_turma);
      matricula = "";
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
  } else if ((estado_tela == 11 || estado_tela == 12) && tempoAtual - tempoAnterior >= intervalo) {
    turma(num_turma);
    enviado = 0;
  }
}