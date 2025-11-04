"""
This script aims at modifying the model file (.keras) in order to have a .tflite file which we can then import in the arduino.
It also make a list of the operations required by the model.
"""

import tensorflow as tf
import numpy as np

model_keras_file_path = './models/2layers/lstm_temperature_model.keras' # change model file here 
out_folder  = './models/arduino_model/'

# Load
model = tf.keras.models.load_model(model_keras_file_path)

print("Model summary:")
model.summary()

# INT8
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

# Representative dataset
def representative_dataset():
    for _ in range(100):
        data = np.random.uniform(0, 1, (1, 48, 1)).astype(np.float32) 
        yield [data]

converter.representative_dataset = representative_dataset

tflite_model = converter.convert()

# Save
with open(out_folder+'2layers.tflite', 'wb') as f: # change name here
    f.write(tflite_model)
# xxd -i model.tflite > model.cpp

print(f"\nModel size: {len(tflite_model)/1024} KB")

# Check operations
interpreter = tf.lite.Interpreter(model_path=out_folder+'2layers.tflite') # change name here
interpreter.allocate_tensors()

print("\nModel operations (we have to add them to the arduino):")
ops = set()
for op in interpreter._get_ops_details():
    ops.add(op['op_name'])
for op in sorted(ops):
    print(f"  - {op}")

# Convert to C array
# with open('model.cpp', 'w') as f:
#     f.write('#include "model.h"\n\n')
#     f.write('alignas(16) const unsigned char model_temperature[] = {\n')
#     for i, byte in enumerate(tflite_model):
#         if i % 12 == 0:
#             f.write('\n  ')
#         f.write(f'0x{byte:02x}, ')
#     f.write('\n};\n')
#     f.write(f'const int model_temperature_len = {len(tflite_model)};\n')

# print("\nConverted to C array in model.cpp")