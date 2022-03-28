//library and initialisation for BLE;
#include <ArduinoBLE.h>
const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid1 = "19b10001-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid2 = "19b10002-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid3 = "19b10003-e8f2-537e-4f6c-d104768a1214";

BLEService gestureService(deviceServiceUuid);
BLEByteCharacteristic gestureCharacteristic1(deviceServiceCharacteristicUuid1, BLERead | BLEWrite);
BLEByteCharacteristic gestureCharacteristic2(deviceServiceCharacteristicUuid2, BLERead | BLEWrite);
BLEByteCharacteristic gestureCharacteristic3(deviceServiceCharacteristicUuid3, BLERead | BLEWrite);

void setup() {
  Serial.begin(9600);
  BLE.begin();
  BLE.setLocalName("Arduino Nano 33 BLE (Peripheral)");
  BLE.setAdvertisedService(gestureService);
  gestureService.addCharacteristic(gestureCharacteristic1);
  gestureService.addCharacteristic(gestureCharacteristic2);
  gestureService.addCharacteristic(gestureCharacteristic3);
  BLE.addService(gestureService);
  gestureCharacteristic1.writeValue(0);
  gestureCharacteristic2.writeValue(0);
  gestureCharacteristic3.writeValue(0);
  BLE.advertise();
}
void loop() {
  BLEDevice central = BLE.central();
  delay(500);

  //do things when central is connected;
  if (central) {
    while (central.connected()) {
      if (gestureCharacteristic1.written()) {
        Serial.print(gestureCharacteristic2.value());
        Serial.print(",");
        Serial.print(gestureCharacteristic3.value());
      }
    }
  }
}
