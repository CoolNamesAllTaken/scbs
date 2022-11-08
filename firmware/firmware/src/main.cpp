#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
// #include "pa1616s.hh"
// #include "epaper.hh"
// #include "geeps_gui.hh"
// #include "gui_bitmaps.hh"

#define COMMS_UART_ID         uart1
#define COMMS_UART_BAUD       9600
#define COMMS_UART_DATA_BITS  8
#define COMMS_UART_STOP_BITS  1
#define COMMS_UART_PARITY     UART_PARITY_NONE

#define COMMS_UART_TX_PIN 4 // UART1 TX
#define COMMS_UART_RX_PIN 5 // UART1 RX

const uint16_t LED_PIN = 25;

// PA1616S * gps = NULL;
// EPaperDisplay * display = NULL;
// GUIStatusBar * status_bar = NULL;

// void RefreshGPS() {
//     gpio_put(LED_PIN, 1);
//     gps->Update();
//     gpio_put(LED_PIN, 0);
//     gps->latest_gga_packet.GetUTCTimeStr(status_bar->time_string);
//     gps->latest_gga_packet.GetLatitudeStr(status_bar->latitude_string);
//     gps->latest_gga_packet.GetLongitudeStr(status_bar->longitude_string);
//     status_bar->num_satellites = gps->latest_gga_packet.GetSatellitesUsed();
// }

// void RefreshScreen() {
//     // display->Clear();
//     display->Clear();
//     status_bar->Draw();
//     // Latitude
//     // char latitude_str[NMEAPacket::kMaxPacketFieldLen];
//     // char longitude_str[NMEAPacket::kMaxPacketFieldLen];
//     // gps->latest_gga_packet.GetLatitudeStr(latitude_str);
//     // gps->latest_gga_packet.GetLongitudeStr(longitude_str);
//     // display->DrawText(0, 200, latitude_str);
//     // display->DrawText(0, 220, longitude_str);

//     display->Update();
// }

int main() {
    bi_decl(bi_program_description("This is a test binary."));
    bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    puts("Hi hello starting program.\r\n");
    gpio_put(LED_PIN, 1);

    while (true) {
        sleep_ms(100);
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
    }
}