/*================================================================================
      MANUAL DE OPERAÇÃO - ECOSSISTEMA INTELIGENTE MIXTAPE v4.0 M.E.C.A 4.0
      Simula~c"ao no Wokwi: https://wokwi.com/projects/477727989879656449
================================================================================
* Desenvolvedor: Gabriel Souza | Grupo: Mixtape
* Sistema: Monitoramento Climático Interno de Alta Estabilidade via Barramento
================================================================================*/

#include <LiquidCrystal_I2C.h>  // Driver de controle do display LCD via I2C
#include <RtcDS1302.h>          // Biblioteca de gerenciamento do RTC DS1302
#include <Wire.h>               // Comunicação nativa do barramento SDA / SCL
#include <EEPROM.h>             // Armazenamento em disco não-volátil do Uno
#include "DHT.h"                // Telemetria digital de umidade e temperatura

#define LOG_OPTION 1     
#define SERIAL_OPTION 1  

// --- ENDEREÇAMENTO DOS SENSORES E PERIFÉRICOS ---
#define DHTPIN 2         // Pino digital de dados do sensor DHT11
#define DHTTYPE DHT11    // Definição do modelo físico do sensor
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Display LCD no endereço padrão I2C

// --- MAPEAMENTO DE HARDWARE DO RTC DS1302 ---
#define RTC_RST 10       
#define RTC_DAT 9        
#define RTC_CLK 8        
ThreeWire myWire(RTC_DAT, RTC_CLK, RTC_RST); 
RtcDS1302<ThreeWire> RTC(myWire);            

// --- INTERRUPÇÃO DO TECLADO DE COMANDO ---
#define BTN_UP     3     
#define BTN_DOWN   4     
#define BTN_MENU   5     
#define BTN_SELECT 6     
#define BTN_MODE   7     

// --- MAPA DE MEMÓRIA DA EEPROM (BANCO DE DADOS CIRCULAR) ---
const int maxRecords = 100;    
const int recordSize = 8;      
int startAddress = 0;          
int endAddress = maxRecords * recordSize; 
int currentAddress = 0;        
int lastLoggedMinute = -1;     

// --- PARAMETRIZAÇÃO DAS DIRETRIZES TÉCNICAS (GATILHOS) ---
float trigger_t_min = 20.0;    
float trigger_t_max = 30.0;    
float trigger_u_min = 30.0;    
float trigger_u_max = 60.0;    

float temperaturaEstavel = 24.0;
float umidadeEstavel = 50.0;

// Variáveis de comparação para cálculo de tendência preditiva
float ultimaTemperatura = 24.0; 
float ultimaUmidade = 50.0;

// Registradores de proteção anti-ruído para a data fantasma 2043
int ultimaHoraValida = 12;
int ultimoMinutoValido = 0;
int ultimoDiaValido = 17;
int ultimoMesValido = 10;
int ultimoAnoValido = 2026;

// Timers assíncronos de varredura (Substitutos estáveis do delay)
unsigned long tempoAnteriorDHT = 0;
const unsigned long intervaloDHT = 2100; 
unsigned long tempoAnteriorLCD = 0;
const unsigned long intervaloLCD = 350; 

enum ModoMenu { NORMAL, HISTORICO, AJUSTAR };
ModoMenu modoAtual = NORMAL;   

int horaSelecionada = 0;       
int campoEditando = 0;         
bool alternarTela = false;     

int_fast16_t adjDia, adjMes, adjAno, adjHora, adjMin;

// --- MATRIZES DE BYTES DA ANIMAÇÃO K7 MIXTAPE ORIGINAL ---
byte k7_rolo_f1[] = { B11111, B10001, B10101, B11111, B10101, B10001, B11111, B00000 }; 
byte k7_rolo_f2[] = { B11111, B10101, B10001, B11111, B10001, B10101, B11111, B00000 }; 
byte onda_baixa[]  = { B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111 }; 
byte onda_media[]  = { B00000, B00000, B00000, B11111, B11111, B00000, B11111, B11111 }; 
byte onda_maxima[] = { B11111, B11111, B00000, B11111, B11111, B00000, B11111, B11111 }; 

// --- ÍCONES DE TENDÊNCIA TÉRMICA (BANCO RAM 5, 6, 7) ---
byte seta_subindo[]  = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00000 }; 
byte seta_estavel[]  = { B00000, B00100, B00110, B11111, B11111, B00110, B00100, B00000 }; 
byte seta_descendo[] = { B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00000 }; 

// --- ÍCONES DE TENDÊNCIA DE UMIDADE (BANCO RAM 2, 3, 4) ---
byte gota_subindo[]  = { B00100, B00100, B01110, B10101, B10001, B10001, B01110, B00000 };
byte gota_estavel[]  = { B00000, B00100, B01110, B11111, B01110, B00100, B00000, B00000 };
byte gota_descendo[] = { B01110, B10001, B10001, B10101, B01110, B00100, B00100, B00000 };

void executarAnimacaoMixtape() {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(" LOADING SYSTEM "); 
    
    lcd.createChar(0, k7_rolo_f1);
    lcd.createChar(1, onda_baixa);
    
    lcd.setCursor(1, 1); lcd.print("[");
    lcd.setCursor(2, 1); lcd.write(byte(0)); 
    lcd.setCursor(3, 1); lcd.print("===[");
    lcd.setCursor(7, 1); lcd.print("4.0");    
    lcd.setCursor(10, 1); lcd.print("]==");
    lcd.setCursor(14, 1); lcd.write(byte(0)); 
    lcd.setCursor(15, 1); lcd.print("]");
    delay(400);

    for (int giro = 0; giro < 4; giro++) {
        lcd.createChar(0, k7_rolo_f2); delay(150); 
        lcd.createChar(0, k7_rolo_f1); delay(150); 
    }

    lcd.createChar(1, onda_media); delay(200);
    lcd.createChar(1, onda_maxima); delay(300);

    lcd.clear();
    for(int col = 0; col < 16; col++) {
        lcd.setCursor(col, 0); lcd.print("*");
        lcd.setCursor(15 - col, 1); lcd.print("*");
        delay(30); 
    }
    delay(100);
    lcd.clear();

    lcd.setCursor(4, 0); lcd.print("MIXTAPE");
    lcd.setCursor(0, 1); lcd.print("ADS & ENGENHARIA");
    delay(1800); 
    lcd.clear();
}

void setup() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_MENU, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    pinMode(BTN_MODE, INPUT_PULLUP);

    Wire.begin();  
    dht.begin();   
    lcd.init();    
    lcd.backlight();

    executarAnimacaoMixtape(); 

    // Alocação dos Custom Chars nos registradores internos do Display LCD
    lcd.createChar(5, seta_subindo);
    lcd.createChar(6, seta_estavel);
    lcd.createChar(7, seta_descendo);
    
    lcd.createChar(2, gota_subindo);
    lcd.createChar(3, gota_estavel);
    lcd.createChar(4, gota_descendo);

    RTC.Begin();
    if (RTC.GetIsWriteProtected()) {
        RTC.SetIsWriteProtected(false); 
    }
    if (!RTC.GetIsRunning()) {
        RTC.SetIsRunning(true);
        RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
        RTC.SetDateTime(compiled);
    }
    
    float tInicial = dht.readTemperature();
    float hInicial = dht.readHumidity();
    if (!isnan(tInicial) && !isnan(hInicial)) {
        temperaturaEstavel = tInicial;
        ultimaTemperatura = tInicial;
        umidadeEstavel = hInicial;
        ultimaUmidade = hInicial;
    }
}

void loop() {
    unsigned long tempoAtual = millis();

    // --- 1. AQUISIÇÃO E FILTRAGEM DO DHT11 ---
    if (tempoAtual - tempoAnteriorDHT >= intervaloDHT) {
        tempoAnteriorDHT = tempoAtual;
        float leituraT = dht.readTemperature();
        float leituraH = dht.readHumidity();
        
        if (!isnan(leituraT) && !isnan(leituraH)) {
            temperaturaEstavel = leituraT;
            umidadeEstavel = leituraH;
        }
    }

    // --- 2. VARREDURA E PROTEÇÃO ANTI-RUÍDO RTC (EVITA ANO FANTASMA 2043) ---
    RtcDateTime tempoBruto = RTC.GetDateTime();
    
    if (tempoBruto.IsValid() && tempoBruto.Year() < 2040 && tempoBruto.Year() >= 2026) {
        ultimoAnoValido   = tempoBruto.Year();
        ultimoMesValido   = tempoBruto.Month();
        ultimoDiaValido   = tempoBruto.Day();
        ultimaHoraValida  = tempoBruto.Hour();
        ultimoMinutoValido = tempoBruto.Minute();
    }

    // --- 3. MÁQUINA DE LOG CIRCULAR DA EEPROM (SALVAMENTO ASSÍNCRONO DE MINUTO EM MINUTO) ---
    if (ultimoMinutoValido != lastLoggedMinute) {
        ultimaTemperatura = temperaturaEstavel;          
        ultimaUmidade = umidadeEstavel;
        lastLoggedMinute = ultimoMinutoValido; 

        // Condicional de gravação física caso haja violação de segurança do Data Center
        if (temperaturaEstavel < trigger_t_min || temperaturaEstavel > trigger_t_max || umidadeEstavel < trigger_u_min || umidadeEstavel > trigger_u_max) {
            int tempSalvar = (int)(temperaturaEstavel * 10);
            int humiSalvar = (int)(umidadeEstavel);
            
            EEPROM.write(currentAddress, (byte)ultimaHoraValida);      
            EEPROM.write(currentAddress + 1, (byte)tempSalvar);    
            EEPROM.write(currentAddress + 2, (byte)humiSalvar);    
            
            currentAddress += 3; 
            if (currentAddress >= 300) currentAddress = 0; 
        }
    }

    // --- 4. TRATAMENTO DOS BOTÕES DE OPERAÇÃO ---
    bool pressionouMenu  = (digitalRead(BTN_MENU) == LOW);
    bool pressionouSel   = (digitalRead(BTN_SELECT) == LOW);
    bool pressionouMode  = (digitalRead(BTN_MODE) == LOW);

    if (pressionouMode && modoAtual != AJUSTAR) {
        delay(200);
        unsigned long tempoClique = millis();
        while (digitalRead(BTN_MODE) == LOW) {
            if (millis() - tempoClique > 1500) {
                modoAtual = AJUSTAR; campoEditando = 0;
                adjDia = ultimoDiaValido; adjMes = ultimoMesValido; adjAno = ultimoAnoValido;
                adjHora = ultimaHoraValida; adjMin = ultimoMinutoValido;
                lcd.clear();
                lcd.blink(); 
                break;
            }
        }
    }

    if (pressionouSel) {
        delay(250); 
        lcd.clear();
        if (modoAtual == NORMAL) { modoAtual = HISTORICO; horaSelecionada = ultimaHoraValida; }
        else if (modoAtual == HISTORICO) modoAtual = NORMAL;
        else if (modoAtual == AJUSTAR) {
            campoEditando++; 
            if (campoEditando > 4) { modoAtual = NORMAL; lcd.noBlink(); } 
        }
    }

    if (pressionouMenu && modoAtual == NORMAL) {
        delay(250);
        alternarTela = !alternarTela; 
        lcd.clear();
    }

    // --- 5. RENDERIZAÇÃO ESTÁVEL DO DISPLAY LCD (MÁQUINA DE ESTADOS FINITOS) ---
    if (tempoAtual - tempoAnteriorLCD >= intervaloLCD) {
        tempoAnteriorLCD = tempoAtual;

        if (modoAtual == NORMAL) {
            // Estado Crítico: Disparo de Alerta Climático Visível
            if (temperaturaEstavel < trigger_t_min || temperaturaEstavel > trigger_t_max || umidadeEstavel < trigger_u_min || umidadeEstavel > trigger_u_max) {
                lcd.setCursor(0, 0); lcd.print("!!/-/ALERTA/-/!!");
                lcd.setCursor(0, 1); lcd.print("T:"); lcd.print(temperaturaEstavel, 1);
                lcd.print("C H:"); lcd.print(umidadeEstavel, 1); lcd.print("%   ");
            } else {
                // Estado Estável Aba 1: Leitura Principal e Relógio
                if (!alternarTela) {
                    lcd.setCursor(0, 0); lcd.print("T: "); lcd.print(temperaturaEstavel, 1); lcd.print(" C   ");
                    
                    // Tratamento lógico e desenho da seta de tendência térmica
                    lcd.setCursor(9, 0);
                    if (temperaturaEstavel > ultimaTemperatura + 0.05) lcd.write(5);      
                    else if (temperaturaEstavel < ultimaTemperatura - 0.05) lcd.write(7); 
                    else lcd.write(6);                                             

                    lcd.setCursor(11, 0);
                    if (ultimaHoraValida < 10) lcd.print("0"); lcd.print(ultimaHoraValida); lcd.print(":");
                    if (ultimoMinutoValido < 10) lcd.print("0"); lcd.print(ultimoMinutoValido);

                    lcd.setCursor(0, 1); lcd.print("H: "); lcd.print(umidadeEstavel, 1); lcd.print(" %");
                    
                    // Desenho do ícone de tendência de umidade preenchendo o espaço em branco
                    lcd.setCursor(9, 1);
                    if (umidadeEstavel > ultimaUmidade + 0.1) lcd.write(2);
                    else if (umidadeEstavel < ultimaUmidade - 0.1) lcd.write(4);
                    else lcd.write(3);

                    lcd.setCursor(10, 1); lcd.print(" MXTAPE");
                } else {
                    // Estado Estável Aba 2: Calendário Integral
                    lcd.setCursor(0, 0); lcd.print("DATA: "); 
                    if (ultimoDiaValido < 10) lcd.print("0"); lcd.print(ultimoDiaValido); lcd.print("/");
                    if (ultimoMesValido < 10) lcd.print("0"); lcd.print(ultimoMesValido); lcd.print("/");
                    lcd.print(ultimoAnoValido);

                    lcd.setCursor(0, 1); lcd.print("HORA: ");
                    if (ultimaHoraValida < 10) lcd.print("0"); lcd.print(ultimaHoraValida); lcd.print(":");
                    if (ultimoMinutoValido < 10) lcd.print("0"); lcd.print(ultimoMinutoValido);
                    lcd.print("   ESTAVEL");
                }
            }
        } 
        else if (modoAtual == HISTORICO) {
            // Estado Histórico: Navegação Dinâmica da Memória EEPROM
            bool pUpHist   = (digitalRead(BTN_UP) == LOW);
            bool pDownHist = (digitalRead(BTN_DOWN) == LOW);

            if (pUpHist)   { delay(200); horaSelecionada = (horaSelecionada + 1) % 24; lcd.clear(); }
            if (pDownHist) { delay(200); horaSelecionada = (horaSelecionada - 1 + 24) % 24; lcd.clear(); }

            bool registroAchado = false;
            float tHist = 0.0, hHist = 0.0;

            for (int i = 0; i < 300; i += 3) {
                byte hSalva = EEPROM.read(i);
                byte tSalva = EEPROM.read(i + 1);
                byte hVal   = EEPROM.read(i + 2);
                
                if (hSalva == horaSelecionada && tSalva > 0 && tSalva < 255) {
                    tHist = tSalva / 10.0;
                    hHist = hVal;
                    registroAchado = true;
                    break;
                }
            }

            lcd.setCursor(0, 0); lcd.print("HISTORICO: "); 
            if (horaSelecionada < 10) lcd.print("0");
            lcd.print(horaSelecionada); lcd.print("h   ");
            
            lcd.setCursor(0, 1);
            if (registroAchado) {
                lcd.print("T:"); lcd.print(tHist, 1); 
                lcd.print("C H:"); lcd.print((int)hHist); 
                lcd.print("%      ");
            } else {
                lcd.print("* SEM REGISTRO *"); 
            }
        } 
        else if (modoAtual == AJUSTAR) {
            // Estado Ajustar: Configuração Física Manual dos Registradores do RTC
            int variacao = 0;
            bool pUpAjuste   = (digitalRead(BTN_UP) == LOW);
            bool pDownAjuste = (digitalRead(BTN_DOWN) == LOW);

            if (pUpAjuste)   { delay(150); variacao = 1; }
            if (pDownAjuste) { delay(150); variacao = -1; }

            if (variacao != 0) {
                if (campoEditando == 0) adjDia  = constrain(adjDia + variacao, 1, 31);
                if (campoEditando == 1) adjMes  = constrain(adjMes + variacao, 1, 12);
                if (campoEditando == 2) adjAno  = constrain(adjAno + variacao, 2026, 2035);
                if (campoEditando == 3) adjHora = (adjHora + variacao + 24) % 24;
                if (campoEditando == 4) adjMin  = (adjMin + variacao + 60) % 60;
                
                RtcDateTime novoTempo = RtcDateTime((uint16_t)adjAno, (uint8_t)adjMes, (uint8_t)adjDia, (uint8_t)adjHora, (uint8_t)adjMin, 0);
                RTC.SetDateTime(novoTempo);
            }

            lcd.setCursor(0, 0); lcd.print("dd/mm/aaaa  hora");
            lcd.setCursor(0, 1);
            if(adjDia<10) lcd.print("0"); lcd.print(adjDia); lcd.print("/");
            if(adjMes<10) lcd.print("0"); lcd.print(adjMes); lcd.print("/");
            lcd.print(adjAno); lcd.print("  ");
            if(adjHora<10) lcd.print("0"); lcd.print(adjHora); lcd.print(":");
            if(adjMin<10) lcd.print("0"); lcd.print(adjMin);

            if (campoEditando == 0) lcd.setCursor(1, 1);
            else if (campoEditando == 1) lcd.setCursor(4, 1);
            else if (campoEditando == 2) lcd.setCursor(9, 1);
            else if (campoEditando == 3) lcd.setCursor(12, 1);
            else if (campoEditando == 4) lcd.setCursor(15, 1);
        }
    }

    delay(20); 
}
