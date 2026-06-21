# Automated Car Parking System

An Arduino Uno–based automated car parking system that manages six parking slots using a bitmask allocation algorithm. The system detects vehicle entry and exit using IR sensors, controls entry and exit gates using servo motors, displays parking status on a 16x2 LCD, and logs events through the Serial Monitor.

---

## Features

- Automatic parking slot allocation  
- Supports 6 parking slots  
- IR sensor-based entry and exit detection  
- Servo motor-controlled entry and exit gates  
- 16x2 LCD status display  
- Parking full detection  
- Slot release during vehicle exit  
- Buzzer indication  
- Red and green status LEDs  
- Serial Monitor event logging  
- Developed using Arduino IDE  
- Simulated using Wokwi  

---

## Hardware Used

- Arduino Uno  
- 16x2 LCD  
- 2 Servo Motors  
- 2 IR Sensors (simulated using push buttons)  
- 4 Push Buttons (slot selection)  
- Buzzer  
- Green LED  
- Red LED  


---

## How to Run

1. Open the project in Wokwi.  
2. Start the simulation.  
3. Press the Entry IR button to simulate vehicle entry.  
4. Press the Exit IR button and select the parking slot to simulate vehicle exit.  
5. Observe LCD messages, LEDs, buzzer, servo movement, and Serial Monitor logs.  

---

## Screenshots

Add your images inside the `images/` folder and reference them like this:

- Overall Simulation  
- System Startup  
- Vehicle Entry  
- Parking Full Condition  
- Vehicle Exit  
- Serial Monitor Output  

Example:

![Overall Simulation](images/overall_simulation.png)

---

## License

This project is intended for educational purposes only.
