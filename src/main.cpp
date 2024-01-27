#include <Arduino.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
// #include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "modbus.h"

//
HardwareSerial &Debug = Serial;
HardwareSerial &Modbus = Serial1;
// Replace with your network credentials
const char *ssid = "fresh";
const char *password = "ZAQ12wsx";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object

AsyncWebSocket ws("/ws");
// Set LED GPIO
const int ledPin1 = 12;
const int ledPin2 = 13;
const int ledPin3 = 14;

UART_message buffer1;
UART_message *rxBuffer = &buffer1;
// char rxBuffer[32] = {0};

String message = "";
String sliderValue1 = "0";
String sliderValue2 = "0";
String sliderValue3 = "0";

int dutyCycle1;
int dutyCycle2;
int dutyCycle3;

// setting PWM properties
const int freq = 5000;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int ledChannel3 = 2;

const int resolution = 8;

// Json Variable to Hold Slider Values
// JSONVar sliderValues;
JSONVar lampConfig;

typedef struct Lamp
{
  uint8_t mode;
  uint8_t threshold;
  uint8_t current_status;
  uint8_t ambient_light;
} Lamp;

Lamp lamp = {1, 66, 0, 33};

String getData()
{
  lampConfig["mode"] = lamp.mode;
  lampConfig["threshold"] = lamp.threshold;
  lampConfig["status"] = lamp.current_status;
  lampConfig["ambient_light"] = lamp.ambient_light;
  String jsonString = JSON.stringify(lampConfig);
  return jsonString;
}

// Initialize SPIFFS
void initFS()
{
  if (!SPIFFS.begin())
  {
    Debug.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Debug.println("SPIFFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Debug.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Debug.print('.');
    delay(1000);
  }
  Debug.printf("\nhttp://%s\n", WiFi.localIP().toString().c_str());
  // Debug.println(WiFi.localIP());
}

void notifyClients(String Message)
{
  Debug.printf("-->%s\n", Message.c_str());
  // Debug.println(Message);
  ws.textAll(Message);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len &&
      info->opcode == WS_TEXT)
  {
    data[len] = 0;
    message = (char *)data;
    if (message.indexOf("threshold") >= 0)
    {
      uint8_t value = message.substring(10).toInt();
      Debug.printf("<--threshold=%u\n", value);
      // Debug.println(value);
      lamp.threshold = value;
      notifyClients(getData());
    }
    if (message.indexOf("mode") >= 0)
    {
      uint8_t value = message.substring(5).toInt();
      Debug.printf("<--mode=%d\n", value);
      // Debug.println(value);
      lamp.mode = value;
      notifyClients(getData());
    }
    if (strcmp((char *)data, "getValues") == 0)
    {
      Debug.printf("<--getValues\n");
      notifyClients(getData());
    }
    if (strcmp((char *)data, "lampSwitch") == 0)
    {
      Debug.println("<--lampSwitch");
      lamp.current_status = (!lamp.current_status);
      notifyClients(getData());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Debug.printf("WebSocket client #%u connected from %s\n", client->id(),
                 client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Debug.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// General callback function for any UART -- used with a lambda std::function within HardwareSerial::onReceive()
void processOnReceiving(HardwareSerial &Serial)
{
  // detects which Serial# is being used here
  int8_t uart_num = -1;
  if (&Serial == &Debug)
  {
    uart_num = 0;
  }
  else if (&Serial == &Modbus)
  {
    uart_num = 1;
  }

  // Prints some information on the current Serial (UART0 or USB CDC)
  if (uart_num == -1)
  {
    Debug.println("This is not a know Arduino Serial# object...");
    return;
  }
  Debug.printf("\nOnReceive Callback --> Received Data from UART%d\n"
               "Received %d bytes\nFirst byte is '%c' [0x%02x]\n",
               uart_num, Serial.available(), Serial.peek(), Serial.peek());
  uint8_t charPerLine = 0;
  rxBuffer->msg_length = Serial.read(rxBuffer->msg_data, 31);
  rxBuffer->msg_data[rxBuffer->msg_length] = '\0';
  Debug.println((char *)rxBuffer->msg_data);

  modbus_status_t status = msg_validate(rxBuffer);
  if (status == MB_OK)
  {
    Debug.println("MSG valid");
  }
  else
  {
    Debug.println("MSG invalid");
  }
  // while (Serial.available())
  // {
  //   char c = Serial.read();
  //   Debug.printf("'%c' [0x%02x] ", c, c);
  //   if (++charPerLine == 10)
  //   {
  //     charPerLine = 0;
  //     Debug.println();
  //   }
  // }
}

void setup()
{
  Debug.begin(115200);
  Debug.onReceive([]()
                  { processOnReceiving(Debug); });
  Modbus.begin(9600);
  Modbus.onReceive([]()
                   { processOnReceiving(Modbus); });
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  initFS();
  initWiFi();

  // configure LED PWM functionalitites
  ledcSetup(ledChannel1, freq, resolution);
  ledcSetup(ledChannel2, freq, resolution);
  ledcSetup(ledChannel3, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPin1, ledChannel1);
  ledcAttachPin(ledPin2, ledChannel2);
  ledcAttachPin(ledPin3, ledChannel3);

  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();
}

void loop()
{
  ledcWrite(ledChannel1, dutyCycle1);
  ledcWrite(ledChannel2, dutyCycle2);
  ledcWrite(ledChannel3, dutyCycle3);

  ws.cleanupClients();
}
