# Installation Guide

## Requirements

- Raspberry Pi Pico-W
- USB Cable

## Step 1: Downloading the Firmware

Visit the [Releases page](https://github.com/Papa-DMan/pico_rfu/releases) of this repository, and download the latest release.

## Step 2: Installing the Firmware

There are two methods to install the firmware onto your Raspberry Pi Pico-W:

### Method 1: Firmware Update Feature on Web Interface

1. Connect your Raspberry Pi Pico-W to your PC via a USB cable.
2. Navigate to the web interface by typing in the default hostname (**rfuint**) in your web browser.
3. Navigate to the Firmware Update section (Settings -> Firmware Update).
4. Select the `firmware.uf2` file you just downloaded from the Releases page.
5. Click on `Update Firmware` to start the installation process.
6. Follow the instructions on the webpage to complete the installation process.

### Method 2: Fresh Install Method (Drag-and-Drop Flash)

If this is a fresh install, follow the steps:

1. Press and hold the BOOTSEL button on your Raspberry Pi Pico, and continue holding it while connecting the device to your computer using a USB cable. This will put the Raspberry Pi Pico into USB mass storage device mode.
2. The Raspberry Pi Pico will show up as a mass storage device on your computer, similar to a USB flash drive. Navigate and open this device.
3. You will see a file called `INDEX.HTM` in the root directory. Ignore this file.
4. Drag and drop the `firmware.uf2` file that you downloaded from the Releases page into the root directory of the USB mass storage device (Raspberry Pi Pico).
5. The device will automatically reboot and start running the firmware update.

The new firmware is now successfully installed on your Raspberry Pi Pico-W. You can now disconnect the device from your computer.
