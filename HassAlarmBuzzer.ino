#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#define ALARM_PAYLOAD_CONFIG "{\"avty_t\":\"%s\",\"cmd_t\":\"%s\",\"name\":\"%s\",\"pl_on\":\"%s\"}"
#define PIN_BUZZER D2
#define TONE_KHZ 1100
#define PITCH_LOW 200
#define PITCH_HIGH 1000
#define PANIC_ALARM_DURATION 15000

WiFiClient client;
PubSubClient mqtt(mqtt_server, 1883, callback, client);

char ssid[] = "ssid";
char pass[] = "password";
char mqtt_server[] = "homeassistant.local";
char mqtt_user[] = "mqttuser";
char mqtt_pass[] = "password";

char alarm_topic_cmd[] = "homeassistant/scene/esp8266/cmd";
char alarm_topic_available[] = "homeassistant/scene/esp8266/available";
char alarm_topic_config_armed[] = "homeassistant/scene/esp8266-armed/config";
char alarm_topic_config_disarmed[] = "homeassistant/scene/esp8266-disarmed/config";
char alarm_topic_config_panic[] = "homeassistant/scene/esp8266-panic/config";
bool flag_armed = false;
bool flag_disarmed = false;
bool flag_panic = false;
int pitchStep = 10;
int currentPitch = PITCH_LOW;
unsigned long panicMillis = 0;

void callback(char* topic, byte* payload, unsigned int length);
void reconnect_mqtt();

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);

  WiFi.begin(ssid, pass);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP.restart();
  }
  
  reconnect_mqtt();
}

void loop() {
  if (!mqtt.loop()) {
    reconnect_mqtt();  
  }

  if (flag_armed) {
    flag_armed = false;
    tone(PIN_BUZZER, TONE_KHZ);
    delay(100);
    noTone(PIN_BUZZER);
  } else if (flag_disarmed) {
    flag_disarmed = false;
    tone(PIN_BUZZER, TONE_KHZ);
    delay(100);
    noTone(PIN_BUZZER);
    delay(125);
    tone(PIN_BUZZER, TONE_KHZ);
    delay(100);
    noTone(PIN_BUZZER);
  } else if (flag_panic) {
    tone(PIN_BUZZER, currentPitch, 10);
    currentPitch += pitchStep;
    if(currentPitch >= PITCH_HIGH) {
      pitchStep = -pitchStep;
    } else if(currentPitch <= PITCH_LOW) {
      pitchStep = -pitchStep;
    }
    delay(10);
    if (millis() - panicMillis >= PANIC_ALARM_DURATION) {
      flag_panic = false;
    }
  }
}

void reconnect_mqtt() {
  char payload[255];

  while (!mqtt.connected()) {
    if (mqtt.connect("esp8266", mqtt_user, mqtt_pass, alarm_topic_available, 0, true, "offline")) {
      mqtt.subscribe(alarm_topic_cmd);
      snprintf(payload, sizeof(payload), ALARM_PAYLOAD_CONFIG, alarm_topic_available, alarm_topic_cmd, "Alarm armed", "ARMED");
      mqtt.publish(alarm_topic_config_armed, payload, true);
      delay(1000);
      snprintf(payload, sizeof(payload), ALARM_PAYLOAD_CONFIG, alarm_topic_available, alarm_topic_cmd, "Alarm disarmed", "DISARMED");
      mqtt.publish(alarm_topic_config_disarmed, payload, true);
      delay(1000);
      snprintf(payload, sizeof(payload), ALARM_PAYLOAD_CONFIG, alarm_topic_available, alarm_topic_cmd, "Alarm panic", "PANIC");
      mqtt.publish(alarm_topic_config_panic, payload, true);
      delay(1000);
      mqtt.publish(alarm_topic_available, "online", true);
    } else {
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strncmp("ARMED", (char*)payload, length) == 0) {
    flag_panic = false;
    flag_armed = true;
  } else if (strncmp("DISARMED", (char*)payload, length) == 0) {
    flag_panic = false;
    flag_disarmed = true;
  } else if (strncmp("PANIC", (char*)payload, length) == 0) {
    panicMillis = millis();
    flag_panic = true;
  }
}
