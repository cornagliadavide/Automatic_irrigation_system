# Automatic Irrigation System

Questo è un progetto basato su Arduino per gestire l'irrigazione automatica delle piante. 
Il sistema monitora costantemente l'umidità del terreno e attiva automaticamente una pompa per l'irrigazione quando necessario, evitando che le piante si secchino.

## Componenti Hardware

### Logica e Controllo
* **Arduino Nano** (ATmega328P) — microcontrollore principale
* **Modulo driver MOSFET dual-channel** — pilota la pompa ad alta corrente tramite segnale PWM (Pin D9)
* **Modulo Bluetooth HC-05** — per il monitoraggio/controllo remoto tramite comunicazione seriale (38400 baud)
### Sensori
* **Sensore di umidità del suolo capacitivo** — collegato al pin A0
* **Sensore di livello a galleggiante** — collegato al pin A2
* **Partitore di tensione** (R₁ = 10 kΩ, R₂ = 4.7 kΩ) — per la lettura della tensione della batteria sul pin A4

### Alimentazione ed Energia
* **2× Pannelli solari** da 6V. 3W — collegati in serie (12V)
* **2× Celle Li-Ion 18650**
* **Pacco batterie di riserva** — 4× stilo AA (6V)
* **Modulo di ricarica TP5100** — ricarica in due fasi (corrente/tensione costante), configurato per 2 celle in serie (8.4V)
* **BMS 2S** — protezione da sovraccarica e scarica profonda - Corrente massima di 3A
* **Convertitore Buck XL4015** — step-down regolabile, stabilizza l'uscita a 5V
* **Condensatore** da 2200µF 25V — bypass anti-spunto in parallelo alla pompa

### Attuazione
* **Pompa dell'acqua a immersione** 5V
* **Diodi di protezione**: 1N5822 Schottky (linea solare) + 1N4007 (linea di riserva)

### Struttura e Prototipazione
* **Basetta millefori** (per il montaggio definitivo)
* **Contenitore stampato in 3D** con supporto per i pannelli solari
##  Debugging & Note Operative

Durante la fase di prototipazione e test sono stati individuati e risolti i seguenti punti critici per la stabilità del sistema:

1. **Gestione dello Spunto Meccanico ed Elettrico:**
   * La pompa richiede una corrente di spunto elevata per vincere l'inerzia iniziale della girante.
   * È stato configurato un impulso PWM al valore massimo (`255`) per una durata continua di **5 secondi** per garantire il corretto spunto e l'erogazione costante.
   * L'aggiunta del condensatore previene crolli di tensione improvvisi sulla logica dell'Arduino Nano durante la commutazione del MOSFET.
     
2. **Cablaggio e Connessioni di Potenza:**
   * **Morsetti a vite del MOSFET:** Evitare l'uso di pin rigidi flessibili o guaine termorestringenti all'interno dei morsetti a vite blu del MOSFET. Per garantire il passaggio delle correnti di picco senza cadute di tensione resistive, i fili multifilo della pompa vanno spellati e serrati a **rame nudo** direttamente nei morsetti.
   * **Manutenzione Girante:** In caso di mancato avvio con segnale PWM presente, verificare lo sblocco meccanico manuale della girante da eventuali sedimenti o attriti interni.
     
3. **Isolamento della Linea di Potenza:**
   * L'alimentazione USB del PC alimenta unicamente la logica del microcontrollore e i sensori. Per l'attivazione della pompa è sempre necessaria la presenza della linea di potenza attiva (batterie/Buck converter).

##  Struttura del Progetto
* `arduino_code/`: Contiene lo sketch (.ino) da caricare sulla scheda Arduino.
* `schematic/`: Contiene gli schemi elettrici per collegare correttamente i pin.
