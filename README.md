# DMX Output Device with Webserver and API

This project is a webserver and API-based DMX outputting device powered by the RP2040 port of FreeRTOS. Its main feature includes channel control with different keywords and a solo mode for checking all lights.

## Features

- Channel control with keywords like "AND", "AT", "THRU", "FULL".
- Solo mode enables "+" and "-" buttons for checking all lights.
- Display on website of captured channels and their levels.
- Password authentication for website access.
- Dedicated differential transceiver IC (TI SN75176A) that meets or exceeds the requirements of ANSI Standards EIA/TIA-422-B and ITU Recommendations V.11.
- 3D printable enclosure for the device.

## Default Configuration

Here are the default configurations for the device:

- Hostname: rfuint
- SSID: RemoteFocus
- Password: 12345678
- Web Password: 12345678
- AP Mode: true

Furthermore, this device has the ability to store configuration details.

## Installation

Detailed installation instructions can be found in [INSTALL.md](./INSTALL.md).

## Usage

Once installed, the user interface is inspired by the ETC Expression Console Keypad. Refer to the user manual to familiarize yourself with all the different features and capabilities of the device.

Please note: For web access, password authentication is required.

## Schematics and 3D Model

The wiring schematic and 3D print model for the enclosure have been included in this repository. Please refer to those resources for assembly and construction details.

## License

This project is licensed under the terms of the GNU Affero General Public License - see the [LICENSE](LICENSE) file for details.
