import spidev
import RPi.GPIO as GPIO
import time
import struct

# GPIO pin setup
OPCODE1_PIN = 23
OPCODE2_PIN = 24

GPIO.setmode(GPIO.BCM)
GPIO.setup(OPCODE1_PIN, GPIO.OUT)
GPIO.setup(OPCODE2_PIN, GPIO.OUT)

# SPI setup
spi = spidev.SpiDev()
spi.open(0, 0)           # Open SPI0 (bus 0, device 0)
spi.max_speed_hz = 1000000  # 1 MHz for safety
spi.mode = 0b00

def float_to_bytes(f):
    return list(struct.pack('>f', f))  # Big-endian

def bytes_to_float(b):
    return struct.unpack('>f', bytes(b))[0]

def send_frame(operand1, operand2, opcode, read_result):
    # Set GPIO opcodes
    GPIO.output(OPCODE1_PIN, opcode & 0b01)
    GPIO.output(OPCODE2_PIN, (opcode >> 1) & 0b01)

    # Convert operands to bytes
    op1_bytes = float_to_bytes(operand1)
    op2_bytes = float_to_bytes(operand2)

    tx_frame = op1_bytes + op2_bytes
    # print(f"Sending: Operand1={operand1}, Operand2={operand2}, Opcode={opcode}")
    # print(f"TX Frame: {[hex(b) for b in tx_frame]}")

    # Perform SPI full-duplex transfer (send 64 bits, receive 64 bits)
    rx_frame = spi.xfer2(tx_frame)
    # print(f"RX Frame: {[hex(b) for b in rx_frame]}")

    # The result is expected in the last 4 bytes
    if read_result:
        result_bytes = rx_frame[4:]
        result = bytes_to_float(result_bytes)
        print(f"Received Result: {result}\n")
        return result
    else:
        return None

try:
    time.sleep(0.5)  # Wait for FPGA to be ready
    op_1 = 143.559997558594
    op_2 = -87.4369964599609
    # op_1 = 52.15652458487
    # op_2 = 15.54687121347
    print(f"Testing for operands {op_1}, {op_2}")
    tests = [
        (op_1,op_2, 0),  # Add
        (op_1,-op_2, 0),  # Sub
        (op_1,op_2, 2),  # Mul
    ]
    print("Results from FPGA: ")
    for a, b, opcode in tests:
        send_frame(a, b, opcode, False)
        time.sleep(0.1)
        send_frame(a, b, opcode , True)
        time.sleep(0.1)
    print("Results from PI: ")
    add = op_1 + op_2
    sub = op_1 - op_2
    mul = op_1 * op_2
    print(add)
    print(sub)
    print(mul)
    op_1 = 52.15652458487
    op_2 = 15.54687121347
    print(f"Testing for operands {op_1}, {op_2}")
    tests = [
        (op_1,op_2, 0),  # Add
        (op_1,-op_2, 0),  # Sub
        (op_1,op_2, 2),  # Mul
    ]
    print("Results from FPGA: ")
    for a, b, opcode in tests:
        send_frame(a, b, opcode, False)
        time.sleep(0.1)
        send_frame(a, b, opcode , True)
        time.sleep(0.1)
    print("Results from PI: ")
    add = op_1 + op_2
    sub = op_1 - op_2
    mul = op_1 * op_2
    print(add)
    print(sub)
    print(mul)



finally:
    GPIO.cleanup()
    spi.close()
