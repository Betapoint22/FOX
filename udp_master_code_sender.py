import socket
import pyaudio
import struct

UDP_IP = "192.168.1.100"  # Change to the ESP32's IP address
UDP_PORT = 12345

CHUNK = 1024
FORMAT = pyaudio.paInt24
CHANNELS = 1
RATE = 44100

p = pyaudio.PyAudio()
stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK)

print("Recording...")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    while True:
        data = stream.read(CHUNK)
        packed_data = b''.join(struct.pack('<s', byte) for byte in data)  # Convert 24-bit audio to bytes
        sock.sendto(packed_data, (UDP_IP, UDP_PORT))
except KeyboardInterrupt:
    print("Stopping recording...")
    stream.stop_stream()
    stream.close()
    p.terminate()
    sock.close()
