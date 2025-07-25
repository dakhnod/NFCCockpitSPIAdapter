#include <Arduino.h>
#include <SPI.h>
#include <map>

#ifdef WS2812_PIN
#include <FastLED.h>

CRGB leds[1];
#endif

bool uartBufferAvailable = false;

typedef std::map<uint8_t, void (*)(uint8_t, uint8_t, uint16_t)> classHandler;

uint8_t lastClass, lastInstruction;

uint8_t stub[64];

std::map<uint8_t, int> pinMapping {
  {1, PIN_RESET},
  {2, PIN_IRQ},
  {5, PIN_BUSY},
  {6, PIN_DOWNLOAD}
};

void readSerialBuffer(uint8_t *buffer, int length) {
  for(int i = 0; i < length; i++) {
    buffer[i] = (uint8_t) SERIAL_PORT.read();
  }
}

void sendResponse(uint8_t apiCode, uint8_t componentCode, uint8_t *data=NULL, uint16_t length=0) {
  uint8_t result[length + 6];

  result[0] = lastClass;
  result[1] = lastInstruction;
  result[2] = apiCode;
  result[3] = componentCode;
  memcpy(result + 4, (uint8_t*)(&length), 2);
  memcpy(result + 6, data, length);

  SERIAL_PORT.write(result, length + 6);
  // SERIAL_PORT.flush();
};

void sendResponseByte(uint8_t apiCode, uint8_t componentCode, uint8_t data) {
  sendResponse(apiCode, componentCode, &data, 1);
}

void echoAscending(uint8_t parameter1, uint8_t parameter2, uint16_t length) {

}

void echoDescending(uint8_t parameter1, uint8_t parameter2, uint16_t length) {

}

void echoLengthAscending(uint8_t parameter1, uint8_t parameter2, uint16_t length) {
  uint8_t response[4]  = {
    (uint8_t) SERIAL_PORT.read(),
    0x00,
  };
  memcpy(response + 2, &length, 2);
  sendResponse(0, 0, response, sizeof(response));
}

void echoIdentity(uint8_t parameter1, uint8_t parameter2, uint16_t length) {
  uint8_t buf[length];
  readSerialBuffer(buf, length);
  sendResponse(0, 0, buf, length);
}

void readGPIO(uint8_t index, uint8_t, uint16_t) {
  uint8_t value = digitalRead(pinMapping[index]);
  sendResponse(0, 0, &value, 1);
};

void writeGPIO(uint8_t index, uint8_t value, uint16_t) {
  digitalWrite(pinMapping[index], value);
  sendResponse(0, 0);
}

void waitForHigh(uint8_t index, uint8_t, uint16_t) {
  int pin = pinMapping[index];
  while(!digitalRead(pin));
  sendResponse(0, 0);
}

void waitForLow(uint8_t index, uint8_t, uint16_t) {
  int pin = pinMapping[index];
  while(digitalRead(pin));
  sendResponse(0, 0);
}

void readLibraryString(uint8_t, uint8_t, uint16_t) {
  char firmware[] = "0.1 by Daniel Dakhno";
  sendResponse(0, 0, (uint8_t*) firmware, sizeof(firmware));
}

void readFirmwareString (uint8_t, uint8_t, uint16_t) {
  char firmware[] = "0.1 by Daniel Dakhno";
  sendResponse(0, 0, (uint8_t*) firmware, sizeof(firmware));
}

void readFirmwareDate(uint8_t, uint8_t, uint16_t) {
  char firmware[] = __DATE__;
  sendResponse(0, 0, (uint8_t*) firmware, sizeof(firmware));
}

void awaitBusyState(int state) {
  while(digitalRead(PIN_BUSY) != state);
}

void transmitSPI(uint8_t *data, int length, uint8_t *response, int response_length) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  if(length > 0) {
    FastLED.showColor(CRGB::Red);

    awaitBusyState(LOW);

    digitalWrite(PIN_SS, LOW);

    delay(2);

    for(int i = 0; i < length; i++) {
      SPI.transfer(data[i]);
    }

    // awaitBusyState(HIGH);

    digitalWrite(PIN_SS, HIGH);

    awaitBusyState(LOW);
  }

  if(response_length == 0) {
    FastLED.showColor(CRGB::Black);
    SPI.endTransaction();
    return;
  }

  FastLED.showColor(CRGB::Green);

  memset(response, 0xFF, response_length);

  digitalWrite(PIN_SS, LOW);

  // SPI.transferBytes(stub, response, response_length);
  for(int i = 0; i < response_length; i++) {
    response[i] = SPI.transfer(0xFF);
  }

  // awaitBusyState(HIGH);

  digitalWrite(PIN_SS, HIGH);

  awaitBusyState(LOW);

  SPI.endTransaction();
  digitalWrite(PIN_SS, HIGH);
  
  FastLED.showColor(CRGB::Black);
}

void transmit(uint8_t, uint8_t, uint16_t length) {
  uint8_t buffer[length];

  readSerialBuffer(buffer, length);

  transmitSPI(buffer, length, NULL, 0);

  sendResponse(0, 0);
}

void transceive(uint8_t arg1, uint8_t arg2, uint16_t length) {
  uint8_t buffer[length];
  if(length > 0) {
    readSerialBuffer(buffer, length);
  }
  
  uint16_t responseLength = arg1 | (arg2 << 8);
  uint8_t response[responseLength];
  
  transmitSPI(buffer, length, response, responseLength);

  sendResponse(0, 0, response, responseLength);
}

void receive(uint8_t arg1, uint8_t arg2, uint16_t length) {
  transceive(arg1, arg2, 0);
}

std::map<uint8_t, classHandler> commandMap {
  {
    0x01, { // transceive
    {
      {0x05, transmit},
      {0x0E, receive},
      {0xFD, transceive},
    }
  }},
  {
    0x10, { // GPIO
      {0x0E, readGPIO},
      {0x05, writeGPIO},
      {0xA1, waitForHigh},
      {0xA0, waitForLow},
    }
  },
  {
    0xFF, { // Configuration

    }
  },
  {
    0x0E, { // Versioning
      {0x01, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // library major
      {0x02, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 1); }}, // library minor
      {0x03, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 1); }}, // dev something,
      {0x04, readLibraryString},

      {0x11, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // firmware major
      {0x12, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // firmware minor
      {0x13, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 1); }}, // firmware dev
      {0x14, readFirmwareString},
      {0x15, readFirmwareDate},

      {0x20, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 1); }}, // reader type
      {0x30, [](uint8_t, uint8_t, uint16_t){ char board[] = "ESP32"; sendResponse(0, 0, (uint8_t*) board, sizeof(board)); }},
    }
  },
  {
    0x1B, {
      {0x0A, echoAscending},
      {0x0D, echoDescending},
      {0x1A, echoLengthAscending},
      {0xE0, echoIdentity},
  }},
  {    
    0x5e, { // Task management
      {0x0a, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // stop task
      {0xc0, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // get task count
      {0xca, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 1); }}, // isUpdatetable
   }
  },
  {
    0xc0, { // Configuration
      {0xeb, [](uint8_t, uint8_t, uint16_t){ uint8_t response[] = {0x01, 0x00}; sendResponse(0x00, 0x00, response, 2); }}, // Config_INS_GetBALType
      {0xbe, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // Config_INS_WaitBeforeRX_Strategy
      {0xb5, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // Config_INS_WaitBeforeTX_Strategy
      {0xe5, [](uint8_t, uint8_t, uint16_t){ sendResponse(0x21, 0xF1, NULL, 0); }}, // Config_INS_GetSupportedFrameSize
      {0xef, [](uint8_t, uint8_t, uint16_t){ uint8_t response[] = {0x01, 0x08}; sendResponse(0x00, 0x00, response, 2); }}, // Config_INS_GetFrameSize
      {0x14, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // Config_INS_ConfigIRQPollStrategy
      {0x15, [](uint8_t, uint8_t, uint16_t){ sendResponseByte(0, 0, 0); }}, // Config_INS_WaitIRQDelayWithTestBus
    }
  }
};

void uartOnReceive(){
  uartBufferAvailable = true;
}

void setup() {
  pinMode(PIN_BUSY, INPUT);
  pinMode(PIN_IRQ, INPUT);
  pinMode(PIN_DOWNLOAD, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_SS, OUTPUT);

  digitalWrite(PIN_RESET, 0);
  delay(100);
  digitalWrite(PIN_RESET, 1);

  memset(stub, 0xFF, sizeof(stub));

  #ifdef PIN_MOSI
  SPI.setMOSI(PIN_MOSI);
  #endif
  #ifdef PIN_MISO
  SPI.setMISO(PIN_MISO);
  #endif
  #ifdef PIN_SCK
  SPI.setSCK(PIN_SCK);
  #endif
  SPI.begin();

  #ifdef PIN_RX
  SERIAL_PORT.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
  #else
  SERIAL_PORT.begin(115200);
  #endif

  #ifdef WS2812_PIN
  FastLED.addLeds<WS2812, WS2812_PIN, GRB>(leds, 1);
  FastLED.showColor(CRGB::Black);
  #endif
}

void error(){
  for(;;) {
    #ifdef WS2812_PIN
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(100);
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(100);
    #endif
  }
}

void loop() {
  if(!SERIAL_PORT.available()){
    return;
  }

  delay(1);

  lastClass = SERIAL_PORT.read();
  lastInstruction = SERIAL_PORT.read();

  uint8_t parameter1 = SERIAL_PORT.read();
  uint8_t parameter2 = SERIAL_PORT.read();

  uint16_t payloadLength = 0;


  payloadLength |= SERIAL_PORT.read();
  payloadLength |= SERIAL_PORT.read() << 8;

  if(payloadLength != SERIAL_PORT.available()) {
    // error with payload length
    error();
  }

  auto handler = commandMap.find(std::forward<uint8_t>(lastClass));

  if(handler == commandMap.end()) {
    // error finding class
    error();
  }

  auto insHandler = handler->second.find(std::forward<uint8_t>(lastInstruction));

  if(insHandler == handler->second.end()) {
    // error finding instruction
    error();
  }

  insHandler->second(parameter1, parameter2, payloadLength);
}
