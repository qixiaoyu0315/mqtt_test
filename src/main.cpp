#include <Arduino.h>

#include <WiFi.h>
#include <FastLED.h>
#include <PubSubClient.h>

const char *ssid = "STYL";
const char *password = "styl888777";

const char *mqttServer = "172.16.23.77";
const int mqttPort = 1883;
const char *mqttUser = "esp32s3";
const char *mqttPassword = "123456";
const char *topic = "testtopic/#";

#define NUM_LEDS 8
#define DATA_PIN 4
#define BRIGHTNESS 30

CRGB leds[NUM_LEDS];
uint8_t pos = 0;
bool toggle = false;
bool colorModified = false;
CRGB customColor;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword))
    {
      Serial.println("connected");
      client.subscribe(topic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void blur_x()
{
  if (!colorModified)
  {
    leds[pos] = CHSV(pos * 2, 255, 255);
  }
  else
  {
    leds[pos] = customColor;
  }
  blur1d(leds, NUM_LEDS, 172);
  fadeToBlackBy(leds, NUM_LEDS, 16);
  FastLED.show();
  if (toggle)
  {
    pos = (pos + 1) % NUM_LEDS;
  }
  toggle = !toggle;
  delay(20);
}

void parseRGB(const char *rgbStr, uint8_t &r, uint8_t &g, uint8_t &b)
{
  sscanf(rgbStr, "rgb(%hhu, %hhu, %hhu)", &r, &g, &b);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, "testtopic/3") == 0)
  {
    if ((char)payload[0] == '1')
    {
      digitalWrite(3, HIGH);
    }
    else
    {
      digitalWrite(3, LOW);
    }
  }
  else if (strcmp(topic, "testtopic/2") == 0)
  {
    char payloadStr[length + 1];
    strncpy(payloadStr, (const char *)payload, length);
    payloadStr[length] = '\0';

    uint8_t r, g, b;
    parseRGB(payloadStr, r, g, b);

    customColor = CRGB(r, g, b);
    colorModified = true;

    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = customColor;
    }
    FastLED.show();
  }
  else if (strcmp(topic, "testtopic/1") == 0)
  {
    char brightnessStr[10];
    strncpy(brightnessStr, (const char *)payload, length);
    brightnessStr[length] = '\0';

    int brightness = atoi(brightnessStr);
    if (brightness >= 0 && brightness <= 255)
    {
      FastLED.setBrightness(brightness);
    }
  }
}

void setup()
{
  pinMode(3, OUTPUT);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  blur_x();
}