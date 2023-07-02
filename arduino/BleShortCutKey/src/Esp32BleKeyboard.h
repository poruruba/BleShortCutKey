#ifndef _ESP32_BLE_KEYBOARD_H_
#define _ESP32_BLE_KEYBOARD_H_

#include <Arduino.h>

void Esp32BleKeyboard_begin(const char *p_deviceName = NULL, const char *p_manufacturerName = NULL, uint32_t passkey = 0);
bool Esp32BleKeyboard_isConnected();
void Esp32BleKeyboard_sendKeys(uint8_t mod, const uint8_t *keys, uint8_t num_key = 1);
void Esp32BleKeyboard_sendString(const char* ptr);

#endif
