# Flappy Bird

**Authors:** Can Afacan, Julian Tang, Yueyuan Sui  
**Target Board:** nRF52833  

## Overview
This project implements a Flappy Bird game on the nRF52833 microcontroller. The game utilizes the board's capabilities to create an interactive experience.

## Prerequisites
Before getting started, ensure you have the following:
- nRF52833 development board
- GNU Make
- ARM GCC toolchain
- nRF Command Line Tools
- Nordic Semiconductor's development environment (optional but recommended)

## Getting Started
### 1. Clone the Repository
To begin, clone this repository to your local machine:
```sh
git clone <https://github.com/JulianTang2027/Flappybird.git>
```

### 2. Navigate to the Project Directory
```sh
cd your_repo/flappy_bird
```

### 3. Build and Flash the Firmware
Use the following command to compile and flash the firmware onto the nRF52833 board:
```sh
make flash
```

### 4. Running the Game
Once flashed, the game should start automatically on your nRF52833 board. Follow on-screen instructions or button inputs to play!

## Inputs and Outputs
### Inputs:
- **Tactile Input:** Button presses are used to control the bird's movement.
- **Microphone:** The game can detect audio input to trigger actions (e.g., jump by making a sound).

### Outputs:
- **Speaker:** Game sounds and effects are played through the onboard speaker.
- **Screen:** The game visuals, including obstacles and score, are displayed on the screen.

## Demo
[Watch the demo video](flappybirddemo.mp4)



## Additional Resources
For more details on the nRF52833 microcontroller, visit the [Nordic Semiconductor Documentation](https://docs.nordicsemi.com/bundle/ps_nrf52833/page/keyfeatures_html5.html).

## Troubleshooting
- Ensure all dependencies are installed correctly.
- Check your board connection before flashing.
- If flashing fails, try running:
  ```sh
  make clean
  make flash
  ```

## License
This project is for educational purposes under CE 346. DO NOT copy this code. 

