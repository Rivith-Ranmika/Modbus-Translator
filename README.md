# Modbus-Translator
A Modbus-RTU protocol translator built with an ESP32 to convert DDS238 registers to DTSU666-H registers

This project use eModbus(_https://github.com/eModbus/eModbus_) library, inspired by salakrzy's(_https://github.com/salakrzy/DTSU666_CHINT_to_HUAWEI_translator_)

Very helpful for understanding DTSU666-H Modbus registers(_https://github.com/sitnikov/dtsu666-victron-meter-emulator_)

## Hardware Requirements
* ESP32 development board (I have used ESP32-S3 Super Mini)
* 2 of RS485 to TTL transceiver modules (MAX485)

## Features
* Stop replying when communication fails with DDS238 to stop reply old data
* Convert all 32-bit and 16-bit unsigned data to 32-bit float

_I have used a dedicated power supply, some indicator LEDs, and a box for outdoor use. Use only with a single-phase Huawei inverter; not recommended for export limitation. _
