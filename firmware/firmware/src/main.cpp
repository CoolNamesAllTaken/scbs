#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
// #include "scbs_comms.hh"
#include "scbs.hh"
// #include "pa1616s.hh"
// #include "epaper.hh"
// #include "geeps_gui.hh"
// #include "gui_bitmaps.hh"

SCBS * scbs = NULL;

// #define COMMS_UART_ID         uart1
// #define COMMS_UART_BAUD       9600
// #define COMMS_UART_DATA_BITS  8
// #define COMMS_UART_STOP_BITS  1
// #define COMMS_UART_PARITY     UART_PARITY_NONE

// #define COMMS_UART_TX_PIN 4 // UART1 TX
// #define COMMS_UART_RX_PIN 5 // UART1 RX

// const uint16_t LED_PIN = 25;

// const uint16_t kMaxUARTBufLen = 200;
// char uart_rx_buf[kMaxUARTBufLen];
// uint16_t uart_rx_buf_len = 0;

// uint16_t cell_id = 0;

// void InitUART() {
    
// }

// void FlushUARTBuf() {
//     memset(uart_rx_buf, '\0', kMaxUARTBufLen);
//     uart_rx_buf_len = 0;
// }

// template <class PacketType>
// void TransmitPacket(PacketType packet) {
//     char uart_tx_buf[kMaxUARTBufLen];
//     packet.ToString(uart_tx_buf);
//     uart_puts(COMMS_UART_ID, uart_tx_buf);
// }

// /**
//  * @brief Cell Discover packet handler. Enumerates the device and sends out a packet to the next
//  * device in the chain.
//  * @param[in] packet Incoming BSDIS packet.
// */
// void DISPacketHandler(DISPacket packet_in) {
//     if (packet_in.IsValid()) {
//         printf("scbs:        Formed a valid DIS packet!\r\n");
//         cell_id = packet_in.last_cell_id + 1;
//         DISPacket packet_out = DISPacket(cell_id);
//         TransmitPacket(packet_out);
//     } else {
//         printf("scbs:        Formed a DIS packet but it wasn't valid!\r\n");
//     }
// }



// void MRDPacketHandler(MRDPacket packet) {

// }

// void ReceivePacket() {
//     while (uart_is_readable(COMMS_UART_ID)) {
//         char new_char = uart_getc(COMMS_UART_ID);
//         if (new_char == '\n') {
//             // Encountered end of a string.
//             strncat(uart_rx_buf, &new_char, 1); // add new char to end of buffer
//             gpio_put(LED_PIN, 1);
//             printf("scbs: Received sentence %s", uart_rx_buf);
//             BSPacket packet = BSPacket(uart_rx_buf);
//             if (packet.IsValid()) {
//                 printf("scbs:     Packet is valid!\r\n");
//                 if (packet.GetPacketType() == BSPacket::DIS) {
//                     DISPacketHandler(DISPacket(uart_rx_buf));
//                 } /*else if (packet.GetPacketType() == BSPacket::MRD) {
//                     MRDPacketHandler(MRDPacket(uart_rx_buf, uart_rx_buf_len));
//                 }*/
//             } else {
//                 printf("scbs:     Packet is invalid.\r\n");
//             }
//             FlushUARTBuf();
//             gpio_put(LED_PIN, 0);
//         } else if (uart_rx_buf_len >= kMaxUARTBufLen) {
//             // String too long! Abort.
//             printf("scbs: String too long! Aborting.\r\n");
//             FlushUARTBuf();
//         } else {
//             strncat(uart_rx_buf, &new_char, 1); // add new char to end of buffer
//             uart_rx_buf_len++;
//         }
//     }
// }




int main() {
    bi_decl(bi_program_description("This is a test binary."));
    // bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    stdio_init_all();

    

    puts("Hi hello starting program.\r\n");
    SCBS::SCBSConfig_t scbs_config;
    scbs = new SCBS(scbs_config);
    scbs->Init();
    // gpio_put(LED_PIN, 1);

    // InitUART();
    // FlushUARTBuf();

    while (true) {
        // sleep_ms(100);
        // gpio_put(LED_PIN, 1);
        // sleep_ms(100);
        // gpio_put(LED_PIN, 0);
        scbs->Update();
        // ReceivePacket();
        // uart_putc(COMMS_UART_ID, 'a');
    }
}