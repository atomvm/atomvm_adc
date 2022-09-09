# AtomVM ADC Nif

This AtomVM Nif and Erlang library can be used to read ADC pins on the ESP32 SoC for any Erlang/Elixir programs targeted for AtomVM on the ESP32 platform.

This Nif is included as an add-on to the AtomVM base image.  In order to use this Nif in your AtomVM program, you must be able to build the AtomVM virtual machine, which in turn requires installation of the Espressif IDF SDK and tool chain.

The driver supports both adc interfaces. ADC1 is enabled by default, and unlike the limitation of Espressif's ESP-IDF you can set a different bit with for each channel on ADC1. To use ADC2 it must be enabled in the esp-idf menuconfig setting ```Component config -> ATOMVM_ADC Configuration``` menu. ADC1 can be used freely but ADC2 is used by the Wi-Fi module, and it cannot be used for adc tasks while wifi is active. To ensure that wifi is not interfered with you should use `adc:wifi_acquire/0` before you start the wifi in your application. Wi-Fi can then be stopped by using `adc:wifi_release/0` and you will the be free to use adc2 for reading voltages from adc2. You will need to start the pins (with optional configurations) again before reading, because all of the adc2 processes are stopped when using adc:wifi_acquire.

> Note. Some boards use some of the adc2 pins for other purposes, `ESP-WROVER-KIT: GPIOs 0, 2, 4 and 15`, and `ESP32 DevKitC: GPIO 0` are just two examples. Check the documentation for your board, as always!

* ADC1 pins
    - GPIOs 32-39
* ADC2 pins
    - GPIOs 0, 2, 4, 12-15, 25-27


For more information about the Analog Digital Converter on the ESP32, see the [IDF SDK Documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.2/api-reference/peripherals/adc.html)

Documentation for this component can be found in the following sections:

* [Programmer's Guide](markdown/adc.md)
* [Example Program](examples/adc_example/README.md)
