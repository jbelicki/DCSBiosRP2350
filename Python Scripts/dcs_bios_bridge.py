# -*- coding: utf-8 -*-

#TODO: Add support for multiple COM ports

import socket
import struct
import serial
import threading
import time

# === CONFIGURATION ===
UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 5010      # UDP listening port
UDP_DEST_IP = "127.0.0.1"  # Destination for serial data over UDP
UDP_DEST_PORT = 7778  # Destination UDP port

MULTICAST_GROUP = "239.255.50.10"  # Multicast group
MULTICAST_INTERFACE = "0.0.0.0"    # Interface to join multicast

BAUDRATE = 115200  # Serial baud rate

# Prompt user for serial port
SERIAL_PORT = input("Enter serial port (e.g., COMx) or press Enter to disable serial: ").strip()
ENABLE_SERIAL = bool(SERIAL_PORT)


# === SETUP UDP SOCKET ===
udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Allow address reuse
udp_sock.bind((UDP_IP, UDP_PORT))

# Join multicast group
mreq = struct.pack("=4sl", socket.inet_aton(MULTICAST_GROUP), socket.INADDR_ANY)
udp_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

# === SERIAL CONNECTION ===
ser = None
if ENABLE_SERIAL:
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.1)
        print(f"Connected to serial: {SERIAL_PORT} at {BAUDRATE} baud")
    except serial.SerialException as e:
        print(f"Serial error: {e}. Serial communication disabled.")
        ENABLE_SERIAL = False

# === SERIAL HANDLER ===
def serial_to_udp():
    global ser
    if not ENABLE_SERIAL:
        print("Serial connection disabled. Skipping serial_to_udp thread.")
        return

    while True:
        try:
            if ser and ser.is_open and ser.in_waiting:
                data = ser.read(ser.in_waiting)
                if data:
                    clean_data = data.replace(b'\r\n', b'\n').replace(b'\r', b'\n')
                    print(f"[SERIAL -> UDP] {clean_data.decode(errors='ignore')}", end='')
                    udp_sock.sendto(clean_data, (UDP_DEST_IP, UDP_DEST_PORT))
            else:
                time.sleep(0.005)
        except (serial.SerialException, PermissionError) as e:
            print(f"[SERIAL READ ERROR] {e}. Attempting to reopen serial port...")
            try:
                ser.close()
            except Exception:
                pass  # Ignore close error
            time.sleep(3)
            try:
                ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.1)
                print(f"[SERIAL RECOVERY] Reconnected to {SERIAL_PORT}")
            except Exception as e:
                print(f"[SERIAL RECOVERY FAILED] {e}")
                time.sleep(5)
        except Exception as e:
            print(f"[UNEXPECTED SERIAL ERROR] {e}")
            time.sleep(5)


def is_dcsbios_export_packet(data):
    return len(data) >= 2 and data[0] == 0x55 and data[1] == 0x55

def udp_to_serial():
    global ser
    print("UDP listener started...")
    
    while True:
        try:
            data, addr = udp_sock.recvfrom(1024)

            # ONLY forward if it's a DCS-BIOS export frame
            if not is_dcsbios_export_packet(data):
                continue

            if ENABLE_SERIAL and ser and ser.is_open:
                try:
                    ser.write(data)
                    time.sleep(0.001)  # Prevent flood
                except Exception as e:
                    print(f"[SERIAL WRITE ERROR] {e}")
        except Exception as e:
            print(f"[UDP RECEPTION ERROR] {e}")
            time.sleep(1)
            
# === START THREADS ===
udp_thread = threading.Thread(target=udp_to_serial, daemon=True)
serial_thread = threading.Thread(target=serial_to_udp, daemon=True)

udp_thread.start()
serial_thread.start()

# Keep main thread running
while True:
    time.sleep(1)
