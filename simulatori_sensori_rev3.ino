//scritto da Moratelli Denis   aggiornato il 29/11/2020
//sketch per scheda prototipo su base arduino per simulare sensori per prova schede madri quadro


#include <EEPROM.h>
#include <Wire.h>
#include "posizione_magneti.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define disp2 display.clearDisplay(); display.setTextSize(2); display.setTextColor(WHITE)
#define disp3 display.clearDisplay(); display.setTextSize(3); display.setTextColor(WHITE)
#define disp4 display.clearDisplay(); display.setTextSize(4); display.setTextColor(WHITE)

#define pin_rd 2        // rifasatore discesa
#define pin_id 3       // sensore id - 61u      ULN
#define pin_is 4        // sensore is - 61n     ULN
#define pin_rs 5         // rifasatore salita   ULN
#define pin_sp 6        // startpermit     ULN
#define pin_porta 7  //  porta cabina            ULN           
//#define pin_output 8  // non assegnato   ULN rele 7
#define pin_ap 9   //   apertura porte     optoisolatore 1
#define pin_ch 10  // chiusura porte   optoisolatore 2
#define pin_av 11       // alta velocita   optoisolatore 3
#define pin_bv  12    // bassa velocita   optoisolatore 4
//pin 13    non connesso
//pin 14 A0 non connesso
//pin 15 A1 non connesso
#define pin_s 16        // salita  optoisolatore 5  (pin A2)
#define pin_d 17        // discesa  optoisolatore 6 (pin A3)
//pin 18 (A4) seriale I2c SDA
//pin 19 (A5) seriale I2c SCL
//pin 20 (A6) non connesso
#define pin_menu A7    // ingresso analogico pulsanti menu

Adafruit_SSD1306 display(128, 64, &Wire, 4);

bool is = 1;
bool id = 1;
bool rs = 1;
bool rd = 0;
bool sp = 1;
int pos = 0;                  // posizione nel vano (100 per ogni piano)
int vel = 0;                  // variabile utilizzata tramite millis per velocita aumento posizione
int stato = 0;                // variabile per stato menu
bool attesa = true;           //flag attesa entrata menu
bool first = true;            //flag per lettura eeprom & gestione menu
bool flag_porta = true;       //flag per tempo chiusura
bool flag_tempo = true;       //flag per avanzamento tempo conteggio vano
bool set = EEPROM.read(0);    //   0 epb    1 systemlift
unsigned long previousMillis = 0;
unsigned long millis_porta = 0;
unsigned long millis_tempo = 0;

///////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  //  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display(); display.clearDisplay();


  pinMode(pin_is, OUTPUT);
  pinMode(pin_id, OUTPUT);
  pinMode(pin_rs, OUTPUT);
  pinMode(pin_rd, OUTPUT);
  pinMode(pin_sp, OUTPUT);
  pinMode(pin_porta, OUTPUT);
  digitalWrite(pin_porta, 1);
  pinMode(pin_ap, INPUT_PULLUP);
  pinMode(pin_ch, INPUT_PULLUP);
  pinMode(pin_av, INPUT_PULLUP);
  pinMode(pin_bv, INPUT_PULLUP);
  pinMode(pin_s, INPUT_PULLUP);
  pinMode(pin_d, INPUT_PULLUP);
  pinMode(pin_menu, INPUT);


  if (analogRead(pin_menu) > 250 && analogRead(pin_menu) < 260) {   //leggo pin per entrare nel menu
    stato = 1;                                                      //premere "ok" durante il boot
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  switch (stato) {
    case 0: simulazione(); break;
    case 1: menu_exit(); break;
    case 2: menu_setvano(); break;
    case 21: sub_menu_setvano(); break;
    case 3: menu_tipo(); break;
    case 31: sub_menu_tipo(); break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void simulazione() {

  if (first == true) {
    set = EEPROM.read(0);
    first = false;
  }

  if (millis() - previousMillis >= 100) { // trasmetto al display ogni 100Ms

    previousMillis = millis();
    disp4; display.setCursor(20, 25); display.println(pos);
    if (vel == 300) {
      display.setTextSize(2); display.setCursor(0, 0); display.print("STOP");
    }
    if (vel >= 20 && vel < 110) {
      display.setTextSize(2); display.setCursor(0, 0); display.print("ALTA");
    }
    if (vel >= 110 && vel < 300) {
      display.setTextSize(2); display.setCursor(0, 0); display.print("BASSA");
    }

    display.display();
  }

  int  ap = digitalRead(pin_ap);
  int  ch = digitalRead(pin_ch);
  int  s = digitalRead(pin_s);
  int  d = digitalRead(pin_d);
  int av = digitalRead(pin_av);
  int bv = digitalRead(pin_bv);

  digitalWrite (pin_is , is);
  digitalWrite (pin_id , id);
  digitalWrite (pin_rs , rs);
  digitalWrite (pin_rd , rd);
  digitalWrite (pin_sp , sp);

  if (ap == 0) {
    digitalWrite (pin_porta , 0);                              // apro porte
    flag_porta = true;
  }

  if (ch == 0) {                                               //chiudo porte con ritardo
    if (flag_porta == true) {
      millis_porta = millis();
      flag_porta = false;
    }
    if (millis() - millis_porta > 3000) {
      digitalWrite (pin_porta , 1);
    }
  }


  if (s == 0 || d == 0) {                                 //quando pin salita o discesa sono attivi

    if (vel >= 40 && av == 0 && flag_tempo == true) {             //parto in velocita alta
      vel = vel -= 20 ;                                           //accelerazione
    }

    if (vel >= 110 && (vel >= 110 && bv == 0) && av == 1 && flag_tempo == true) { //parto in bassa
      vel = vel -= 10 ;                                           //accelerazione
    }

    if (vel <= 100 && av == 1  && flag_tempo == true) {            //passo da velocita alta a bassa
      vel = vel += 10 ;                                            //decelerazione
    }
  }


  if (s == 1 && d == 1 ) {
    vel = 300 ;                                                    //velocita partenza
  }

  if (s == 0  && pos < 500 ) {                                     //attivo salita e

    if (flag_tempo == true) {
      millis_tempo = millis();
      flag_tempo = false;
    }
    if (millis() - millis_tempo > vel) {
      flag_tempo = true;
      pos = ++pos;                                                //aumenta posizione
    }
  }

  if (d == 0 && pos > 0 ) {                                       //attivo discesa e
    if (flag_tempo == true) {
      millis_tempo = millis();
      flag_tempo = false;
    }
    if (millis() - millis_tempo > vel) {
      flag_tempo = true;
      pos = --pos;                                                   //diminuisco posizione
    }
  }


  id = LOW ; is = LOW ; rd = HIGH ; rs = HIGH ; sp = HIGH ;

  if (s == 0 || d == 0) {                                            // funzione startpermit
    sp = LOW;
  }

  posizione_magneti();                                  //  chiamata funzione per lettura magneti

}

//////////////////////////////////////////////////////////////////////////////////////////////

void setStato(int s) {
  stato = s;
  first = true;
  display.clearDisplay();
  delay(300);
}

////////////////////////////////////////////////////////////////////////////////////////////////

void menu_exit() {
  if (attesa) {
    disp2; display.setCursor(0, 10); display.println("ATTENDERE PREGO"); display.display();
    attesa = false;
    delay(4000);
  }
  if (first) {
    disp2;
    display.setCursor(0, 10);
    display.println("EXIT&SAVE");
    display.setCursor(0, 30);    display.println("UP");
    display.setCursor(0, 50);    display.println("DOWN");
    display.display();
    first = false;
  }

  int key = readkey();
  switch (key) {
    case 1:
      EEPROM.update(0, set);
      disp2; display.setCursor(0, 10); display.println("SALVO IN  EEPROM"); display.display();
      delay(4000);
      setStato(0); break;
    case 2: setStato(2); break;
    case 3: setStato(3); break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////

void menu_setvano() {
  if (first) {
    disp2; display.setCursor(0, 0); display.println("TIPO      MAGNETI"); display.display();
    first = false;
    delay(200);
  }
  int key = readkey();
  switch (key) {
    case 1: setStato(21); break;
    case 2: setStato(3); break;
    case 3: setStato(1); break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////

void sub_menu_setvano() {
  if (first) {
    if (set == true) {
      disp3; display.setCursor(20, 20); display.println("system"); display.display();
      first = false;
      delay(100);
    }
    if (set == false) {
      disp3; display.setCursor(20, 20); display.println("kone"); display.display();
      first = false;
      delay(100);
    }
  }

  int key = readkey();
  switch (key) {
    case 1: setStato(2); break;
    case 2: set = !set; setStato(21); break;
    case 3: set = !set; setStato(21); break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////

void menu_tipo() {
  if (first) {
    disp2; display.setCursor(0, 0); display.println("TIPO      IMPIANTO"); display.display();
    first = false;
    delay(200);
  }
  int key = readkey();
  switch (key) {
    case 1: setStato(31); break;
    case 2: setStato(1); break;
    case 3: setStato(2); break;
  }
}

//////////////////////////////////////////////////////////////////////////////////

void sub_menu_tipo() {

  if (first) {
    disp2; display.setCursor(0, 0); display.println("USI FUTURI"); display.display();
    first = false;
    delay(200);
  }

  int key = readkey();
  switch (key) {
    case 1: setStato(3); break;
    case 2: setStato(3); break;
    case 3: setStato(3); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int readkey() {
  // return   pulsante ok=1    pulsante su = 2    pulsante giu = 3    nessun pulsante = 0 \\

  if (analogRead(pin_menu) > 250 && analogRead(pin_menu) < 260) {
    return 1;
  }
  else if (analogRead(pin_menu) > 500 && analogRead(pin_menu) < 515) {
    return 2;
  }
  else if (analogRead(pin_menu) > 760 && analogRead(pin_menu) < 770) {
    return 3;
  }
  else return 0;
}


////////////////////////////////FINE\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
