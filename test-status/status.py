import serial
import time

SERIAL_PORT = "/dev/cu.usbmodem1101"
BAUD = 115200

HEADER_1 = 0xAA
HEADER_2 = 0x55

MSG_STATUS = 0x01

STATUS_IDLE = 0x00
STATUS_RECEIVED = 0x01
STATUS_SAVED = 0x02
STATUS_POSTING = 0x03
STATUS_POSTED = 0x04
STATUS_SLEEPING = 0x05
STATUS_ERROR = 0x06


ser = serial.Serial(SERIAL_PORT, BAUD)

# Give the Pico time to reset after opening serial
time.sleep(2)


def send_status(status):
    packet = bytes([
        HEADER_1,
        HEADER_2,
        MSG_STATUS,
        1,
        status
    ])

    ser.write(packet)
    ser.flush()

    print(f"Sent status: {status}")


states = [
    ("RECEIVED", STATUS_RECEIVED),
    ("SAVED", STATUS_SAVED),
    ("POSTING", STATUS_POSTING),
    ("POSTED", STATUS_POSTED),
    ("SLEEPING", STATUS_SLEEPING),
    ("ERROR", STATUS_ERROR),
    ("IDLE", STATUS_IDLE),
]


try:
    while True:

        for name, status in states:
            print(f"--- {name} ---")

            send_status(status)

            time.sleep(5)

finally:
    ser.close()