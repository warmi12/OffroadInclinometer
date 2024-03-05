## Description 
The project is a prototype, its purpose was to examine the feasibility of implementing the main assumptions needed to implement another (larger) project.

A device designed to measure the angle of inclination of a car in difficult conditions.
The measurement covers two of the three angles known as pitch and roll.

The solution is based on the FreeRTOS real-time system.
FreeRTOS was used to obtain maximum performance of the RP2040. The SMP version allows to use two RP2040 cores.

The software consists of two tasks:
1) IMUTask - the aim of this task is to read data from the IMU (accelerometer, gyroscope) as often as possible. As well as determining the current inclination angles.
2) DisplayTask - the purpose of this task is to display current data using LVGL lib.

## Hardware
To make prototyping as easy as possible, a kit containing RP2040, IMU and LCD was used (https://www.waveshare.com/rp2040-lcd-1.28.htm).

## Visualization
![car](https://github.com/warmi12/OffroadInclinometer/assets/46669932/66a68ba5-1d69-4584-b54c-0c624dc40780)

## Setup
```
$ git clone --recurse-submodules https://github.com/warmi12/OffroadInclinometer
$ cd OffroadInclinometer/
$ mkdir build
$ cd build
$ cmake ..
$ make -j8
```

## Presentation
https://www.youtube.com/shorts/GkgAsqdhfCM
