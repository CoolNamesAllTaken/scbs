from scbs_utils import *

parser = argparse.ArgumentParser(description="SCBS master utilities.")
parser.add_argument("serial_port", type=str, help="Serial port to use (e.g. 'COM3').")
args = parser.parse_args()

ser = serial.Serial(args.serial_port,
        baudrate=9600,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=0.5 #[sec]
    )
ser.close()
ser.open()

while (True):
    transmit(ser, packetize("BSMWR,{},{}".format(1000, 2.5)))
    print("\tResponse: {}".format(ser.readline()))