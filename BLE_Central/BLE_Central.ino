//libraries for tensorflow
#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

//libraries and initialisations for BLE
#include <ArduinoBLE.h>
const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid1 = "19b10001-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid2 = "19b10002-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid3 = "19b10003-e8f2-537e-4f6c-d104768a1214";

//variables to store gestures;
int i = 0, j, k, toggle = 0;


// This is the model you trained in Tiny Motion Trainer, converted to
// a C style byte array.
#include "model.h"

// Values from Tiny Motion Trainer
#define MOTION_THRESHOLD 0.188
#define CAPTURE_DELAY 200 // This is now in milliseconds
#define NUM_SAMPLES 20

// Array to map gesture index to a name
const char *GESTURES[] = {
  "jump", "leftRotate"
};

//capture variables;
#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

bool isCapturing = false;

// Num samples read from the IMU sensors
// "Full" by default to start in idle
int numSamplesRead = 0;


//==============================================================================
// TensorFlow variables
//==============================================================================

// Global variables used for TensorFlow Lite (Micro)
tflite::MicroErrorReporter tflErrorReporter;

// Auto resolve all the TensorFlow Lite for MicroInterpreters ops, for reduced memory-footprint change this to only
// include the op's you need.
tflite::AllOpsResolver tflOpsResolver;

// Setup model
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// Create a static memory buffer for TensorFlow Lite for MicroInterpreters, the size may need to
// be adjusted based on the model you are using
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];


//==============================================================================
// Setup / Loop
//==============================================================================

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

//start BLE and IMU
  Serial.begin(9600);
  BLE.begin();
  BLE.setLocalName("Nano 33 BLE (Central)");
  BLE.advertise();
  IMU.begin();

  // Get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  // Allocate memory for the model's input and output tensors
  tflInterpreter->AllocateTensors();

  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

void loop() {
  // Variables to hold IMU data
  float aX, aY, aZ, gX, gY, gZ;

  //look for peripheral;
  BLEDevice peripheral; 
  do
  {
    BLE.scanForUuid(deviceServiceUuid);
    peripheral = BLE.available();
  } while (!peripheral);
  
  //do things when connected to peripheral; 
  if (peripheral) {
    BLE.stopScan();  peripheral.connect();
    peripheral.discoverAttributes();
    BLECharacteristic gestureCharacteristic1 = peripheral.characteristic(deviceServiceCharacteristicUuid1);
    BLECharacteristic gestureCharacteristic2 = peripheral.characteristic(deviceServiceCharacteristicUuid2);
    BLECharacteristic gestureCharacteristic3 = peripheral.characteristic(deviceServiceCharacteristicUuid3);
    while (peripheral.connected()) {

      // Wait for motion above the threshold setting
      while (!isCapturing) {
        if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {

          IMU.readAcceleration(aX, aY, aZ);
          IMU.readGyroscope(gX, gY, gZ);

          // Sum absolute values
          float average = fabs(aX / 4.0) + fabs(aY / 4.0) + fabs(aZ / 4.0) + fabs(gX / 2000.0) + fabs(gY / 2000.0) + fabs(gZ / 2000.0);
          average /= 6.;

          // Above the threshold?
          if (average >= MOTION_THRESHOLD) {
            isCapturing = true;
            numSamplesRead = 0;
            break;
          }
        }
      }

      // above thershold to start classifying gestures 
      while (isCapturing) {

        // Check if both acceleration and gyroscope data is available
        if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {

          // read the acceleration and gyroscope data
          IMU.readAcceleration(aX, aY, aZ);
          IMU.readGyroscope(gX, gY, gZ);

          // Normalize the IMU data between -1 to 1 and store in the model's
          // input tensor. Accelerometer data ranges between -4 and 4,
          // gyroscope data ranges between -2000 and 2000
          tflInputTensor->data.f[numSamplesRead * 6 + 0] = aX / 4.0;
          tflInputTensor->data.f[numSamplesRead * 6 + 1] = aY / 4.0;
          tflInputTensor->data.f[numSamplesRead * 6 + 2] = aZ / 4.0;
          tflInputTensor->data.f[numSamplesRead * 6 + 3] = gX / 2000.0;
          tflInputTensor->data.f[numSamplesRead * 6 + 4] = gY / 2000.0;
          tflInputTensor->data.f[numSamplesRead * 6 + 5] = gZ / 2000.0;

          numSamplesRead++;

          // Do we have the samples we need?
          if (numSamplesRead == NUM_SAMPLES) {

            // Stop capturing
            isCapturing = false;

            // Run inference
            TfLiteStatus invokeStatus = tflInterpreter->Invoke();
            if (invokeStatus != kTfLiteOk) {
              Serial.println("Error: Invoke failed!");
              while (1);
              return;
            }

            // Loop through the output tensor values from the model
            int maxIndex = 0;
            float maxValue = 0;
            for (int i = 0; i < NUM_GESTURES; i++) {
              float _value = tflOutputTensor->data.f[i];
              if (_value > maxValue) {
                maxValue = _value;
                maxIndex = i;
              }
            }
            i = random(1, 4);
            j = maxIndex + 1;
            gestureCharacteristic1.writeValue((byte)i);
            gestureCharacteristic2.writeValue((byte)j);
            // Add delay to not double trigger
            delay(CAPTURE_DELAY);
          }
        }
      }
      //conditions for switch;
      if (analogRead(A7) == HIGH)
      {
        delay(50);
        if (analogRead(A7) == HIGH)
        {
          i = random(1, 4);
          if (k == 1 && toggle == 0)
          {
            k = 0;
            toggle = 1;
          }
          else if (k == 0 && toggle == 0)
          {
            k = 1;
            toggle = 1;
          }
        }
        gestureCharacteristic1.writeValue((byte)i);
        gestureCharacteristic3.writeValue((byte)k);
        toggle = 0;
      }
    }
  }
}
