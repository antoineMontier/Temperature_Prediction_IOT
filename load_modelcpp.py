"""
This script loads a model stored in a .cpp file and use it to compute a prediction
Its aim was to figure out why our outputs differ between the arduino card and the computer
"""

import re
import numpy as np
import tensorflow as tf

model_cpp_path = "./scripts_arduino/prediction/script/model.cpp"

# Read C array
with open(model_cpp_path, "r", encoding="utf-8") as f:
    s = f.read()

# regex : 0x??
hex_bytes = re.findall(r"0x([0-9a-fA-F]{2})", s)
model_bytes = bytes(int(h, 16) for h in hex_bytes)
print(f" bytes: {len(model_bytes)}")

interpreter = tf.lite.Interpreter(model_content=model_bytes)
interpreter.allocate_tensors()

inp_details = interpreter.get_input_details()[0]
out_details = interpreter.get_output_details()[0]
print("IN: ", inp_details)
print("OUT:", out_details)

# example data
sample_data = np.array([
  11.8,11.8,11.8,11.8,11.8,11.8,11.8,11.8,
  11.7,11.7,11.7,11.7,11.6,11.5,11.5,11.5,
  11.5,11.6,11.6,11.7,11.8,11.9,11.9,12,12.1,
  12.1,12.3,12.4,12.4,12.5,12.4,12.4,12.4,
  12.4,12.6,12.6,12.6,12.7,12.8,12.9,13.1,
  13.2,13.3,13.2,13.1,13.1,12.9,13
])

assert sample_data.size == 48, "input must contain 48 values"

# min-max
kInputMin = -5.0
kInputMax = 25.0
scaled = (sample_data - kInputMin) / (kInputMax - kInputMin)  # values in [0,1]

# prepare input
in_dtype = inp_details["dtype"]
if in_dtype == np.int8 or in_dtype == np.uint8: # int
    in_scale, in_zp = inp_details["quantization"]
    q = np.round(scaled / in_scale).astype(np.int32) + int(in_zp)
    q = np.clip(q, -128, 127) if in_dtype == np.int8 else np.clip(q, 0, 255)
    q = q.astype(in_dtype)
    input_tensor = q.reshape(1, 48, 1)
    print("First quantized input:", int(input_tensor.flatten()[0]))
    print("Last quantized input: ", int(input_tensor.flatten()[-1]))
else: # float
    input_tensor = scaled.reshape(1, 48, 1).astype(in_dtype)

# invoke (run the model)
interpreter.set_tensor(inp_details["index"], input_tensor)
interpreter.invoke()

# output
out = interpreter.get_tensor(out_details["index"])[0][0]
out_dtype = out_details["dtype"]
if out_dtype == np.int8 or out_dtype == np.uint8: # int
    out_scale, out_zp = out_details["quantization"]
    q_out = int(out)
    scaled_out = (q_out - int(out_zp)) * float(out_scale)   # value in [0,1] (if model trained that way)
    temp_out = scaled_out * (kInputMax - kInputMin) + kInputMin
    print("Quantized output:", q_out)
    print("Dequantized scaled output [0..1]:", scaled_out)
    print(f"Final temperature: {temp_out:.4f} °C")
else:
    # float output
    scaled_out = float(out)
    temp_out = scaled_out * (kInputMax - kInputMin) + kInputMin
    print("Float output (scaled):", scaled_out)
    print(f"Final temperature: {temp_out:.4f} °C")
