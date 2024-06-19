# FreeRTOS-Multitasking

## Introduction
This project is a coursework exercise on FreeRTOS. We have created a system comprising multiple tasks to read data from sensors, display data on an LCD, control a buzzer, and an LED.

## Project Tasks
1. **Sensor Data Reading Task**: This task reads data from two sensors, DHT11 and HCSR-05, and stores it in a global variable.
2. **LCD Task**: This task displays the values of the two sensors, DHT11 and HCSR-05, on the LCD. The sensor values are updated on the LCD every second.
3. **Buzzer Running Task**: This task runs the buzzer passively.
4. **LED Control Task**: This task includes two buttons to turn an LED on and off. This task responds immediately when there is an event from the button.
5. **Semaphore Granting Task**: This task runs a button to grant a Semaphore. A Semaphore grant lasts for 2 seconds and can be granted up to 5 times per Semaphore grant.
6. **LED Blinking Task**: This task is to blink an LED. The LED will blink 3 times per second when granted a Semaphore and must wait for the Semaphore granting process of the above task to complete.

## Directory Structure
This project includes the following directories:
- `MainSource`: Contains the main source code of the project.
- `Examples`: Contains demo source code to test each component.
- `libraries`: Contains the necessary libraries for the project.

## Installation
To install and run this project, you need a FreeRTOS and Arduino IDE development environment. You can download the project source code from here and open it in your development environment.

1. **Install Arduino IDE**: You can download and install Arduino IDE from the official Arduino website.
2. **Download and Install Libraries**: Copy the `libraries` folder into the `libraries` folder of your Arduino IDE on your computer.
3. **Open the Project**: Open Arduino IDE, select `File -> Open...` and navigate to the `main` directory of the project.
4. **Compile and Upload**: Press the `Upload` button to compile and upload the source code to your Arduino board.

## Contribution
We welcome any contributions from the community. If you want to contribute, please create a Pull Request.
