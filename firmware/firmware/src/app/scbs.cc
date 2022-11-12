#include "scbs.hh"
#include <stdio.h> // for printing
#include "hardware/gpio.h"


/** Public Functions **/

SCBS::SCBS(SCBSConfig_t config) {
    config_ = config;
}

void SCBS::Init() {
    gpio_init(config_.led_pin);
    gpio_set_dir(config_.led_pin, GPIO_OUT);

    uart_init(config_.uart_id, config_.uart_baud);
    gpio_set_function(config_.uart_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(config_.uart_rx_pin, GPIO_FUNC_UART);

    uart_set_hw_flow(config_.uart_id, false, false); // no CTS/RTS
    uart_set_format(config_.uart_id, config_.uart_data_bits, config_.uart_stop_bits, config_.uart_parity);
    uart_set_fifo_enabled(config_.uart_id, true);

    memset(uart_rx_buf_, '\0', kMaxUARTBufLen);
    uart_rx_buf_len_ = 0;

    // give em a little blink
    gpio_put(config_.led_pin, 1);
    sleep_ms(100);
    gpio_put(config_.led_pin, 0);

    printf("SCBS::Init(): Init completed.\r\n");
}

void SCBS::Update() {
    if (ReceivePacket() != 0) {
        gpio_put(config_.led_pin, 1);

        BSPacket packet = BSPacket(uart_rx_buf_);
        if (packet.IsValid()) {
            printf("SCBS::Update(): Packet is valid!\r\n");
            switch(packet.GetPacketType()) {
                case BSPacket::DIS:
                    DISPacketHandler(DISPacket(uart_rx_buf_));
                    break;
                case BSPacket::MRD:
                    break;
                case BSPacket::MWR:
                    break;
                case BSPacket::SRD:
                    break;
                case BSPacket::SWR:
                    break;
                case BSPacket::SRS:
                    break;
                default:
                    printf("SCBS::Update():     Unrecognized packet type.\r\n");
            }
        } else {
            printf("SCBS::Update(): Packet is invalid.\r\n");
        }
        FlushUARTBuf();
        
        gpio_put(config_.led_pin, 0);
    }
    
    
}

uint16_t SCBS::GetCellID() {
    return cell_id_;
}

/** Private Functions **/

/**
 * @brief Cell Discover packet handler. Enumerates the device and sends out a packet to the next
 * device in the chain.
 * @param[in] packet Incoming BSDIS packet.
*/
void SCBS::DISPacketHandler(DISPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::DISPacketHandler: Formed a valid DIS packet!\r\n");
        cell_id_ = packet_in.last_cell_id + 1;
        DISPacket packet_out = DISPacket(cell_id_);
        TransmitPacket(packet_out);
    } else {
        printf("SCBS::DISPacketHandler: Formed a DIS packet but it wasn't valid!\r\n");
    }
}

void SCBS::MRDPacketHandler(MRDPacket packet_in) {
    
}

void SCBS::MRDPacketHandler(MRDPacket packet_in) {
    if (packet_in.IsValid()) {

    }

}

void SCBS::FlushUARTBuf() {
    memset(uart_rx_buf_, '\0', kMaxUARTBufLen);
    uart_rx_buf_len_ = 0;
}

void SCBS::AppendCharToUARTBuf(char new_char) {
    // add new char to end of buffer
    strncat(uart_rx_buf_, &new_char, 1);
    uart_rx_buf_len_++;
}

template <class PacketType>
void SCBS::TransmitPacket(PacketType packet) {
    char uart_tx_buf[kMaxUARTBufLen];
    packet.ToString(uart_tx_buf);
    uart_puts(config_.uart_id, uart_tx_buf);
    uart_puts(config_.uart_id, (char *)"\r\n");
}

uint16_t SCBS::ReceivePacket() {
    while (uart_is_readable(config_.uart_id) && uart_rx_buf_len_ < kMaxUARTBufLen-1) {
        char new_char = uart_getc(config_.uart_id);
        if (new_char == '\n') {
            // Encountered end of a string.
            AppendCharToUARTBuf(new_char);
            printf("SCBS::ReceivePacket(): Received sentence %s", uart_rx_buf_);
            return strlen(uart_rx_buf_);
        } else if (uart_rx_buf_len_ >= kMaxUARTBufLen) {
            // String too long! Abort.
            printf("SCBS::ReceivePacket(): String too long! Aborting.\r\n");
            FlushUARTBuf();
        } else {
            AppendCharToUARTBuf(new_char);
        }
    }
    return 0; // Don't alert anyone until the UARTRxBuf gets a full packet.
}