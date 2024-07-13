#include <ClosedCube_TCA9548A.h>
#include <rpcWiFi.h>
#include <WifiUdp.h>
#include <Wire.h>

#include "Free_Fonts.h"
#include "SPI.h"
#include "TFT_eSPI.h"

#include "Adafruit_SGP30.h"
#include "SHTSensor.h"
#include "QMP6988.h"

// #define DEBUG_MODE
#define LCD_BACKLIGHT (72Ul)
#define PaHub_I2C_ADDRESS 0x70
#define ENV3_UNIT_CHANNEL_NUM 1
#define TVOC_UNIT_CHANNEL_NUM 2

ClosedCube::Wired::TCA9548A tca9548a;

Adafruit_SGP30 sgp;
SHTSensor sht30;
QMP6988 qmp6988;

// the IP address of your InfluxDB host
// the port that the InfluxDB UDP plugin is listening on
WiFiUDP udp;
// byte host_ip[] = {192, 168, 0, 32};
byte host_ip[] = {118, 27, 104, 237};

char ssid[] = "";     //  your network SSID (name) 
char pass[] = "";    // your network password
int udp_port = 8089;
int status = WL_IDLE_STATUS;     // the Wifi radio's status

char str_i[2];
int i = 15;
long last_millis = 0;

TFT_eSPI tft = TFT_eSPI();
unsigned long drawTime = 0;
bool is_turn_on_tft = true;

void connect_wpa_network() {
  // attempt to connect using WPA2 encryption
  tft.setCursor(0, 20);
  tft.println("[Wifi] Connecting...");

  #ifdef DEBUG_MODE
    Serial.println("Attempting to connect to WPA network...");
  #endif
  WiFi.mode(WIFI_STA);
  status = WiFi.begin(ssid, pass);
  
  // if unable to connect, halt
  if (status != WL_CONNECTED) { 
    #ifdef DEBUG_MODE
      Serial.println("Couldn't get a WiFi connection");
    #endif
    while (1);
  } 
  // if the conneciton succeeded, print network info
  else {
    tft.fillScreen(TFT_BLACK);            // Clear screen
    tft.setCursor(0, 20);
    tft.println("[Wifi] Connected.\r\n");
    #ifdef DEBUG_MODE
      Serial.println("Connected to network");
    #endif

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    tft.printf("IP: ");
    tft.println(ip);
    #ifdef DEBUG_MODE
      Serial.println("Connected to network");
      Serial.print("IP Address: ");
      Serial.println(ip);
    #endif

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    tft.printf("RSSI: %ddBm\r\n");
    #ifdef DEBUG_MODE
      Serial.print("signal strength (RSSI): ");
      Serial.print(rssi);
      Serial.println(" dBm");
    #endif

    while (digitalRead(WIO_KEY_C) != LOW);
  }
}

void initialize_tft() {
  tft.begin();
  tft.setRotation(3);
  
  // Set text datum to top left
  tft.setTextDatum(TL_DATUM);
  
  // Set text colour to orange with black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setFreeFont(FF2);                 // Select the font
}

void initialize_env3_unit() {
  tca9548a.selectChannel(ENV3_UNIT_CHANNEL_NUM);
  delay(10);

  if (sht30.init()) {
    #ifdef DEBUG_MODE
      Serial.print("Init ENV3 Unit: success\n");
    #endif
  } else {
    #ifdef DEBUG_MODE
      Serial.print("Init ENV3 Unit: failed\n");
    #endif
  }
  sht30.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // only supported by SHT3x

  qmp6988.init();
}

void initialize_tvoc_unit() {
  tca9548a.selectChannel(TVOC_UNIT_CHANNEL_NUM);
  delay(10);

  if (!sgp.begin()){
    #ifdef DEBUG_MODE
      Serial.println("TVOC Unit not found :(");
    #endif
    while (1);
  }
}

void calibrate_tvoc_unit() {
  tca9548a.selectChannel(TVOC_UNIT_CHANNEL_NUM);
  delay(10);

  while (i > 0) {
    if(millis() - last_millis > 1000) {
      tft.fillScreen(TFT_BLACK);            // Clear screen
      tft.setCursor(0, 20);
      tft.println("Init TVOC Unit...");
      #ifdef DEBUG_MODE
        Serial.println("Init TVOC Unit...");
      #endif

      last_millis = millis();
      i--;
    }
  }
}

void get_env3_unit_data(String* str_temperature, String* str_humidity) {
  char tft_output[50];
  float temperature = 0.0;
  float humidity = 0.0;

  tca9548a.selectChannel(ENV3_UNIT_CHANNEL_NUM);
  delay(10);
  
  if (sht30.readSample()) {
    temperature = sht30.getTemperature();
    humidity = sht30.getHumidity();
  }
  sprintf(tft_output, "Temp:  %3.1f[C]\r\nHumi:  %3.1f[RH]\r\n", temperature, humidity);
  tft.printf(tft_output);

  *str_temperature = String(temperature);
  *str_humidity = String(humidity);
}

void get_tvoc_unit_data(String* str_eCO2, String* str_TVOC) {
  char tft_output[50];
  int eCO2 = 0;
  int TVOC = 0;

  tca9548a.selectChannel(TVOC_UNIT_CHANNEL_NUM);
  delay(10);

  if (!sgp.IAQmeasure()) {
    #ifdef DEBUG_MODE
      Serial.println("[TVOC Unit] Measurement failed");
    #endif
    return;
  }

  eCO2 = sgp.eCO2;
  TVOC = sgp.TVOC;
  sprintf(tft_output, "eCO2: %5d[ppm]\r\nTVOC: %5d[ppb]\r\n", eCO2, TVOC);
  tft.printf(tft_output);

  *str_eCO2 = String(eCO2);
  *str_TVOC = String(TVOC);
}

void setup() {
  pinMode(WIO_KEY_C, INPUT_PULLUP);

  #ifdef DEBUG_MODE
    Serial.begin(9600);
    while(!Serial); // Wait for Serial to be ready
    delay(1000);
  #endif

  Wire.begin();
  tca9548a.address(PaHub_I2C_ADDRESS);

  initialize_tft();
  connect_wpa_network();
  initialize_env3_unit();
  initialize_tvoc_unit();
}

void loop() {
  String line;
  String str_temperature, str_humidity;
  String str_eCO2, str_TVOC;

  if (digitalRead(WIO_KEY_C) == LOW) {
    if (is_turn_on_tft == true) {
      digitalWrite(LCD_BACKLIGHT, LOW);
      is_turn_on_tft = false;
    } else {
      digitalWrite(LCD_BACKLIGHT, HIGH);
      is_turn_on_tft = true;
    }
  }

  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setCursor(0, 20);

  calibrate_tvoc_unit();
  get_env3_unit_data(&str_temperature, &str_humidity);
  get_tvoc_unit_data(&str_eCO2, &str_TVOC);

  line = String("WioTerminal temperature=" + str_temperature);
  line += String(",humidity=" + str_humidity);
  line += String(",eCO2=" + str_eCO2);
  line += String(",TVOC=" + str_TVOC);

  #ifdef DEBUG_MODE
    // concatenate the temperature into the line protocol
    Serial.println(line);
    Serial.println("Sending UDP packet...");
  #endif

  // send the packet
  udp.beginPacket(host_ip, udp_port);
  udp.print(line);
  udp.endPacket();

  // wait 1.0 second
  delay(1000);
}
