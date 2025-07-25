import serial
import sys

port = serial.Serial(sys.argv[1], 115200, timeout=1)

print('writing transmit command')
port.write(b'\x01\x05\x00\x00\x03\x00\x07\x12\x02')

print('reading response')
assert port.read(6) == b'\x01\x05\x00\x00\x00\x00', f'Unexpected response to transmit command'

print('writing receive command')
port.write(b'\x01\x0e\x02\x00\x00\x00')

print('reading response')
response = port.read(8)
assert response[:6] == b'\x01\x0E\x00\x00\x02\x00', f'Unexpected response to receive command'

assert response[6:8] != b'\xff\xff', f'NXP Board seems to be stuck in reset mode'

print(f'Firmware appears to be {response[7]}.{response[6]}')