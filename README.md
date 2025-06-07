# BYTE-90: A Retro PC and MAC Inspired Interactive Designer Art Toy

**Open source firmware coming soon**

BYTE-90 is a retro PC and Mac inspired interactive designer art toy that displays animated emotes through various interactions. It detects motion, responds to taps and orientation changes, pairs and communicates with other BYTE-90 devices to exchange animated conversations.

Designed and 3D printed by ALXV LABS (Alex Vong) as a designer toy, BYTE-90 is available as either a dev-kit or a collectible designer toy. This product is a limited run, and your support enables continued future development.

While the base software is open-source, **the animations and 3D printed files remain proprietary** to maintain brand consistency and authenticity. You can purchase BYTE-90 that includes all hardware, software, and animations, or build your own using the off-the-shelf hardware specified below.

[BYTE-90 by ALXV Labs](https://labs.alxvtoronto.com/)

> **Important**: Before contributing forking or using this project commercially, please read our [Legal & Contributing Guidelines](CONTRIBUTING.md).

## Features

- **Interactive Animations**: Displays various animated GIFs through interactions and sequence events
- **Retro Screen Effects**: Choose from different vintage display effects including scanlines, dithering overlay, retro green, or classic orange CRT aesthetics
- **Motion Detection**: Advanced accelerometer-based detection for taps, double-taps, shakes, tilts, and orientation changes
- **Device-to-Device Communication**: Seamless pairing with other BYTE-90 devices via ESP-NOW protocol for animated conversations
- **Intelligent Power Management**: Progressive sleep modes with automatic inactivity detection to maximize battery life
- **Over-the-Air Updates**: Wireless firmware and animation updates via dedicated web interface

### Current Firmware Version
- Version: 1.0.1
- Release Date: [TBD]
- Compatibility: XIAO ESP32S3 hardware (Other ESP requires modification to pins)

## Hardware Components

### Core Components

**Microcontroller**: [Seeedstudio XIAO ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- Dual-core Xtensa LX7 processor (240MHz)
- 8MB Flash / 512KB SRAM
- Built-in Wi-Fi and Bluetooth
- Compact form factor (21×17.5mm)
- Optimized for GIF processing and animation storage

**Display**: [Waveshare 1.5" RGB OLED Module (SSD1351)](https://www.waveshare.com/wiki/1.5inch_RGB_OLED_Module)
- 128×128 pixel full-color display
- RGB565 color format (65,536 colors)
- SPI interface with 20MHz frequency
- 16-level brightness control (6.25% to 100%)
- High contrast ratio perfect for retro aesthetics
- Low power consumption with display on/off control

**Motion Sensor**: ADXL345 Triple-Axis Accelerometer
- ±16g measurement range with configurable sensitivity
- I2C interface for communication
- Hardware interrupt capability for deep sleep wake
- Tap/double-tap detection with configurable thresholds
- Activity/inactivity detection for power management
- Orientation sensing for tilt/flip detection

**User Interface**: Single tactile push button
- Supports single click, long press (3+ seconds), and double-click detection
- Software debouncing (50ms) with 300ms double-click window
- Mode switching and interaction control

**Storage**: 8MB Internal Flash Memory
- Custom partition layout for firmware and animations
- LittleFS filesystem for efficient file management
- Supports 2-3MB of animation data

**Power System**: 
- USB-C charging and data interface (1-2 hours for full charge)
- Integrated battery management with visual charge indicators:
  - Red flashing light: Charging in progress
  - Solid red light: Charging complete
- Optional LiPo battery (103040 1200mAh, 3.7V with PH 2.0 connector)
- Battery life: Up to 2 days with intelligent power management
- Can operate via USB-C power without battery

## Battery Safety:

- Use ONLY the specified 3.7V lithium battery (103040 size) with PH 2.0 connector
- Verify 3.7V voltage (NOT 3.9V) before installation
- Check connector polarity alignment carefully
- Handle battery connector with care to avoid damage

**Critical Safety Checks**:
1. Check battery's connector polarity alignment if using another type of battery
2. Ensure battery is charged and functional
3. Handle connector carefully

**Critical Safety Note**: 
Always verify battery voltage is **3.7V** (not 3.9V) and **correct polarity alignment** before installation.

## Pin Configuration
- [WaveShare Display Pinout](https://www.waveshare.com/img/devkit/LCD/1.5inch-RGB-OLED-Module/1.5inch-RGB-OLED-Module-details-3.jpg)
- [XIAO ESP32S3 Pinout](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/2.jpg)

```
ESP32S3 XIAO Pin Assignments:
├── Display (SSD1351) - SPI Interface
│   ├── VCC -> 3.3V (or 5V if PCB supports)
│   ├── GND -> GND
│   ├── DIN -> D10 (GPIO9) - MOSI/SPI Data
│   ├── SCK -> D8 (GPIO7) - SPI Clock  
│   ├── CS  -> D7 (GPIO44) - Chip Select
│   ├── DC  -> D6 (GPIO43) - Data/Command
│   └── RST -> D0 (GPIO1) - Reset
├── Accelerometer (ADXL345) - I2C Interface
│   ├── VCC -> 3.3V
│   ├── GND -> GND
│   ├── SDA -> D4 (GPIO5) - I2C Data
│   ├── SCL -> D5 (GPIO6) - I2C Clock
│   └── INT -> D1 (GPIO2) - Interrupt for Deep Sleep Wake
├── Button Interface
│   ├── Input -> A3 (GPIO4) - with internal pull-up
│   └── Ground -> GND
└── Power (Optional LiPo Battery)
    ├── Positive -> BAT+ (Red wire)
    └── Negative -> BAT- (Black wire)
```

**Pin Reference Table:**
| XIAO Pin | GPIO | Function | Component |
|----------|------|----------|-----------|
| D0 | GPIO1 | RST | Display Reset |
| D1 | GPIO2 | INT | ADXL345 INT1 Interrupt |
| D4 | GPIO5 | SDA | ADXL345 I2C Data |
| D5 | GPIO6 | SCL | ADXL345 I2C Clock |
| D6 | GPIO43 | DC | Display Data/Command |
| D7 | GPIO44 | CS | Display Chip Select |
| D8 | GPIO7 | SCK | Display SPI Clock |
| D10 | GPIO9 | MOSI | Display SPI Data |
| A3 | GPIO4 | INPUT | Button (with pull-up) |

## System Architecture

### Operating Modes

**ESP Mode (Default Runtime)**
- Primary operational state for interactive use
- Continuous animation playback and motion monitoring
- ESP-NOW communication active for device pairing
- Automatic power state transitions based on activity
- Real-time response to user interactions

**Update Mode (Configuration & Maintenance)**
- Activated via Settings Menu
- Creates Wi-Fi access point: "BYTE90_Setup" (password: `00000000`)
- Web-based configuration interface at `192.168.4.1`
- Enables firmware updates, animation uploads, and system diagnostics
- Automatically restarts device after successful updates

### Power Management System

BYTE-90 implements progressive power management with four distinct states:

**Active State** - Full Operation
- Current draw: ~80-120mA
- All sensors and communication active
- Immediate response to interactions
- Full display brightness

**Display Dimming** - 30 Minutes of Inactivity
- Display brightness reduced to 25%
- All functionality remains active
- Motion detection continues normally

**Sleep Mode** - 1 Hour of Inactivity
- Sleep animations displayed
- Current draw: ~40-60mA
- Further display dimming
- Reduced sensor polling

**Deep Sleep** - 1.5 Hours of Inactivity
- 20-second countdown preperation time
- Current draw: <1mA
- **Only accelerometer interrupt active**
- Wake on tap or significant motion
- Essential for 2-day battery life

## Interactive Animation System

### Motion Detection & Responses

The BYTE-90 uses sophisticated motion detection with hardware-based tap detection and software-based motion analysis:

**Tap Detection** (Hardware ADXL345 Interrupt)
- **Single Tap**: 14.0G threshold, 30ms duration window
- **X/Y-axis taps**: Trigger acknowledgment animations
- **Hardware debouncing**: Prevents false triggers

**Double-Tap Detection** (Within 250ms Window)
- **X/Y-axis double-tap**: Trigger surprised or shocked animations
- **Latency window**: 100ms between tap detections

**Shake Detection** (Continuous Motion Analysis)
- **Threshold**: 8.0 m/s² average magnitude
- **Triggers**: Dizzy/confused animations
- **Lockout period**: 500ms after tap events to prevent false positives
- **Sample averaging**: Multiple readings for stability

**Orientation Detection** (Multi-Axis Tilt Sensing)
- **Full Tilt**: ±9.0 m/s² threshold for left/right orientation (±90°)
- **Half Tilt**: ±4.2 m/s² threshold for 45-degree angle detection
- **Upside Down**: Z-axis ≤ -8.0 m/s² (accounts for PCB mounting)
- **Triggers**: Crash animations, tumbling effects, recovery sequences

**Sudden Acceleration Detection** (Dynamic Movement)
- **Acceleration threshold**: 6.0 m/s² for rapid movement
- **Change threshold**: 4.0 m/s² difference between readings
- **Lockout protection**: 600ms cooldown after tap/shake events
- **Gravity compensation**: Removes static 9.8 m/s² component
- **Triggers**: Startle animations, surprise reactions

### BYTE-90 Device Pairing (Communication Mode) 

**Uses ESP-NOW Protocol for BYTE-90 Device Pairing:**
- **Activation**: Toggles via Settings Menu
- **Visual indicator**: Connection icon in top-right corner
- **Range**: Up to 200 meters in open space
- **Latency**: <50ms for animation triggers
- **Auto-discovery**: Automatic pairing with nearby BYTE-90 devices
- **Conversations**: Paired devices engage in animated sequences
- **Multi-device**: Support for multiple simultaneous connections

## User Interface & Controls

### Button Operations
- **Single Click**: Toggles Settings Menu
- **Double Click**: Enters or Selects menu setting
- **Long Press (3+ seconds)**: Enter Update Mode for configuration

### Motion Interactions
- **Single Tap**: 
  - X/Y-axis: Acknowledgment animations
- **Double Tap**: 
  - Z-axis: Toggle CRT glitch effects
  - X/Y-axis: Shocked/surprised animations
- **Shake**: Dizzy/confused animations
- **Tilt**: Crash and recovery animations based on angle
- **Sudden Movement**: Startle/surprise reactions

## Configuration & Updates

### Web Interface Access

1. **Enter Update Mode**: Hold button for 3+ seconds
2. **Connect to Wi-Fi**: Join "BYTE90_Setup" network (password: `00000000`)
3. **Access Interface**: Navigate to `http://192.168.4.1`
4. **Configure**: Set up Wi-Fi, upload firmware/animations
5. **Exit**: Single button press returns to normal operation

**Windows 11 compatibility**: Windows 11 has a compatibility issue with DHCP access points, it fails or takes a long time to assign IP preventing connection to the Web Interface. A workaround is to manually assign the IP address once connected or use an iOS or Android device.

### Over-the-Air Updates
- **Firmware updates**: Upload `.bin` files via web interface
- **Animation packages**: Upload animation `.bin` files
- **Automatic validation**: File integrity and format verification
- **Rollback protection**: Safe update process with error recovery

## Development & Building

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- Git version control
- Python 3.7+ for build tools
- USB-C cable for programming and debugging

### Required Libraries
```ini
lib_deps = 
    bitbank2/AnimatedGIF@^2.1.1                    # GIF decoding
    adafruit/Adafruit GFX Library@^1.11.11         # Graphics
    adafruit/Adafruit SSD1351 library@^1.3.2       # Display driver
    adafruit/Adafruit ADXL345@^1.3.4               # Accelerometer
```

### Build Configuration
```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200
build_flags = 
	-DCORE_DEBUG_LEVEL=5
board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
lib_deps = 
	bitbank2/AnimatedGIF@2.1.1
	adafruit/Adafruit GFX Library@^1.11.11
	adafruit/Adafruit SSD1351 library@^1.3.2
	adafruit/Adafruit ADXL345@^1.3.4
```

### Animation Asset Requirements
- **Resolution**: Exactly 128×128 pixels
- **Frame rate**: 16 FPS recommended
- **Color depth**: 8-bit indexed color (256 colors max)
- **Format**: Optimized GIF with LZW compression [Use EZgif.com](https://ezgif.com/)

## Device Modes

### Personality Modes
Configure different retro computing aesthetics to call mode based animations:

- **BYTE_MODE (Default)** - Original BYTE-90 character designs
- **MAC_MODE** - Retro Macintosh-inspired Happy Mac expressions  
- **PC_MODE** - Retro PC Happy Mac expressions

## Troubleshooting

### Common Issues

**Device Not Responding**
- **Charge first**: New devices need 1-2 hours initial charging
- **Reset**: Press "Reset" button on Xiao ESP32-S3 board
- **USB power**: Try direct USB-C power without battery

**Random Restarts or Freeze**
- **Cause**: Critically low battery
- **Solution**: Charge immediately for 1-2 hours
- **Hardware Reset**: Press reset button on board

**Update Mode Issues**
- **Windows 11**: Known compatibility issue with ESP32 access points
- **Workaround**: Use macOS/iOS devices or manual IP assignment
- **Check**: Ensure "BYTE90_Setup" network is visible
- **Password**: `00000000` (eight zeros)
- **Alternative**: Use USB flashing method via PlatformIO (Development knowledge required)

**Motion Detection Problems**
- **Calibration**: Place on flat surface to check baseline readings
- **Thresholds**: May require firmware modification for sensitivity
- **Lockouts**: Check if interactions suppressed by timing windows
- **FIFO buffer**: Verify 16-sample stream mode operation

## Frequently Asked Questions

**Q: How long does the battery last?**
A: Up to 2 days with intelligent power management and progressive sleep modes.

**Q: Can I check battery percentage?**
A: Currently not available due to hardware limitations. Feature planned for future releases.

**Q: How do I turn off BYTE-90?**
A: No power switch - device uses automatic sleep modes. For complete shutdown, disconnect battery.

**Q: Why do interactions seem delayed?**
A: Intelligent lockout periods prevent false triggers:
- 500ms after tap events
- 600ms after acceleration detection
- Priority system: shake > tap > acceleration > orientation

**Q: Can I modify the animations or sensitivity?**
A: Requires programming knowledge. Animations remain proprietary, but motion thresholds can be adjusted in firmware.

**Q: Can I create commercial products using this firmware?**
A: Yes, under GPL v3.0 terms, but you cannot use BYTE-90 branding or proprietary assets include 3D printing designs and Animations. See [Legal Guidelines](CONTRIBUTING.md) for details.

**Q: What's the difference between the open source firmware and commercial version?**
A: Open source includes core functionality; commercial version includes full hardware and proprietary animations with device support and exclusive access to new feature releases.

## Support & Community

### What We Support

- **Core firmware** functionality and documented bugs
- Hardware compatibility issues with **OFFICIAL BYTE-90 hardware**
- Basic troubleshooting for common issues

### What We Don't Support

- Custom animation development
- Hardware modifications or alternative component integration
- Third-party component compatibility
- Commercial deployment assistance or licensing guidance
- Extensive debugging of modified or forked firmware

**Official Resources:**
- [Support Page](https://labs.alxvtoronto.com/pages/support) - User guides and FAQ
- [GitHub Repository](https://github.com/alxv2016/Byte90-alxvlabs) - Open source firmware
- [ALXV Labs](https://labs.alxvtoronto.com/) - Product information
- [Legal & Contributing Guidelines](CONTRIBUTING.md) - Licensing and contribution rules

**Support Level**: Limited technical implementation support. Users expected to have Arduino/ESP32 development experience.

## Contributing

We welcome contributions from the community! Before contributing, please:

1. Read our [Legal & Contributing Guidelines](CONTRIBUTING.md)
2. Review the contribution guidelines for code quality standards
3. Test thoroughly on actual hardware before submitting
4. Follow our attribution requirements for derivative works

For bug reports, feature requests, and detailed contribution guidelines, see [Legal & Contributing Guidelines](CONTRIBUTING.md).

### Acknowledgements
- **Adafruit Industries**: Hardware libraries and sensor drivers
- **Bitbank2**: AnimatedGIF library for efficient GIF rendering
- **Espressif Systems**: ESP32 development framework and tools
- **Seeedstudio**: XIAO ESP32S3 development board design
- **Community Contributors**: Open source development and testing

## BYTE-90 Intellectual Property Rights

### Brand Protection Notice
**BYTE-90 is a brand name of ALXV Labs.** Use of the BYTE-90 name or branding in derivative projects requires explicit written permission. For more information on BYTE-90 Legal & Community Guidelines: [CONTRIBUTING.md](CONTRIBUTING.md)

### Protected Assets (© 2025 Alex Tieu Long Vong, ALXV Labs):
- BYTE-90 logo and brand identity
- All animations and character designs
- Visual identity and design elements
- 3D printed models and digital files
- Product packaging and marketing materials

### Prohibited Actions:
1. Distribution of BYTE-90 animations, designs, or 3D models
2. Modification and distribution of proprietary assets
3. Creating competing commercial products using **BYTE-90 branding**
4. Commercial exploitation without authorization using BYTE-90 branding
5. Sharing proprietary assets through other platforms or media
6. Creation of derivative works based on proprietary assets

### Permitted Fair Use:
- Educational discussion and analysis
- Non-commercial review and commentary
- Technical documentation referencing functionality
- Academic research and study

---

*Designed and developed by Alex Vong, ALXV LABS. This project represents the intersection of retro computing nostalgia and modern interactive design, creating a unique interactive designer toy experience for makers and collectors alike.*

**Version**: 1.0.1 | **Last Updated**: June 7, 2025 | **License**: GPL v3.0 | **Legal**: [CONTRIBUTING.md](CONTRIBUTING.md)
