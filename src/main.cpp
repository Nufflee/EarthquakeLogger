#define DEBUG_NTPClient

#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>
#include <WiFi.h>
#include <NTPClient.h>
#include "secrets.h"
#include "soc/rtc_wdt.h"

/*
1. auto reconnect
*/

LSM9DS1 imu;

WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void readAccelTask(void *);
void socketReceiveTask(void *);
void ntpTask(void *);
void sendBufferTask(void *);
void sendBuffer(int16_t *, size_t);

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  if (imu.begin() == false)
  {
    Serial.println("Failed to communicate with LSM9DS1.");
    Serial.println("Double-check wiring.");
    Serial.println("Default settings in this sketch will "
                   "work for an out of the box LSM9DS1 "
                   "Breakout, but may need to be modified "
                   "if the board jumpers are.");
    while (1)
      ;
  }

  imu.configInt(XG_INT1, INT_DRDY_XL, INT_ACTIVE_LOW, INT_PUSH_PULL);

  pinMode(12, OUTPUT);
  pinMode(13, INPUT_PULLUP);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("...");
  }

  if (!client.connect("192.168.1.8", 6969))
  {
    Serial.println("Connection to host failed!");

    ESP.restart();

    return;
  }

  timeClient.begin();

  timeClient.forceUpdate();

  printf("Connected to WiFi with IP %s!\n", WiFi.localIP().toString().c_str());
  printf("Connected to host!\n");

  imu.calibrate(true);

  xTaskCreate(readAccelTask, "readAccelTask", 20000, NULL, 5, NULL);
  xTaskCreate(socketReceiveTask, "socketReceiveTask", 20000, NULL, 3, NULL);
  xTaskCreate(sendBufferTask, "sendBufferTask", 20000, NULL, 3, NULL);
  xTaskCreate(ntpTask, "ntpTask", 20000, NULL, 2, NULL);
}

int16_t buffer[1000 * 9];
int i = 0;

// 12 bytes + 3*2 bytes = 18 bytes = 9 int16_ts

void readAccelTask(void *)
{
  int last_time = millis();

  while (true)
  {
    if (imu.accelAvailable())
    {
      imu.readAccel();

      String time = timeClient.getFormattedTime();

      memcpy(buffer + i, (uint8_t *)time.c_str(), 12);
      i += 6;
      buffer[i++] = imu.ax;
      buffer[i++] = imu.ay;
      buffer[i++] = imu.az;
    }

    vTaskDelay(1);
  }
}

// 7:13 - quake

int16_t *send_buffer[1000 * 9];

void sendBufferTask(void *)
{
  while (true)
  {
    if (i + 1 >= 9000)
    {
      memcpy(send_buffer, buffer, sizeof(buffer));

      i = 0;

      int bytesSent = client.write((uint8_t *)send_buffer, sizeof(buffer));

      memset(send_buffer, 0, sizeof(buffer));

      printf("\nSent %d bytes to the host!\n", bytesSent);
    }

    vTaskDelay(1);
  }

  /*
  for (int i = 0; i < 3000; i += 3)
  {
    printf("x = %d, y = %d, z = %d\n", buffer[i], buffer[i + 1], buffer[i + 2]);
  }
  */
}

void ntpTask(void *)
{
  while (true)
  {
    timeClient.forceUpdate();

    vTaskDelay(70000);
  }
}

void socketReceiveTask(void *)
{
  while (true)
  {
    if (!client.connected() || !WiFi.isConnected())
    {
      ESP.restart();
    }

    String recieved = client.readStringUntil('\n');

    if (recieved == "restart")
    {
      ESP.restart();
    }

    vTaskDelay(1);
  }
}

void loop()
{
}