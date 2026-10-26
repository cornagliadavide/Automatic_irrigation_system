/*
 * PROGETTO: Sistema di Irrigazione Solare Ibrido (Gestione Energetica + Log Bluetooth)
 * DESCRIZIONE: Controllo PWM a CICLI di ASSORBIMENTO, Debounce Galleggiante, 
 * Isteresi Li-Ion e Trasmissione Telemetrica CSV via HC-05 per Tesi.
 */

#include <SoftwareSerial.h>

// --- 0. CONFIGURAZIONE BLUETOOTH (HC-05) ---
SoftwareSerial BTSerial(2, 3); // D2 = RX, D3 = TX

// --- 1. CONFIGURAZIONE PIN ---
const int pinSensore   = A0; 
const int pinBatteria  = A4; // Impostato su A4 come da KiCad
const int pinSerbatoio = A2; 
const int pinDriver    = 9;  

// --- 2. CALIBRAZIONE UMIDITÀ ---
const int VAL_ACQUA  = 210;  
const int VAL_ARIA   = 580;  
const int SOGLIA_ATT = 280;  
const int PWM_MIN    = 120;  
const int PWM_MAX    = 255;  

// --- 3. TEMPISTICHE IRRIGAZIONE E ASSORBIMENTO ---
const unsigned long TEMPO_EROGAZIONE = 2000;  // 2 secondi di erogazione
const unsigned long TEMPO_PAUSA      = 15000; // 15 secondi di pausa assorbimento

unsigned long timerMotore = 0;
bool inPausaAssorbimento = false;

// --- 4. VARIABILI GALLEGGIANTE ---
int statoGalleggianteStabile = LOW;  
int ultimaLetturaIstantanea  = LOW;  
unsigned long tempoUltimoCambio = 0; 
const unsigned long tempoFiltro = 3000; 

// --- 5. CONFIGURAZIONE BATTERIA ---
const float R1 = 10000.0; 
const float R2 = 4700.0;  
const float SOGLIA_RIPRISTINO = 7.4; 
const float SOGLIA_BLOCCO     = 6.4; 
const float SOGLIA_SCOLLEGATA = 2.0; 

bool pompaBloccataPerBatteria = false; 
bool modalitaBackupAttiva = false;     
unsigned long ultimoInvioSeriale = 0;


void setup() {
  Serial.begin(9600);
  BTSerial.begin(38400);
  
  pinMode(pinSensore, INPUT);
  pinMode(pinBatteria, INPUT);
  pinMode(pinSerbatoio, INPUT_PULLUP); 
  pinMode(pinDriver, OUTPUT);
  
  timerMotore = millis();
  
  String intestazione = "Tempo_s;Umidita;Tensione_V;PWM;Stato_Sistema";
  Serial.println(intestazione);
  BTSerial.println(intestazione);
}


void loop() {
  // =========================================================
  // LETTURA SENSORI E BATTERIA
  // =========================================================
  int valoreUmidita = analogRead(pinSensore);
  
  int valoreGrezzoBat = analogRead(pinBatteria);
  float vPin = valoreGrezzoBat * (5.0 / 1023.0); 
  float vBatteria = vPin * ((R1 + R2) / R2);    
  
  // =========================================================
  // ALGORITMO BATTERIA E BACKUP
  // =========================================================
  if (vBatteria > SOGLIA_RIPRISTINO) {
    pompaBloccataPerBatteria = false; 
    modalitaBackupAttiva = false;
  } 
  else if (vBatteria <= SOGLIA_BLOCCO && vBatteria > SOGLIA_SCOLLEGATA) {
    pompaBloccataPerBatteria = true; 
    modalitaBackupAttiva = false;
  }
  else if (vBatteria <= SOGLIA_SCOLLEGATA) {
    pompaBloccataPerBatteria = false; 
    modalitaBackupAttiva = true;
  }

  // =========================================================
  // DEBOUNCE GALLEGGIANTE (Serbatoio)
  // =========================================================
  int letturaIstantaneaGalleggiante = digitalRead(pinSerbatoio);
  if (letturaIstantaneaGalleggiante != ultimaLetturaIstantanea) {
    tempoUltimoCambio = millis(); 
  }
  if ((millis() - tempoUltimoCambio) > tempoFiltro) {
    statoGalleggianteStabile = letturaIstantaneaGalleggiante;
  }
  ultimaLetturaIstantanea = letturaIstantaneaGalleggiante;

  // =========================================================
  // LOGICA PWM CON CICLI DI ASSORBIMENTO
  // =========================================================
  int pwmOutput = 0;

  if (valoreUmidita > SOGLIA_ATT) {
    if (inPausaAssorbimento) {
      if (millis() - timerMotore > TEMPO_PAUSA) {
        inPausaAssorbimento = false; 
        timerMotore = millis(); 
      }
    } 
    else {
      pwmOutput = map(valoreUmidita, SOGLIA_ATT, VAL_ARIA, PWM_MIN, PWM_MAX);
      pwmOutput = constrain(pwmOutput, PWM_MIN, PWM_MAX); 

      if (millis() - timerMotore > TEMPO_EROGAZIONE) {
        inPausaAssorbimento = true;
        timerMotore = millis(); 
        pwmOutput = 0;
      }
    }
  } 
  else {
    inPausaAssorbimento = false;
    timerMotore = millis(); 
    pwmOutput = 0;
  }

  // =========================================================
  // BLOCCHI FINALI DI SICUREZZA
  // =========================================================
  if (statoGalleggianteStabile == HIGH || pompaBloccataPerBatteria) {
    pwmOutput = 0; 
  }

  analogWrite(pinDriver, pwmOutput);

  // =========================================================
  // TRASMISSIONE DATI IN FORMATO CSV (Ogni 2 secondi)
  // =========================================================
  if (millis() - ultimoInvioSeriale > 2000) {
    
    String stato = "OK";
    if (statoGalleggianteStabile == HIGH) {
      stato = "NO_ACQUA";
    } 
    else if (pompaBloccataPerBatteria) {
      stato = "BATTERIA_SCARICA";
    } 
    else if (valoreUmidita > SOGLIA_ATT && inPausaAssorbimento) {
      stato = "PAUSA_ASSORBIMENTO";
    }

    String rigaCSV = String(millis() / 1000) + ";" +
                      String(valoreUmidita) + ";" +
                      String(vBatteria, 2) + ";" +
                      String(pwmOutput) + ";" +
                      stato;

    Serial.println(rigaCSV);
    BTSerial.println(rigaCSV);
    
    ultimoInvioSeriale = millis();
  }
}
