#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

const String ssid = "ESP8266-IoT";
const String password = "########";

const uint ADDR = 0, MAX_NAME_LEN = 20, MAX_DEVICES = 9;
const byte AVAILABLE_PINS[MAX_DEVICES] = { 0, 2, 4, 5, 12, 13, 14, 15, 16 };

struct Device {
  byte pin;
  char name[MAX_NAME_LEN];

  Device &operator=(Device &device)
  {
    pin = device.pin;
    strncpy(name, device.name, MAX_NAME_LEN);
    return *this;
    }
};

struct Data {
  byte count;
  Device devices[MAX_DEVICES];

  bool addDevice(byte pin, String name) {
    if (count >= MAX_DEVICES) {
      count = MAX_DEVICES;
      return false;
    }

    for(size_t i = 0; i<count; i++) if(devices[i].pin == pin) return false;

    for (size_t i = 0; i < MAX_DEVICES; i++) if(pin == AVAILABLE_PINS[i]) {
    devices[count].pin = pin;
    snprintf(devices[count].name, MAX_NAME_LEN, "%s", name.c_str());

    count++;
    return true;
  }

    return false;
  }

  bool removeDevice(size_t pos) {
    if (count <= 0) {
      count = 0;
      return false;
    }

    if (pos >= count) {
      return false;
    }

    for (size_t i = pos; i < count - 1; i++) {
      devices[i] = devices[i + 1];
    }

    count--;
    return true;
  }

  DynamicJsonDocument toJson() {
    DynamicJsonDocument doc(1024); //JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(count) + count * JSON_OBJECT_SIZE(2)
    doc["count"] = count;

    for (size_t i = 0; i < count; i++) {
      JsonObject obj = doc["devices"].createNestedObject();
      obj["name"] = devices[i].name;
      obj["pin"] = devices[i].pin;
    }

    return doc;
  }

  size_t find(byte pin) {
    for (size_t i = 0; i < count; i++)
      if (devices[i].pin == pin)
        return i;

    return -1;
  }
} data;

AsyncWebServer server(80);
void setup() {
  Serial.begin(9600);
  WiFi.softAP(ssid, password);
  EEPROM.begin(sizeof(Data));

  EEPROM.get(ADDR, data);

  Serial.println(String(data.count) + " devices : ");
  for (size_t i = 0; i < data.count; i++) {
    Serial.println(String(data.devices[i].pin) + " : " + String(data.devices[i].name));
}

  server.on("/getDevices", [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    EEPROM.get(ADDR, data);
    serializeJson(data.toJson(), *response);
    request->send(response);
  });
  server.on("/addDevice", [](AsyncWebServerRequest *request) {
    if (request->hasParam("pin") && request->hasParam("name"))
    {
      EEPROM.get(ADDR, data);

      byte pin = request->getParam("pin")->value().toInt();
      size_t idx = data.find(pin);
      if (idx != -1)
      {
        Serial.println("Pin " + String(pin) + " already registered to " + data.devices[idx].name);
        request->send(400);
      } else {
        String name = request->getParam("name")->value();
        if(data.addDevice(pin, name))
        {
          EEPROM.put(ADDR, data);
          EEPROM.commit();
          AsyncResponseStream *response = request->beginResponseStream("application/json");
          serializeJson(data.toJson(), *response);
          request->send(response);
        } else {
          Serial.println("something went wrong");
          request->send(500);
        }
      }
    }
    else
    {
      Serial.println("name/pin not provided");
      request->send(400);
    }
  });
  server.on("/removeDevice", [](AsyncWebServerRequest *request) {
    if (request->hasParam("pin"))
    {
      byte pin = request->getParam("pin")->value().toInt();

      EEPROM.get(ADDR, data);
      size_t idx = data.find(pin);
      if (idx == -1)
      {
        Serial.println(String(pin) + " not found");
        request->send(404);
      }
      else
      {
        if(data.removeDevice(idx))
        {
          EEPROM.put(ADDR, data);
          EEPROM.commit();
          AsyncResponseStream *response = request->beginResponseStream("application/json");
          serializeJson(data.toJson(), *response);
          request->send(response);
        }
        else
        {
          Serial.println("something went wrong");
          request->send(500);
        }
      }
    }
    else
    {
      Serial.println("pin not provided");
      request->send(400);
    }
  });
  server.on("/getAvailablePins", [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<JSON_ARRAY_SIZE(MAX_DEVICES)> doc;
    JsonArray array = doc.to<JsonArray>();
    EEPROM.get(ADDR, data);
    for(size_t i=0; i<MAX_DEVICES; i++) {
      if (data.find(AVAILABLE_PINS[i]) == -1) {
        array.add(AVAILABLE_PINS[i]);
      }
  }
    serializeJson(doc, *response);
    request->send(response);
  });

  server.begin();
  }

void loop() {

}
