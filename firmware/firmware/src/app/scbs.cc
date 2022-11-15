#include "scbs.hh"
#include <stdio.h> // for printing
#include <stdlib.h> // for strtof

const uint16_t kMaxPWMCount = 1000; // Clock is 125MHz, shoot for 125kHz PWM.
const uint16_t kPWMDefaultDuty = 0; // out of kMaxPWMCount.
const float kPowerSupplyVoltage = 5.0f; // [V]
const float kMaxOutputVoltage = 4.5f; // [V]
const float kMinOutputVoltage = 0.0f; // [V]

const float kOutputVoltageCalCoeffX3 = -0.006624709f;
const float kOutputVoltageCalCoeffX2 = 0.07218648f;
const float kOutputVoltageCalCoeffX = -0.278278555f;
const float kOutputVoltageCalCoeffConst = 0.079586014f;

/** Public Functions **/

SCBS::SCBS(SCBSConfig_t config) {
    config_ = config;
}

void SCBS::Init() {
    // Set up status LED.
    gpio_init(config_.led_pin);
    gpio_set_dir(config_.led_pin, GPIO_OUT);

    // Set up comms UART.
    uart_init(config_.uart_id, config_.uart_baud);
    gpio_set_function(config_.uart_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(config_.uart_rx_pin, GPIO_FUNC_UART);

    uart_set_hw_flow(config_.uart_id, false, false); // no CTS/RTS
    uart_set_format(config_.uart_id, config_.uart_data_bits, config_.uart_stop_bits, config_.uart_parity);
    uart_set_fifo_enabled(config_.uart_id, true);

    memset(uart_rx_buf_, '\0', kMaxUARTBufLen);
    uart_rx_buf_len_ = 0;

    // Set up PWM output for voltage control.
    gpio_set_function(config_.pwm_pin, GPIO_FUNC_PWM);
    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(config_.pwm_pin);
    pwm_set_wrap(slice_num, kMaxPWMCount);
    pwm_set_chan_level(slice_num, config_.pwm_channel, kPWMDefaultDuty);
    pwm_set_enabled(slice_num, true);

    // give em a little blink
    gpio_put(config_.led_pin, 1);
    sleep_ms(100);
    gpio_put(config_.led_pin, 0);

    printf("SCBS::Init(): Init completed.\r\n");
}

void SCBS::Update() {
    // Communication Process
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
                    MRDPacketHandler(MRDPacket(uart_rx_buf_));
                    break;
                case BSPacket::MWR:
                    MWRPacketHandler(MWRPacket(uart_rx_buf_));
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
    
    // GPIO Process
    SetOutputVoltage(output_voltage_);
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

void SCBS::MWRPacketHandler(MWRPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MWRPacketHandler: Formed a valid MWR packet!\r\n");

        switch(packet_in.reg_addr) {
            case kRegAddrSetOutputVoltage: {
                float new_output_voltage = strtof(packet_in.value, NULL);
                if (new_output_voltage > kMaxOutputVoltage) {
                    new_output_voltage = kMaxOutputVoltage;
                } else if (new_output_voltage < kMinOutputVoltage) {
                    new_output_voltage = kMinOutputVoltage;
                }
                output_voltage_ = new_output_voltage;
                break;
            } case kRegAddrReadOutputCurrent: {
                printf("SCBS::MWRPacketHandler: Writing to register 0x%x is not supported.\r\n", packet_in.reg_addr);
                return; // drop incoming packet.
                break;
            } default: {
                printf("SCBS::MWRPacketHandler: Register address 0x%x was not recognized.\r\n", packet_in.reg_addr);
                return; // drop incoming packet.
            }
        }
        // Pass to next device in the chain.
        TransmitPacket(packet_in);
    } else {
        printf("SCBS::MRDPacketHandler: Formed an MRD packet but it wasn't valid!\r\n");
    }
}

void SCBS::MRDPacketHandler(MRDPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MRDPacketHandler: Formed a valid MRD packet!\r\n");
        if (packet_in.num_values >= MRDPacket::kMaxNumValues) {
            printf("SCBS::MRDPacketHandler: Incoming packet had too many values! Throwing a tantrum to draw attention.\r\n");
            return; // drop incoming packet
        }
        char my_value[BSPacket::kMaxPacketFieldLen] = "";
        memset(my_value, '\0', BSPacket::kMaxPacketFieldLen);
        switch(packet_in.reg_addr) {
            case kRegAddrSetOutputVoltage:
                snprintf(my_value, BSPacket::kMaxPacketFieldLen-1, "%.2f", output_voltage_);
                break;
            case kRegAddrReadOutputCurrent:
                snprintf(my_value, BSPacket::kMaxPacketFieldLen-1, "%.2f", output_current_);
                break;
            default:
                printf("SCBS::MRDPacketHandler: Register address 0x%x was not recognized.\r\n", packet_in.reg_addr);
                return; // drop incoming packet.
        }
        // Frankenstein new value into the received packet buffer and send it to the next device.
        strncpy(packet_in.values[packet_in.num_values], my_value, MRDPacket::kMaxPacketFieldLen);
        MRDPacket packet_out = MRDPacket(packet_in.reg_addr, packet_in.values, packet_in.num_values+1);
        TransmitPacket(packet_out);
    } else {
        printf("SCBS::MRDPacketHandler: Formed an MRD packet but it wasn't valid!\r\n");
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

void SCBS::SetOutputVoltage(float voltage) {
    // Relies on MWR handler to rail the voltage to nice setpoints. Absolute rails here.
    float voltage2 = voltage*voltage;
    float voltage3 = voltage2*voltage;
    float estimated_offset = 
        voltage3*kOutputVoltageCalCoeffX3 + 
        voltage2*kOutputVoltageCalCoeffX2 + 
        voltage*kOutputVoltageCalCoeffX + 
        kOutputVoltageCalCoeffConst;
    voltage -= estimated_offset; // calibrate
    if (voltage > kPowerSupplyVoltage) {
        voltage = kPowerSupplyVoltage;
    } else if (voltage < 0.0f) {
        voltage = 0.0f;
    }
    uint16_t duty = 1000 - static_cast<uint16_t>(voltage / kPowerSupplyVoltage * kMaxPWMCount); // out of kMaxPWMCount
    pwm_set_chan_level(pwm_gpio_to_slice_num(config_.pwm_pin), config_.pwm_channel, duty);
}