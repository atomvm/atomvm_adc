# AtomVM ADC Nif

This AtomVM Nif and Erlang library can be used to read ADC pins on the ESP32 SoC for any Erlang/Elixir programs targeted for AtomVM on the ESP32 platform.

This Nif is included as an add-on to the AtomVM base image.  In order to use this Nif in your AtomVM program, you must be able to build the AtomVM virtual machine, which in turn requires installation of the Espressif IDF SDK and tool chain.

> Note.  Currently, this Nif only supports the IDF SDK ADC1 interface, which provides 8 channels (GPIOs 32-39).  There is currently no support for the ADC2 interface.

For more information about the Analog Digital Converter on the ESP32, see the [IDF SDK Documentation](https://docs.espressif.com/projects/esp-idf/en/v3.3.4/api-reference/peripherals/adc.html)

Documentation for this Nif can be found in the following sections:

* [Build Instructions](doc/build.md)
* [Programmer's Guide](doc/guide.md)
* [Example Program](doc/example.md)
