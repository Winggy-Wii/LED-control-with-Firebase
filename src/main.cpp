#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <addons/TokenHelper.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#include <sub.h>

#define API_KEY "AIzaSyAKdB8p-d4R26M2ZP4B3xodTtCPoduOhas"
#define DATABASE_URL "https://esp-firebase-demo-8cb7b-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "nghiepquy6@gmail.com"
#define USER_PASSWORD "12345678"

AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

// Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";

IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

CRGBPalette16 gPal;

String parentPath = "/test/stream/data";
String childPath[4] = {"/blue", "/red", "/green", "/brightness"};
String Transistor[4] = {};

volatile bool dataChanged = false;
boolean restart = false;

uint8_t RedPWM = 255;
uint8_t GreenPWM = 255;
uint8_t BluePWM = 255;
uint8_t Brightness = 255;
uint8_t tries = 0;
uint8_t count = 0;

void streamCallback(MultiPathStreamData stream)
{
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (stream.get(childPath[i]))
    {
      Serial.printf("path: %s, event: %s, type: %s, value: %s%s", stream.dataPath.c_str(), stream.eventType.c_str(), stream.type.c_str(), stream.value.c_str(), i < numChild - 1 ? "\n" : "");
      Transistor[i] = stream.value.c_str();
    }
  }

  Serial.println();
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", stream.payloadLength(), stream.maxPayloadLength());
  dataChanged = true;
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void ICACHE_RAM_ATTR CLK_ISR()
{
  count++;
  count = (count > 4) ? 0 : count;
}

void Fire2012WithPalette()
{
  // Array of temperature readings at each simulation cell
  static uint8_t heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; i++)
  {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = NUM_LEDS - 1; k >= 2; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING)
  {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < NUM_LEDS; j++)
  {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    uint8_t colorindex = scale8(heat[j], 240);
    CRGB color = ColorFromPalette(gPal, colorindex);
    int pixelnumber;
    if (gReverseDirection)
    {
      pixelnumber = (NUM_LEDS - 1) - j;
    }
    else
    {
      pixelnumber = j;
    }
    right_leds[pixelnumber] = color;
    left_leds[pixelnumber] = color;
  }
}

void initFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else
  {
    Serial.println("LittleFS mounted successfully");
  }
}

// Read File from LittleFS
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- frite failed");
  }
  file.close();
}

// Initialize WiFi
bool initWiFi()
{
  if (ssid == "" || ip == "")
  {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  WiFi.begin(ssid.c_str(), pass.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    if (tries++ > 15)
    {
      Serial.println("Failed to connect.");
      return false;
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect.");
    return false;
  }

  Serial.println(WiFi.localIP());
  return true;
}

void setup()
{
  delay(3000);
  attachInterrupt(digitalPinToInterrupt(14), CLK_ISR, FALLING);
  Serial.begin(115200);
  initFS();
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile(LittleFS, gatewayPath);
  FastLED.addLeds<WS2812B, LEFT_LED_PIN, GRB>(left_leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, RIGHT_LED_PIN, GRB>(right_leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
  ControlLED(RedPWM, GreenPWM, BluePWM, Brightness);
  if (initWiFi())
  {
    // if you get here you have connected to the WiFi
    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.database_url = DATABASE_URL;
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    stream.setBSSLBufferSize(1024, 256);
    if (!Firebase.beginMultiPathStream(stream, parentPath))
      Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());

    Firebase.setMultiPathStreamCallback(stream, streamCallback, streamTimeoutCallback);
  }
  else
  {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/wifimanager.html", "text/html"); });

    server.serveStatic("/", LittleFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      restart = true;
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip); });
    server.begin();
  }
}

void loop()
{
  if (restart)
  {
    delay(5000);
    ESP.restart();
  }
  if (dataChanged)
  {
    dataChanged = false;
    BluePWM = Transistor[0].toInt();
    RedPWM = Transistor[1].toInt();
    GreenPWM = Transistor[2].toInt();
    Brightness = Transistor[3].toInt();
    gPal = CRGBPalette16(CRGB::Black, CRGB(RedPWM, GreenPWM, BluePWM), CRGB::White);
  }
  switch (count)
  {
  case 3:
    fade(RedPWM, GreenPWM, BluePWM, Brightness);
    break;
  case 2:
  {
    Fire2012WithPalette(); // run simulation frame, using palette colors
    FastLED.setBrightness(Brightness);
    FastLED.show(); // display this frame
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    break;
  }
  case 4:
    Cylon(Brightness);
    break;
  case 1:
    Wave(RedPWM, GreenPWM, BluePWM, Brightness);
    break;
  default:
    ControlLED(RedPWM, GreenPWM, BluePWM, Brightness);
    break;
  }
}