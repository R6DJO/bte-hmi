#include "init.h"

// Replace with your network credentials
const char *ssid = "fresh";
const char *password = "ZAQ12wsx";

// Initialize SPIFFS
void initFS()
{
  if (!SPIFFS.begin())
  {
    Debug.println("HMI: An error has occurred while mounting SPIFFS");
  }
  else
  {
    Debug.println("HMI: SPIFFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Debug.print("HMI Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Debug.print('.');
    delay(1000);
  }
  Debug.printf("\nHMI: http://%s\n", WiFi.localIP().toString().c_str());
}