#ifndef _LORA_HPP_INCLUDED
#define _LORA_HPP_INCLUDED

RTC_DATA_ATTR uint16_t sendCount = 0x0000;

#include "WLAN.hpp"
#include "STORAGE.hpp"

#include <SPI.h>
#include <mbedtls/md.h>

/// SPIModT and LoraDriver based on RadioLib (https://github.com/jgromes/RadioLib)
/// They are under:
/// MIT License
/// 
/// Copyright(c) 2018 Jan Gromeš
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this softwareand associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright noticeand this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
template<int pin_spimod_miso, int pin_spimod_mosi, int pin_spimod_sck, int pin_spimod_cs, int pin_spimod_int0, int pin_spimod_rst>
class SPIModT {
  #pragma region Constances
  // common status codes
  #define ERR_NONE                                      0 //No error, method executed successfully.
  #define ERR_UNKNOWN                                   -1 //There was an unexpected, unknown error. If you see this, something went incredibly wrong. Your Arduino may be possessed, contact your local exorcist to resolve this error.
  #define ERR_CHIP_NOT_FOUND                            -2 //Radio chip was not found during initialization. This can be caused by specifying wrong chip type in the constructor, (i.e. calling SX1272 constructor for SX1278 chip) or by a fault in your wiring (incorrect slave select pin).
  #define ERR_MEMORY_ALLOCATION_FAILED                  -3 //Failed to allocate memory for temporary buffer. This can be cause by not enough RAM or by passing invalid pointer.
  #define ERR_PACKET_TOO_LONG                           -4 //Packet supplied to transmission method was longer than limit.
  #define ERR_TX_TIMEOUT                                -5 //Timed out waiting for transmission finish.
  #define ERR_RX_TIMEOUT                                -6 //Timed out waiting for incoming transmission.
  #define ERR_CRC_MISMATCH                              -7 //The calculated and expected CRCs of received packet do not match. This means that the packet was damaged during transmission and should be sent again.
  #define ERR_INVALID_BANDWIDTH                         -8 //The supplied bandwidth value is invalid for this module.
  #define ERR_INVALID_SPREADING_FACTOR                  -9 //The supplied spreading factor value is invalid for this module.
  #define ERR_INVALID_CODING_RATE                       -10 //The supplied coding rate value is invalid for this module.
  #define ERR_INVALID_BIT_RANGE                         -11 //Internal only.
  #define ERR_INVALID_FREQUENCY                         -12 //The supplied frequency value is invalid for this module.
  #define ERR_INVALID_OUTPUT_POWER                      -13 //The supplied output power value is invalid for this module.
  #define PREAMBLE_DETECTED                             -14 //LoRa preamble was detected during channel activity detection. This means that there is some LoRa device currently transmitting in your channel.
  #define CHANNEL_FREE                                  -15 //No LoRa preambles were detected during channel activity detection. Your channel is free.
  #define ERR_SPI_WRITE_FAILED                          -16 //Real value in SPI register does not match the expected one. This can be caused by faulty SPI wiring.
  #define ERR_INVALID_CURRENT_LIMIT                     -17 //The supplied current limit value is invalid.
  #define ERR_INVALID_PREAMBLE_LENGTH                   -18 //The supplied preamble length is invalid.
  #define ERR_INVALID_GAIN                              -19 //The supplied gain value is invalid.
  #define ERR_WRONG_MODEM                               -20 //User tried to execute modem-exclusive method on a wrong modem. For example, this can happen when you try to change LoRa configuration when FSK modem is active.
  #define ERR_INVALID_NUM_SAMPLES                       -21 //The supplied number of RSSI samples is invalid.
  #define ERR_INVALID_RSSI_OFFSET                       -22 //The supplied RSSI offset is invalid.
  #define ERR_INVALID_ENCODING                          -23 //The supplied encoding is invalid.

  #define RADIOLIB_USE_SPI                              0x00
  #define RADIOLIB_INT_0                                0x01
  #pragma endregion

  public:
    void init(uint8_t interface, uint8_t gpio) {
      if (pin_spimod_rst != -1) {
        pinMode(pin_spimod_rst, OUTPUT);

        // perform reset
        digitalWrite(pin_spimod_rst, LOW);
        delay(10);
        digitalWrite(pin_spimod_rst, HIGH);
        delay(10);
      }
      // select interface
      switch (interface) {
      case RADIOLIB_USE_SPI:
        pinMode(pin_spimod_cs, OUTPUT);
        digitalWrite(pin_spimod_cs, HIGH);
        this->_spi->begin(pin_spimod_sck, pin_spimod_miso, pin_spimod_mosi, pin_spimod_cs);
        break;
      }

      // select GPIO
      switch (gpio) {
      /*case RADIOLIB_INT_NONE:
        break;*/
      case RADIOLIB_INT_0:
        pinMode(pin_spimod_int0, INPUT);
        break;
      }
    }

    void term() {
      // stop SPI
      this->_spi->end();
    }

    int getInt0() const { return(pin_spimod_int0); }

    int16_t SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb = 7, uint8_t lsb = 0, uint8_t checkInterval = 2) {
      if ((msb > 7) || (lsb > 7) || (lsb > msb)) {
        return(ERR_INVALID_BIT_RANGE);
      }

      uint8_t currentValue = this->SPIreadRegister(reg);
      uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
      uint8_t newValue = (currentValue & ~mask) | (value & mask);
      this->SPIwriteRegister(reg, newValue);

      // check register value each millisecond until check interval is reached
      // some registers need a bit of time to process the change (e.g. SX127X_REG_OP_MODE)
      uint32_t start = micros();
      uint8_t readValue = 0;
      while (micros() - start < (checkInterval * 1000)) {
        readValue = this->SPIreadRegister(reg);
        if (readValue == newValue) {
          // check passed, we can stop the loop
          return(ERR_NONE);
        }
      }

      return(ERR_SPI_WRITE_FAILED);
    }

    int16_t SPIgetRegValue(uint8_t reg, uint8_t msb = 7, uint8_t lsb = 0) {
      if ((msb > 7) || (lsb > 7) || (lsb > msb)) {
        return(ERR_INVALID_BIT_RANGE);
      }

      uint8_t rawValue = this->SPIreadRegister(reg);
      uint8_t maskedValue = rawValue & ((0b11111111 << lsb) & (0b11111111 >> (7 - msb)));
      return(maskedValue);
    }

    uint8_t SPIreadRegister(uint8_t reg) {
      uint8_t resp = 0;
      this->SPItransfer(this->SPIreadCommand, reg, NULL, &resp, 1);
      return(resp);
    }

    void SPIwriteRegister(uint8_t reg, uint8_t data) {
      this->SPItransfer(this->SPIwriteCommand, reg, &data, NULL, 1);
    }

    void SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes) {
      this->SPItransfer(this->SPIwriteCommand, reg, data, NULL, numBytes);
    }

    void SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes) {
      // start SPI transaction
      this->_spi->beginTransaction(this->_spiSettings);

      // pull CS low
      digitalWrite(pin_spimod_cs, LOW);

      // send SPI register address with access command
      this->_spi->transfer(reg | cmd);

      // send data or get response
      if (cmd == this->SPIwriteCommand) {
        for (size_t n = 0; n < numBytes; n++) {
          this->_spi->transfer(dataOut[n]);
        }
      }
      else if (cmd == this->SPIreadCommand) {
        for (size_t n = 0; n < numBytes; n++) {
          dataIn[n] = this->_spi->transfer(0x00);
        }
      }

      // release CS
      digitalWrite(pin_spimod_cs, HIGH);

      // end SPI transaction
      this->_spi->endTransaction();
    }

  private:
    SPIClass* _spi = new SPIClass();
    SPISettings _spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE0);
    uint8_t SPIreadCommand = 0b00000000;
    uint8_t SPIwriteCommand = 0b10000000;
};
typedef SPIModT<pin_lora_miso, pin_lora_mosi, pin_lora_sck, pin_lora_ss, pin_lora_di0, pin_lora_rst> SPIMod;

class LoraDriver : SPIMod {
  #pragma region Constances
  // SX127x physical layer properties
  #define SX127X_CRYSTAL_FREQ                           32.0
  #define SX127X_DIV_EXPONENT                           19
  #define SX127X_MAX_PACKET_LENGTH                      255
  #define SX127X_MAX_PACKET_LENGTH_FSK                  64
  // SX127X_REG_SYNC_WORD
  #define SX127X_SYNC_WORD                              0x12        //  7     0     default LoRa sync word
  #define SX127X_SYNC_WORD_LORAWAN                      0x34        //  7     0     sync word reserved for LoRaWAN networks
  // SX127x series common LoRa registers
  #define SX127X_REG_FIFO                               0x00
  #define SX127X_REG_OP_MODE                            0x01
  #define SX127X_REG_FRF_MSB                            0x06
  #define SX127X_REG_FRF_MID                            0x07
  #define SX127X_REG_FRF_LSB                            0x08
  #define SX127X_REG_PA_CONFIG                          0x09
  #define SX127X_REG_PA_RAMP                            0x0A
  #define SX127X_REG_OCP                                0x0B
  #define SX127X_REG_LNA                                0x0C
  #define SX127X_REG_FIFO_ADDR_PTR                      0x0D
  #define SX127X_REG_FIFO_TX_BASE_ADDR                  0x0E
  #define SX127X_REG_FIFO_RX_BASE_ADDR                  0x0F
  #define SX127X_REG_FIFO_RX_CURRENT_ADDR               0x10
  #define SX127X_REG_IRQ_FLAGS_MASK                     0x11
  #define SX127X_REG_IRQ_FLAGS                          0x12
  #define SX127X_REG_RX_NB_BYTES                        0x13
  #define SX127X_REG_RX_HEADER_CNT_VALUE_MSB            0x14
  #define SX127X_REG_RX_HEADER_CNT_VALUE_LSB            0x15
  #define SX127X_REG_RX_PACKET_CNT_VALUE_MSB            0x16
  #define SX127X_REG_RX_PACKET_CNT_VALUE_LSB            0x17
  #define SX127X_REG_MODEM_STAT                         0x18
  #define SX127X_REG_PKT_SNR_VALUE                      0x19
  #define SX127X_REG_PKT_RSSI_VALUE                     0x1A
  #define SX127X_REG_RSSI_VALUE                         0x1B
  #define SX127X_REG_HOP_CHANNEL                        0x1C
  #define SX127X_REG_MODEM_CONFIG_1                     0x1D
  #define SX127X_REG_MODEM_CONFIG_2                     0x1E
  #define SX127X_REG_SYMB_TIMEOUT_LSB                   0x1F
  #define SX127X_REG_PREAMBLE_MSB                       0x20
  #define SX127X_REG_PREAMBLE_LSB                       0x21
  #define SX127X_REG_PAYLOAD_LENGTH                     0x22
  #define SX127X_REG_MAX_PAYLOAD_LENGTH                 0x23
  #define SX127X_REG_HOP_PERIOD                         0x24
  #define SX127X_REG_FIFO_RX_BYTE_ADDR                  0x25
  #define SX127X_REG_FEI_MSB                            0x28
  #define SX127X_REG_FEI_MID                            0x29
  #define SX127X_REG_FEI_LSB                            0x2A
  #define SX127X_REG_RSSI_WIDEBAND                      0x2C
  #define SX127X_REG_DETECT_OPTIMIZE                    0x31
  #define SX127X_REG_INVERT_IQ                          0x33
  #define SX127X_REG_DETECTION_THRESHOLD                0x37
  #define SX127X_REG_SYNC_WORD                          0x39
  #define SX127X_REG_DIO_MAPPING_1                      0x40
  #define SX127X_REG_DIO_MAPPING_2                      0x41
  #define SX127X_REG_VERSION                            0x42
  // SX127x series common FSK registers
  // NOTE: FSK register names that are conflicting with LoRa registers are marked with "_FSK" suffix
  #define SX127X_REG_BITRATE_MSB                        0x02
  #define SX127X_REG_BITRATE_LSB                        0x03
  #define SX127X_REG_FDEV_MSB                           0x04
  #define SX127X_REG_FDEV_LSB                           0x05
  #define SX127X_REG_RX_CONFIG                          0x0D
  #define SX127X_REG_RSSI_CONFIG                        0x0E
  #define SX127X_REG_RSSI_COLLISION                     0x0F
  #define SX127X_REG_RSSI_THRESH                        0x10
  #define SX127X_REG_RSSI_VALUE_FSK                     0x11
  #define SX127X_REG_RX_BW                              0x12
  #define SX127X_REG_AFC_BW                             0x13
  #define SX127X_REG_OOK_PEAK                           0x14
  #define SX127X_REG_OOK_FIX                            0x15
  #define SX127X_REG_OOK_AVG                            0x16
  #define SX127X_REG_AFC_FEI                            0x1A
  #define SX127X_REG_AFC_MSB                            0x1B
  #define SX127X_REG_AFC_LSB                            0x1C
  #define SX127X_REG_FEI_MSB_FSK                        0x1D
  #define SX127X_REG_FEI_LSB_FSK                        0x1E
  #define SX127X_REG_PREAMBLE_DETECT                    0x1F
  #define SX127X_REG_RX_TIMEOUT_1                       0x20
  #define SX127X_REG_RX_TIMEOUT_2                       0x21
  #define SX127X_REG_RX_TIMEOUT_3                       0x22
  #define SX127X_REG_RX_DELAY                           0x23
  #define SX127X_REG_OSC                                0x24
  #define SX127X_REG_PREAMBLE_MSB_FSK                   0x25
  #define SX127X_REG_PREAMBLE_LSB_FSK                   0x26
  #define SX127X_REG_SYNC_CONFIG                        0x27
  #define SX127X_REG_SYNC_VALUE_1                       0x28
  #define SX127X_REG_SYNC_VALUE_2                       0x29
  #define SX127X_REG_SYNC_VALUE_3                       0x2A
  #define SX127X_REG_SYNC_VALUE_4                       0x2B
  #define SX127X_REG_SYNC_VALUE_5                       0x2C
  #define SX127X_REG_SYNC_VALUE_6                       0x2D
  #define SX127X_REG_SYNC_VALUE_7                       0x2E
  #define SX127X_REG_SYNC_VALUE_8                       0x2F
  #define SX127X_REG_PACKET_CONFIG_1                    0x30
  #define SX127X_REG_PACKET_CONFIG_2                    0x31
  #define SX127X_REG_PAYLOAD_LENGTH_FSK                 0x32
  #define SX127X_REG_NODE_ADRS                          0x33
  #define SX127X_REG_BROADCAST_ADRS                     0x34
  #define SX127X_REG_FIFO_THRESH                        0x35
  #define SX127X_REG_SEQ_CONFIG_1                       0x36
  #define SX127X_REG_SEQ_CONFIG_2                       0x37
  #define SX127X_REG_TIMER_RESOL                        0x38
  #define SX127X_REG_TIMER1_COEF                        0x39
  #define SX127X_REG_TIMER2_COEF                        0x3A
  #define SX127X_REG_IMAGE_CAL                          0x3B
  #define SX127X_REG_TEMP                               0x3C
  #define SX127X_REG_LOW_BAT                            0x3D
  #define SX127X_REG_IRQ_FLAGS_1                        0x3E
  #define SX127X_REG_IRQ_FLAGS_2                        0x3F
  // SX127X_REG_HOP_PERIOD
  #define SX127X_HOP_PERIOD_OFF                         0b00000000  //  7     0     number of periods between frequency hops; 0 = disabled
  #define SX127X_HOP_PERIOD_MAX                         0b11111111  //  7     0
  // SX127x common LoRa modem settings
  // SX127X_REG_OP_MODE                                                 MSB   LSB   DESCRIPTION
  #define SX127X_FSK_OOK                                0b00000000  //  7     7     FSK/OOK mode
  #define SX127X_LORA                                   0b10000000  //  7     7     LoRa mode
  #define SX127X_ACCESS_SHARED_REG_OFF                  0b00000000  //  6     6     access LoRa registers (0x0D:0x3F) in LoRa mode
  #define SX127X_ACCESS_SHARED_REG_ON                   0b01000000  //  6     6     access FSK registers (0x0D:0x3F) in LoRa mode
  #define SX127X_SLEEP                                  0b00000000  //  2     0     sleep
  #define SX127X_STANDBY                                0b00000001  //  2     0     standby
  #define SX127X_FSTX                                   0b00000010  //  2     0     frequency synthesis TX
  #define SX127X_TX                                     0b00000011  //  2     0     transmit
  #define SX127X_FSRX                                   0b00000100  //  2     0     frequency synthesis RX
  #define SX127X_RXCONTINUOUS                           0b00000101  //  2     0     receive continuous
  #define SX127X_RXSINGLE                               0b00000110  //  2     0     receive single
  #define SX127X_CAD                                    0b00000111  //  2     0     channel activity detection
  // SX127X_REG_OCP
  #define SX127X_OCP_OFF                                0b00000000  //  5     5     PA overload current protection disabled
  #define SX127X_OCP_ON                                 0b00100000  //  5     5     PA overload current protection enabled
  #define SX127X_OCP_TRIM                               0b00001011  //  4     0     OCP current: I_max(OCP_TRIM = 0b1011) = 100 mA
  // SX127X_REG_MODEM_CONFIG_2
  #define SX127X_SF_6                                   0b01100000  //  7     4     spreading factor:   64 chips/bit
  #define SX127X_SF_7                                   0b01110000  //  7     4                         128 chips/bit
  #define SX127X_SF_8                                   0b10000000  //  7     4                         256 chips/bit
  #define SX127X_SF_9                                   0b10010000  //  7     4                         512 chips/bit
  #define SX127X_SF_10                                  0b10100000  //  7     4                         1024 chips/bit
  #define SX127X_SF_11                                  0b10110000  //  7     4                         2048 chips/bit
  #define SX127X_SF_12                                  0b11000000  //  7     4                         4096 chips/bit
  #define SX127X_TX_MODE_SINGLE                         0b00000000  //  3     3     single TX
  #define SX127X_TX_MODE_CONT                           0b00001000  //  3     3     continuous TX
  #define SX127X_RX_TIMEOUT_MSB                         0b00000000  //  1     0
  // SX127X_REG_DETECT_OPTIMIZE
  #define SX127X_DETECT_OPTIMIZE_SF_6                   0b00000101  //  2     0     SF6 detection optimization
  #define SX127X_DETECT_OPTIMIZE_SF_7_12                0b00000011  //  2     0     SF7 to SF12 detection optimization
  // SX127X_REG_DETECTION_THRESHOLD
  #define SX127X_DETECTION_THRESHOLD_SF_6               0b00001100  //  7     0     SF6 detection threshold
  #define SX127X_DETECTION_THRESHOLD_SF_7_12            0b00001010  //  7     0     SF7 to SF12 detection threshold
  // SX127X_REG_PA_CONFIG
  #define SX127X_PA_SELECT_RFO                          0b00000000  //  7     7     RFO pin output, power limited to +14 dBm
  #define SX127X_PA_SELECT_BOOST                        0b10000000  //  7     7     PA_BOOST pin output, power limited to +20 dBm
  #define SX127X_OUTPUT_POWER                           0b00001111  //  3     0     output power: P_out = 2 + OUTPUT_POWER [dBm] for PA_SELECT_BOOST
                                                                    //                            P_out = -1 + OUTPUT_POWER [dBm] for PA_SELECT_RFO
  // SX127X_REG_PA_DAC
  #define SX127X_PA_BOOST_OFF                           0b00000100  //  2     0     PA_BOOST disabled
  #define SX127X_PA_BOOST_ON                            0b00000111  //  2     0     +20 dBm on PA_BOOST when OUTPUT_POWER = 0b1111
  // SX127X_REG_LNA
  #define SX127X_LNA_GAIN_1                             0b00100000  //  7     5     LNA gain setting:   max gain
  #define SX127X_LNA_GAIN_2                             0b01000000  //  7     5                         .
  #define SX127X_LNA_GAIN_3                             0b01100000  //  7     5                         .
  #define SX127X_LNA_GAIN_4                             0b10000000  //  7     5                         .
  #define SX127X_LNA_GAIN_5                             0b10100000  //  7     5                         .
  #define SX127X_LNA_GAIN_6                             0b11000000  //  7     5                         min gain
  #define SX127X_LNA_BOOST_OFF                          0b00000000  //  1     0     default LNA current
  #define SX127X_LNA_BOOST_ON                           0b00000011  //  1     0     150% LNA current
  // SX127X_REG_PACKET_CONFIG_1
  #define SX127X_PACKET_FIXED                           0b00000000  //  7     7     packet format: fixed length
  #define SX127X_PACKET_VARIABLE                        0b10000000  //  7     7                    variable length (default)
  #define SX127X_DC_FREE_NONE                           0b00000000  //  6     5     DC-free encoding: disabled (default)
  #define SX127X_DC_FREE_MANCHESTER                     0b00100000  //  6     5                       Manchester
  #define SX127X_DC_FREE_WHITENING                      0b01000000  //  6     5                       Whitening
  #define SX127X_CRC_OFF                                0b00000000  //  4     4     CRC disabled
  #define SX127X_CRC_ON                                 0b00010000  //  4     4     CRC enabled (default)
  #define SX127X_CRC_AUTOCLEAR_OFF                      0b00001000  //  3     3     keep FIFO on CRC mismatch, issue payload ready interrupt
  #define SX127X_CRC_AUTOCLEAR_ON                       0b00000000  //  3     3     clear FIFO on CRC mismatch, do not issue payload ready interrupt
  #define SX127X_ADDRESS_FILTERING_OFF                  0b00000000  //  2     1     address filtering: none (default)
  #define SX127X_ADDRESS_FILTERING_NODE                 0b00000010  //  2     1                        node
  #define SX127X_ADDRESS_FILTERING_NODE_BROADCAST       0b00000100  //  2     1                        node or broadcast
  #define SX127X_CRC_WHITENING_TYPE_CCITT               0b00000000  //  0     0     CRC and whitening algorithms: CCITT CRC with standard whitening (default)
  #define SX127X_CRC_WHITENING_TYPE_IBM                 0b00000001  //  0     0                                   IBM CRC with alternate whitening
  // SX127X_REG_DIO_MAPPING_1
  #define SX127X_DIO0_RX_DONE                           0b00000000  //  7     6
  #define SX127X_DIO0_TX_DONE                           0b01000000  //  7     6
  #define SX127X_DIO0_CAD_DONE                          0b10000000  //  7     6
  #define SX127X_DIO1_RX_TIMEOUT                        0b00000000  //  5     4
  #define SX127X_DIO1_FHSS_CHANGE_CHANNEL               0b00010000  //  5     4
  #define SX127X_DIO1_CAD_DETECTED                      0b00100000  //  5     4
  // SX127X_REG_FIFO_TX_BASE_ADDR
  #define SX127X_FIFO_TX_BASE_ADDR_MAX                  0b00000000  //  7     0     allocate the entire FIFO buffer for TX only
  // SX127X_REG_DIO_MAPPING_1
  #define SX127X_DIO0_CONT_SYNC_ADDRESS                 0b00000000  //  7     6
  #define SX127X_DIO0_CONT_TX_READY                     0b00000000  //  7     6
  #define SX127X_DIO0_CONT_RSSI_PREAMBLE_DETECTED       0b01000000  //  7     6
  #define SX127X_DIO0_CONT_RX_READY                     0b10000000  //  7     6
  #define SX127X_DIO0_PACK_PAYLOAD_READY                0b00000000  //  7     6
  #define SX127X_DIO0_PACK_PACKET_SENT                  0b00000000  //  7     6
  #define SX127X_DIO0_PACK_CRC_OK                       0b01000000  //  7     6
  #define SX127X_DIO0_PACK_TEMP_CHANGE_LOW_BAT          0b11000000  //  7     6
  #define SX127X_DIO1_CONT_DCLK                         0b00000000  //  5     4
  #define SX127X_DIO1_CONT_RSSI_PREAMBLE_DETECTED       0b00010000  //  5     4
  #define SX127X_DIO1_PACK_FIFO_LEVEL                   0b00000000  //  5     4
  #define SX127X_DIO1_PACK_FIFO_EMPTY                   0b00010000  //  5     4
  #define SX127X_DIO1_PACK_FIFO_FULL                    0b00100000  //  5     4
  #define SX127X_DIO2_CONT_DATA                         0b00000000  //  3     2
  // SX127X_REG_IRQ_FLAGS
  #define SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT              0b10000000  //  7     7     timeout
  #define SX127X_CLEAR_IRQ_FLAG_RX_DONE                 0b01000000  //  6     6     packet reception complete
  #define SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR       0b00100000  //  5     5     payload CRC error
  #define SX127X_CLEAR_IRQ_FLAG_VALID_HEADER            0b00010000  //  4     4     valid header received
  #define SX127X_CLEAR_IRQ_FLAG_TX_DONE                 0b00001000  //  3     3     payload transmission complete
  #define SX127X_CLEAR_IRQ_FLAG_CAD_DONE                0b00000100  //  2     2     CAD complete
  #define SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL     0b00000010  //  1     1     FHSS change channel
  #define SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED            0b00000001  //  0     0     valid LoRa signal detected during CAD operation
  // SX1278 specific register map
  #define SX1278_REG_MODEM_CONFIG_3                     0x26
  #define SX1278_REG_PLL_HOP                            0x44
  #define SX1278_REG_TCXO                               0x4B
  #define SX1278_REG_PA_DAC                             0x4D
  #define SX1278_REG_FORMER_TEMP                        0x5B
  #define SX1278_REG_REG_BIT_RATE_FRAC                  0x5D
  #define SX1278_REG_AGC_REF                            0x61
  #define SX1278_REG_AGC_THRESH_1                       0x62
  #define SX1278_REG_AGC_THRESH_2                       0x63
  #define SX1278_REG_AGC_THRESH_3                       0x64
  #define SX1278_REG_PLL                                0x70
  // SX127X_REG_VERSION
  #define SX1278_CHIP_VERSION                           0x12
  // SX1278_REG_MODEM_CONFIG_1
  #define SX1278_BW_7_80_KHZ                            0b00000000  //  7     4     bandwidth:  7.80 kHz
  #define SX1278_BW_10_40_KHZ                           0b00010000  //  7     4                 10.40 kHz
  #define SX1278_BW_15_60_KHZ                           0b00100000  //  7     4                 15.60 kHz
  #define SX1278_BW_20_80_KHZ                           0b00110000  //  7     4                 20.80 kHz
  #define SX1278_BW_31_25_KHZ                           0b01000000  //  7     4                 31.25 kHz
  #define SX1278_BW_41_70_KHZ                           0b01010000  //  7     4                 41.70 kHz
  #define SX1278_BW_62_50_KHZ                           0b01100000  //  7     4                 62.50 kHz
  #define SX1278_BW_125_00_KHZ                          0b01110000  //  7     4                 125.00 kHz
  #define SX1278_BW_250_00_KHZ                          0b10000000  //  7     4                 250.00 kHz
  #define SX1278_BW_500_00_KHZ                          0b10010000  //  7     4                 500.00 kHz
  #define SX1278_CR_4_5                                 0b00000010  //  3     1     error coding rate:  4/5
  #define SX1278_CR_4_6                                 0b00000100  //  3     1                         4/6
  #define SX1278_CR_4_7                                 0b00000110  //  3     1                         4/7
  #define SX1278_CR_4_8                                 0b00001000  //  3     1                         4/8
  #define SX1278_HEADER_EXPL_MODE                       0b00000000  //  0     0     explicit header mode
  #define SX1278_HEADER_IMPL_MODE                       0b00000001  //  0     0     implicit header mode
  // SX1278_REG_MODEM_CONFIG_3
  #define SX1278_LOW_DATA_RATE_OPT_OFF                  0b00000000  //  3     3     low data rate optimization disabled
  #define SX1278_LOW_DATA_RATE_OPT_ON                   0b00001000  //  3     3     low data rate optimization enabled
  #define SX1278_AGC_AUTO_OFF                           0b00000000  //  2     2     LNA gain set by REG_LNA
  #define SX1278_AGC_AUTO_ON                            0b00000100  //  2     2     LNA gain set by internal AGC loop
  // SX1278_REG_MODEM_CONFIG_2
  #define SX1278_RX_CRC_MODE_OFF                        0b00000000  //  2     2     CRC disabled
  #define SX1278_RX_CRC_MODE_ON                         0b00000100  //  2     2     CRC enabled
  // SX1278_REG_PA_CONFIG
  #define SX1278_MAX_POWER                              0b01110000  //  6     4     max power: P_max = 10.8 + 0.6*MAX_POWER [dBm]; P_max(MAX_POWER = 0b111) = 15 dBm
  #define SX1278_LOW_POWER                              0b00100000  //  6     4
  #pragma endregion

  public:
    int16_t begin(uint32_t freq = 866750000, uint32_t bw = 125000, uint8_t sf = 10, uint8_t cr = 7, uint8_t syncWord = SX127X_SYNC_WORD, int8_t power = 20, uint8_t currentLimit = 150, uint16_t preambleLength = 8, uint8_t gain = 0) {
      int16_t state = this->beginSX127x(SX1278_CHIP_VERSION, syncWord, currentLimit, preambleLength);
      if (state != ERR_NONE) {
        return(state);
      }

      // configure settings not accessible by API
      state = this->config();
      if (state != ERR_NONE) {
        return(state);
      }

      // configure publicly accessible settings
      state = this->setFrequency(freq);
      if (state != ERR_NONE) {
        return(state);
      }

      state = this->setBandwidth(bw);
      if (state != ERR_NONE) {
        return(state);
      }

      state = this->setSpreadingFactor(sf);
      if (state != ERR_NONE) {
        return(state);
      }

      state = this->setCodingRate(cr);
      if (state != ERR_NONE) {
        return(state);
      }

      state = this->setOutputPower(power);
      if (state != ERR_NONE) {
        return(state);
      }

      state = this->setGain(gain);
      if (state != ERR_NONE) {
        return(state);
      }

      return(state);
    }

    int16_t beginSX127x(uint8_t chipVersion, uint8_t syncWord, uint8_t currentLimit, uint16_t preambleLength) {
      // set module properties
      this->init(RADIOLIB_USE_SPI, RADIOLIB_INT_0);

      // try to find the SX127x chip
      if (!this->findChip(chipVersion)) {
        this->term();
        return(ERR_CHIP_NOT_FOUND);
      }

      // check active modem
      int16_t state;
      if (this->getActiveModem() != SX127X_LORA) {
        // set LoRa mode
        state = this->setActiveModem(SX127X_LORA);
        if (state != ERR_NONE) {
          return(state);
        }
      }

      // set LoRa sync word
      state = this->setSyncWord(syncWord);
      if (state != ERR_NONE) {
        return(state);
      }

      // set over current protection
      state = this->setCurrentLimit(currentLimit);
      if (state != ERR_NONE) {
        return(state);
      }

      // set preamble length
      state = this->setPreambleLength(preambleLength);
      if (state != ERR_NONE) {
        return(state);
      }

      // initalize internal variables
      this->_dataRate = 0.0;

      return(state);
    }

    bool findChip(uint8_t ver) {
      uint8_t i = 0;
      bool flagFound = false;
      while ((i < 10) && !flagFound) {
        uint8_t version = this->SPIreadRegister(SX127X_REG_VERSION);
        if (version == ver) {
          flagFound = true;
        }
        else {
          delay(1000);
          i++;
        }
      }

      return(flagFound);
    }

    int16_t setSyncWord(uint8_t syncWord) {
      // check active modem
      if (this->getActiveModem() != SX127X_LORA) {
        return(ERR_WRONG_MODEM);
      }

      // set mode to standby
      this->setMode(SX127X_STANDBY);

      // write register
      return(this->SPIsetRegValue(SX127X_REG_SYNC_WORD, syncWord));
    }

    int16_t getActiveModem() {
      return(this->SPIgetRegValue(SX127X_REG_OP_MODE, 7, 7));
    }

    int16_t setActiveModem(uint8_t modem) {
      // set mode to SLEEP
      int16_t state = this->setMode(SX127X_SLEEP);

      // set modem
      state |= this->SPIsetRegValue(SX127X_REG_OP_MODE, modem, 7, 7, 5);

      // set mode to STANDBY
      state |= this->setMode(SX127X_STANDBY);
      return(state);
    }

    int16_t setCurrentLimit(uint8_t currentLimit) {
      // check allowed range
      if (!(((currentLimit >= 45) && (currentLimit <= 240)) || (currentLimit == 0))) {
        return(ERR_INVALID_CURRENT_LIMIT);
      }

      // set mode to standby
      int16_t state = this->setMode(SX127X_STANDBY);

      // set OCP limit
      uint8_t raw;
      if (currentLimit == 0) {
        // limit set to 0, disable OCP
        state |= this->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_OFF, 5, 5);
      }
      else if (currentLimit <= 120) {
        raw = (currentLimit - 45) / 5;
        state |= this->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_ON | raw, 5, 0);
      }
      else if (currentLimit <= 240) {
        raw = (currentLimit + 30) / 10;
        state |= this->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_ON | raw, 5, 0);
      }
      return(state);
    }

    int16_t setPreambleLength(uint16_t preambleLength) {
      // set mode to standby
      int16_t state = this->setMode(SX127X_STANDBY);
      if (state != ERR_NONE) {
        return(state);
      }

      // check active modem
      uint8_t modem = this->getActiveModem();
      if (modem == SX127X_LORA) {
        // check allowed range
        if (preambleLength < 6) {
          return(ERR_INVALID_PREAMBLE_LENGTH);
        }

        // set preamble length
        state = this->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB, (uint8_t)((preambleLength >> 8) & 0xFF));
        state |= this->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB, (uint8_t)(preambleLength & 0xFF));
        return(state);

      }
      else if (modem == SX127X_FSK_OOK) {
        // set preamble length
        state = this->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB_FSK, (uint8_t)((preambleLength >> 8) & 0xFF));
        state |= this->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB_FSK, (uint8_t)(preambleLength & 0xFF));
        return(state);
      }

      return(ERR_UNKNOWN);
    }

    int16_t config() {
      this->SPIsetRegValue(SX127X_REG_HOP_PERIOD, SX127X_HOP_PERIOD_OFF);
    }

    int16_t setFrequency(uint32_t freq) {
      // check frequency range
      if ((freq < 137000000) || (freq > 1020000000)) {
        return(ERR_INVALID_FREQUENCY);
      }

      // SX1276/77/78 Errata fixes
      if (this->getActiveModem() == SX127X_LORA) {
        // sensitivity optimization for 500kHz bandwidth
        // see SX1276/77/78 Errata, section 2.1 for details
        if (this->_bw == 500000) {
          if ((freq >= 862000000) && (freq <= 1020000000)) {
            this->SPIwriteRegister(0x36, 0x02);
            this->SPIwriteRegister(0x3a, 0x64);
          }
          else if ((freq >= 410000000) && (freq <= 525000000)) {
            this->SPIwriteRegister(0x36, 0x02);
            this->SPIwriteRegister(0x3a, 0x7F);
          }
        }

        // mitigation of receiver spurious response
        // see SX1276/77/78 Errata, section 2.3 for details
        if (this->_bw == 7800) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x48);
          this->SPIsetRegValue(0x30, 0x00);
          freq += 7800;
        }
        else if (this->_bw == 10400) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x44);
          this->SPIsetRegValue(0x30, 0x00);
          freq += 10400;
        }
        else if (this->_bw == 15600) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x44);
          this->SPIsetRegValue(0x30, 0x00);
          freq += 15600;
        }
        else if (this->_bw == 20800) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x44);
          this->SPIsetRegValue(0x30, 0x00);
          freq += 20800;
        }
        else if (this->_bw == 31250) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x44);
          this->SPIsetRegValue(0x30, 0x00);
          freq += 31250;
        }
        else if (this->_bw == 41700) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x44);
          this->SPIsetRegValue(0x30, 0x00);
          freq += 41700;
        }
        else if (this->_bw == 62500) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x40);
          this->SPIsetRegValue(0x30, 0x00);
        }
        else if (this->_bw == 125000) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x40);
          this->SPIsetRegValue(0x30, 0x00);
        }
        else if (this->_bw == 250000) {
          this->SPIsetRegValue(0x31, 0b0000000, 7, 7);
          this->SPIsetRegValue(0x2F, 0x40);
          this->SPIsetRegValue(0x30, 0x00);
        }
        else if (this->_bw == 500000) {
          this->SPIsetRegValue(0x31, 0b1000000, 7, 7);
        }
      }

      // set frequency
      return(this->setFrequencyRaw(freq));
    }

    int16_t setFrequencyRaw(uint32_t newFreq) {
      // set mode to standby
      int16_t state = this->setMode(SX127X_STANDBY);

      // calculate register values
      float f = (float)newFreq / 1000000;
      uint32_t FRF = (f * (uint32_t(1) << SX127X_DIV_EXPONENT)) / SX127X_CRYSTAL_FREQ;

      // write registers
      state |= this->SPIsetRegValue(SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
      state |= this->SPIsetRegValue(SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
      state |= this->SPIsetRegValue(SX127X_REG_FRF_LSB, FRF & 0x0000FF);
      return(state);
    }

    int16_t setBandwidth(uint32_t bw) {
      // check active modem
      if (this->getActiveModem() != SX127X_LORA) {
        return(ERR_WRONG_MODEM);
      }

      uint8_t newBandwidth;

      // check allowed bandwidth values
      if (bw ==  7800) {
        newBandwidth = SX1278_BW_7_80_KHZ;
      }
      else if (bw == 10400) {
        newBandwidth = SX1278_BW_10_40_KHZ;
      }
      else if (bw == 15600) {
        newBandwidth = SX1278_BW_15_60_KHZ;
      }
      else if (bw == 20800) {
        newBandwidth = SX1278_BW_20_80_KHZ;
      }
      else if (bw == 31250) {
        newBandwidth = SX1278_BW_31_25_KHZ;
      }
      else if (bw == 41700) {
        newBandwidth = SX1278_BW_41_70_KHZ;
      }
      else if (bw == 62500) {
        newBandwidth = SX1278_BW_62_50_KHZ;
      }
      else if (bw == 125000) {
        newBandwidth = SX1278_BW_125_00_KHZ;
      }
      else if (bw == 250000) {
        newBandwidth = SX1278_BW_250_00_KHZ;
      }
      else if (bw == 500000) {
        newBandwidth = SX1278_BW_500_00_KHZ;
      }
      else {
        return(ERR_INVALID_BANDWIDTH);
      }

      // set bandwidth and if successful, save the new setting
      int16_t state = this->setBandwidthRaw(newBandwidth);
      if (state == ERR_NONE) {
        this->_bw = bw;

        // calculate symbol length and set low data rate optimization, if needed
        float symbolLength = (float)(uint32_t(1) << this->_sf) / ((float)this->_bw / 1000);
        if (symbolLength >= 16.0) {
          state = this->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_ON, 3, 3);
        }
        else {
          state = this->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_OFF, 3, 3);
        }
      }
      return(state);
    }

    int16_t setBandwidthRaw(uint8_t newBandwidth) {
      // set mode to standby
      int16_t state = this->standby();

      // write register
      state |= this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, newBandwidth, 7, 4);
      return(state);
    }

    int16_t setSpreadingFactor(uint8_t sf) {
      // check active modem
      if (this->getActiveModem() != SX127X_LORA) {
        return(ERR_WRONG_MODEM);
      }

      uint8_t newSpreadingFactor;

      // check allowed spreading factor values
      switch (sf) {
      case 6:
        newSpreadingFactor = SX127X_SF_6;
        break;
      case 7:
        newSpreadingFactor = SX127X_SF_7;
        break;
      case 8:
        newSpreadingFactor = SX127X_SF_8;
        break;
      case 9:
        newSpreadingFactor = SX127X_SF_9;
        break;
      case 10:
        newSpreadingFactor = SX127X_SF_10;
        break;
      case 11:
        newSpreadingFactor = SX127X_SF_11;
        break;
      case 12:
        newSpreadingFactor = SX127X_SF_12;
        break;
      default:
        return(ERR_INVALID_SPREADING_FACTOR);
      }

      // set spreading factor and if successful, save the new setting
      int16_t state = this->setSpreadingFactorRaw(newSpreadingFactor);
      if (state == ERR_NONE) {
        this->_sf = sf;

        // calculate symbol length and set low data rate optimization, if needed
        float symbolLength = (float)(uint32_t(1) << this->_sf) / ((float)this->_bw / 1000);
        if (symbolLength >= 16.0) {
          state = this->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_ON, 3, 3);
        }
        else {
          state = this->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_OFF, 3, 3);
        }
      }
      return(state);
    }

    int16_t setSpreadingFactorRaw(uint8_t newSpreadingFactor) {
      // set mode to standby
      int16_t state = this->standby();

      // write registers
      if (newSpreadingFactor == SX127X_SF_6) {
        state |= this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1278_HEADER_IMPL_MODE, 0, 0);
        state |= this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX127X_SF_6 | SX127X_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_ON, 7, 2);
        state |= this->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_6, 2, 0);
        state |= this->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_6);
      }
      else {
        state |= this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1278_HEADER_EXPL_MODE, 0, 0);
        state |= this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, newSpreadingFactor | SX127X_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_ON, 7, 2);
        state |= this->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_7_12, 2, 0);
        state |= this->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_7_12);
      }
      return(state);
    }

    int16_t setCodingRate(uint8_t cr) {
      // check active modem
      if (this->getActiveModem() != SX127X_LORA) {
        return(ERR_WRONG_MODEM);
      }

      uint8_t newCodingRate;

      // check allowed coding rate values
      switch (cr) {
      case 5:
        newCodingRate = SX1278_CR_4_5;
        break;
      case 6:
        newCodingRate = SX1278_CR_4_6;
        break;
      case 7:
        newCodingRate = SX1278_CR_4_7;
        break;
      case 8:
        newCodingRate = SX1278_CR_4_8;
        break;
      default:
        return(ERR_INVALID_CODING_RATE);
      }

      // set coding rate and if successful, save the new setting
      int16_t state = this->setCodingRateRaw(newCodingRate);
      if (state == ERR_NONE) {
        this->_cr = cr;
      }
      return(state);
    }

    int16_t setCodingRateRaw(uint8_t newCodingRate) {
      // set mode to standby
      int16_t state = this->standby();

      // write register
      state |= this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, newCodingRate, 3, 1);
      return(state);
    }

    int16_t setOutputPower(int8_t power) {
      // check allowed power range
      if (!(((power >= -3) && (power <= 17)) || (power == 20))) {
        return(ERR_INVALID_OUTPUT_POWER);
      }

      // set mode to standby
      int16_t state = this->standby();

      // set output power
      if (power < 2) {
        // power is less than 2 dBm, enable PA on RFO
        state |= this->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_RFO, 7, 7);
        state |= this->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_LOW_POWER | (power + 3), 6, 0);
        state |= this->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_OFF, 2, 0);
      }
      else if ((power >= 2) && (power <= 17)) {
        // power is 2 - 17 dBm, enable PA1 + PA2 on PA_BOOST
        state |= this->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST, 7, 7);
        state |= this->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_MAX_POWER | (power - 2), 6, 0);
        state |= this->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_OFF, 2, 0);
      }
      else if (power == 20) {
        // power is 20 dBm, enable PA1 + PA2 on PA_BOOST and enable high power mode
        state |= this->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST, 7, 7);
        state |= this->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_MAX_POWER | (power - 5), 6, 0);
        state |= this->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_ON, 2, 0);
      }
      return(state);
    }

    int16_t setGain(uint8_t gain) {
      // check active modem
      if (this->getActiveModem() != SX127X_LORA) {
        return(ERR_WRONG_MODEM);
      }

      // check allowed range
      if (gain > 6) {
        return(ERR_INVALID_GAIN);
      }

      // set mode to standby
      int16_t state = this->standby();

      // set gain
      if (gain == 0) {
        // gain set to 0, enable AGC loop
        state |= this->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_ON, 2, 2);
      }
      else {
        state |= this->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_OFF, 2, 2);
        state |= this->SPIsetRegValue(SX127X_REG_LNA, (gain << 5) | SX127X_LNA_BOOST_ON);
      }
      return(state);
    }

    int16_t standby() {
      // set mode to standby
      return(this->setMode(SX127X_STANDBY));
    }

    int16_t setMode(uint8_t mode) {
      return(this->SPIsetRegValue(SX127X_REG_OP_MODE, mode, 2, 0, 5));
    }

    int16_t setCRC(bool enableCRC) {
      if (this->getActiveModem() == SX127X_LORA) {
        // set LoRa CRC
        if (enableCRC) {
          return(this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX1278_RX_CRC_MODE_ON, 2, 2));
        }
        else {
          return(this->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX1278_RX_CRC_MODE_OFF, 2, 2));
        }
      }
      else {
        // set FSK CRC
        if (enableCRC) {
          return(this->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_CRC_ON, 4, 4));
        }
        else {
          return(this->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_CRC_OFF, 4, 4));
        }
      }
    }

    int16_t transmit(const char* str, uint8_t addr = 0) {
      return(this->transmit((uint8_t*)str, strlen(str), addr));
    }

    int16_t transmit(String& str, uint8_t addr = 0) {
      return(this->transmit(str.c_str(), addr));
    }

    int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0) {
      // set mode to standby
      int16_t state = this->setMode(SX127X_STANDBY);

      int16_t modem = this->getActiveModem();
      uint32_t start = 0;
      if (modem == SX127X_LORA) {
        // calculate timeout (150 % of expected time-one-air)
        float symbolLength = (float)(uint32_t(1) << this->_sf) / ((float)this->_bw / 1000);
        float de = 0;
        if (symbolLength >= 16.0) {
          de = 1;
        }
        float ih = (float)this->SPIgetRegValue(SX127X_REG_MODEM_CONFIG_1, 0, 0);
        float crc = (float)(this->SPIgetRegValue(SX127X_REG_MODEM_CONFIG_2, 2, 2) >> 2);
        float n_pre = (float)((this->SPIgetRegValue(SX127X_REG_PREAMBLE_MSB) << 8) | this->SPIgetRegValue(SX127X_REG_PREAMBLE_LSB));
        float n_pay = 8.0 + max(ceil((8.0 * (float)len - 4.0 * (float)this->_sf + 28.0 + 16.0 * crc - 20.0 * ih) / (4.0 * (float)this->_sf - 8.0 * de)) * (float)this->_cr, 0.0);
        uint32_t timeout = ceil(symbolLength * (n_pre + n_pay + 4.25) * 1500.0);

        // start transmission
        state = this->startTransmit(data, len, addr);
        if (state != ERR_NONE) {
          return(state);
        }

        // wait for packet transmission or timeout
        start = micros();
        while (!digitalRead(this->getInt0())) {
          if (micros() - start > timeout) {
            this->clearIRQFlags();
            return(ERR_TX_TIMEOUT);
          }
        }

      }
      else if (modem == SX127X_FSK_OOK) {
        // calculate timeout (5ms + 500 % of expected time-on-air)
        uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (this->_br * 1000.0)) * 5000000.0);

        // start transmission
        state = this->startTransmit(data, len, addr);
        if (state != ERR_NONE) {
          return(state);
        }

        // wait for transmission end or timeout
        start = micros();
        while (!digitalRead(this->getInt0())) {
          if (micros() - start > timeout) {
            this->clearIRQFlags();
            this->standby();
            return(ERR_TX_TIMEOUT);
          }
        }
      }
      else {
        return(ERR_UNKNOWN);
      }

      // update data rate
      uint32_t elapsed = micros() - start;
      this->_dataRate = (len * 8.0) / ((float)elapsed / 1000000.0);

      // clear interrupt flags
      this->clearIRQFlags();

      // set mode to standby to disable transmitter
      return(this->standby());
    }

    int16_t startTransmit(uint8_t* data, size_t len, uint8_t addr) {
      // set mode to standby
      int16_t state = this->setMode(SX127X_STANDBY);

      int16_t modem = this->getActiveModem();
      if (modem == SX127X_LORA) {
        // check packet length
        if (len >= SX127X_MAX_PACKET_LENGTH) {
          return(ERR_PACKET_TOO_LONG);
        }

        // set DIO mapping
        this->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_TX_DONE, 7, 6);

        // clear interrupt flags
        this->clearIRQFlags();

        // set packet length
        state |= this->SPIsetRegValue(SX127X_REG_PAYLOAD_LENGTH, len);

        // set FIFO pointers
        state |= this->SPIsetRegValue(SX127X_REG_FIFO_TX_BASE_ADDR, SX127X_FIFO_TX_BASE_ADDR_MAX);
        state |= this->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_TX_BASE_ADDR_MAX);

        // write packet to FIFO
        this->SPIwriteRegisterBurst(SX127X_REG_FIFO, data, len);

        // start transmission
        state |= this->setMode(SX127X_TX);
        if (state != ERR_NONE) {
          return(state);
        }

        return(ERR_NONE);

      }
      else if (modem == SX127X_FSK_OOK) {
        // check packet length
        if (len >= SX127X_MAX_PACKET_LENGTH_FSK) {
          return(ERR_PACKET_TOO_LONG);
        }

        // set DIO mapping
        this->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_PACK_PACKET_SENT, 7, 6);

        // clear interrupt flags
        this->clearIRQFlags();

        // set packet length
        this->SPIwriteRegister(SX127X_REG_FIFO, len);

        // check address filtering
        uint8_t filter = this->SPIgetRegValue(SX127X_REG_PACKET_CONFIG_1, 2, 1);
        if ((filter == SX127X_ADDRESS_FILTERING_NODE) || (filter == SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
          this->SPIwriteRegister(SX127X_REG_FIFO, addr);
        }

        // write packet to FIFO
        this->SPIwriteRegisterBurst(SX127X_REG_FIFO, data, len);

        // start transmission
        state |= this->setMode(SX127X_TX);
        if (state != ERR_NONE) {
          return(state);
        }

        return(ERR_NONE);
      }

      return(ERR_UNKNOWN);
    }

    void clearIRQFlags() {
      int16_t modem = this->getActiveModem();
      if (modem == SX127X_LORA) {
        this->SPIwriteRegister(SX127X_REG_IRQ_FLAGS, 0b11111111);
      }
      else if (modem == SX127X_FSK_OOK) {
        this->SPIwriteRegister(SX127X_REG_IRQ_FLAGS_1, 0b11111111);
        this->SPIwriteRegister(SX127X_REG_IRQ_FLAGS_2, 0b11111111);
      }
    }

    uint8_t random() {
      return this->SPIreadRegister(SX127X_REG_RSSI_WIDEBAND);
    }

    int16_t scanChannel() {
      // check active modem
      if (this->getActiveModem() != SX127X_LORA) {
        return(ERR_WRONG_MODEM);
      }

      // set mode to standby
      int16_t state = this->setMode(SX127X_STANDBY);

      // set DIO pin mapping
      state |= this->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_CAD_DONE, 7, 4);

      // clear interrupt flags
      this->clearIRQFlags();

      // set mode to CAD
      state |= setMode(SX127X_CAD);
      if (state != ERR_NONE) {
        return(state);
      }

      // wait for channel activity detected or timeout
      while (!digitalRead(this->getInt0())) {
        if ((this->SPIreadRegister(SX127X_REG_IRQ_FLAGS) & SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) == 1) {
          this->clearIRQFlags();
          return(PREAMBLE_DETECTED);
        }
      }

      // clear interrupt flags
      this->clearIRQFlags();

      return(CHANNEL_FREE);
    }

  private:
    uint8_t _cr;
    uint8_t _sf;
    uint32_t _bw;
    float _br = 48.0;
    float _dataRate;
};

/// <summary>
/// Class that sends data over LORA.
/// </summary>
/// <example>
/// https://www.semtech.com/products/wireless-rf/lora-transceivers/SX1272
/// https://www.semtech.com/uploads/documents/sx1272.pdf
/// The SX1272 offers three bandwidth options of 125 kHz, 250 kHz, and 500 kHz with spreading factors ranging from 6 to 12.
/// The SX1273 offers the same bandwidth options with spreading factors from 6 to 9.
/// https://www.semtech.com/products/wireless-rf/lora-transceivers/SX1276
/// https://www.semtech.com/uploads/documents/DS_SX1276-7-8-9_W_APP_V5.pdf
/// The SX1276 and SX1279 offer bandwidth options ranging from 7.8 kHz to 500 kHz with spreading factors ranging from 6 to 12, and covering all available frequency bands.
/// The SX1277 offers the same bandwidth and frequency band options with spreading factors from 6 to 9.
/// The SX1278 offers bandwidths and spreading factor options, but only covers the lower UHF bands.
/// </example> 
/// <typeparam name="pin_miso">Pin number of MISO pin on the controller</typeparam>
/// <typeparam name="pin_mosi">Pin number of MOSI pin on the controller</typeparam>
/// <typeparam name="pin_sck">Pin number of SCK pin on the controller</typeparam>
/// <typeparam name="pin_ss">Pin number of CS pin on the controller</typeparam>
/// <typeparam name="pin_rst">Pin number of the controller pin where the resetpin is attached</typeparam>
/// <typeparam name="pin_dio">Pin number of the controller pin where the data in 0 is attached</typeparam>
/// <typeparam name="baseband">Frequency of the lowest frequenc for each devices</typeparam>
/// <typeparam name="channeloffset">offset between every channel frequency</typeparam>
/// <typeparam name="espname">String of the node name</typeparam>
/// <typeparam name="lbt">(listen before talk) if true, this class will wait and listen to the LORA module before send, otherwise it will send directly</typeparam>
/// <typeparam name="binary">if true, the data will packed into a short binary message, otherwise it will send as long plain text.</typeparam>
template<long baseband, long channeloffset, bool lbt>
class LoraT {
  public:
    /// <summary>Constructor for LORA class, setup the io pins</summary>
    /// <typeparam name="wlanclass">Needs an instance of wlanclass for debug output</typeparam>
    /// <typeparam name="storage">Needs an instance of storage for reading frequency correction</typeparam>
    LoraT(Wlan* wlanclass, Storage * storage) {
      this->wlan = wlanclass;
      this->storage = storage;
      this->lora = new LoraDriver();
    }

    /// <summary>Setup the LORA settings and start the module</summary>
    void Begin() {
      this->wlan->Box("Setup Lora!", 80);
      if(this->lora->begin(this->CalculateFrequency() + this->storage->GetFreqoffset()) != ERR_NONE) {
        this->wlan->Box("Lora Failed!", 90);
      } else {
        this->lora->setCRC(true);
        this->wlan->Box("Lora successful", 90);
        this->_lora_enabled = true;
      }
    }

    #pragma region Debug-Mode for Frequenztuning
    /// <summary>Enable Debugmode</summary>
    void Debugmode() {
      //this->lora->setSignalBandwidth(1);
      this->lora->setBandwidth(7800);
    }

    /// <summary>Set an offset to the center frequency</summary>
    /// <typeparam name="o">Frequency offset in Hz</typeparam>
    void SetFreqOffset(int32_t o) {
      this->lora->setFrequency(this->CalculateFrequency() + o);
    }

    /// <summary>Send TEST TEST TEST over lora</summary>
    void DebugSend() {
      this->lora->transmit("TEST TEST TEST");
    }
    #pragma endregion

    #pragma region Send Data
    /// <summary>Send a string over LORA</summary>
    /// <typeparam name="data">Text that should be send</typeparam>
    void SendLora(String data) {
      long startWait, endWait;
      if(lbt) {
        startWait = millis();
        while(this->lora->scanChannel() != CHANNEL_FREE) {
          delay(1);
        }
        endWait = millis();
      }
      this->lora->transmit(data);
      this->wlan->Log(String("################################################\n"));
      if(lbt) {
        this->wlan->Log(String("Waiting: ") + String(endWait-startWait) + String(" ms\n"));
      }
      this->wlan->Log(data + String("\n"));
    }

    /// <summary>Send a binary array over LORA</summary>
    /// <typeparam name="data">Byte array of the data</typeparam>
    /// <typeparam name="size">length of the array</typeparam>
    void SendLora(uint8_t* data, uint8_t size) {
      long startWait, endWait;
      if(lbt) {
        startWait = millis();
        while(this->lora->scanChannel() != CHANNEL_FREE) {
          delay(1);
        }
        endWait = millis();
      }
      this->lora->transmit(data, size);
      this->wlan->Log(String("################################################\n"));
      if(lbt) {
        this->wlan->Log(String("Waiting: ") + String(endWait - startWait) + String(" ms\n"));
      }

      String g;
      for(uint8_t i = 0; i < size; i++) {
        g = g + (data[i] < 16 ? String("0") + String(data[i], HEX) : String(data[i], HEX)) + String(" ");
      }
      this->wlan->Log(g + String("\n"));
    }

    /// <summary>Send gps and battery information over LORA</summary>
    /// <typeparam name="gps">struct gpsInfoField, with all nessesary gps informations</typeparam>
    /// <typeparam name="batt">voltage value of the battery</typeparam>
    /// <typeparam name="panic">optional, if true data will send as panic item</typeparam>
    void Send(gpsInfoField gps, float batt, bool panic = false) {
      //Data 1+2+4+4+1+2+3+3+1 = 21 Char
      uint8_t lora_data[21];
      if(panic) {
        lora_data[0] = 'p';
      } else {
        lora_data[0] = 'b';
      }
      for(uint8_t i = 0; i < 2; i++) {
        if(this->storage->GetEspname().length() > i) {
          lora_data[i + 1] = this->storage->GetEspname().charAt(i);
        } else {
          lora_data[i + 1] = 0;
        }
      }
      uint64_t lat = *(uint64_t*)&gps.latitude;  lora_data[3]  = (lat >> 0) & 0xFF; lora_data[4]  = (lat >> 8) & 0xFF; lora_data[5]  = (lat >> 16) & 0xFF; lora_data[6]  = (lat >> 24) & 0xFF;
      uint64_t lon = *(uint64_t*)&gps.longitude; lora_data[7] = (lon >> 0) & 0xFF; lora_data[8]  = (lon >> 8) & 0xFF; lora_data[9]  = (lon >> 16) & 0xFF; lora_data[10]  = (lon >> 24) & 0xFF; 
      if(gps.hdop >= 25.5) { 
        lora_data[11] = 255; 
      } else if(gps.hdop <= 25.5 && gps.hdop > 0) { 
        lora_data[11] = (uint8_t)(gps.hdop * 10); 
      } else { 
        lora_data[11] = 0; 
      }
      lora_data[12] = (uint8_t)((((uint16_t)(gps.height * 10)) >> 0) & 0xFF); lora_data[13] = (uint8_t)((((uint16_t)(gps.height * 10)) >> 8) & 0xFF);
      lora_data[14] = String(gps.time.substring(0, 2)).toInt(); lora_data[15] = String(gps.time.substring(2, 4)).toInt(); lora_data[16] = String(gps.time.substring(4, 6)).toInt();
      lora_data[17] = gps.day; lora_data[18] = gps.month; lora_data[19] = (uint8_t)(gps.year - 2000);
      lora_data[20] = (uint8_t)((batt * 100)-230);
      uint16_t counter = sendCount++;
      uint8_t* sha = this->CreateSha(counter);
      this->SendLora(lora_data, 21);
      if(panic) {
        this->lora->setSpreadingFactor(11);
        this->SendLora(lora_data, 21);
        this->lora->setSpreadingFactor(12);
        this->SendLora(lora_data, 21);
        this->lora->setSpreadingFactor(10);
      }
    }

    /// <summary>Send status information over LORA</summary>
    /// <typeparam name="version">Internal versionsnumber of the node.</typeparam>
    /// <typeparam name="ip">Ip address of the node, if connected to wifi</typeparam>
    /// <typeparam name="ssid">Ssid of the connected wifi</typeparam>
    /// <typeparam name="wififlag">true if wifi is currently connected</typeparam>
    /// <typeparam name="battery">voltage value of the battery</typeparam>
    /// <typeparam name="freqoffset">currently used frequency offset</typeparam>
    /// <typeparam name="runningStatus">0 = for Shutdown node now, 1 = normal Statup (wifi on, no sleeping), 2 = Powersafe mode</typeparam>
    void Send(uint8_t version, String ip, String ssid, bool wififlag, float battery, int32_t freqoffset, uint8_t runningStatus) {
      this->SendLora("deb\n" + this->storage->GetEspname() + "\n" + String(version) + "," + ip + "," + ssid + "," + (wififlag ? "t" : "f") + "," + String(battery, 2) + "," + String(freqoffset)+","+String(runningStatus));
    }
    #pragma endregion

    uint8_t* CreateSha(uint16_t counter) {
      uint8_t* key = new uint8_t[36];

      for (uint8_t i = 0; i < 32; i++) {
        key[i] = this->storage->GetKey()[i];
      }
      key[32] = this->storage->GetEspname().charAt(0);
      key[33] = this->storage->GetEspname().charAt(1);
      key[34] = (counter >> 8) & 0xFF;
      key[35] = (counter >> 0) & 0xFF;
      

      String p = String("pSHA: ");
      for (uint8_t i = 0; i < 36; i++) {
        p = p + (key[i] < 16 ? String("0") + String(key[i], HEX) : String(key[i], HEX)) + String(" ");
      }
      this->wlan->Log(p + String("\n"));

      mbedtls_md_context_t ctx;
      mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

      mbedtls_md_init(&ctx);
      mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
      mbedtls_md_starts(&ctx);
      mbedtls_md_update(&ctx, (const unsigned char*)key, 36);
      uint8_t shaResult[32];
      mbedtls_md_finish(&ctx, shaResult);
      mbedtls_md_free(&ctx);

      String a = String("aSHA: ");
      for (uint8_t i = 0; i < 32; i++) {
        a = a + (shaResult[i] < 16 ? String("0") + String(shaResult[i], HEX) : String(shaResult[i], HEX)) + String(" ");
      }
      this->wlan->Log(a + String("\n"));

      return shaResult;
    }

    /// <summary>Calculate the device frequency from the first letter of the name of the device</summary>
    /// <returns>return the frequency in hz</returns>
    uint32_t CalculateFrequency() {
      return ((this->storage->GetEspname().charAt(0) % 8) * channeloffset) + baseband;
    }

    ///<summaryGet a Random Byte from Lora Device</summary>
    ///<returns>a random byte</returns>
    uint8_t GetRandom() {
      return this->lora->random();
    }
  private:
    Wlan * wlan;
    LoraDriver* lora;
    Storage * storage;
    bool _lora_enabled = false;
};

typedef LoraT<lora_baseband, lora_channeloffset, listenbeforetalk> Lora;

#endif // !_LORA_HPP_INCLUDED