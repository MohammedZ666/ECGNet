# ECGNet

This project contains the microcontroller code for the paper "Dense Neural Network Based Arrhythmia Classification on Low-cost and Low-compute Micro-controller". 

- Authors - Md Abu Obaida Zishan, H M Shihab, Sabik Sadman Islam, Maliha Alam Riya, and Jannatun Noor.
- Supervisor - Janntun Noor
- Journal - Expert Systems with Applications
- Lab - [Computing for Sustainability and Social Good, BRAC University, Dhaka, Bangladesh](https://www.researchgate.net/lab/Computing-for-Sustainability-and-Social-Good-C2SG-Lab-Jannatun-Noor)

Here, we deployed a neural network to detect arrhythmia. Here, [`main.cpp`](https://github.com/MohammedZ666/ECGNet/blob/main/main.cpp) contains the MCU code for detecting arrhythmia in real-time. [`model.h`](https://github.com/MohammedZ666/ECGNet/blob/main/model.h) contains, the qunatized neural network weights. Finally, [`training.ipynb`](https://github.com/MohammedZ666/ECGNet/blob/main/training.ipynb) contains the training code. 


## Installation steps

1) Download and extract avr-gnu toolchain for Linux/Windows from [here](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers).
2) Add the `/bin` directory after extraction to PATH.
3) Download the AVRDUDE binary for Windows from [here](https://github.com/mariusgreuel/avrdude/releases/tag/v7.1-windows) and add to PATH.
4) For debian-linux, run `sudo apt install avrdude`. For other distros, install avrdude with your respective package manager.  


## Run

To run the project, connect your workstation with an Arduino Nano/Uno. Change the last line of the [`Makefile`](https://github.com/MohammedZ666/ECGNet/blob/main/Makefile) file to include the port where the Arduino is connected: 

`avrdude -F -v -v -c arduino -p ATMEGA328P -P YOUR_PORT_GOES_HERE -b 115200 -U flash:w:build/main.hex`

Additionally, connect [AD8232 SparkFun Single-Lead Heart Rate Monitor](https://www.sparkfun.com/products/12650) for full reproducibility of our work as the connection schema provided below:

![image](https://github.com/MohammedZ666/ECGNet/assets/50874919/8bc560e2-bf31-4bc0-8653-e0de25c5364d)

## Training

For training the neural network and generating your own [`model.h`](https://github.com/MohammedZ666/ECGNet/blob/main/model.h) file, run [`training.ipynb`](https://github.com/MohammedZ666/ECGNet/blob/main/training.ipynb) with google-colab (recommended) or locally.


