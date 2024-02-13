#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "###############"
#define BLYNK_TEMPLATE_NAME "Weather Station WT32 ESP01"
#define BLYNK_AUTH_TOKEN "###############"

#include <ETH.h>
#include <BlynkSimpleEsp32_SSL.h>

#include <BH1750.h>
#include <BMP085.h>

#include "DHT.h"

float temperature; // parameters
float humidity;

char auth[] = BLYNK_AUTH_TOKEN; // replace this with your auth token
char ssid[] = ""; // replace this with your wifi name (SSID)
char pass[] = ""; // replace this with your wifi password

#define DHTPIN 4 // dht sensor is connected to IO4
#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301

#define WHITE_DHTPIN 14 // dht sensor is connected to IO4
#define WHITE_DHTTYPE DHT22     // DHT 11

DHT dht(DHTPIN, DHTTYPE); // initialise dht sensor
DHT dhtWhite(WHITE_DHTPIN, WHITE_DHTTYPE); // initialise dht sensor
BlynkTimer timer;

static bool eth_connected = false;
void WiFiEvent(WiFiEvent_t event)
{
  //Serial.println("Wifi Event");
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

int LED = 2;

void sendSensor() // function to read sensor values and send them to Blynk
{
  digitalWrite(LED, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.println("dht temperature: "+String(temperature,2));
  Serial.println("dht humidity: "+String(humidity,2));

  humidity = dhtWhite.readHumidity();
  temperature = dhtWhite.readTemperature();
  if (isnan(humidity) || isnan(temperature)) 
  {
    Serial.println("Failed to read from DHT white sensor!");
    return;
  }  
  Serial.println("dht temperature HIGH PRECISION: "+String(temperature,2));
  Serial.println("dht humidity HIGH PRECISION: "+String(humidity,2));

  Serial.println("sending temp to Blynk temperature: "+String(temperature,2));
  Serial.println("sending temp to Blynk humidity: "+String(humidity,2));
  Blynk.virtualWrite(V0, temperature); // send all the values to their respective virtual pins
  Blynk.virtualWrite(V1, humidity);
  digitalWrite(LED, LOW);   // turn the LED off
}

void setup()
{
  Serial.begin(9600);

  pinMode(LED, OUTPUT);
  
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  
  Serial.println("Going to BM init");

  dht.begin();
  dhtWhite.begin();

  delay(1000);

  Serial.println("Going to Blynk config");
  Blynk.config(auth);
  Blynk.connect();
  Serial.println("Blynk ready");

  timer.setInterval(15000L, sendSensor); // sendSensor function will run every 1000 milliseconds
}

void loop()
{
  if (eth_connected)
  {
     Blynk.run();
  }
  timer.run();
}
