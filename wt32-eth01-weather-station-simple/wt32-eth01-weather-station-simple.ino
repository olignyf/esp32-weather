//
//
// NOTE: For flashing PIN IO0 must be shorted with GND during flashing, but then must be disconnected and unit rebooted for things to run.
// WARNING: Pin IO2 needs to be unconnected for flashing to work, but can be used for input/output
// WARNING: Pin IO39 cannot be used for output on WT32-ETH01
// WARNING: Pin IO36 cannot be used for output on WT32-ETH01

int SDA_PIN = 14;
int SCL_PIN = 15;
#define DHTPIN 4 // dht sensor is connected to IO4
#define WHITE_DHTPIN 2 /// dht sensor is connected to IO2 

#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "###############"
#define BLYNK_TEMPLATE_NAME "Weather Station WT32 ESP01"
#define BLYNK_AUTH_TOKEN "###############"

#include <ETH.h>
#include <BlynkSimpleEsp32_SSL.h>

#include <BH1750.h>
#include <BMP085.h>

#include "DHT.h"
#include <Wire.h>

float temperature; // parameters
float humidity;
float pressure;
float mbar;

#define BH1750_DEFAULT_I2CADDR 0x23// because ADD is left floating 
BH1750 myBH1750;

BMP085 myBarometer; // initialise pressure sensor
#define ALTITUDE 1655.0 // Altitude of SparkFun's HQ in Boulder, CO. in meters

char auth[] = BLYNK_AUTH_TOKEN; // replace this with your auth token
char ssid[] = ""; // replace this with your wifi name (SSID)
char pass[] = ""; // replace this with your wifi password

#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301

#define WHITE_DHTTYPE DHT22     // DHT 11

DHT dht(DHTPIN, DHTTYPE); // initialise dht sensor
DHT dhtWhite(WHITE_DHTPIN, WHITE_DHTTYPE); // initialise dht sensor
BlynkTimer timer;

static bool eth_connected = false;
void WiFiEvent(WiFiEvent_t event)
{
  // Serial.println("Wifi Event");
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



void sendSensor() // function to read sensor values and send them to Blynk
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  float humidity2 = dhtWhite.readHumidity();
  float temperature2 = dhtWhite.readTemperature();
  if (isnan(humidity2) || isnan(temperature2)) 
  {
    Serial.println("Failed to read from DHT white sensor!");
  }
  else
  { // to compare both
    Serial.println("dht temperature: "+String(temperature,2));
    Serial.println("dht humidity: "+String(humidity,2));

    Serial.println("dht temperature HIGH PRECISION: "+String(temperature2,2));
    Serial.println("dht humidity HIGH PRECISION: "+String(humidity2,2));
  }

  pressure = myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP()); // read pressure value in pascals
  mbar = pressure / 100; // convert millibar to pascals
  
  Serial.println("sending temp to Blynk temperature: "+String(temperature,2));
  Serial.println("sending temp to Blynk humidity: "+String(humidity,2));
  Serial.println("sending temp to Blynk mbar: "+String(mbar,2));
  Blynk.virtualWrite(V0, temperature); // send all the values to their respective virtual pins
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, mbar);
  //Blynk.virtualWrite(V3, visible);
  //Blynk.virtualWrite(V4, ir);
  //Blynk.virtualWrite(V5, uv);
}

void setup()
{
  Serial.begin(9600);
  
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  
  Serial.println("Going to Wire begin");
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Going to BMP180 init");
  myBarometer.init();
  Serial.println("DHT init");
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
