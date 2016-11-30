Arduino M48T58 programmer.
Copyright 2016 Stephane Albert

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

#include <stdlib.h>

#define MAX_ADDR = 0x2000

#define USE_EXPANDER 1
#define EXPANDER_ADDR 0x20

#if USE_EXPANDER == 0
// Arduino Mega
#define W 23
#define E1 35
#define E2 46
#define G 31
#else
// MCP23017
#define W 2
#define E1 3
#define E2 4
#define G 5
#endif

#if USE_EXPANDER == 0
// [A0; A12] Address pins of the M48
unsigned int addressPins[] = {38, 36, 34, 32, 30, 28, 26, 24, 25, 27, 33, 29, 22};
// [DQ0; DQ7] Data pins of the M48T58/M48Y58
unsigned int dataPins[] = {40, 42, 44, 45, 43, 41, 39, 37};
#else
unsigned int dataPins[] = {6, 7, 8, 9, 10, 11, 12, 13};

void initExpander(unsigned int i2cAddr) {
    Wire.beginTransmission(EXPANDER_ADDR);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission()
    Wire.beginTransmission(EXPANDER_ADDR);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.endTransmission()
}

void writeAddrWithExpander(unsigned int addr) {
    Wire.beginTransmission(EXPANDER_ADDR);
    Wire.write(0x12);
    Wire.write(addr & 0xFF);
    Wire.endTransmission()
    Wire.beginTransmission(EXPANDER_ADDR);
    Wire.write(0x13);
    Wire.write(addr >> 8 & 0xFF);
    Wire.endTransmission()
}
#endif

void initPins(unsigned int *pinAddr, int mode) {
    unsigned int *maxAddr = pinAddr + sizeof(pinAddr) / sizeof(*pinAddr) - 1;
    while (pinAddr < maxAddr) {
        pinMode(*pinAddr, mode);
        ++pinAddr;
    }
}

void setDisable() {
    digitalWrite(E2, LOW);
    digitalWrite(E1, HIGH);
    digitalWrite(W, HIGH);
    digitalWrite(G, HIGH);

}

void setRead() {
    digitalWrite(W, HIGH);
    digitalWrite(E1, LOW);
    digitalWrite(E2, HIGH);
    digitalWrite(G, LOW);
}

void setWrite() {
    digitalWrite(W, LOW);
    digitalWrite(E1, LOW);
    digitalWrite(E2, HIGH);
    digitalWrite(G, LOW);
}

void setup() {
    Serial.begin(9600);
    pinMode(W, OUTPUT);
    pinMode(E1, OUTPUT);
    pinMode(E2, OUTPUT);
    pinMode(G, OUTPUT);
    setDisable();
    #if USE_EXPANDER == 0
    initPins(addressPins, OUTPUT);
    #else
    initExpander(EXPANDER_ADDR);
    #endif
}

void setAddr(unsigned int addr) {
    #if USE_EXPANDER == 1
    writeAddrWithExpander(addr);
    #else
    for (unsigned int i=0; i < 13; ++i)
        digitalWrite(addressPins[i], (addr >> i) & 1);
    #endif
}

byte readData() {
    setRead();
    byte data = 0;
    for (byte i=0; i < 8; ++i)
        data |= digitalRead(dataPins[i]) << i;
    setDisable();
    return data
}

void writeData(byte data) {
    setWrite();
    for (byte i=0; i < 8; ++i)
        digitalWrite(dataPins[i], (data >> i) & 1);
    setDisable();
}

byte read(unsigned int addr) {
    setaddr(addr);
    return readdata();
}

void write(unsigned int addr, byte data) {
    setaddr(addr);
    writedata(data);
}

void writeSerial() {
    unsigned int bytesRead;
    unsigned int curAddr = 0;
    byte buffer[256];
    byte *bufferPos;
    initPins(dataPins, OUTPUT);
    // NOTE(sheeprine): We can read more than MAX_ADDR but we'll not write it.
    while(bytesRead = Serial.readBytes(buffer, 256) && curAddr < MAX_ADDR) {
        bufferPos = buffer;
        for (unsigned int maxAddr=curAddr + bytesRead; curAddr < maxAddr; ++curAddr) {
            write(curAddr, *bufferPos)
            ++bufferPos;
        }
    }
}

void readToSerial() {
    unsigned int curAddr = 0;
    byte readByte;
    initPins(dataPins, INPUT);
    while(curAddr < MAX_ADDR) {
        readByte = read(curAddr);
        Serial.print(readByte)
        ++curAddr;
    }
}

void setFromSerial() {
    unsigned int addr = 0;
    unsigned int nbBytes;
    byte byteToWrite;
    char buffer[5];
    nbBytes = Serial.readBytesUntil(buffer, 4, ';');
    buffer[nbBytes] = NULL;
    byteToWrite = Serial.read();
    addr = strtol(buffer, NULL, 16);
    write(addr, byteToWrite);
}

void processMenu() {
    char op;
    Serial.println("Arduino ready.");
    while(!Serial.available());
    op = Serial.read();
    switch (op) {
        case 'W':
            Serial.println("Ready to write.");
            writeSerial();
            break;
        case 'D':
            Serial.println("Ready to dump.");
            readSerial();
            break;
        case 'S':
            setFromSerial();
            break;
        default:
            Serial.println("Not implemented.");
    }
}

void loop() {
    processMenu();
    delay(1000);
}
