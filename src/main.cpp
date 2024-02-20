// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#define DEBUG

#include "main.h"
#include "init.h"

// tmp-string
char tmp_string[256];

// UART objects
HardwareSerial &debug = Serial;
HardwareSerial &modbus = Serial2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Buffer for RX/TX msg
UART_message uart_buffer1;
UART_message uart_buffer2;
UART_message *rxBuffer = &uart_buffer1;
UART_message *txBuffer = &uart_buffer1;

// Buffer for JSON from HMI to Web-client
char buffer[64];
char *wsJSON = buffer;

// Modbus MSG recived flag
volatile uint8_t mb_msg_rxd = false;
MODBUS_message rx_response;
MODBUS_message mb_request;
MODBUS_registers LCU_registers;
// for info
// LCU_registers.MB_address = 0x01;
// DO[0].0 - mode 1-automatic/0-manual
// DO[1].0 - light 1-on/0-off
// AI[0] - current light level
// AO[1] - light threshold
// LCU_registers.DO_start_address = 0x0100;
// LCU_registers.DI_start_address = 0x0200;
// LCU_registers.AO_start_address = 0x0300;
// LCU_registers.AI_start_address = 0x0400;

// Lamp data
typedef struct Lamp
{
    uint8_t mode;
    uint8_t threshold;
    uint8_t current_status;
    uint8_t ambient_light;
} Lamp;

Lamp lamp = {1, 66, 0, 33};

void mb_buf_print(UART_message *msg);
void periodicUpdateSensors(void);
void requestAmbientLightValue(void);
void requestCurrentLampStatus(void);
void setLampMode(Lamp *lamp, MODBUS_registers *registers);
void requestCurrentLampMode(void);
void setLampTreshold(uint16_t threshold);
void update_lamp(MODBUS_registers *LCU_registers, Lamp *lamp);
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
            debug.printf("HMI<--USR threshold=%u\n", value);
            lamp.threshold = value;
            generateJSON();
            ws.textAll(wsJSON);
            setLampTreshold(lamp.threshold);
        }
        if (strncmp((const char *)data, "mode", 4) == 0)
        {
            uint8_t value = atoi((const char *)data + 5);
            debug.printf("HMI<--Usr mode=%d\n", value);
            switch (value)
            {
            case 0:
                lamp.mode = 0;
                lamp.current_status = 0;
                break;
            case 1:
                lamp.mode = 1;
                break;
            case 2:
                lamp.mode = 0;
                lamp.current_status = 1;
                break;
            }
            generateJSON();
            ws.textAll(wsJSON);
            setLampMode(&lamp, &LCU_registers);
        }
        if (strcmp((char *)data, "getValues") == 0)
        {
            debug.printf("HMI<--Usr getValues\n");
            generateJSON();
            ws.textAll(wsJSON);
        }
        if (strcmp((char *)data, "lampSwitch") == 0)
        {
            debug.println("HMI<--Usr lampSwitch");
        }
    }
}

void WSonEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        debug.printf("HMI: WebSocket client #%u connected from %s\n", client->id(),
                     client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        debug.printf("HMI: WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

// General callback function for any UART
// Used with a lambda std::function within HardwareSerial::onReceive()
void processOnReceiving(HardwareSerial &Serial)
{
    // detects which Serial# is being used here
    int8_t uart_num = -1;
    if (&Serial == &debug)
    {
        uart_num = 0;
    }
    else if (&Serial == &modbus)
    {
        uart_num = 1;
    }

    // Prints some information on the current Serial (UART0 or USB CDC)
    if (uart_num == -1)
    {
        debug.println("HMI: This is not a know Arduino Serial# object...");
        return;
    }
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
    debug.print("[ ");
    for (uint8_t i = 0; i < msg->msg_length; i++)
    {
        debug.printf("0x%02X ", msg->msg_data[i]);
    }
    debug.println("]");
}

void debug_print(const char *msg)
{
    unsigned long time = millis();
    unsigned long min = (time / 1000) / 60;
    unsigned long sec = (time / 1000) % 60;
    unsigned long dsec = (time) % 1000;
    debug.printf("%s at %d:%02d.%03d\n", msg, min, sec, dsec);
}

void hex_to_string(uint8_t *buffer, uint8_t size, char *result)
{
    sprintf(result, "[ ");
    for (int i = 0; i < size; i++)
    {
        sprintf(result + i * 5 + 2, "0x%02X ", buffer[i]);
    }
    strcat(result, "]");
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
    debug.begin(115200);
    debug.onReceive([]()
                    { processOnReceiving(debug); });
    modbus.begin(9600);
    modbus.onReceive([]()
                     { processOnReceiving(modbus); },
                     true);

    initFS();
    initWiFi();
    initWebServer();

    LCU_registers.MB_address = 0x01;
    LCU_registers.DO_start_address = 0x0100;
    LCU_registers.DI_start_address = 0x0200;
    LCU_registers.AO_start_address = 0x0300;
    LCU_registers.AI_start_address = 0x0400;
}

void loop()
{
    if (mb_msg_rxd)
    {
        mb_msg_rxd = false;
        if (msg_validate(rxBuffer))
        {
            msg_parse_from_slave(rxBuffer, &rx_response);
            response_processing(&rx_response, &mb_request, &LCU_registers);
            update_lamp(&LCU_registers, &lamp);
            generateJSON();
            ws.textAll(wsJSON);
        }
    }
    periodicUpdateSensors();
    ws.cleanupClients();
}

void periodicUpdateSensors(void)
{
    // опрос регистров по очереди с паузами 1500 - 500 - 1500 ms...
    static uint64_t previousMillis = 0;
    static uint8_t queue = 0;
    uint64_t interval = 1500;
    uint64_t step = 500;
    uint64_t currentMillis = millis();
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
        }
        queue++;
        if (queue > 1)
        {
            queue = 0;
        }
        previousMillis = currentMillis;
    }
}

void requestAmbientLightValue(void)
{
#ifdef DEBUG
    debug_print("HMI: Request ambient light");
#endif
    mb_request.device_address = 0x01;
    mb_request.command = READ_AI;
    mb_request.start_address = 0x0400;
    mb_request.data_length = 0x0001;
    prepare_request_mbmsg(&mb_request, txBuffer);
#ifdef DEBUG
    mb_buf_print(txBuffer);
#endif
    modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

void requestCurrentLampStatus(void)
{
#ifdef DEBUG
    debug_print("HMI: Request current lamp status");
#endif
    mb_request.device_address = 0x01;
    mb_request.command = READ_DO;
    mb_request.start_address = 0x0100;
    mb_request.data_length = 0x0002;
    prepare_request_mbmsg(&mb_request, txBuffer);
#ifdef DEBUG
    mb_buf_print(txBuffer);
#endif
    modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

void setLampMode(Lamp *lamp, MODBUS_registers *registers)
{
#ifdef DEBUG
    debug_print("HMI: Set light mode");
#endif
    registers->DO[0] = lamp->mode;
    registers->DO[1] = lamp->current_status;
    mb_request.device_address = 0x01;
    mb_request.command = WRITE_DO_MULTI;
    mb_request.start_address = 0x0100;
    mb_request.data_length = 0x0002;
    mb_request.byte_count = 0x04;
    mb_request.data[0] = lamp->mode & 0x01;
    mb_request.data[1] = lamp->current_status & 0x01;
    prepare_request_mbmsg(&mb_request, txBuffer);
#ifdef DEBUG
    mb_buf_print(txBuffer);
#endif
    modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

void setLampTreshold(uint16_t threshold)
{
#ifdef DEBUG
    debug_print("HMI: Set light threshold");
#endif
    mb_request.device_address = 0x01;
    mb_request.command = WRITE_AO;
    mb_request.start_address = 0x0301;
    mb_request.data_length = 0x0001;
    mb_request.data[0] = threshold * 655;
    prepare_request_mbmsg(&mb_request, txBuffer);
#ifdef DEBUG
    mb_buf_print(txBuffer);
#endif
    modbus.write(txBuffer->msg_data, txBuffer->msg_length);
}

void update_lamp(MODBUS_registers *registers, Lamp *lamp)
{
    lamp->ambient_light = registers->AI[0] / 655;
    lamp->mode = registers->DO[0];
    lamp->current_status = registers->DO[1];
}
