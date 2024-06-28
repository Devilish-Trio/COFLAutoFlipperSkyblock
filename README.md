# Automatic Image Recognition and Interaction

This C++ project uses OpenCV and Windows API for automatic image recognition and interaction. It's designed to detect specific patterns or colors in a region of the screen and perform actions based on those detections.

## Table of Contents
1. [Features](#features)
2. [Dependencies](#dependencies)
3. [Key Components](#key-components)
4. [Functions](#functions)
5. [Main Loop](#main-loop)
6. [Usage](#usage)

## Features

- Screen capture of a specific region
- Template matching for image detection
- Color detection within specified ranges
- Automatic mouse clicking
- Keyboard hook for pausing/resuming operations
- Multi-threaded color checking for improved performance

## Dependencies

- OpenCV
- Windows API
- C++11 or later

## Key Components

- **Global Variables**: Store configuration settings and shared resources.
- **Keyboard Hook**: Allows pausing/resuming the program with a key press.
- **Screen Capture**: Efficiently captures a specific screen region.
- **Image Processing**: Detects patterns and colors in captured images.
- **Mouse Control**: Simulates mouse clicks at specified coordinates.

## Functions

### Keyboard and Input Handling

#### `KeyboardProc`
- Callback function for the keyboard hook.
- Toggles the `paused` state when 'Q' is pressed.

#### `keyboard_hook`
- Sets up a Windows keyboard hook to listen for key presses.

### Image Capture and Processing

#### `hwnd2mat`
- Captures a specific region of the screen and converts it to an OpenCV Mat object.
- Implements caching to reduce unnecessary captures.

#### `detectPotato`
- Performs template matching to detect a specific image ("potato") in the captured screen.
- Uses multi-scale matching and non-maximum suppression for improved accuracy.

#### `nonMaximumSuppression`
- Applies non-maximum suppression to template matching results.

#### `checkForSpecifiedColor`
- Checks if a specific color range is present in the captured image.
- Utilizes OpenMP for parallel processing.

### User Interaction

#### `click`
- Simulates a mouse click at specified coordinates.

#### `buy_item`
- Performs a sequence of clicks to "buy" an item when a match is found.

### Utility Functions

#### `save_image`
- Saves a Mat object as an image file.

#### `check_pause`
- Checks if the program is paused and handles the pause state.

#### `debug`
- Saves captured images for debugging purposes.

## Main Loop

The `main` function sets up the environment and enters an infinite loop that:
1. Captures the screen region
2. Checks for the "potato" image
3. Checks for specific colors
4. Performs actions based on detections
5. Sleeps briefly to control the loop rate

## Usage

1. Set up the required dependencies.
2. Adjust the global variables for your specific use case (screen coordinates, color ranges, etc.).
3. Compile and run the program.
4. Use the 'Q' key to pause/resume operations.


* Converted from Python to C++ with GPT4.
* Added image cacheing and task parallelism for extra speed.
* Fully external and does not hook into Minecraft at all! 

# Packages
* opencv4.2.2020.5.26
* tbb.4.2.3.1
* tbb.redist.4.2.3.1

# How to Use?
If you can't figure it out, then idk, I guess that sounds like a you problem
