import serial
import time

SERIAL_PORT = "/dev/cu.usbmodem1101"
BAUD = 115200

HEADER_1 = 0xAA
HEADER_2 = 0x55

MSG_STATUS = 0x01

STATUS_IDLE= 0x00
STATUS_RUN = 0x01
STATUS_TWIST= 0x02


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
    ("RUN", STATUS_IDLE),
    ("JUMP", STATUS_RUN),
    ("TWIST", STATUS_TWIST),
]


try:
    while True:

        for name, status in states:
            print(f"--- {name} ---")

            send_status(status)

            time.sleep(6)

finally:
    ser.close()