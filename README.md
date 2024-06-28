# External COFL Auto Flipper
Fully external C++ program which does not hook into Minecraft at all!

## Table of Contents
1. [Features](#features)
2. [Dependencies](#dependencies)
3. [Functions](#functions)
4. [Main Loop](#main-loop)
5. [Usage](#usage)

## Features

- Screen capture the area where buy is located
- Template matching for image detection
- Color detection within specified ranges using gray scale
- Automatic mouse clicking to buy item
- Keyboard hook for pausing/resuming the tool
- Multi-threaded color checking for improved performance

## Dependencies

- opencv4.2.2020.5.26
- tbb.4.2.3.1
- tbb.redist.4.2.3.1

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
3. Checks for specific colors (buy symbol)
4. Performs the actions to buy or exit

## Usage
If you can't figure it out, then idk, I guess that sounds like a you problem

* Converted from Python to C++ with GPT4.
* Added image cacheing and task parallelism for extra speed.
