#include <Arduino.h>
#include <SPIFFS.h>
#include <M5Unified.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include "Esp32BleKeyboard.h"

#include "endpoint_packet.h"
#include "file_utils.h"
#include "wifi_utils.h"
#include "main_conf.h"

#define BLE_PASSKEY 123456

#define LED_PIN 10
#define IR_SEND_PIN  32
#define IR_RECV_PIN  33

IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
uint16_t *p_ir_send_data = NULL;
int ir_send_data_length = 0;
bool irStartReceive = false;
bool irReceiving = false;
bool irReceived = false;
bool irStartSend = false;

void ir_dump(decode_results *results);
void lcd_println(const char *p_message, bool append = false);

static bool nowBleConnected = false;

enum OPERATION_MODE {
  MODE_HID = 0,
  MODE_EDIT
};
enum OPERATION_MODE mode = MODE_HID;

void debug_dump(const uint8_t *p_bin, uint16_t len){
  for( uint16_t i = 0 ; i < len ; i++ ){
    Serial.print((p_bin[i] >> 4) & 0x0f, HEX);
    Serial.print(p_bin[i] & 0x0f, HEX);
  }
  Serial.println("");
}

static JsonObject find_ir_value(uint64_t value){
  long ret = file_load_json(DATA_JSON_FNAME);
  if( ret != 0 )
    return JsonVariant();

  JsonArray list = jsonDoc.as<JsonArray>();
  for( int i = 0 ; i < list.size() ; i++ ){
    JsonObject item = list[i];
    uint32_t item_value = item["value"];
    if( item_value == value )
      return item;
  }

  return JsonVariant();
}

void lcd_println(const char *p_message, bool append){
  if( !append ){
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
  }
  M5.Lcd.println(p_message);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(2);
  
  Serial.println("setup start");
  lcd_println("Setup starting");

  long ret = file_init();
  if( ret != 0 ){
    Serial.println("file_init failed");
    while(1);
  }

  if( !SPIFFS.exists(DATA_JSON_FNAME) ){
    File fp = SPIFFS.open(DATA_JSON_FNAME, FILE_WRITE);
    if( !fp ){
        Serial.printf("SPIFFS.open failed\n");
        while(1);
    }else{
      fp.print("[]");
      fp.close();
    }
  }

  pinMode(LED_PIN, OUTPUT);

  if( M5.BtnA.isPressed() ){
    mode = MODE_EDIT;
    Serial.println("MODE_EDIT");
    digitalWrite(LED_PIN, LOW);

    wifi_try_connect(true);

    long ret = packet_initialize();
    if( ret != 0 ){
      Serial.println("packet_initialize failed");
      while(1);
    }
  }else{
    mode = MODE_HID;
    Serial.println("MODE_HID");
    digitalWrite(LED_PIN, HIGH);

    long ret = file_load_json(DATA_JSON_FNAME);
    if( ret != 0 ){
      Serial.println("file_load_json failed");
      while(1);
    }
    irsend.begin();
    Esp32BleKeyboard_begin(NULL, NULL, BLE_PASSKEY);
  }

  irrecv.enableIRIn();
  irrecv.pause();

  Serial.println("setup finished");
  lcd_println("Setup Finished");
  if( mode == MODE_EDIT ){
    uint32_t ipaddress = get_ip_address();
    M5.Lcd.printf("IP:%d.%d.%d.%d\n", (ipaddress >> 24) & 0xff, (ipaddress >> 16) & 0xff, (ipaddress >> 8) & 0xff, (ipaddress >> 0) & 0xff);
  }
}

void loop() {
  M5.update();

  if( mode == MODE_EDIT ){
    static unsigned long start_tim;

    if( irStartReceive ){
      irStartReceive = false;
      irReceived = false;
      irReceiving = true;
      start_tim = millis();
//      irrecv.enableIRIn();
      irrecv.resume();
    }else if( irReceiving ){
      if (irrecv.decode(&results)) {
        irrecv.pause();
        Serial.println("Received");
//        irrecv.disableIRIn();
        irReceived = true;
        ir_dump(&results);
        irReceiving = false;
      }else{
        unsigned long end_tim = millis();
        if( end_tim - start_tim >= 10000){
          irrecv.pause();
//          irrecv.disableIRIn();
          irReceiving = false;
          Serial.println("ir timeout");
        }
      }
    }else if( irStartSend ){
      irStartSend = false;
      irrecv.pause();
      irsend.sendRaw(p_ir_send_data, ir_send_data_length, 38);
      delay(100);
      irrecv.resume();
    }
  }else if( mode == MODE_HID ){
    if(Esp32BleKeyboard_isConnected) {
      if( !nowBleConnected ){
        Serial.println("Ble Connected");
        lcd_println("Ble Connected");
        nowBleConnected = true;
        irrecv.resume();
      }

      if (irrecv.decode(&results)) {
        Serial.println("irReceived");

        if( results.decode_type == decode_type_t::NEC && !results.repeat){
          Serial.printf("value=%d\n", results.value);
          JsonObject obj = find_ir_value(results.value);
          if( !obj.isNull() ){
            const char *p_key_type = obj["key_type"];
            if( strcmp(p_key_type, "text") == 0 ){
              const char *p_text = obj["text"];
              Serial.printf("text=%s\n", p_text);
              Esp32BleKeyboard_sendString(p_text);
              delay(200);
            }else
            if( strcmp(p_key_type, "key") == 0 ){
              uint8_t code = obj["code"];
              uint8_t mod = obj["mod"];
              Serial.printf("mod=%d code=%d\n", mod, code);
              Esp32BleKeyboard_sendKeys(mod, &code, 1);
              delay(200);
            }else{
              Serial.println("Unkown key_type");
            }
          }else{
            Serial.println("Not found");
          }
        }else{
          Serial.println("unknown decode_type_t");
        }
        irrecv.resume();
      }
    }else{
      if( nowBleConnected ){
        Serial.println("Ble Disonnected");
        lcd_println("Ble Disconnected");
        nowBleConnected = false;
        irrecv.pause();
      }
    }      
  }

  if( M5.BtnA.pressedFor(2000) ){
    Serial.println("Now Rebooting");
    digitalWrite(LED_PIN, HIGH);
    lcd_println("Now Rebooting");
    delay(2000);
    ESP.restart();
  }
  delay(1);
}

void ir_dump(decode_results *results) {
  // Dumps out the decode_results structure.
  // Call this after IRrecv::decode()
  uint16_t count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } else if (results->decode_type == RC5X) {
    Serial.print("Decoded RC5X: ");
  } else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  } else if (results->decode_type == RCMM) {
    Serial.print("Decoded RCMM: ");
  } else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->address, HEX);
    Serial.print(" Value: ");
  } else if (results->decode_type == LG) {
    Serial.print("Decoded LG: ");
  } else if (results->decode_type == JVC) {
    Serial.print("Decoded JVC: ");
  } else if (results->decode_type == AIWA_RC_T501) {
    Serial.print("Decoded AIWA RC T501: ");
  } else if (results->decode_type == WHYNTER) {
    Serial.print("Decoded Whynter: ");
  } else if (results->decode_type == NIKAI) {
    Serial.print("Decoded Nikai: ");
  }
  serialPrintUint64(results->value, 16);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): {");

  for (uint16_t i = 1; i < count; i++) {
    if (i % 100 == 0)
      yield();  // Preemptive yield every 100th entry to feed the WDT.
    if (i & 1) {
      Serial.print(results->rawbuf[i] * kRawTick, DEC);
    } else {
      Serial.print(", ");
      Serial.print((uint32_t) results->rawbuf[i] * kRawTick, DEC);
    }
  }
  Serial.println("};");
}
