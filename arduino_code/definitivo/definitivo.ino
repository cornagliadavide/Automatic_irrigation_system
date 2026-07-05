/*
 * PROGETTO: Sistema di Irrigazione Solare Ibrido (Gestione Energetica)
 * DESCRIZIONE: Controllo PWM a CICLI di ASSORBIMENTO, Debounce Galleggiante, 
 * Isteresi Li-Ion e Rilevamento Modalità Backup (Stilo AA).
 */

// --- 1. CONFIGURAZIONE PIN ---
const int pinSensore   = A0; 
const int pinSerbatoio = A2; 
const int pinBatteria  = A4; 
const int pinDriver    = 9;  

// --- 2. CALIBRAZIONE UMIDITÀ ---
const int VAL_ACQUA  = 210;  
const int VAL_ARIA   = 580;  
const int SOGLIA_ATT = 280;  // Deve essere > 450 per partire
const int PWM_MIN    = 120;  
const int PWM_MAX    = 255;  

// --- 3. TEMPISTICHE IRRIGAZIONE E ASSORBIMENTO ---
// Questi valori puoi modificarli a tuo piacimento in base al tuo vaso
const unsigned long TEMPO_EROGAZIONE = 2000;  // 5 secondi di erogazione acqua
const unsigned long TEMPO_PAUSA      = 15000; // 15 secondi di pausa assorbimento nel terreno

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
const float SOGLIA_SCOLLEGATA = 2.0; // Sotto i 2V significa interruttore aperto (Backup)

bool pompaBloccataPerBatteria = false; 
bool modalitaBackupAttiva = false;     
unsigned long ultimoInvioSeriale = 0;


void setup() {
  Serial.begin(9600);
  
  pinMode(pinSensore, INPUT);
  pinMode(pinBatteria, INPUT);
  pinMode(pinSerbatoio, INPUT_PULLUP); 
  pinMode(pinDriver, OUTPUT);
  
  // Inizializza il timer del motore
  timerMotore = millis();
  
  Serial.println("--- Sistema Avviato: Logica a Cicli di Assorbimento ---");
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
    // 1. Li-Ion cariche
    pompaBloccataPerBatteria = false; 
    modalitaBackupAttiva = false;
  } 
  else if (vBatteria <= SOGLIA_BLOCCO && vBatteria > SOGLIA_SCOLLEGATA) {
    // 2. Li-Ion scariche (tra 2V e 6.4V) -> BLOCCO
    pompaBloccataPerBatteria = true; 
    modalitaBackupAttiva = false;
  }
  else if (vBatteria <= SOGLIA_SCOLLEGATA) {
    // 3. Interruttore Li-Ion aperto (~0V) -> SBLOCCO (Uso AA)
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

  // Se il terreno è più secco della soglia desiderata
  if (valoreUmidita > SOGLIA_ATT) {
    
    // Controlliamo in che fase del ciclo ci troviamo
    if (inPausaAssorbimento) {
      // 1. Siamo in pausa. Attendiamo che l'acqua penetri nella terra.
      if (millis() - timerMotore > TEMPO_PAUSA) {
        // Pausa finita! Resettiamo per iniziare una nuova erogazione
        inPausaAssorbimento = false; 
        timerMotore = millis(); 
      }
    } 
    else {
      // 2. Non siamo in pausa. Possiamo erogare acqua in modo proporzionale.
      pwmOutput = map(valoreUmidita, SOGLIA_ATT, VAL_ARIA, PWM_MIN, PWM_MAX);
      pwmOutput = constrain(pwmOutput, PWM_MIN, PWM_MAX); 

      // Controlliamo se la pompa è accesa da troppo tempo
      if (millis() - timerMotore > TEMPO_EROGAZIONE) {
        // Tempo scaduto! Forza lo spegnimento e avvia la pausa
        inPausaAssorbimento = true;
        timerMotore = millis(); // Riusiamo la variabile per contare la pausa
        pwmOutput = 0;
      }
    }
  } 
  else {
    // Il terreno è sufficientemente umido! 
    // Resettiamo tutto in modo da essere pronti per il futuro
    inPausaAssorbimento = false;
    timerMotore = millis(); 
    pwmOutput = 0;
  }

  // =========================================================
  // BLOCCHI FINALI DI SICUREZZA (Sovrascrivono tutto)
  // =========================================================
  if (statoGalleggianteStabile == HIGH || pompaBloccataPerBatteria) {
    pwmOutput = 0; 
  }

  analogWrite(pinDriver, pwmOutput);

  // =========================================================
  // STAMPA SERIALE DIAGNOSTICA (Ogni 2 secondi)
  // =========================================================
  if (millis() - ultimoInvioSeriale > 2000) {
    Serial.print("Umidità: ");
    Serial.print(valoreUmidita);
    Serial.print(" | Bat: ");
    Serial.print(vBatteria);
    Serial.print(" V | Stato: ");
    
    if (statoGalleggianteStabile == HIGH) {
      Serial.println("BLOCCATA (Manca Acqua nel Serbatoio)");
    } 
    else if (pompaBloccataPerBatteria) {
      Serial.println("BLOCCATA (Batterie Li-Ion scariche o spente)");
    } 
    else if (valoreUmidita > SOGLIA_ATT && inPausaAssorbimento) {
      Serial.print("IN PAUSA ASSORBIMENTO ");
      if(modalitaBackupAttiva) Serial.println("(Backup AA)"); else Serial.println("(Solare/Li-Ion)");
    }
    else if (modalitaBackupAttiva) {
      Serial.println("OK (Modalità Backup AA attiva)");
    } 
    else {
      Serial.println("OK (Pronta all'irrigazione)");
    }
    
    ultimoInvioSeriale = millis();
  }
}