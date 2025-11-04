/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <TensorFlowLite.h>
#include <algorithm>

#include "model.h"
#include "Arduino.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/c/common.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

constexpr int kTensorArenaSize = 200 * 1024; // 200 KB (utilise pour les calculs), < 256 KB (SRAM)
// Keep aligned to 16 bytes for CMSIS
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

bool setup_complete = false;
}

constexpr float kInputMin = -5.0f;
constexpr float kInputMax = 25.0f;
bool led_initialized = false;

void RunInference();
float scaleTemperature(float temperature);
float descaleTemperature(float scaledValue);

void setup() {
  if (setup_complete) {
    MicroPrintf("Setup already done, skipping re-initialization\n");
    return;
  }
  MicroPrintf("Running Setup\n");
  tflite::InitializeTarget();

  model = tflite::GetModel(model_temperature);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Resolver for unrolled GRU model
  static tflite::MicroMutableOpResolver<15> resolver;
  
  // Core operations from the TFLite model
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddFullyConnected();
  resolver.AddAdd();
  resolver.AddSub();
  resolver.AddMul();
  resolver.AddLogistic();  // Sigmoid activation
  resolver.AddTanh();
  resolver.AddShape();
  resolver.AddStridedSlice();
  resolver.AddTranspose();
  resolver.AddUnpack();
  resolver.AddPack();
  resolver.AddFill();
  resolver.AddSplit();

  memset(tensor_arena, 0, kTensorArenaSize); // clear memory (in case of multiple calls)

  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  MicroPrintf("Arena used bytes: %d\n", interpreter->arena_used_bytes());

  input = interpreter->input(0);
  output = interpreter->output(0);

  // Print input/output tensor info
  MicroPrintf("Input shape: [%d, %d, %d]\n", 
              input->dims->data[0], 
              input->dims->data[1], 
              input->dims->data[2]);
  MicroPrintf("Output shape: [%d, %d]\n", 
              output->dims->data[0], 
              output->dims->data[1]);

  inference_count = 0;
  MicroPrintf("Setup finished\n");

  setup_complete = true;
}



void loop() {
  if (!setup_complete) {
    MicroPrintf("In Loop() but Setup isn't done");
    delay(5000);
    return;  // Don't run loop until setup succeeds
  }
  RunInference();
  delay(60*1000);
}

void RunInference() {
  // 1) 48 half-hour temps:
  float sample_data[48] = {
    11.8,11.8,11.8,11.8,11.8,11.8,11.8,11.8,
    11.7,11.7,11.7,11.7,11.6,11.5,11.5,11.5,
    11.5,11.6,11.6,11.7,11.8,11.9,11.9,12,12.1,
    12.1,12.3,12.4,12.4,12.5,12.4,12.4,12.4,
    12.4,12.6,12.6,12.6,12.7,12.8,12.9,13.1,
    13.2,13.3,13.2,13.1,13.1,12.9,13
  };


  // 2) Get the quantized input buffer:
  int8_t* input_buffer = interpreter->typed_input_tensor<int8_t>(0);

  // 3) Quantize each sample into [-128..127] using scaleTemperature:
  float scale  = input->params.scale;
  int   zpoint = input->params.zero_point;

  for (int i = 0; i < 48; ++i) {
    // Scale to [0, 1]
    float scaledValue = scaleTemperature(sample_data[i]);

    // quantize: q = round(normalized/scale) + zero_point
    float qf = scaledValue / scale + (float)zpoint;
    int32_t q = lrintf(qf);
    if (q > 127) q = 127;
    if (q < -128) q = -128;
    input_buffer[i] = static_cast<int8_t>(q);

    // optional per-item debug for first/last
    if (i == 0 || i == 47) {
      Serial.print("i="); Serial.print(i);
      Serial.print(" temp="); Serial.print(sample_data[i], 2);
      Serial.print(" -> scaled="); Serial.print(scaledValue, 4);
      Serial.print(" -> quantized="); Serial.println((int)input_buffer[i]);
    }
  }

  // Print summary input processing
  Serial.println("\n=== Input Processing ===");
  float first_scaled = scaleTemperature(sample_data[0]);
  float last_scaled  = scaleTemperature(sample_data[47]);
  Serial.print("First temp: "); Serial.print(sample_data[0], 2); Serial.print("°C -> scaled: "); Serial.println(first_scaled, 4);
  Serial.print("Last temp: ");  Serial.print(sample_data[47], 2); Serial.print("°C -> scaled: ");  Serial.println(last_scaled, 4);
  Serial.print("First quantized: "); Serial.println((int)input_buffer[0]);
  Serial.print("Last quantized: ");  Serial.println((int)input_buffer[47]);

  // 4) Run the model:
  if (interpreter->Invoke() != kTfLiteOk) {
    MicroPrintf("Invoke failed\n");
    return;
  }

  // 5) Dequantize the output and print
  int8_t qy = interpreter->typed_output_tensor<int8_t>(0)[0];
  float scaled_y = (qy - output->params.zero_point) * output->params.scale;

  Serial.println("\n=== Output Processing ===");
  Serial.print("Quantized output from TFLite: "); Serial.println((int)qy);
  Serial.print("Dequantized scaled output [0,1]: "); Serial.println(scaled_y, 6);

  // final temperature
  float predictedTemp = descaleTemperature(scaled_y);
  Serial.print("Final temperature: "); Serial.print(predictedTemp, 2); Serial.println("°C");

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "Predicted next temp: %.2f°C\n", predictedTemp);
  MicroPrintf(buffer);
  Serial.println(String(predictedTemp));
}

// Function to scale a temperature value to [0, 1]
float scaleTemperature(float temperature) {
  return (temperature - kInputMin) / (kInputMax - kInputMin);
}

// Function to descale a value from [0, 1] back to temperature
float descaleTemperature(float scaledValue) {
  return scaledValue * (kInputMax - kInputMin) + kInputMin;
}