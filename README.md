# Modbus-Translator
A Modbus protocol translator built with an ESP32 to convert DDS238 registers to DTSU666-H registers

This project use eModbus(https://github.com/eModbus/eModbus) library, inspired by salakrzy's(https://github.com/salakrzy/DTSU666_CHINT_to_HUAWEI_translator)

## Hardware Requirements
* ESP32 development board (I have use esp32-s3 super mini)
* 2 of RS485 to TTL transceiver modules (MAX485)

## Features
* Stop replying when communication fails with DDS238 to stop reply old data
* Convert all 32-bit and 16-bit unsigned data to 32-bit float
