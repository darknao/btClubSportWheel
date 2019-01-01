#include <SPI.h>

#define CS 10
SPISettings settingsA(12000000, MSBFIRST, SPI_MODE0);
unsigned int CS_WAIT = 5;


void setup() {
  // put your setup code here, to run once:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();
  SPI.setClockDivider(0);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
uint8_t buf;

SPI.beginTransaction(settingsA);
digitalWrite(CS, LOW);
delayMicroseconds(CS_WAIT);
  for(int i=0; i<36; i++) {
    buf = SPI.transfer(0x00);
    Serial.print(buf < 16 ? "0" : "");
    Serial.print(buf, HEX);
    Serial.print(" ");
  }

digitalWrite(CS, HIGH);
SPI.endTransaction();
Serial.println();
delayMicroseconds(10);

delay(1000); // 1 sec sleep to avoid spamming the console
}
