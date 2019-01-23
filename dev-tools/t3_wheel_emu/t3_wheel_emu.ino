/*
* Teensy 3.x only!
*/

#define CS 10
#define dataLength 33

uint8_t returnData[dataLength] = {0xA5, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0x87,
                                  0x47, 0x7B, 0x9A, 0x79, 0xA0, 0xC9, 0xB5, 0xD8, 0x1D, 0x72, 0x20,
                                  0x2C, 0xFA, 0x62, 0xFF, 0x93, 0x04, 0xCB, 0x98, 0xD0, 0x12, 0xAF};
volatile uint32_t sequence;


void setup() {
  SIM_SCGC6 |= SIM_SCGC6_SPI0;
  
  SPI0_MCR = 0x00000000;
  SPI0_RSER = 0x00020000;
  CORE_PIN13_CONFIG = PORT_PCR_MUX(2);
  CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  CORE_PIN12_CONFIG = PORT_PCR_MUX(2);
  CORE_PIN10_CONFIG = PORT_PCR_MUX(2);
    
  SPI0_CTAR0_SLAVE = SPI_CTAR_FMSZ(7) | SPI_CTAR_CPHA;
  SPI0_MCR &= ~SPI_MCR_HALT & ~SPI_MCR_MDIS;
  attachInterrupt(digitalPinToInterrupt(CS),cableselect,RISING);

  sequence = 0;

}


void loop() {

  while ((SPI0_SR & SPI_SR_TFFF)) {
    //noInterrupts();
    SPI0_PUSHR_SLAVE = returnData[sequence++];
    //interrupts();
    SPI0_SR |= SPI_SR_TFFF;
  }
  if (sequence >= dataLength)
    sequence = 0;
  
}

void cableselect() {
  SPI0_MCR |= SPI_MCR_HALT;
  while (SPI0_SR & SPI_SR_TXRXS) {} 
  SPI0_MCR |= SPI_MCR_CLR_RXF + SPI_MCR_HALT;
  SPI0_MCR |= SPI_MCR_CLR_TXF + SPI_MCR_HALT;
  SPI0_MCR &= ~SPI_MCR_HALT; // Start the SPI module again
  while (!(SPI0_SR & SPI_SR_TXRXS)) {} // verify SPI module is running
  sequence = 0;
}

