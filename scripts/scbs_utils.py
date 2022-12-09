import argparse
from ast import literal_eval
import serial

parser = argparse.ArgumentParser(description="SCBS master utility.")
parser.add_argument("serial_port", type=str, help="Serial port to use (e.g. 'COM3').")
# parser.add_argument("command", type="str", help=
# """
# Command to run. Options include:
# DIS - Cell Discover
# MRD - Multi Read
# MWR - Multi Write
# SRD - Single Read*
# SWR - Single Write*
# SRS - Single Response*
# """)
args = parser.parse_args()

MAX_REG_ADDR = 0x9999

def validate_address(address_str):
    address = literal_eval(address_str)
    if (address > MAX_REG_ADDR or address < 0):
        raise Exception("Address {:x} is out of range.".format(address))

def calculate_checksum(contents_str):
    """
    @brief Calculates XOR checksum of a string.
    @param[in] contents_str String including header and contents (what would go between $ and *).
    @retval Checksum (integer).
    """
    checksum = 0
    for i in range(len(contents_str)):
        checksum ^= ord(contents_str[i]) # XOR with character's ASCII value
    
    return checksum

def packetize(contents_str):
    """
    @brief Adds $ and * token, as well as checksum.
    @param[in] contents_str String including header and contents (what would go between $ and *).
    @retval Complete packet string.
    """
    return "${}*{:x}\r\n".format(contents_str, calculate_checksum(contents_str))

def transmit(port, packet):
    print("\tSending: {}".format(packet), end="")
    port.write(bytes(packet, 'utf-8'))
    port.flush()


# def transmit()

def send_dis():
    args_str = input("Enter ID of first cell:")
    print()

def main():
    print(
"""Welcome to the SCBS Master Utility!
Supported Commands:
    DIS - Cell Discover
        DIS <PREV_CELL_ID>
    MRD - Multi Read
        MRD <REG_ADDR>
    MWR - Multi Write
        MWR <REG_ADDR> <VALUE>
    SRD - Single Read
        SRD <CELL_ID> <REG_ADDR>
    SWR - Single Write
        SWR <CELL_ID> <REG_ADDR> <VALUE>
    SRS - Single Response
        SRS <CELL_ID> <VALUE>
Type EXIT to quit."""
    )
    ser = serial.Serial(args.serial_port,
        baudrate=9600,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=5 #[sec]
    )
    ser.close()
    ser.open()
    while(True):
        command = input(">>> ")
        command_words = command.split(" ")
        num_args = len(command_words)
        if command_words[0].lower() == "exit":
            return
        elif command_words[0] == "DIS":
            if (num_args != 2):
                print("Invalid number of arguments for BSDIS! Excpected 1 but got {}.".format(num_args))
                continue
            transmit(ser, packetize("BSDIS,{}".format(command_words[1])))
            print("\tResponse: {}".format(ser.readline()))
        elif command_words[0] == "MRD":
            if (num_args != 2):
                print("Invalid number of arguments for BSDIS! Excpected 1 but got {}.".format(num_args))
                continue
            transmit(ser, packetize("BSMRD,{}".format(command_words[1])))
            print("\tRespons: {}".format(ser.readline()))
        elif command_words[0] == "MWR":
            if (num_args != 3):
                print("Invalid number of arguments for BSDIS! Excpected 2 but got {}.".format(num_args))
                continue
            transmit(ser, packetize("BSMWR,{},{}".format(command_words[1], command_words[2])))
            print("\tResponse: {}".format(ser.readline()))
        elif command_words[0] == "SRD":
            if (num_args != 3):
                print("Invalid number of arguments for BSSRD! Expected 3 but got {}.".format(num_args))
                continue
            transmit(ser, packetize("BSSRD,{},{}".format(command_words[1], command_words[2])))
            print("\tResponse: {}".format(ser.readline()))
        elif command_words[0] == "SWR":
            if (num_args != 4):
                print("Invalid number of arguments for BSSRD! Expected 4 but got {}.".format(num_args))
                continue
            transmit(ser, packetize("BSSWR,{},{},{}".format(command_words[1], command_words[2], command_words[3])))
            print("\tResponse: {}".format(ser.readline()))
        elif command_words[0] == "SRS":
            if (num_args != 3):
                print("Invalid number of arguments for BSSRS! Expected 3 but got {}.".format(num_args))
                continue
            transmit(ser, packetize("BSSRS,{},{}".format(command_words[1], command_words[2])))
            print("\tResponse: {}".format(ser.readline()))
        else:
            print("Unrecognized argument.")
            
    
    ser.close()
    # print(calculate_checksum("BSDIS,0"))

if __name__ == "__main__":
    main()
