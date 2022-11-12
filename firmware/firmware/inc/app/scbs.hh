#ifndef _SCBS_HH_
#define _SCBS_HH_

#include "pico/stdlib.h"
#include "hardware/gpio.h" // for UART inst
#include "hardware/uart.h"
#include "scbs_comms.hh"

#include <stdint.h>

class SCBS {
public:
    static const uint16_t kMaxUARTBufLen = 200;

    typedef struct {
        uart_inst_t * uart_id = uart1;
        uint16_t uart_baud = 9600;
        uint16_t uart_tx_pin = 4;
        uint16_t uart_rx_pin = 5;
        uint16_t uart_data_bits = 8;
        uint16_t uart_stop_bits = 1;
        uart_parity_t uart_parity = UART_PARITY_NONE;
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

    SCBSConfig_t config_;

    char uart_rx_buf_[kMaxUARTBufLen];
    uint16_t uart_rx_buf_len_ = 0;

    uint16_t cell_id_;
};

#endif /* _SCBS_HH_ */