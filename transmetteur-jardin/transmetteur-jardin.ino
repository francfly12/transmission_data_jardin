/*
///////////////////////////////////////////////////////////
// TRANSMETTEUR DE JARDIN — version 1
// Arduino Mega 2560 (ports série hardware)
//
// Mesure la puissance de deux blocs de panneaux solaires :
//   - PZEM1 : bloc fixe (6 panneaux, ~1800 W)
//   - PZEM2 : tracker mobile (2 x 450 W)
// et transmet les deux valeurs par radio HC12, toutes les 3 s.
// Seule la puissance (W) est transmise.
//
// Câblage :
//   PZEM1 sur Serial1 : TX1 (D18) -> RX du PZEM1
//                       RX1 (D19) -> TX du PZEM1
//   PZEM2 sur Serial2 : TX2 (D16) -> RX du PZEM2
//                       RX2 (D17) -> TX du PZEM2
// Note: si la partie AC des PZEM n'est pas alimentée, on a la valeur P=ERREUR
//   HC12  sur Serial3 : TX3 (D14) -> RXD du HC12
//                       RX3 (D15) -> TXD du HC12
//   LED embarquée : D13 (flash bref à chaque trame envoyée)
//
// Trame radio : A<p1>B<p2>\n    ex : A1802.5B913.0\n
//   - '\n' final OBLIGATOIRE (leçon ALBI : sans lui, le récepteur
//     ne détecte jamais la fin de trame -> ok=0 rej=0)
//   - marqueurs A et B distincts pour un parseur strchr simple
//
// Leçons ALBI réutilisées ici :
//   - PZEM004Tv30 (Jakub Mandula) : debug par défaut sur Serial1,
//     il FAUT #define PZEM_DEFAULT_DEBUG_PORT Serial AVANT l'include
//     (sinon du debug part sur le port d'un PZEM !)
//   - #include <SoftwareSerial.h> requis avant l'include PZEM
//   - pzem.power() retourne NaN en cas d'erreur : on ne transmet PAS
//     si une lecture échoue (le récepteur garde sa dernière valeur)
//   - timing par millis(), pas de delay() bloquant
///////////////////////////////////////////////////////////
*/

#include <Arduino.h>
#include <SoftwareSerial.h>              // requis par la bibliothèque PZEM
#define PZEM_DEFAULT_DEBUG_PORT Serial   // debug sur USB, AVANT l'include PZEM
#include <PZEM004Tv30.h>                 // bibliothèque PZEM-004T v3.0

PZEM004Tv30 pzem1(&Serial1);   // bloc fixe 6 panneaux
PZEM004Tv30 pzem2(&Serial2);   // tracker mobile 2 x 450 W

const unsigned long PERIODE_MS = 3000;   // une mesure toutes les 3 s
const uint8_t PIN_LED = 13;              // LED embarquée de la Mega

unsigned long dernierCycleMs = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(9600);    // moniteur série USB (fonctionne aussi sans PC)
  Serial3.begin(9600);   // HC12, même vitesse que sur ALBI

  delay(200);            // laisse le temps aux ports de démarrer
  Serial.println(F("Transmetteur de jardin v1"));
}

void loop() {
  if (millis() - dernierCycleMs < PERIODE_MS) return;
  dernierCycleMs += PERIODE_MS;

  float p1 = pzem1.power();   // NaN si le PZEM1 ne répond pas
  float p2 = pzem2.power();   // NaN si le PZEM2 ne répond pas

  Serial.print(F("P1 = "));
  Serial.print(isnan(p1) ? F("ERREUR") : String(p1, 1));
  Serial.print(F(" W   P2 = "));
  Serial.print(isnan(p2) ? F("ERREUR") : String(p2, 1));
  Serial.println(F(" W"));

  // si une des deux lectures échoue : pas de trame radio envoyée
  if (isnan(p1) || isnan(p2)) return;

  // trame : A<p1>B<p2>\n
  Serial3.print('A');
  Serial3.print(p1, 1);
  Serial3.print('B');
  Serial3.print(p2, 1);
  Serial3.print('\n');

  // flash bref de la LED : trame envoyée (2 ms, comme sur ALBI)
  digitalWrite(PIN_LED, HIGH);
  delay(2);
  digitalWrite(PIN_LED, LOW);
}
