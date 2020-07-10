/**
 * Hardware auf Thinigverse: https://www.thingiverse.com/thing:4152063
 * Der Sektch ist inspiriert durch https://github.com/oOmicha1986Oo/ChaosUhr
 * Auch wenn einige Teile übernommen wurden, lohnte sich ein Fork nicht da klar war das nicht viel übrig bleiben wird.
 */

/**
 * Version: 0.0.1
 * Initiale Version
 */

#define VERSION             "0.0.1" 

#define ONCE_PER_SECOND     1
#define ONCE_PER_MINUTE     60
#define ONCE_PER_HOUR       3600
#define ONCE_PER_DAY        86400
#define ONCE_PER_MONNTH     2592000
#define ONCE_PER_YEAR       31104000

#define FASTLED_FORCE_SOFTWARE_SPI

/* Konfiguratiom */
// Hardware
#define DATA_PIN            D6                              // Output PIN für Matrix (D6 für Wemos, für andere Boards evtl. nur die 6)
#define MATRIX_WIDTH        20                              // LEDs in Matrixbreite
#define MATRIX_HEIGHT       15                              // LEDs in Matrixhöhe
#define NUM_LEDS            (MATRIX_WIDTH * MATRIX_HEIGHT)  // Anzahl LEDs der Matrix

// Software
#define HOSTNAME            "ChaosClock"                    // Wird als Hostname im Client-Modus und als SSID für den WiFi-Manager verwendet. (Default: ChaosClock)
#define AP_TIMEOUT          600                             // Timeout für WiFi-Autoconnect (Default: 600 Sekunden)
#define BRIGHTNESS          20                              // Helligkeit Uhr
#define NTP_SERVER_1        "192.168.0.200"                 // Erster NTP-Server
#define NTP_SERVER_2        "de.pool.ntp.org"               // Zweiter NTP-Server
#define NTP_SERVER_3        "europe.pool.ntp.org"           // Dritter NTP-Server
#define SYNCPERIOD          ONCE_PER_DAY                    // Syncronisierungsintervall mit NTP-Server. (Default: ONCE_PER_DAY)
/* Konfiguration Ende */

#include <WiFiManager.h> 
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <Fonts/TomThumb.h>
#include <time.h>
#include <LittleFS.h>
#include <pgmspace.h>

#include "BombJack.h"

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} t_color;

typedef struct listitem {
  void (*animationFunc)();
  int currentCycle;
  int maxCycles;
  int brightnessFactor;
  struct listitem *nextItem;
} t_animationList;

typedef enum { single = 0, dual = 1 } t_mixMode;
const char* const dayNames[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};

time_t current_timestamp;
CRGB leds[NUM_LEDS]; 
FastLED_NeoMatrix *matrix;
t_animationList animations, *currentAnimation = &animations;
int numAnimations = 0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

/* Initialisierung */
void setup() {
  /* Serielle Schnittstelle zum debuggen konfigurieren */
  setupSerial();
  
  /* WiFi-Verbindung herstellen */
  setupWifi();
  
  /* LED-Matrix configurieren */
  setupMatrix();

  /* Startscreen */
  // Versions-Information anzeigen
  matrix->fillScreen(0);       
  matrix->setTextColor(matrix->Color(255, 255, 255));
  matrix->setCursor(5, 6);
  matrix->print("FW: "); 
  matrix->setCursor(3, 13);
  matrix->print(VERSION);
  matrix->show();
  delay(5000);

  // Ladebildschirm
  matrix->fillScreen(0);       
  matrix->setTextColor(matrix->Color(255, 79, 0));
  matrix->setCursor(2, 6);
  matrix->print("LOAD");  
  matrix->setCursor(0, 13);
  matrix->print("CHAOS");
  matrix->show();
  delay(5000);

  setupTime();
  
  LittleFS.begin();
  File f;
  
  f = LittleFS.open("/bombjack01.cci", "w");
  if (!f) {
    Serial.println("file open failed");
  } else {
    for(int i = 0; i < NUM_LEDS; i++) {
      long color = pgm_read_dword(&(BombJack01[i]));  // Read array from Flash
      f.write((byte *)&color, 4);
    }
    f.close();
    Serial.println("Wrote bombjack01.cci");
  }

  f = LittleFS.open("/bombjack02.cci", "w");
  if (!f) {
    Serial.println("file open failed");
  } else {
    for(int i = 0; i < NUM_LEDS; i++) {
      long color = pgm_read_dword(&(BombJack02[i]));  // Read array from Flash
      f.write((byte *)&color, 4);
    }
    f.close();
    Serial.println("Wrote bombjack02.cci");
  }

  LittleFS.end();

  currentAnimation = &animations;
  currentAnimation->animationFunc = showDateTime;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 2;
  currentAnimation->brightnessFactor = 1;
  currentAnimation->nextItem = (t_animationList *)malloc(sizeof(t_animationList));

  currentAnimation = currentAnimation->nextItem;
  currentAnimation->animationFunc = rainbow;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 1100;
  currentAnimation->brightnessFactor = 1;
  currentAnimation->nextItem = (t_animationList *)malloc(sizeof(t_animationList));

  currentAnimation = currentAnimation->nextItem;
  currentAnimation->animationFunc = rainbowWithGlitter;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 1100;
  currentAnimation->brightnessFactor = 1;
  currentAnimation->nextItem = (t_animationList *)malloc(sizeof(t_animationList));

  currentAnimation = currentAnimation->nextItem;
  currentAnimation->animationFunc = confetti;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 1100;
  currentAnimation->brightnessFactor = 2;
  currentAnimation->nextItem = (t_animationList *)malloc(sizeof(t_animationList));

  currentAnimation = currentAnimation->nextItem;
  currentAnimation->animationFunc = sinelon;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 1100;
  currentAnimation->brightnessFactor = 2;
  currentAnimation->nextItem = (t_animationList *)malloc(sizeof(t_animationList));

  currentAnimation = currentAnimation->nextItem;
  currentAnimation->animationFunc = bpm;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 1100;
  currentAnimation->brightnessFactor = 1;
  currentAnimation->nextItem = (t_animationList *)malloc(sizeof(t_animationList));

  currentAnimation = currentAnimation->nextItem;
  currentAnimation->animationFunc = juggle;
  currentAnimation->currentCycle = 0;
  currentAnimation->maxCycles = 1100;
  currentAnimation->brightnessFactor = 1;
  currentAnimation->nextItem = &animations;

  numAnimations = 7;

  currentAnimation = &animations;
}

void setupSerial() {
  Serial.begin(115200); 
  Serial.println("\nChaosClock remastered v" VERSION);  
}

void setupWifi() {
  WiFi.hostname(HOSTNAME);

  WiFiManager wifiManager;
  wifiManager.setTimeout(AP_TIMEOUT);
  if (!wifiManager.autoConnect(HOSTNAME)) {
    Serial.println("Keine WiFi-Verbindung hergestellt");

    // WiFi komplett deaktivieren
    WiFi.mode(WIFI_OFF);
  } else {
    Serial.println("Wifi-Verbindung hergestellt)");
    Serial.print("Meine IP: ");
    Serial.println(WiFi.localIP());

    // WiFi-AccessPoint deaktivieren
    WiFi.mode(WIFI_STA);
  }
}

void setupMatrix() {
  FastLED.addLeds<NEOPIXEL,DATA_PIN>(leds, NUM_LEDS);   
  FastLED.setBrightness(BRIGHTNESS);
  
  matrix = new FastLED_NeoMatrix(leds, MATRIX_WIDTH, MATRIX_HEIGHT, 1, 1, NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setBrightness(BRIGHTNESS);
  matrix->setFont(&TomThumb);
}

void setupTime() {
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);  // deinen NTP Server einstellen (von 0 - 5 aus obiger Liste) alternativ lassen sich durch Komma getrennt bis zu 3 Server angeben
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);   // Zeitzone einstellen https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
}

void loop() {
  // Zunächst Urzeit-Variable aktualisieren, überprüfung ob Zeitstempel mindestens 1 Sekunde abweicht
  // Synchronisierungsintervall auswerten und ggf. mit NTP-Server synchronisieren
  if (current_timestamp != time(&current_timestamp) && (current_timestamp % SYNCPERIOD) == 0) {
    Serial.println("sync Time");
    setupTime();      
  }

  if (currentAnimation->currentCycle >= currentAnimation->maxCycles) {
    // next Animation
    if (currentAnimation != &animations) {
      currentAnimation = &animations;
    } else {
      int c = random(1, numAnimations);
  
      while (c-- > 0) {
        currentAnimation = currentAnimation->nextItem;
      }
    }
    
    currentAnimation->currentCycle = 0;
    currentAnimation->animationFunc();
  } else {
    currentAnimation->animationFunc();
  }

  gHue++;
}

t_color mixColor (t_mixMode mixMode) {
    t_color color = { 255, 255, 255 };
    
    static uint8_t singleMix = 3;
    static uint8_t dualMix = 3;

    if (mixMode == t_mixMode::single) {
      switch (--singleMix) {
        case 0:
          color.r = random(100,255);
          break;
        case 1:
          color.g = random(100,255);
          break;
        case 2:
          color.b = random(100,255);
          break;
      }

      if (singleMix == 0)
        singleMix = 3;
    } else if (mixMode == t_mixMode::dual) {
      switch (--dualMix) {
        case 0:
          color.r = random(100,255);
          color.g = random(100,255);
          break;
        case 1:
          color.g = random(100,255);
          color.b = random(100,255);
          break;
        case 2:
          color.r = random(100,255);
          color.b = random(100,255);
          break;
      }

      if (dualMix == 0)
        dualMix = 3;
    }

    return color;
}

void showDateTime() {
  static int x = matrix->width();
  static t_color timeColor = mixColor(single);
  static t_color dateColor = mixColor(dual);

  struct tm lt;
  char datum[30]; 
  int offset, scroll;

  // Unix-Timestamp in time_t konvertieren
  localtime_r(&current_timestamp, &lt);

  matrix->clear();
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);

  // Anzeige der Uhrzeit
  matrix->setTextColor(matrix->Color(timeColor.r, timeColor.g, timeColor.b));
  matrix->setCursor(2, 6);
  matrix->printf("%.2d:%.2d", lt.tm_hour, lt.tm_min); 

  // Datum zusammensetzen, Wochentag + Tag, Monat und Jahr
  offset = strlen(dayNames[lt.tm_wday]) + 3;
  sprintf(datum, "%s   ", dayNames[lt.tm_wday]);
  strftime(&datum[offset], sizeof(datum) - offset, "%d.%m.%Y", &lt);

  // Gesamtanzahl an LED-Spalten ermitteln
  scroll = (strlen(datum) * -3) - ((strlen(dayNames[lt.tm_wday])) *2 ) +3 ; // Anzahl Pixel + Leerzeichen   

  // Anzeige des Datums
  matrix->setTextColor(matrix->Color(dateColor.r, dateColor.g, dateColor.b));  
  matrix->setCursor(x, 13);
  matrix->printf(datum);

  // Datum scrollen
  if(--x < scroll) {
    timeColor = mixColor(single);
    dateColor = mixColor(dual);
    
    x = matrix->width();
    currentAnimation->currentCycle++;
  }

  // Anzeige aktualisieren
  matrix->show();
  delay(125);  
}

//  ***************************
//  *   DEMO Reel Animation   *
//  ***************************

void rainbow() {   
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);
  fill_rainbow( leds, NUM_LEDS, gHue*2, 7);
  FastLED.show();
  currentAnimation->currentCycle++;
}

void rainbowWithGlitter() {  
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);
  fill_rainbow( leds, NUM_LEDS, gHue*2, 7);
  addGlitter(50);  
  FastLED.show();   
  currentAnimation->currentCycle++;
}

void confetti() {
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);
  fadeToBlackBy( leds, NUM_LEDS, 50);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue*2 + random16(150), 210, 255);
  FastLED.show(); 
  currentAnimation->currentCycle++;
}

void sinelon(){    
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);
  fadeToBlackBy( leds, NUM_LEDS,10);
  int pos = beatsin16( 25, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 210);
  FastLED.show();  
  currentAnimation->currentCycle++;
}

void bpm(){  
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);
  uint8_t BeatsPerMinute = 45;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  FastLED.show();   
  FastLED.delay(10);
  currentAnimation->currentCycle++;
}

void juggle() {  
  FastLED.setBrightness(BRIGHTNESS * currentAnimation->brightnessFactor);
  fadeToBlackBy( leds, NUM_LEDS, 60);
  byte dothue = 0;
  for( int i = 0; i < 9; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 210, 255);
    dothue += 28;
  }
  FastLED.show();   
  FastLED.delay(10);
  currentAnimation->currentCycle++;
}  

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }        
}
