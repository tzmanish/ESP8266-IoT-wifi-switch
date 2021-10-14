#include <EEPROM.h>

const uint ADDR = 0, MAX_NAME_LEN = 20, MAX_DEVICES = 9;

struct Device {
  byte pin; // 0,2,4,5,12,13,14,15,16
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

  bool addDevice(byte pin, String name){
    if(count >= MAX_DEVICES){
      count = MAX_DEVICES;
      return false;
    }

    devices[count].pin = pin;
    snprintf(devices[count].name, MAX_NAME_LEN, "%s", name.c_str());

    count++;
    return true;
  }

  bool removeDevice(byte pos){
    if(count <= 0){
      count = 0;
      return false;
    }

    if(pos >= count){
      return false;
    }

    for(int i=pos; i<count-1; i++){
      devices[i] = devices[i+1];
    }

    count--;
    return true;
  }
} data;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(sizeof(Data));
}

void loop() {
  EEPROM.get(ADDR,data);

  Serial.println(String(data.count) + " devices : ");
  for(int i=0; i<data.count; i++){
    Serial.println(String(i) + ". " + String(data.devices[i].pin) + " : " + String(data.devices[i].name));
  }

  String name = "device-" + String(data.count);
  if(!data.addDevice(data.count, name)){
    Serial.println("already have max number of devices");
    exit(0);
  }

  // if(!data.removeDevice(0)){
  //   Serial.println("nothing to delete.");
  //   exit(0);
  // }

  EEPROM.put(ADDR, data);
  EEPROM.commit();

  delay(1000);
}
