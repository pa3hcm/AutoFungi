
/*
  Project:   AutoFungi
  Source:    http://github.com/pa3hcm/AutoFungi
  Author:    Ernest Neijenhuis PA3HCM
  License:   GPLv3
*/

// Tested on ESP32 board with ESP-WROOM-32 microcontroller


/////////////////////////////////////////////////////////////////////////
// Constants

const char *VERSION = "0.1b";

const int WDT_TIMEOUT = 10;  // Timeout (in sec) of the Watch Dog Timer

const int DHT_PIN = 4;  // data pin of the DHT22 temp/humidity sensor
#define DHT_TYPE DHT22  // We use a DHT22 type sensor

const int MOIST_PIN = 36; // analog input from the moisture sensor
const int MOIST_MIN = 1000; // lower moisture limit
const int MOIST_MAX = 1800; // upper moisture limit
const int PUMP_PIN = 5;
const int PUMP_MAX_ON = 30; // max time (sec) to run the waterpump
const int PUMP_MIN_OFF = 30; // keep pump off for a while after been on

/////////////////////////////////////////////////////////////////////////
// Global variables

bool pump = false;  // used to enable/disable the waterpump
long long int pump_start_time = 0;  // used to measure on-time of the waterpump


/////////////////////////////////////////////////////////////////////////
// Libraries

// Watchdog
#include <esp_task_wdt.h>

// DHT22 sensor
#include <DHT.h>
DHT dht(DHT_PIN, DHT_TYPE);


/////////////////////////////////////////////////////////////////////////
// setup() routine

void setup() {
  // Setup serial monitoring
  Serial.begin(115200);
  
  // Configure WDT (Watch Dog Timer)
  Serial.println("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  
  // Configure DHT
  pinMode(LED_BUILTIN, INPUT);
  Serial.println("Configuring DHT22...");
  dht.begin();
  
  // Configure MOIST
  Serial.println("Configuring MOIST...");
  pinMode(MOIST_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  // Builtin LED used to show heartbeat/activity
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("AutoFungi configured, now starting main loop.");
}


/////////////////////////////////////////////////////////////////////////
// Main routine
void loop() {
  // Reset WDT
  esp_task_wdt_reset();

  // Activity starts
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.printf("ESP32: sta = AutoFungi v%4s - up %011.3fs\n", VERSION, esp_timer_get_time()/1000000.0);

  // Read DHT sensor
  float dht_temp = dht.readTemperature(false);
  float dht_hum = dht.readHumidity();
  Serial.printf("DHT22: tmp = %+5.1fC\n", dht_temp);
  Serial.printf("DHT22: hum = %2.0f%%\n", dht_hum);

  // Read soil moisture
  int moist = analogRead(MOIST_PIN);
  Serial.printf("MOIST: lvl = %4i\n", moist);
  
  // Determine if pump should be enabled or not
  if (moist < MOIST_MIN) {  // Enable pump when soil is too dry
    pump = true;
  }
  if (moist > MOIST_MAX) {  // Disable pump when soil is wet enough
    pump = false;
  }

  // Pump should not run for more than PUMP_TIMEOUT seconds
  if (pump) {
    if (pump_start_time == 0) {  // pump is not on yet
      pump_start_time = esp_timer_get_time() / 1000000;
      Serial.print("EVENT: PUMP_ON\n");
      Serial.println(pump_start_time);
    } else {
      if ((pump_start_time + PUMP_MAX_ON) < (esp_timer_get_time() / 1000000)) {  // pump on too long
        pump = false;
      }
    }
  } else {
    if (pump_start_time != 0) {  // pump is off but has been on recently
      if ((pump_start_time + PUMP_MAX_ON + PUMP_MIN_OFF) < (esp_timer_get_time() / 1000000)) { 
        pump_start_time = 0;
      }
    }
  }
  
  if (pump) {  // Control the pump
    Serial.printf("MOIST: pmp = on - since %07llu\n", pump_start_time);
    digitalWrite(PUMP_PIN, HIGH);
  } else {
    if (pump_start_time == 0) {
      Serial.printf("MOIST: pmp = off\n");
    } else {
      Serial.printf("MOIST: pmp = off - since %07llu\n", pump_start_time + PUMP_MAX_ON);
    }
    digitalWrite(PUMP_PIN, LOW);
  }

  // Activity stops
  Serial.printf("-----\n");
  digitalWrite(LED_BUILTIN, LOW);

  // Sleep for a while
  delay(3000);
}