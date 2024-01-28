

#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "REPLACE_ME"
#define BLYNK_TEMPLATE_NAME "Weather Station ESP"
#define BLYNK_AUTH_TOKEN "REPLACE_ME"

#define SDA_PIN 13
#define SCL_PIN 14

#include <WiFi.h> // importing all the required libraries
#include <WiFiClient.h>

// DISPLAY BEGINs
#include <OneBitDisplay.h>
// if your system doesn't have enough RAM for a back buffer, comment out
// this line (e.g. ATtiny85)
#define USE_BACKBUFFER
#ifdef USE_BACKBUFFER
static uint8_t ucBackBuffer[1024];
#else
static uint8_t *ucBackBuffer = NULL;
#endif
// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#define RESET_PIN -1
// let OneBitDisplay figure out the display address
#define OLED_ADDR -1
// don't rotate the display
#define FLIP180 0
// don't invert the display
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 1
#define MY_OLED OLED_128x64
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
OBDISP obd;
// DISPLAY ENDs

#include <BH1750.h>
//#include "SI114X.h"
//#include <SI1145_WE.h>
//#include <BH1750FVI.h>

#include <BMP085.h>
//#include "SFE_BMP180.h"

#include <Blynk.h>
#include <BlynkSimpleEsp32.h>
#include "Arduino.h"
#include "DHT.h"
#include <Wire.h>

float temperature; // parameters
float humidity;
float pressure;
float mbar;
float uv;
float visible;
float ir;

//SI114X SI1145 = SI114X(); // initialise sunlight sensor
//SI1145_WE SI1145 = SI1145_WE(&Wire);
#define BH1750_DEFAULT_I2CADDR 0x23// because ADD is left floating 
//BH1750FVI myBH1750(BH1750_DEFAULT_I2CADDR, BH1750_ONE_TIME_HIGH_RES_MODE, BH1750_SENSITIVITY_DEFAULT, BH1750_ACCURACY_DEFAULT);
BH1750 myBH1750;

BMP085 myBarometer; // initialise pressure sensor
#define ALTITUDE 1655.0 // Altitude of SparkFun's HQ in Boulder, CO. in meters

char auth[] = BLYNK_AUTH_TOKEN; // replace this with your auth token
char ssid[] = "REPLACE_ME"; // replace this with your wifi name (SSID)
char pass[] = "REPLACE_ME"; // replace this with your wifi password

#define DHTPIN 5 // dht sensor is connected to D5
#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301

DHT dht(DHTPIN, DHTTYPE); // initialise dht sensor
BlynkTimer timer;

void sendSensor() // function to read sensor values and send them to Blynk
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  Serial.println("dht temparature: "+String(temperature,2));

  pressure = myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP()); // read pressure value in pascals
  mbar = pressure / 100; // convert millibar to pascals
    
  float lightLevel = myBH1750.readLightLevel();  //start measurment -> wait for result -> read result -> retrun result or 4294967295 if communication error is occurred
  visible = lightLevel;

  Serial.println("sending temp to Blynk temparature: "+String(temperature,2));
  Serial.println("sending temp to Blynk humidity: "+String(humidity,2));
  Serial.println("sending temp to Blynk mbar: "+String(mbar,2));
  //Serial.println("sending temp to Blynk uv: "+String(uv, 2));
  //Serial.println("sending temp to Blynk ir: "+String(ir, 2));
  Serial.println("sending temp to Blynk visible: "+String(visible, 2));
  Blynk.virtualWrite(V0, temperature); // send all the values to their respective virtual pins
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, mbar);
  Blynk.virtualWrite(V3, visible);
  Blynk.virtualWrite(V4, ir);
  Blynk.virtualWrite(V5, uv);

  char szTemp[32];
  //sprintf(szTemp, "%0.1f°C", temperature);
  obdFill(&obd, OBD_WHITE, 1);
  obdWriteString(&obd, 0,28,0,(char *)"Temperature", FONT_8x8, OBD_BLACK, 1);
  sprintf(szTemp, "                           ");
  obdWriteString(&obd, 0,0,8,(char *)szTemp, FONT_6x8, OBD_WHITE, 1);
  sprintf(szTemp, " %0.1f C", temperature);
  obdWriteString(&obd, 0,0,24,(char *)szTemp, FONT_16x16, OBD_BLACK, 1);
  sprintf(szTemp, " %0.1f %%", humidity);
  obdWriteString(&obd, 0,0,16+32,(char *)szTemp, FONT_16x16, OBD_BLACK, 1);
}

void setupDisplay()
{
  int rc;
  //   obdI2CInit(OBDISP*, type, oled_addr, rotate180, invert, bWire,    SDA_PIN, SCL_PIN, RESET_PIN, speed)
  rc = obdI2CInit(&obd, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND)
  {
    char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D",(char *)"SH1106 @ 0x3C",(char *)"SH1106 @ 0x3D"};
    obdFill(&obd, OBD_WHITE, 1);
    obdWriteString(&obd, 0,0,0,msgs[rc], FONT_8x8, OBD_BLACK, 1);
    obdSetBackBuffer(&obd, ucBackBuffer);
    delay(100);
  }
}

void setup()
{
  Serial.begin(115200);
 
  Serial.println("Going to Wire begin");
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Going to setup display");
  setupDisplay();

  Serial.println("Going to BM init");
  myBarometer.init();

  Serial.println("DHT init");
  dht.begin();

  Serial.println("Beginning Si1145!");
  while (myBH1750.begin() != true)
  {
    Serial.println(F("ROHM BH1750FVI is not present"));    //(F()) saves string to flash & keeps dynamic memory free
    delay(5000);
  }

  Serial.println(F("ROHM BH1750FVI is present"));

  /* If using FVI library change BH1750 settings on the fly 
  myBH1750.setCalibration(1.06);                           //call before "readLightLevel()", 1.06=white LED & artifical sun
  myBH1750.setSensitivity(1.00);                           //call before "readLightLevel()", 1.00=no optical filter in front of the sensor
  myBH1750.setResolution(BH1750_CONTINUOUS_HIGH_RES_MODE); //call before "readLightLevel()", continuous measurement with 1.00 lux resolution
*/

  delay(1000);
  Blynk.begin(auth, ssid, pass);
  Serial.println("Blynk ready");
  delay(1000);
  timer.setInterval(5000L, sendSensor); // sendSensor function will run every X milliseconds
}

void loop()
{
  Blynk.run();
  timer.run();
  delay(100);
}

