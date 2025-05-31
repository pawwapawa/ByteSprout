# BYTE-90: A Retro PC and MAC Inspired Interactive Designer Art Toy
**Open source coming soon**

BYTE-90 is a retro PC and Mac inspired interactive designer art toy that displays animated emotes through various interactions. It detects motion, responds to taps and orientation changes, pairs and communicates with other BYTE-90 devices to exchange animated conversations.

Designed and 3D printed by ALXV LABS (Alex Vong) as a designer toy, BYTE-90 is available as either a dev-kit or a collectible designer toy. This product is a limited run, and your support enables continued future development.

While the base software is open-source, **the animations and 3D printed files remain proprietary** to maintain brand consistency and authenticity. You can purchase BYTE-90 that includes all hardware, software, and animations, or build your own using the off-the-shelf hardware specified below.

[BYTE-90 by ALXV Labs](https://labs.alxvtoronto.com/)

## Features

- **Interactive Animations**: Displays various animated GIFs through interactions and sequence events
- **Retro Screen Effects**: Choose from different vintage display effects including scanlines, dithering overlay, retro green, or classic orange CRT aesthetics
- **Motion Detection**: Advanced accelerometer-based detection for taps, double-taps, shakes, tilts, and orientation changes
- **Device-to-Device Communication**: Seamless pairing with other BYTE-90 devices via ESP-NOW protocol for animated conversations
- **Intelligent Power Management**: Progressive sleep modes with automatic inactivity detection to maximize battery life
- **Over-the-Air Updates**: Wireless firmware and animation updates via dedicated web interface

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

### Pin Configuration

**Critical Safety Note**: Always verify battery voltage is 3.7V (not 3.9V) and correct polarity alignment before installation.

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
│   ├── SCL -> D5 (GPIO4) - I2C Clock
│   └── INT -> D1 (GPIO1) - Interrupt for Deep Sleep Wake
├── Button Interface
│   ├── Input -> A3 (GPIO3) - with internal pull-up
│   └── Ground -> GND
└── Power (Optional LiPo Battery)
    ├── Positive -> BAT+ (Red wire)
    └── Negative -> BAT- (Black wire)
```

**Pin Reference Table:**
| XIAO Pin | GPIO | Function | Component |
|----------|------|----------|-----------|
| D0 | GPIO1 | RST | Display Reset |
| D1 | GPIO1 | INT | ADXL345 Interrupt |
| D4 | GPIO5 | SDA | ADXL345 I2C Data |
| D5 | GPIO4 | SCL | ADXL345 I2C Clock |
| D6 | GPIO43 | DC | Display Data/Command |
| D7 | GPIO44 | CS | Display Chip Select |
| D8 | GPIO7 | SCK | Display SPI Clock |
| D10 | GPIO9 | MOSI | Display SPI Data |
| A3 | GPIO3 | INPUT | Button (with pull-up) |

## System Architecture

### Operating Modes

**ESP Mode (Default Runtime)**
- Primary operational state for interactive use
- Continuous animation playback and motion monitoring
- ESP-NOW communication active for device pairing
- Automatic power state transitions based on activity
- Real-time response to user interactions

**Update Mode (Configuration & Maintenance)**
- Activated via long button press (3+ seconds)
- Creates Wi-Fi access point: "BYTE90_Setup" (password: `00000000`)
- Web-based configuration interface at `192.168.4.1`
- Enables firmware updates, animation uploads, and system diagnostics
- Automatically restarts device after successful updates

### Power Management System

The BYTE-90 implements progressive power management with four distinct states:

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
- 20-second countdown warning with device mode display
- Current draw: <1mA
- Only accelerometer interrupt active
- Wake on tap or significant motion
- Essential for 2-day battery life

## Interactive Animation System

### Motion Detection & Responses

The BYTE-90 uses sophisticated motion detection with hardware-based tap detection and software-based motion analysis:

**Tap Detection** (Hardware ADXL345 Interrupt)
- **Single Tap**: 14.0G threshold, 30ms duration window
- **Z-axis taps**: Cycle visual effects (scanlines, dithering, CRT modes)
- **X/Y-axis taps**: Trigger acknowledgment animations
- **Hardware debouncing**: Prevents false triggers

**Double-Tap Detection** (Within 250ms Window)
- **Z-axis double-tap**: Toggle CRT glitch effects on/off
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

### Device Communication

**ESP-NOW Protocol for Device-to-Device Communication:**
- **Activation**: Single button press toggles Communication Mode
- **Visual indicator**: Connection icon in top-right corner
- **Range**: Up to 200 meters in open space
- **Latency**: <50ms for animation triggers
- **Auto-discovery**: Automatic pairing with nearby BYTE-90 devices
- **Conversations**: Paired devices engage in animated sequences
- **Multi-device**: Support for multiple simultaneous connections

## User Interface & Controls

### Button Operations
- **Single Click**: Toggle Communication Mode (ESP-NOW) on/off
- **Long Press (3+ seconds)**: Enter Update Mode for configuration
- **Double Click**: Force enter deep sleep mode

### Motion Interactions
- **Single Tap**: 
  - Z-axis: Cycle visual effects (scanlines, dithering, CRT modes)
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

### Required Libraries
```ini
lib_deps = 
    bitbank2/AnimatedGIF@^2.1.1                    # GIF decoding
    adafruit/Adafruit GFX Library@^1.11.11         # Graphics
    adafruit/Adafruit SSD1351 library@^1.3.2       # Display driver
    adafruit/Adafruit ADXL345@^1.3.4               # Accelerometer
    arduino-libraries/ArduinoHttpClient@^0.4.0     # OTA updates
```

### Build Configuration
```ini
[env:seeed_xiao_esp32s3]
platform = espressif32@^6.0.0
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
board_build.flash_mode = qio
board_build.f_cpu = 240000000L
```

### Animation Assets
- **Resolution**: Exactly 128×128 pixels
- **Frame rate**: 16 FPS recommended
- **Color depth**: 8-bit indexed color (256 colors max)
- **Format**: Optimized GIF with LZW compression

## Device Modes

### Personality Themes
Configure different retro computing aesthetics:

**BYTE_MODE (Default)** - Original BYTE-90 character designs
**MAC_MODE** - Classic Macintosh-inspired interface and expressions  
**PC_MODE** - DOS/Windows 95 era aesthetics and animations

## Troubleshooting

### Common Issues

**Device Not Responding**
- **Charge first**: New devices need 1-2 hours initial charging
- **Reset**: Press "Reset" button on circuit board
- **USB power**: Try direct USB-C power without battery

**Random Restarts**
- **Cause**: Critically low battery
- **Solution**: Charge immediately for 1-2 hours

**Update Mode Issues**
- **Windows 11**: Known compatibility issue with ESP32 access points
- **Workaround**: Use macOS/iOS devices or manual IP assignment
- **Check**: Ensure "BYTE90_Setup" network is visible
- **Password**: `00000000` (eight zeros)

**Motion Detection Problems**
- **Calibration**: Place on flat surface to check baseline readings
- **Thresholds**: May require firmware modification for sensitivity
- **Lockouts**: Check if interactions suppressed by timing windows
- **FIFO buffer**: Verify 16-sample stream mode operation

### Battery Safety
**Compatible Battery**: 1200mAh 3.7V lithium (103040 size) with PH 2.0 connector

**Critical Safety Checks**:
1. Verify 3.7V voltage (NOT 3.9V)
2. Check connector polarity alignment
3. Ensure battery is charged and functional
4. Handle connector carefully
5. Inspect for soldering damage

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

## Support & Community

**Official Resources:**
- [Support Page](https://labs.alxvtoronto.com/pages/support) - User guides and FAQ
- [GitHub Repository](https://github.com/alxv2016/Byte90-alxvlabs) - Open source firmware
- [ALXV Labs](https://labs.alxvtoronto.com/) - Product information

**Support Level**: Limited technical implementation support. Users expected to have Arduino/ESP32 development experience.

## Legal & Licensing

### Software License
**GNU General Public License v3.0** - Source code freely available for modification and redistribution with proper attribution.

### BYTE-90 Intellectual Property Rights

**Protected Assets (© 2025 Alex Vong, ALXV Labs):**
- BYTE-90 logo and brand identity
- All animations and character designs
- Visual identity and design elements
- 3D printed models and digital files

**Prohibited Actions:**
1. Distribution of BYTE-90 animations, designs, or 3D models
2. Modification of proprietary assets
3. Commercial exploitation without authorization
4. Sharing through other platforms or media
5. Creation of derivative works based on proprietary assets

### Acknowledgements
- **Adafruit Industries**: Hardware libraries and sensor drivers
- **Bitbank2**: AnimatedGIF library for efficient GIF rendering
- **Espressif Systems**: ESP32 development framework and tools
- **Seeedstudio**: XIAO ESP32S3 development board design

---

*Designed and developed by Alex Vong, ALXV LABS. This project represents the intersection of retro computing nostalgia and modern interactive design, creating a unique interactive designer toy experience for makers and collectors alike.*
