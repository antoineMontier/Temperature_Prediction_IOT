# Temperature Prediction

> 10/2025

[Full french report](./report.pdf)

## Hardware used

- Arduino Nano 33 BLE Rev2
- DHT11 temperature sensor

## Project architecture

```
.
├── compare_lstm.ipynb
├── data
│   ├── data.csv
│   ...
│   └── last_year.csv
├── data_gathered
│   └── raw.csv
├── eval_model.ipynb
├── gru_generator.ipynb
├── load_modelcpp.py
├── lstm_generator.ipynb
├── mlp_generator.ipynb
├── models
│   ├── arduino_model
│   │   ├── 2layers.tflite
│   │   ├── gru_light.tflite
│   │   ├── model.h
│   │  ...
│   │   └── temperature_model_light.tflite
│   ├── gru
│   │   ├── gru_temperature_model.h5
│   │   └── gru_temperature_model.keras
│  ...
│   └── MLP
│       ├── mlp_temperature_model.h5
│       └── mlp_temperature_model.keras
├── README.md
├── report.pdf
├── scripts_arduino
│   ├── data_gathering
│   │   ├── bluetooth
│   │   │   └── bluetooth.ino
│   │   ├── bluetooth_communication_4canaux.ino
│   │   ├── bluetooth_communication_async.ino
│   │   ├── bluetooth_led.ino
│   │   ├── code_temp.ino
│   │   ├── communication_bluetooth.ino
│   │   ├── sender_other_architecture.ino
│   │   └── senderv1_final.ino
│   ├── main
│   │   └── main.ino
│   ├── noBLE
│   │   └── noBLE.ino
│   └── prediction
│       └── script
│           ├── model.cpp
│           ├── model.h
│           └── script.ino
├── test_quantization_problem.py
└── to_tflite.py
```

## WorkFlow

### Data gathering

- [Data gathering script](./scripts_arduino/data_gathering/senderv1_final.ino)
- [Data gathered](./data_gathered/raw.csv)

### Models

#### Making the models

- [LSTM](./lstm_generator.ipynb)
- [LSTM comparison](./compare_lstm.ipynb)
- [GRU](./gru_generator.ipynb)
- [MLP](./mlp_generator.ipynb)

#### Evaluating the models

- [Evaluation of models](./eval_model.ipynb)

#### Converting the models

- [Convert models](./to_tflite.py)

#### Debugging

- [Test quantization](./test_quantization_problem.py)
- [Test generated C array](./load_modelcpp.py)
- [Hello world example](./scripts_arduino/remake_project_helloword/hello_world/hello_world.ino)

### Final: prediction & communication script

- [Without BLE](./scripts_arduino/noBLE/noBLE.ino)
- [With BLE](./scripts_arduino/main/main.ino)

## Datasets

Obtained via 3 different ways:

- [Historical Weather API](https://open-meteo.com/en/docs/historical-weather-api)
- [EC&D](https://www.ecad.eu/dailydata/)
- By hand, leaving the DHT sensor outside