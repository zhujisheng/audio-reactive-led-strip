[中文](README-Chinese.md)

# Music LED Strip
Real-time LED strip music visualization running on ESP32, based on Arduino or [ESPHome](https://esphome.io/) platform（ESPHome can connect to [HomeAssistant](https://www.home-assistant.io) easily）.

The Microphone Shield and Nodemcu 32s, can also be used as a remote microphone 
of a intelligent audio device. Some projects related:

- [Home Assistant Add-on: voice_assistant](https://github.com/zhujisheng/hassio-addons/tree/master/voice_assistant)
- [remote microphone](https://github.com/zhujisheng/Home-Assistant-DIY/blob/master/%E5%8F%82%E8%80%83%E6%96%87%E6%A1%A3%EF%BC%8821-30%EF%BC%89/24.%E8%BF%9C%E7%A8%8B%E9%BA%A6%E5%85%8B%E9%A3%8E.pdf)

## Effect Show
<img src="images/music_led_strip.gif" width="550">

## Hardware
#### Hardware needed
- Nodemcu 32S
- [Microphone Shield of HAChina](https://item.taobao.com/item.htm?spm=2013.1.0.0.79a151cb3dCJf5&id=592861974901&scm=1007.12144.95220.42296_0_0&pvid=66038172-197b-4d87-a5ee-a8d90ff6b8a0&utparam=%7B%22x_hestia_source%22%3A%2242296%22%2C%22x_object_type%22%3A%22item%22%2C%22x_hestia_subsource%22%3A%22default%22%2C%22x_mt%22%3A0%2C%22x_src%22%3A%2242296%22%2C%22x_pos%22%3A1%2C%22wh_pid%22%3A-1%2C%22x_pvid%22%3A%2266038172-197b-4d87-a5ee-a8d90ff6b8a0%22%2C%22scm%22%3A%221007.12144.95220.42296_0_0%22%2C%22x_object_id%22%3A592861974901%7D)
- Led Strip

Note:
- The FastLED libary is used by the program, so the led strip we support is [the same as FastLED](https://github.com/FastLED/FastLED#supported-led-chipsets)

#### Microphone Shield
The [Microphone Shield of Hachina](https://item.taobao.com/item.htm?spm=2013.1.0.0.79a151cb3dCJf5&id=592861974901&scm=1007.12144.95220.42296_0_0&pvid=66038172-197b-4d87-a5ee-a8d90ff6b8a0&utparam=%7B%22x_hestia_source%22%3A%2242296%22%2C%22x_object_type%22%3A%22item%22%2C%22x_hestia_subsource%22%3A%22default%22%2C%22x_mt%22%3A0%2C%22x_src%22%3A%2242296%22%2C%22x_pos%22%3A1%2C%22wh_pid%22%3A-1%2C%22x_pvid%22%3A%2266038172-197b-4d87-a5ee-a8d90ff6b8a0%22%2C%22scm%22%3A%221007.12144.95220.42296_0_0%22%2C%22x_object_id%22%3A592861974901%7D) includes a PDM microphone, an Touch Pad, post connectors(making connection to led strip easily).

The Microphone Shield can connect to  nodemcu-32s directly and easily.

<img src="images/mic-module-1.jpg" width="200"><img src="images/mic-module-2.jpg" width="200"><img src="images/mic-nodemcu32s.jpg" width="390">

Note:
- You can use your own microphone linked to ESP32 instead of the Microphone Shield
- The software only support PDM microphone. If you use I2S microphone, you should modify the program(I2s is supported by ESP32-IDF)


#### Phisical link

<img src="images/hardware-connection.JPG" width="550">

## How to use on ESPHome
##### copy file `music_leds_esphome.h` and directory `include` to the ESPHome's config directory
`git clone https://github.com/zhujisheng/audio-reactive-led-strip`

`cp -r audio-reactive-led-strip/includes/ ~/esphome_config/`

`cp audio-reactive-led-strip/music_leds_esphome.h ~/esphome_config/`

Note: You can use other commands or tools to download and copy.

##### Config ESPHome Yaml

** config led strip **

0. Generate the basic configuration yaml in ESPHome by wizard.
1. Add includes of `music_leds_esphome.h` in domain `esphome` (as example below)
2. Config the [fastled light](https://esphome.io/components/light/fastled.html)
3. Add `addressable_lambda` effect of the fastled light (as example below)
4. Compile and upload the firmware.
5. Config the esphome in HomeAssistant

Note：
- When you upload the firmware, you should press the IO0 button on NodeMCU 32S
- ESP32 may can't start up under poor power supply.
- If the number of LEDS is not 60, please modify the `num_leds` in ESPHome's configuration YAML, and `N_PIXELS` in `music_leds_esphome.h` meanwhile.

```yaml
esphome:
  name: ......
  platform: ESP32
  board: ......
  includes:
    - music_leds_esphome.h
    - includes/FFT.h
    - includes/ExpFilter.h
    - includes/VisualEffect.h
    - includes/gaussian_filter1d.h

......

light:
  - platform: fastled_clockless
#  - platform: fastled_spi
    id: LedsStrip
    chipset: NEOPIXEL
    #chipset: APA102
    pin: GPIO21
    #data_pin: GPIO21
    #clock_pin: GPIO17
    num_leds: 60
    #rgb_order: BGR
    name: "MUSIC LEDS"
    effects:
      - addressable_rainbow:
      - addressable_lambda:
          name: Scroll with Music
          update_interval: 0s
          lambda: |-
            music_leds.ShowFrame(MODE_SCROLL, &it);
      - addressable_lambda:
          name: Energy with Music
          update_interval: 0s
          lambda: |-
            music_leds.ShowFrame(MODE_ENERGY, &it);
      - addressable_lambda:
          name: Spectrum with Music
          update_interval: 0s
          lambda: |-
            music_leds.ShowFrame(MODE_SPECTRUM, &it);
```

** Config TouchPad **

Configuration below is for the TouchPad on the microphone shield.
you can find [here](https://esphome.io/components/binary_sensor/esp32_touch.html), more about the TouchPad of ESP32. 

```yaml
esp32_touch:
#  setup_mode: True
binary_sensor:
  - platform: esp32_touch
    name: "Touch Pad on ESP32"
    pin: GPIO32
    threshold: 1000
    on_press:
      then:
        - light.toggle: LedsStrip
```

## How to use in Arduino
##### Install ESP32 Board in Arduino

Add ESP32's package url in `Additional Boards Manager URLs` of the menu `Preferences`:
`https://dl.espressif.com/dl/package_esp32_index.json`
<img src="images/arduino-esp32-1.png" width="550">

Open `Boards manager...` in menu `Tools`. Search `ESP32`, then install it.
<img src="images/arduino-esp32-2.png" width="550">

##### Install Fastled library in Arduino

Search and install `FastLED` in menu `Sketch`/`Include Library`/`Manage Libraries...`.
<img src="images/arduino-fastled.JPG" width="550">

##### Clone the project
`git clone https://github.com/zhujisheng/audio-reactive-led-strip`

##### Compile&Upload
1. Open `audio-reactive-led-strip.ino` in Arduino
2. Connect to ESP32 by the USB
3. Select the correct `Board` and `Port` in the menu `Tools`<img src="images/arduino-esp32-3.png" width="550">
4. Upload
5. you should modify `N_PIXELS` and the calling method of `FastLED.addLeds` in file`audio-reactive-led-strip.ino`, if you want to use led strip with not 60 leds on it, or it's SPI connectted.


## Thanks
[https://github.com/scottlawsonbc/audio-reactive-led-strip](https://github.com/scottlawsonbc/audio-reactive-led-strip)

[Zack-Xu](https://github.com/Zack-Xu), designer of the microphone shield PCB.