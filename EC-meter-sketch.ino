#include <EEPROM.h>
#include <DFRobot_EC.h> 
#include <U8g2lib.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin für DS18B20 Temperatursensor
#define ONE_WIRE_BUS 2

// OneWire und DallasTemperature Instanzen initialisieren
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Konstruktor für das NHD-C12864 Display im Page-Mode (schonend für den RAM-Speicher)
U8G2_ST7565_NHD_C12864_1_4W_SW_SPI u8g2(U8G2_R2, 13, 11, 10, 9, 8);

#define EC_PIN A1 
#define VREF 5000.0 

float voltage, ecValue;
float currentTemperature = 25.0; 
const float compTemperature = 25.0; // Fester Wert für die EC-Messung (keine Kompensation)

DFRobot_EC ec;

void setup() { 
  Serial.begin(9600); 
  ec.begin(); 
  // DS18B20 Sensor starten
  sensors.begin();
  // Wartezeit deaktivieren -> Macht die Messung asynchron / nicht-blockierend
  sensors.setWaitForConversion(false);
  // Erste Messung direkt anfordern
  sensors.requestTemperatures();
  // Display initialisieren 
  u8g2.begin(); 
  u8g2.setContrast(150); 
  u8g2.setFont(u8g2_font_t0_11_tf); 
  u8g2.enableUTF8Print();
}

void loop() { 
  static unsigned long timepoint = millis(); 
  int analogRaw = analogRead(EC_PIN); 
  voltage = (float)analogRaw / 1023.0 * VREF; 
  
  if (millis() - timepoint > 1000U) { 
    timepoint = millis(); 
    
    // Den Temperaturwert blitzschnell auslesen
    float tempRead = sensors.getTempCByIndex(0);
    
    // Prüfen, ob der Sensor korrekt antwortet (DEVICE_DISCONNECTED_C ist -127.0)
    if (tempRead != DEVICE_DISCONNECTED_C) {
      currentTemperature = tempRead;
    }
    
    // Sofort die nächste Messung für die Folgesekunde im Hintergrund anstoßen
    sensors.requestTemperatures();
    
    // EC-Wert berechnen
    ecValue = ec.readEC(voltage, compTemperature); 
    
    // Ausgabe serieller Monitor
    Serial.print("EC-Wert: "); 
    Serial.print(ecValue, 2); 
    Serial.print(" mS/cm | "); 
    Serial.print("Temperatur: ");
    if (tempRead == DEVICE_DISCONNECTED_C) {
      Serial.println("Fehler (Sensor getrennt)");
    } else {
      Serial.print(currentTemperature, 1);
      Serial.println(" °C");
    }
    // Ausgabe auf dem Display
    u8g2.firstPage(); 
    do { 
      u8g2.setCursor(0, 15); 
      u8g2.print("EC: "); 
      u8g2.print(ecValue, 2); 
      u8g2.print(" mS/cm"); 
      u8g2.setCursor(0, 32); 
      u8g2.print("Temp: ");
      if (tempRead == DEVICE_DISCONNECTED_C) {
        u8g2.print("ERR"); // "ERR" wird angezeigt, wenn der Sensor entfernt wird
      } else {
        u8g2.print(currentTemperature, 1);
        u8g2.print(" °C");
      }
    } while ( u8g2.nextPage() ); 
  } 
  // Kalibrierung läuft flüssig im Hintergrund
  ec.calibration(voltage, compTemperature); 
}
