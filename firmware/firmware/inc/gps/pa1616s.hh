#ifndef _PA1616S_HH_
#define _PA1616S_HH_

#include "pico/stdlib.h"
#include "hardware/gpio.h" // for UART inst
#include "hardware/uart.h"

#include "nmea_utils.hh"

class PA1616S {
public:
    typedef struct {
        uart_inst_t * uart_id;
        uint uart_baud = 9600;
        uint uart_tx_pin;
        uint uart_rx_pin;
        uint data_bits = 8;
        uint stop_bits = 1;
        uart_parity_t parity = UART_PARITY_NONE;
    } PA1616SConfig_t;

    static const uint16_t kMaxUARTBufLen = 200;
    GGAPacket latest_gga_packet;

    PA1616S(PA1616SConfig_t config);
    void Init();
    void Update();
    
private:
    PA1616SConfig_t config_;
    char uart_buf_[kMaxUARTBufLen];
    uint16_t uart_buf_len_;

    void FlushUARTBuf();
};

#endif /* PA1616S_HH_ */