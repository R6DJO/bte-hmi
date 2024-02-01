#define DEBUG

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "init.h"
#include "modbus.h"

void mb_buf_print(UART_message *msg);
void periodicUpdateSensors(void);
void requestAmbientLightValue(void);
void requestCurrentLampStatus(void);
void requestCurrentLampMode(void);

// UART objects
HardwareSerial &Debug = Serial;
HardwareSerial &Modbus = Serial1;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Buffer for RX/TX msg
UART_message buffer1;
UART_message buffer2;
UART_message *rxBuffer = &buffer1;
UART_message *txBuffer = &buffer1;

char buffer3[64];
char *wsJSON = buffer3;

// Modbus MSG recived flag
volatile uint8_t mb_msg_rxd = false;
MODBUS_message rx_msg;
MODBUS_message mb_request;
MODBUS_registers LCU_registers;
// LCU_registers.MB_address = 0x01;
// AI[0] - current light level
// AO[1] - light threshold
// DO[0].0 - mode 1-automatic/0-manual
// DO[1].0 - light 1-on/0-off

// Lamp data
typedef struct Lamp
{
  uint8_t mode;
  uint8_t threshold;
  uint8_t current_status;
  uint8_t ambient_light;
} Lamp;

Lamp lamp = {1, 66, 0, 33};

void generateJSON()
{
  sprintf(wsJSON, "{\"mode\":%d,\"threshold\":%d,\"status\":%d,\"ambient_light\":%d}", lamp.mode, lamp.threshold, lamp.current_status, lamp.ambient_light);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len &&
      info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strncmp((const char *)data, "threshold", 9) == 0)
    {
      uint8_t value = atoi((const char *)data + 10);
      Debug.printf("HMI<--USR threshold=%u\n", value);
      lamp.threshold = value;
      generateJSON();
      ws.textAll(wsJSON);
    }
    if (strncmp((const char *)data, "mode", 4) == 0)
    {
      uint8_t value = atoi((const char *)data + 5);
      Debug.printf("HMI<--Usr mode=%d\n", value);
      lamp.mode = value;
      generateJSON();
      ws.textAll(wsJSON);
    }
    if (strcmp((char *)data, "getValues") == 0)
    {
      Debug.printf("HMI<--Usr getValues\n");
      generateJSON();
      ws.textAll(wsJSON);
    }
    if (strcmp((char *)data, "lampSwitch") == 0)
    {
      Debug.println("HMI<--Usr lampSwitch");
      lamp.current_status = (!lamp.current_status);
      generateJSON();
      ws.textAll(wsJSON);
    }
  }
}

void WSonEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Debug.printf("HMI: WebSocket client #%u connected from %s\n", client->id(),
                 client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Debug.printf("HMI: WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
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
    Debug.println("HMI: This is not a know Arduino Serial# object...");
    return;
  }
#ifdef DEBUG
  Debug.printf("\nHMI: OnReceive Callback --> Received Data from UART%d\n"
               "Received %d bytes\nFirst byte is '%c' [0x%02x]\n",
               uart_num, Serial.available(), Serial.peek(), Serial.peek());
#endif
  uint8_t charPerLine = 0;
  rxBuffer->msg_length = Serial.read(rxBuffer->msg_data, 31);
  rxBuffer->msg_data[rxBuffer->msg_length] = '\0';
#ifdef DEBUG
  mb_buf_print(rxBuffer);
#endif
  if (uart_num == 1)
  {
    mb_msg_rxd = true;
  }
}

void mb_buf_print(UART_message *msg)
{
  modbus_status_t status = msg_validate(txBuffer);
  if (status == MB_OK)
  {
    Debug.println("HMI: Message valid");
  }
  else
  {
    Debug.println("HMI: Message ivalid");
  }
  for (uint8_t i = 0; i < txBuffer->msg_length; i++)
  {
    Debug.printf("[0x%02X] ", txBuffer->msg_data[i]);
  }
  Debug.println();
}

void initWebServer()
{
  ws.onEvent(WSonEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
}

void setup()
{
  // Add callback for UART RX
  Debug.begin(115200);
  Debug.onReceive([]()
                  { processOnReceiving(Debug); });
  Modbus.begin(9600);
  Modbus.onReceive([]()
                   { processOnReceiving(Modbus); });

  initFS();
  initWiFi();
  initWebServer();

  LCU_registers.MB_address = 0x01;
}

void loop()
{
  if (mb_msg_rxd)
  {
    mb_msg_rxd = false;
    if (msg_validate(rxBuffer))
    {
      msg_parse(rxBuffer, &rx_msg);
      response_processing(&rx_msg, &mb_request, &LCU_registers);
    }
  }
  periodicUpdateSensors();
  ws.cleanupClients();
}

void periodicUpdateSensors(void)
{
  // опрос регистров по очереди с паузами 3000 - 500 - 500 - 3000...
  static unsigned long previousMillis = 0;
  static uint8_t queue = 0;
  unsigned long interval = 3000;
  unsigned long step = 500;
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis >= interval && queue == 0) || (currentMillis - previousMillis >= step && queue != 0))
  {
    switch (queue)
    {
    case 0:
      requestAmbientLightValue();
      break;
    case 1:
      requestCurrentLampStatus();
      break;
    case 2:
      requestCurrentLampMode();
      break;
    }
    queue++;
    if (queue > 2)
    {
      queue = 0;
    }
    previousMillis = currentMillis;
  }
}

void requestAmbientLightValue(void)
{
#ifdef DEBUG
  Debug.printf("Request ambient light at:       %u\n", millis());
#endif
  mb_request.device_address = 0x01;
  mb_request.command = 0x01;
  mb_request.start_address = 0x0100;
  mb_request.data_length = 0x0001;
  prepare_request_registers(&mb_request, txBuffer);
#ifdef DEBUG
  mb_buf_print(txBuffer);
#endif
  Modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

void requestCurrentLampStatus(void)
{
#ifdef DEBUG
  Debug.printf("Request current light status at:%u\n", millis());
#endif
  mb_request.device_address = 0x01;
  mb_request.command = 0x02;
  mb_request.start_address = 0x0100;
  mb_request.data_length = 0x0001;
  prepare_request_registers(&mb_request, txBuffer);
#ifdef DEBUG
  mb_buf_print(txBuffer);
#endif
  Modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

void requestCurrentLampMode(void)
{
#ifdef DEBUG
  Debug.printf("Request current light mode at:  %u\n", millis());
#endif
  mb_request.device_address = 0x01;
  mb_request.command = 0x03;
  mb_request.start_address = 0x0100;
  mb_request.data_length = 0x0001;
  prepare_request_registers(&mb_request, txBuffer);
#ifdef DEBUG
  mb_buf_print(txBuffer);
#endif
  Modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

// void response_processing(MODBUS_message *response, MODBUS_registers *LCU_registers)
// {
//   // AI[0] - current light level
//   // AO[1] - light threshold
//   // DO[0].0 - mode 1-automatic/0-manual
//   // DO[1].0 - light 1-on/0-off
//   ;
// }