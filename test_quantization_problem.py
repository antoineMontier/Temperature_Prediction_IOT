"""
this script loads a model and compute all parameters in order to debug why our predictions differ between the computer and the arduino. 
Different from the `load_modelcpp.py` script because it's not loading from a C array.
"""

import tensorflow as tf
import numpy as np
np.random.seed(0) # block the GPU
# Load the INT8 quantized model
model_path = './models/arduino_model/2layers.tflite'
interpreter = tf.lite.Interpreter(model_path=model_path)
interpreter.allocate_tensors()

# Get input/output details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("=== Input Details ===")
print(f"dtype: {input_details[0]['dtype']}")
print(f"shape: {input_details[0]['shape']}")
print(f"quantization: scale={input_details[0]['quantization'][0]}, zero_point={input_details[0]['quantization'][1]}")

print("\n=== Output Details ===")
print(f"dtype: {output_details[0]['dtype']}")
print(f"shape: {output_details[0]['shape']}")
print(f"quantization: scale={output_details[0]['quantization'][0]}, zero_point={output_details[0]['quantization'][1]}")

# test data
data2 = np.array([11.8, 11.8, 11.8, 11.8, 11.8, 11.8, 11.8, 11.8,
    11.7, 11.7, 11.7, 11.7, 11.6, 11.5, 11.5, 11.5,
    11.5, 11.6, 11.6, 11.7, 11.8, 11.9, 11.9, 12, 12.1,
    12.1, 12.3, 12.4, 12.4, 12.5, 12.4, 12.4, 12.4,
    12.4, 12.6, 12.6, 12.6, 12.7, 12.8, 12.9, 13.1,
    13.2, 13.3, 13.2, 13.1, 13.1, 12.9, 13])

# Scale to [0, 1]
data2_scaled = (data2 - (-5.0)) / (25.0 - (-5.0))

print(f"\n=== Input Processing ===")
print(f"First temp: {data2[0]:.2f}°C -> scaled: {data2_scaled[0]:.4f}")
print(f"Last temp: {data2[-1]:.2f}°C -> scaled: {data2_scaled[-1]:.4f}")

# Quantize input
input_scale = input_details[0]['quantization'][0]
input_zero_point = input_details[0]['quantization'][1]
data2_quantized = np.round(data2_scaled / input_scale).astype(np.int32) + input_zero_point
data2_quantized = np.clip(data2_quantized, -128, 127).astype(np.int8)

print(f"First quantized: {data2_quantized[0]}")
print(f"Last quantized: {data2_quantized[-1]}")

# Set input
interpreter.set_tensor(input_details[0]['index'], data2_quantized.reshape(1, 48, 1))

# Run inference
interpreter.invoke()

# Get quantized output
output_quantized = interpreter.get_tensor(output_details[0]['index'])[0][0]
print(f"\n=== Output Processing ===")
print(f"Quantized output from TFLite: {output_quantized}")

# Dequantize output
output_scale = output_details[0]['quantization'][0]
output_zero_point = output_details[0]['quantization'][1]
output_scaled = (output_quantized - output_zero_point) * output_scale

print(f"Dequantized scaled output [0,1]: {output_scaled:.6f}")
print(f"Final temperature: {output_scaled * 30 - 5:.2f}°C")


"""
-8
Tensor params:
  Input  scale: 0.003919
  Input  zero_point: -128
  Output scale: 0.005667
  Output zero_point: -97
  Model  range: -5.00 .. 25.00
Dequantized output (helper): 10.129940
Dequantized output (manual): 10.129940
result: 
10.13
"""

kInputMax=25.0
kInputMin=-5.0
scale=0.005667
zeropoint=-97
scaledquantizeValue=6

print((scaledquantizeValue - zeropoint) * scale * (kInputMax - kInputMin) + kInputMin)

scaledquantizeValue=-8

print((scaledquantizeValue - zeropoint) * scale * (kInputMax - kInputMin) + kInputMin)
