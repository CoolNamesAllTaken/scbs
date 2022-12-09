#ifndef _SCBS_HH_
#define _SCBS_HH_

#include "pico/stdlib.h"
#include "hardware/gpio.h" // for UART inst
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "scbs_comms.hh"

#include <stdint.h>

#define SCBS_FIRMWARE_VERSION "scbs_pico-0.1.0"

class SCBS {
public:
    static const uint16_t kMaxUARTBufLen = 200;

    static const uint32_t kRegAddrSetOutputVoltage = 0x1000;
    static const uint32_t kRegAddrReadOutputCurrent = 0x2000;
    static const uint32_t kRegAddrReadFirmwareVersion = 0x3000;

    static const uint16_t kErrCodeNone = 0x00;
    static const uint16_t kErrCodeAddrNotRecognized = 0x01;
    static const uint16_t kErrCodePacketLengthExceeded = 0x02;
    static const uint16_t kErrCodeWriteNotSupported = 0x03;
    static const uint16_t kErrCodeReceivedInvalidPacket = 0x0F;

    typedef struct {
        uart_inst_t * uart_id = uart1;
        uint16_t uart_baud = 9600;
        uint16_t uart_tx_pin = 4;
        uint16_t uart_rx_pin = 5;
        uint16_t uart_data_bits = 8;
        uint16_t uart_stop_bits = 1;
        uart_parity_t uart_parity = UART_PARITY_NONE;

        uint16_t pwm_pin = 16;
        pwm_chan pwm_channel = PWM_CHAN_A;

        uint16_t csense_pin = 28;
        uint16_t csense_adc_input = 2;

        uint16_t led_pin = 25;
    } SCBSConfig_t;

    SCBS(SCBSConfig_t config);

    void Init();
    void Update();

    uint16_t GetCellID();

private:
    void DISPacketHandler(DISPacket packet_in);
    void MWRPacketHandler(MWRPacket packet_in);
    void MRDPacketHandler(MRDPacket packet_in);
    void SWRPacketHandler(SWRPacket packet_in);
    void SRDPacketHandler(SRDPacket packet_in);
    void SRSPacketHandler(SRSPacket packet_in);

    uint16_t WriteRegister(uint32_t reg_addr, char value_in[BSPacket::kMaxPacketFieldLen]);
    uint16_t ReadRegister(uint32_t reg_addr, char value_out[BSPacket::kMaxPacketFieldLen]);

    void FlushUARTBuf();
    void AppendCharToUARTBuf(char new_char);

    void TransmitError(uint16_t err_code);
    template <class PacketType>
    void TransmitPacket(PacketType packet);
    uint16_t ReceivePacket();

    float SetOutputVoltage(float voltage);
    void ReadOutputCurrent();
    float GetOutputCurrent();

    void TurnOnStatusLED(uint32_t on_time_ms);

    SCBSConfig_t config_;

    char uart_rx_buf_[kMaxUARTBufLen];
    uint16_t uart_rx_buf_len_ = 0;

    uint16_t cell_id_;
    float output_voltage_ = 0.0f; // [V]
    float output_current_ = 0.0f; // [mA]

    bool status_led_on_ = false;
    uint32_t status_led_off_timestamp_ = 0;
};

#endif /* _SCBS_HH_ */