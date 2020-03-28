#define DEBUG_NTPClient

#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>
#include <WiFi.h>
#include <NTPClient.h>
#include "secrets.h"
#include "soc/rtc_wdt.h"

LSM9DS1 imu;

WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void readAccelTask(void *);
void socketReceiveTask(void *);
void ntpTask(void *);
void sendBufferTask(void *);
void sendBuffer(int16_t *, size_t);
void accelInterruptHandler();

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

  pinMode(12, OUTPUT);

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

  xTaskCreatePinnedToCore(readAccelTask, "readAccelTask", 20000, NULL, 10, NULL, 0);
  xTaskCreatePinnedToCore(socketReceiveTask, "socketReceiveTask", 20000, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(sendBufferTask, "sendBufferTask", 20000, NULL, 3, NULL, 1);
  xTaskCreate(ntpTask, "ntpTask", 20000, NULL, 2, NULL);
}

#define BUFFER_SIZE 1000 * 9

int16_t buffer1[BUFFER_SIZE];
int16_t buffer2[BUFFER_SIZE];
int16_t *buffer = buffer1;
int16_t *send_buffer;

bool newDataToSend = false;

int i = 0;

// 12 bytes + 3*2 bytes = 18 bytes = 9 int16_ts

void readAccelTask(void *)
{
  while (true)
  {
    if (i >= BUFFER_SIZE)
    {
      if (buffer == buffer1)
      {
        send_buffer = buffer1;
        buffer = buffer2;

        printf("Buffer changed to buffer2!\n");
      }
      else
      {
        send_buffer = buffer2;
        buffer = buffer1;

        printf("Buffer changed to buffer1!\n");
      }

      i = 0;
      newDataToSend = true;
    }

    if (imu.accelAvailable() && i < BUFFER_SIZE)
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

void sendBufferTask(void *)
{
  while (true)
  {
    if (newDataToSend)
    {
      int bytesSent = client.write((uint8_t *)send_buffer, BUFFER_SIZE * sizeof(uint16_t));

      printf("Sent %d bytes to the host!\n", bytesSent);

      newDataToSend = false;
    }

    vTaskDelay(250);
  }
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
      printf("disconnected\n");

      ESP.restart();
    }

    vTaskDelay(1000);
  }
}

void loop()
{
  vTaskDelay(2000);
}