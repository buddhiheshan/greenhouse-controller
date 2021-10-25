#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DEBUG 1
#ifdef DEBUG
#define PRINT Serial.printf
#else
#define PRINT(...)
#endif

#define MSG_BUFFER_SIZE (25)

// Wifi ssid and password
const char *ssid = "Dialog 4G 255";
const char *password = "Dialog4G311/13";

// MQTT broker
const char *mqtt_server = "broker.hivemq.com";

// Subcribing topics
// implementation topics
const char *sensor_readings_COM = "CO326/2021/GH/1/demo/sensor_readings_COM";
const char *thresholds = "CO326/2021/GH/1/demo/thresholds";

//  Publishing topics
// demo topics
const char *control_signals_COM = "CO326/2021/GH/1/demo/control_signals_COM";
const char *control_signals_SCADA = "CO326/2021/GH/1/demo/control_signals_SCADA";
const char *sensor_readings_SCADA = "CO326/2021/GH/1/demo/sensor_readings_SCADA";

// implementation topics
const char *temp_topic = "CO326/2021/GH/1/temp";
const char *soil_moist_topic = "CO326/2021/GH/1/soil_moisture";
const char *humidity_topic = "CO326/2021/GH/1/humidity";
const char *light_intensity_topic = "CO326/2021/GH/1/light_intensity";
const char *water_tank_topic = "CO326/2021/GH/1/water_tank_level";
const char *fertilizer_tank_topic = "CO326/2021/GH/1/fertilizer_tank_level";

const char *fan_topic = "CO326/2021/GH/1/fan";
const char *light_topic = "CO326/2021/GH/1/lights";

const char *water_supply_topic = "CO326/2021/GH/1/water_supply";
const char *fertilizer_low_topic = "CO326/2021/GH/1/fertilizer_low_level";

// threshold values
int temperature_limit1 = 100;
int temperature_limit2 = 100;
int temperature_limit3 = 100;
int soil_moisture_limit = 50;
int humidity_limit = 50;
int light_intensity_limit1 = 50;
int light_intensity_limit2 = 50;
int light_intensity_limit3 = 50;
int fertilizer_value_open_time = 0;
int fertilizer_interval = 0;

// sensor readings
int temperature = 0;
int soil_moisture = 100;
int humidity = 100;
int light_intensity = 100;
char fertilizer_tank[2] = "M";
char water_tank[2] = "M";

char msg[MSG_BUFFER_SIZE];

WiFiClient espClient;
PubSubClient client(espClient);
TaskHandle_t Task1;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

// on MQTT message
void callback(char *topic, byte *message, unsigned int length)
{
  // Serial.print("Message arrived on topic: ");
  // Serial.print(topic);
  // Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    // Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  char buf[messageTemp.length()];
  messageTemp.toCharArray(buf, sizeof(buf) + 1);

  if (!strcmp(topic, thresholds))
  {
    sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &temperature_limit1, &temperature_limit2, &temperature_limit3, &soil_moisture_limit, &humidity_limit, &light_intensity_limit1, &light_intensity_limit2, &light_intensity_limit3, &fertilizer_value_open_time, &fertilizer_interval);
    PRINT("Limits updated Temp=%d,%d,%d\tSoil_moist=%d\tHumid=%d\tLight intensity=%d,%d,%d\tFertilizer value open time=%d\tFertilizer interval=%d\n", temperature_limit1, temperature_limit2, temperature_limit3, soil_moisture_limit, humidity_limit, light_intensity_limit1, light_intensity_limit2, light_intensity_limit3, fertilizer_value_open_time, fertilizer_interval);
  }

  if (!strcmp(topic, sensor_readings_COM))
  {
    sscanf(buf, "%d,%d,%d,%d,%c,%c", &temperature, &soil_moisture, &humidity, &light_intensity, water_tank, fertilizer_tank);
    PRINT("Readings updated Temp = %d\tSoil_moist = %d\tHumid = %d\tLight intensity = %d\tWater tank = %s\tFertilizer tank = %s\n", temperature, soil_moisture, humidity, light_intensity, water_tank, fertilizer_tank);

    itoa(temperature, msg, 10);
    client.publish(temp_topic, msg);
    itoa(soil_moisture, msg, 10);
    client.publish(soil_moist_topic, msg);
    itoa(humidity, msg, 10);
    client.publish(humidity_topic, msg);
    itoa(light_intensity, msg, 10);
    client.publish(light_intensity_topic, msg);
    client.publish(water_tank_topic, water_tank);
    client.publish(fertilizer_tank_topic, fertilizer_tank);

    client.publish(sensor_readings_SCADA, buf, 2);
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Greenhouse Controller"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe(sensor_readings_COM);
      client.subscribe(thresholds);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void Task1code(void *parameter)
{
  for (;;)
  {
    if (!client.connected())
    {
      reconnect();
    }
    // client.loop();
  }
}

// the setup function runs once when you press reset or power the board
void setup()
{

  Serial.begin(115200);

  // delete old config
  WiFi.disconnect(true);

  delay(3000);

  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println();
  Serial.println("Wait for WiFi... ");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  xTaskCreatePinnedToCore(
      Task1code, /* Function to implement the task */
      "Task1",   /* Name of the task */
      10000,     /* Stack size in words */
      NULL,      /* Task input parameter */
      0,         /* Priority of the task */
      &Task1,    /* Task handle. */
      0);        /* Core */
}

int modified_COM_signals = 0;
int modified_SCADA_signals = 0;

int fertilizer = 0;

int water_supply = 0;
int fertilizer_low = 0;
int watering = 0;
int fan = 0;
int humidifier = 0;
int light = 0;

// the loop function runs over and over again forever
void loop()
{
  // water supply
  if (!water_supply && !strcmp(water_tank, "L"))
  {
    water_supply = 1;
    modified_SCADA_signals = 1;
    itoa(water_supply, msg, 10);
    client.publish(water_supply_topic, msg, 2);
    PRINT("Open water input\t\twater_supply = %d\n", water_supply);
  }
  if (water_supply && !strcmp(water_tank, "H"))
  {
    water_supply = 0;
    modified_SCADA_signals = 1;
    itoa(water_supply, msg, 10);
    client.publish(water_supply_topic, msg, 2);
    PRINT("Close water input\t\twater_supply = %d\n", water_supply);
  }

  // fertilizer low
  if (!fertilizer_low && !strcmp(fertilizer_tank, "L"))
  {
    fertilizer_low = 1;
    PRINT("Fertilizer Low\t\t fertilizer_low = %d\n", fertilizer_low);
    itoa(fertilizer_low, msg, 10);
    client.publish(fertilizer_low_topic, msg, 2);
  }
  if (fertilizer_low && !strcmp(fertilizer_tank, "M"))
  {
    fertilizer_low = 0;
    PRINT("Fertilizer Medium\t\t fertilizer_low = %d\n", fertilizer_low);
    itoa(fertilizer_low, msg, 10);
    client.publish(fertilizer_low_topic, msg, 2);
  }

  // watering plants
  if (!watering && (soil_moisture < soil_moisture_limit))
  {
    watering = 1;
    PRINT("Start watering plants\t\t watering = %d\n", watering);
    modified_COM_signals = 1;
  }
  if (watering && (soil_moisture > soil_moisture_limit))
  {
    watering = 0;
    PRINT("Stop watering plants\t\t watering = %d\n", watering);
    modified_COM_signals = 1;
  }

  // fan control
  if (fan != 0 && (temperature < temperature_limit1))
  {
    fan = 0;
    PRINT("Stop fans\t\t fan = %d\n", fan);
    modified_COM_signals = 1;
  }
  if (fan != 1 && (temperature >= temperature_limit1) && (temperature < temperature_limit2))
  {
    fan = 1;
    PRINT("Start fans\t\t fan = %d\n", fan);
    modified_COM_signals = 1;
  }
  if (fan != 2 && (temperature >= temperature_limit2) && (temperature < temperature_limit3))
  {
    fan = 2;
    PRINT("Start fans\t\t fan = %d\n", fan);
    modified_COM_signals = 1;
  }
  if (fan != 3 && (temperature >= temperature_limit3))
  {
    fan = 3;
    PRINT("Start fans\t\t fan = %d\n", fan);
    modified_COM_signals = 1;
  }

  // humidifier control
  if (!humidifier && (humidity < humidity_limit))
  {
    humidifier = 1;
    PRINT("Start humidifier\t\t humidifier = %d\n", humidifier);
    modified_COM_signals = 1;
  }
  if (humidifier && (humidity > humidity_limit))
  {
    humidifier = 0;
    PRINT("Stop humidifier\t\t humidifier = %d\n", humidifier);
    modified_COM_signals = 1;
  }

  // lights control
  if (light != 3 && (light_intensity < light_intensity_limit1))
  {
    light = 3;
    PRINT("Start lights\t\t light = %d\n", light);
    modified_COM_signals = 1;
  }
  if (light != 2 && (light_intensity >= light_intensity_limit1) && (light_intensity < light_intensity_limit2))
  {
    light = 2;
    PRINT("Start lights\t\t light = %d\n", light);
    modified_COM_signals = 1;
  }
  if (light != 1 && (light_intensity >= light_intensity_limit2) && (light_intensity < light_intensity_limit3))
  {
    light = 1;
    PRINT("Start lights\t\t light = %d\n", light);
    modified_COM_signals = 1;
  }
  if (light != 0 && (light_intensity >= light_intensity_limit3))
  {
    light = 0;
    PRINT("Stop lights\t\t light = %d\n", light);
    modified_COM_signals = 1;
  }

  // send control data to SCADA
  if ((modified_COM_signals || modified_SCADA_signals) && client.connected())
  {
    snprintf(msg, MSG_BUFFER_SIZE, "%d,%d,%d,%d,%d,%d", fan, watering, fertilizer, humidifier, light, water_supply);
    client.publish(control_signals_SCADA, msg, 2);

    itoa(fan, msg, 10);
    client.publish(fan_topic, msg, 2);
    itoa(light, msg, 10);
    client.publish(light_topic, msg, 2);

    PRINT("Published to SCADA\n");
    modified_SCADA_signals = 0;
  }

  //send control data to com unit
  if (modified_COM_signals && client.connected())
  {
    snprintf(msg, MSG_BUFFER_SIZE, "%d,%d,%d,%d,%d", fan, watering, fertilizer, humidifier, light);
    client.publish(control_signals_COM, msg, 2);
    PRINT("Published to COM\n");
    modified_COM_signals = 0;
  }

  client.loop();

  delay(10);
}