import serial
import time

s = serial.Serial(
    port="/dev/tty.usbserial-AB0NW409", baudrate=230400, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE
)

packet = bytearray();
packet.append(ord('R'));
packet.append(ord('E'));
packet.append(ord('A'));
packet.append(ord('D'));

packet.append(0x00);
packet.append(0x00);
packet.append(0xea);
packet.append(0x0d);

packet.append(0x13);
packet.append(0x02);

time.sleep(.25);
s.write(packet);
time.sleep(.5);

