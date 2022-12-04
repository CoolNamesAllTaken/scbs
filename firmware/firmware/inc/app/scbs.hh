#ifndef _SCBS_HH_
#define _SCBS_HH_

#include "pico/stdlib.h"
#include "hardware/gpio.h" // for UART inst
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "scbs_comms.hh"

#include <stdint.h>

class SCBS {
public:
    static const uint16_t kMaxUARTBufLen = 200;

    static const uint32_t kRegAddrSetOutputVoltage = 0x1000;
    static const uint32_t kRegAddrReadOutputCurrent = 0x2000;

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
    void MRDPacketHandler(MRDPacket packet_in);
    void MWRPacketHandler(MWRPacket packet_in);

    void FlushUARTBuf();
    void AppendCharToUARTBuf(char new_char);

    template <class PacketType>
    void TransmitPacket(PacketType packet);
    uint16_t ReceivePacket();

    void SetOutputVoltage(float voltage);
    void ReadOutputCurrent();
    float GetOutputCurrent();

    SCBSConfig_t config_;

    char uart_rx_buf_[kMaxUARTBufLen];
    uint16_t uart_rx_buf_len_ = 0;

    uint16_t cell_id_;
    float output_voltage_ = 0.0f; // [V]
    float output_current_ = 0.0f; // [mA]
};

#endif /* _SCBS_HH_ */