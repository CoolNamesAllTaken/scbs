#include "scbs.hh"
#include <stdio.h> // for printing
#include <stdlib.h> // for strtof

const uint16_t kMaxPWMCount = 1000; // Clock is 125MHz, shoot for 125kHz PWM.
const uint16_t kPWMDefaultDuty = 0; // out of kMaxPWMCount.
const float kPowerSupplyVoltage5V = 5.0f; // [V]
const float kMaxOutputVoltage = 4.5f; // [V]
const float kMinOutputVoltage = 0.0f; // [V]

const float kOutputVoltageCalCoeffX3 = -0.006624709f;
const float kOutputVoltageCalCoeffX2 = 0.07218648f;
const float kOutputVoltageCalCoeffX = -0.278278555f;
const float kOutputVoltageCalCoeffConst = 0.079586014f;

const uint32_t kPacketReceivedBlinkTimeMs = 10;
const uint32_t kRegisterWriteBlinkTimeMs = 500;
const uint32_t kRegisterReadBlinkTimeMs = 100;

// const float kPowerSupplyVoltage3V3 = 3.3f;
const uint16_t kMaxADCCount = 1<<12;
// const float kADCConversionFactor = kPowerSupplyVoltage3V3 / kMaxADCCount;
const float kMaxCsenseCurrent = 200.0f;

/** Public Functions **/

/**
 * @brief Constructor, copies configuration into the new SCBS object.
*/
SCBS::SCBS(SCBSConfig_t config) {
    config_ = config;
}

/**
 * @brief Init function, should be called only once when peripherals are being initialized.
*/
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

    // Set up current sense ADC input.
    adc_init();
    adc_gpio_init(config_.csense_pin);

    // give em a little blink
    gpio_put(config_.led_pin, 1);
    sleep_ms(100);
    gpio_put(config_.led_pin, 0);

    printf("SCBS::Init(): Init completed.\r\n");
}

/**
 * @brief Update function, should be called every loop.
*/
void SCBS::Update() {
    // Communication Process
    if (ReceivePacket() != 0) {
        TurnOnStatusLED(kPacketReceivedBlinkTimeMs);

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
                    SRDPacketHandler(SRDPacket(uart_rx_buf_));
                    break;
                case BSPacket::SWR:
                    SWRPacketHandler(SWRPacket(uart_rx_buf_));
                    break;
                case BSPacket::SRS:
                    SRSPacketHandler(SRSPacket(uart_rx_buf_));
                    break;
                default:
                    printf("SCBS::Update():     Unrecognized packet type.\r\n");
            }
        } else {
            printf("SCBS::Update(): Packet is invalid.\r\n");
        }
        FlushUARTBuf();
    }
    
    // GPIO Process
    SetOutputVoltage(output_voltage_);
    ReadOutputCurrent();

    // Update status LED
    if (status_led_on_ && time_us_32() > status_led_off_timestamp_) {
        gpio_put(config_.led_pin, 0);
        status_led_on_ = false;
    }
}

/**
 * @brief Returns the ID number of the SCBS object.
*/
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
        TransmitError(kErrCodeReceivedInvalidPacket);
    }
}

/**
 * @brief Handler for an MWR (Multipl WRite) packet. Performs a register write and forwards the packet to the next device,
 * or returns an SRS packet with an error code if something went wrong.
 * @retval packet_in Incoming MWR packet.
*/
void SCBS::MWRPacketHandler(MWRPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MWRPacketHandler: Formed a valid MWR packet!\r\n");

        uint16_t err_code = WriteRegister(packet_in.reg_addr, packet_in.value);
        if (err_code != kErrCodeNone) {
            printf("SCBS::MWRPacketHandler: Register write to address 0x%X failed with code 0x%X.\r\n", packet_in.reg_addr, err_code);
            TransmitError(err_code);
            return; // drop original packet
        } else {
            // Pass to next device in the chain.
            TransmitPacket(packet_in);
        }
    } else {
        printf("SCBS::MWRPacketHandler: Formed an MWR packet but it wasn't valid!\r\n");
        TransmitError(kErrCodeReceivedInvalidPacket);
    }
}

/**
 * @brief Handler for an MRD (Multiple ReaD) packet. Performs a register read and appends the value to the end of the
 * MRD packet and forwards to the next device, or returns an SRS packet with an error code if something went wrong.
 * @param[in] packet_in Incoming MRD packet.
*/
void SCBS::MRDPacketHandler(MRDPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MRDPacketHandler: Formed a valid MRD packet!\r\n");
        if (packet_in.num_values >= MRDPacket::kMaxNumValues) {
            printf("SCBS::MRDPacketHandler: Incoming packet had too many values! Throwing a tantrum to draw attention.\r\n");
            TransmitError(kErrCodePacketLengthExceeded);
            return; // drop original packet
        }
        char my_value[BSPacket::kMaxPacketFieldLen] = "";
        memset(my_value, '\0', BSPacket::kMaxPacketFieldLen);
        uint16_t err_code = ReadRegister(packet_in.reg_addr, my_value);
        if (err_code != kErrCodeNone) {
            printf("SCBS::MRDPacketHandler: Register read from address 0x%X failed with code 0x%X.\r\n", packet_in.reg_addr, err_code);
            TransmitError(err_code);
        } else {
            // Frankenstein new value into the received packet buffer and send it to the next device.
            strncpy(packet_in.values[packet_in.num_values], my_value, MRDPacket::kMaxPacketFieldLen);
            MRDPacket packet_out = MRDPacket(packet_in.reg_addr, packet_in.values, packet_in.num_values+1);
            TransmitPacket(packet_out);
        }
    } else {
        printf("SCBS::MRDPacketHandler: Formed an MRD packet but it wasn't valid!\r\n");
        TransmitError(kErrCodeReceivedInvalidPacket);
    }
}

/**
 * @brief Handler for an SWR (Single WRite) packet. Performs a register write and responds with an SRS packet
 * containing the error code if this is the cell being written to, otherwise forwards the packet if it's valid.
 * @param[in] packet_in Incoming SWR packet.
*/
void SCBS::SWRPacketHandler(SWRPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MWRPacketHandler: Formed a valid SWR packet!\r\n");

        if (packet_in.cell_id == cell_id_) {
            // This single write packet is destined for me! Process and send a response.
            uint16_t err_code = WriteRegister(packet_in.reg_addr, packet_in.value);
            TransmitError(err_code); // Send back error code or OK if all went well.
        } else {
            TransmitPacket(packet_in); // It's for someone else, forward to next device.
        }
    } else {
        printf("SCBS::SWRPacketHandler: Formed an SWR packet but it wasn't valid!\r\n");
        TransmitError(kErrCodeReceivedInvalidPacket);
    }
}

/**
 * @brief Handler for an SRD (Single ReaD) packet. Performs a register read and responds with an
 * SRS packet containing the error code or read value if this is the cell being read from, otherwise
 * forwards the packet if it's valid.
 * @param[in] packet_in Incoming SRD packet.
*/
void SCBS::SRDPacketHandler(SRDPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MWRPacketHandler: Formed a valid SRD packet!\r\n");

        if (packet_in.cell_id == cell_id_) {
            // This single packet read is destined for me! Process and send a response.
            char my_value[BSPacket::kMaxPacketFieldLen] = "";
            memset(my_value, '\0', BSPacket::kMaxPacketFieldLen);
            uint16_t err_code = ReadRegister(packet_in.reg_addr, my_value);
            if (err_code != kErrCodeNone) {
                TransmitError(err_code); // Something went wrong, send back an error code.
            } else {
                SRSPacket packet_out = SRSPacket(cell_id_, my_value);
                TransmitPacket(packet_out); // Send back the value that was read.
            }
        } else {
            TransmitPacket(packet_in); // It's for someone else, forward to next device.
        }
    } else {
        printf("SCBS::SWRPacketHandler: Formed an SRD packet but it wasn't valid!\r\n");
        TransmitError(kErrCodeReceivedInvalidPacket);
    }
}

/**
 * @brief Handler for an SRS (Single ReSponse) packet. Forwards the packet if it's valid.
 * @param[in] packet_in Incoming SRS packet.
*/
void SCBS::SRSPacketHandler(SRSPacket packet_in) {
    if (packet_in.IsValid()) {
        printf("SCBS::MWRPacketHandler: Formed a valid SRS packet!\r\n");
        TransmitPacket(packet_in);
    } else {
        printf("SCBS::SRSPacketHandler: Formed an SRS packet but it wasn't valid!\r\n");
        TransmitError(kErrCodeReceivedInvalidPacket);
    }
}

/**
 * @brief Converts a value from a string to the relevant datatype and writes it to a register. Called by various packet handler functions.
 * @param[in] reg_addr Address of register to read.
 * @param[in] value_in String buffer to read value from.
 * @retval Error code, or kErrCodeNone if write succeeded.
*/
uint16_t SCBS::WriteRegister(uint32_t reg_addr, char value_in[BSPacket::kMaxPacketFieldLen]) {
    switch(reg_addr) {
        case kRegAddrSetOutputVoltage: {
            float new_output_voltage = strtof(value_in, NULL);
            output_voltage_ = SetOutputVoltage(new_output_voltage);
            break;
        } case kRegAddrReadOutputCurrent: {
            printf("SCBS::WriteRegister: Writing to register 0x%X is not supported.\r\n", reg_addr);
            return kErrCodeWriteNotSupported;
            break;
        } default: {
            printf("SCBS::MWRPacketHandler: Register address 0x%x was not recognized.\r\n", reg_addr);
            return kErrCodeAddrNotRecognized;
        }
    }
    TurnOnStatusLED(kRegisterWriteBlinkTimeMs);
    return kErrCodeNone;
}

/**
 * @brief Reads a register and returns the corresponding value as a string. Called by various packet handler functions.
 * @param[in] reg_addr Address of register to read.
 * @param[out] value_out String buffer to read value into.
 * @retval Error code, or kErrCodeNone if read succeeded.
*/
uint16_t SCBS::ReadRegister(uint32_t reg_addr, char value_out[BSPacket::kMaxPacketFieldLen]) {
    switch(reg_addr) {
        case kRegAddrSetOutputVoltage:
            snprintf(value_out, BSPacket::kMaxPacketFieldLen-1, "%.2f", output_voltage_);
            break;
        case kRegAddrReadOutputCurrent:
            snprintf(value_out, BSPacket::kMaxPacketFieldLen-1, "%.2f", output_current_);
            break;
        case kRegAddrReadFirmwareVersion:
            snprintf(value_out, BSPacket::kMaxPacketFieldLen-1, SCBS_FIRMWARE_VERSION);
            break;
        default:
            printf("SCBS::ReadRegister: Register address 0x%x was not recognized.\r\n", reg_addr);
            return kErrCodeAddrNotRecognized;
    }
    TurnOnStatusLED(kRegisterReadBlinkTimeMs);
    return kErrCodeNone;
}

/**
 * @brief Clears the UART buffer by setting it all to end of string characters and zeroing the length. Used after a packet
 * has been ingested.
*/
void SCBS::FlushUARTBuf() {
    memset(uart_rx_buf_, '\0', kMaxUARTBufLen);
    uart_rx_buf_len_ = 0;
}

/**
 * @brief Adds a character to the UART buffer (for receiving packets) and updates the length of the buffer.
 * @param[in] new_char Character to add to the end of the UART buffer.
*/
void SCBS::AppendCharToUARTBuf(char new_char) {
    // add new char to end of buffer
    strncat(uart_rx_buf_, &new_char, 1);
    uart_rx_buf_len_++;
}

/**
 * @brief Forms a SRS packet with the given error code and sends it. Error codes are printed in hex and prefixed with "ERR:".
 * The success case has kErrCodeNone replaced with "OK".
 * @param[in] err_code Error code to place in the SRS packet, or kErrCodeNone if success.
*/
void SCBS::TransmitError(uint16_t err_code) {
    char err_str[BSPacket::kMaxPacketFieldLen];
    memset(err_str, '\0', BSPacket::kMaxPacketFieldLen);
    if (err_code == kErrCodeNone) {
        snprintf(err_str, BSPacket::kMaxPacketFieldLen-1, "OK");
    } else {
        snprintf(err_str, BSPacket::kMaxPacketFieldLen-1, "ERR:%X", err_code);
    }
    
    SRSPacket packet_out = SRSPacket(cell_id_, err_str);
    TransmitPacket(packet_out);
}

/**
 * @brief Transmits a BSPacket or one of its child classes by turning it into a string and dumping it onto the UART.
*/
template <class PacketType>
void SCBS::TransmitPacket(PacketType packet) {
    char uart_tx_buf[kMaxUARTBufLen];
    packet.ToString(uart_tx_buf);
    uart_puts(config_.uart_id, uart_tx_buf);
    uart_puts(config_.uart_id, (char *)"\r\n");
}

/**
 * @brief Ingests any available characters from the UART. Returns the length of the packet received once
 * a full packet has been ingested. Looks for the '\n' character to find the end of a packet.
 * @retval 0 if a full packet hasn't yet been received, strlen of packet if a full packet has been received.
*/
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

/**
 * @brief Sets the output voltage.
 * @param[in] voltage Voltage to set on the output (in Volts).
 * @retval Target voltage that was actually set after bounds were enforced.
*/
float SCBS::SetOutputVoltage(float voltage) {
    // Enforce rails set by SCBS device specs.
    if (voltage > kMaxOutputVoltage) {
        voltage = kMaxOutputVoltage;
    } else if (voltage < kMinOutputVoltage) {
        voltage = kMinOutputVoltage;
    }

    // Relies on MWR handler to rail the voltage to nice setpoints. Absolute rails here.
    float voltage2 = voltage*voltage;
    float voltage3 = voltage2*voltage;
    float estimated_offset = 
        voltage3*kOutputVoltageCalCoeffX3 + 
        voltage2*kOutputVoltageCalCoeffX2 + 
        voltage*kOutputVoltageCalCoeffX + 
        kOutputVoltageCalCoeffConst;

    
    float pwm_voltage = voltage - estimated_offset; // calibrate
    // Make sure calibration didn't take things off the rails.
    if (pwm_voltage > kPowerSupplyVoltage5V) {
        pwm_voltage = kPowerSupplyVoltage5V;
    } else if (pwm_voltage < 0.0f) {
        pwm_voltage = 0.0f;
    }
    uint16_t duty = 1000 - static_cast<uint16_t>(pwm_voltage / kPowerSupplyVoltage5V * kMaxPWMCount); // out of kMaxPWMCount
    pwm_set_chan_level(pwm_gpio_to_slice_num(config_.pwm_pin), config_.pwm_channel, duty);

    return voltage; // Return voltage railed by SCBS specs.
}

/**
 * @brief Reads the current sense ADC input and updates the SCBS object's internal output current value.
*/
void SCBS::ReadOutputCurrent() {
    adc_select_input(config_.csense_adc_input);
    uint16_t adc_counts = adc_read();
    output_current_ = adc_counts * kMaxCsenseCurrent / kMaxADCCount;
}

/**
 * @brief Returns the last updated output current (does not read the ADC).
 * @retval output_current_ Output current, in milliamps.
*/
float SCBS::GetOutputCurrent() {
    return output_current_;
}

/**
 * @brief Turns on the status LED for the designated interval. Relies on Update() to turn off the LED after the interval
 * has elapsed (does not busy wait).
 * @param[in] on_time_ms Length of time to keep the LED on for.
*/
void SCBS::TurnOnStatusLED(uint32_t on_time_ms) {
    gpio_put(config_.led_pin, 1);
    status_led_on_ = true;
    status_led_off_timestamp_ = time_us_32() + 1e3*on_time_ms;
    // NOTE: time_us_32() will loop every 1hr 11min 35sec and could cause an abnormally short blink
}