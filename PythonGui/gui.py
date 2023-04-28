import tkinter as tk
from tkinter import ttk

import serial
import serial.tools.list_ports

import time

import struct

import threading

# Add a global variable to keep track of the last update time
time_last_updated = 0
update_threshold = 0.5  # seconds

def config_serial():
    # Get a list of available COM ports and print them out for the user to select
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No Virtual COM ports found!")
        return None
    print("Available COM Ports:")
    for index, (port, desc, hwid) in enumerate(sorted(ports), start=1):
        print("{}: {} - {}".format(index, desc, port))
    com_port_index = int(input("Select a COM port (1, 2, 3...) ")) - 1

    # Print the selected COM port and return a `serial.Serial` object with a baud rate of 1000000 and timeout of 1s
    selected_port_desc = sorted(ports)[com_port_index][1]
    print(f"You have selected: {selected_port_desc}")
    selected_port_name = sorted(ports)[com_port_index][0]
    return serial.Serial(selected_port_name, baudrate=1000000, timeout=1)

def escape_data(bytestring):
    # Define constants
    FRAME_DELIMITER = 0x7E
    ESCAPE = 0x7D
    XOR_VALUE = 0x20
    ESCAPE_BYTES = {FRAME_DELIMITER, ESCAPE}

    # Escape data bytes
    escaped_data = bytearray()
    for byte in bytestring:
        if byte in ESCAPE_BYTES:
            escaped_data.append(ESCAPE)
            escaped_data.append(byte ^ XOR_VALUE)
        else:
            escaped_data.append(byte)

    # Assemble the final escaped frame
    escaped_frame = bytearray([FRAME_DELIMITER])
    escaped_frame.extend(escaped_data)
    escaped_frame.extend(bytearray([FRAME_DELIMITER]))

    return escaped_frame

def unescape_data(escaped_frame):
    # Define constants
    FRAME_DELIMITER = 0x7E
    ESCAPE = 0x7D
    XOR_VALUE = 0x20

    # Check and remove frame delimiters
    if escaped_frame[0] != FRAME_DELIMITER or escaped_frame[-1] != FRAME_DELIMITER:
        raise ValueError("Invalid frame delimiters")

    escaped_data = escaped_frame[1:-1]

    # Unescape data bytes
    unescaped_data = bytearray()
    i = 0
    while i < len(escaped_data):
        byte = escaped_data[i]

        if byte == ESCAPE:
            i += 1
            unescaped_data.append(escaped_data[i] ^ XOR_VALUE)
        else:
            unescaped_data.append(byte)
        i += 1

    return unescaped_data


def unpack_bytestream(bytestream):
    # Unpack the frequency bytes (4 bytes)
    freq, = struct.unpack("<I", bytestream[:4])

    # Unpack the on_time_byte (1 byte)
    on_time_raw, = struct.unpack("<B", bytestream[4:5])
    on_time = "Long" if on_time_raw == 1 else "Short"

    # Unpack the out_voltage_byte (1 byte)
    out_voltage_raw, = struct.unpack("<B", bytestream[5:6])
    out_voltage = "5V" if out_voltage_raw == 0 else "0.45V"

    # Unpack the bias_v_bytes (2 bytes)
    bias_v_fixed, = struct.unpack("<H", bytestream[6:8])
    bias_v = (bias_v_fixed - 5000) / 1000  # Convert back to floating-point

    # Unpack the running_byte (1 byte)
    running_raw, = struct.unpack("<B", bytestream[8:9])
    running = "ON" if running_raw == 0 else "OFF"

    return freq, on_time, out_voltage, bias_v, running

# Initialize the serial 
ser = config_serial()
# Initial handshake to get current settings from function generator
start = b'\xAA'
ser.write(escape_data(start))
time.sleep(0.5)

# Read in return valued from function generator
data_str = ser.read(ser.inWaiting())
print(data_str)
decodedBytestream = unescape_data(data_str)
print(decodedBytestream)
initFreq, initOnTime, initOutVoltage, initBiasV, initRunning = unpack_bytestream(decodedBytestream)
print("Freq:", initFreq, "On Time:", initOnTime, "Out Voltage:", initOutVoltage, "Bias V:", initBiasV, "Running:", initRunning)

def on_value_change(*args):
    global time_last_updated
    elapsed_time = time.time() - time_last_updated

    if elapsed_time > update_threshold:
        freq = freq_var.get()
        on_time = on_time_var.get()
        out_voltage = out_voltage_var.get()
        bias_v = float(bias_v_entry.get())
        running = running_var.get()

        # Pack values into a bytestream
        freq_bytes = struct.pack("<I", freq)  # 4 bytes for unsigned 32-bit frequency
        on_time_byte = b'\x01' if on_time == "Long" else b'\x00'  # 1 byte for on_time
        out_voltage_byte = b'\x01' if out_voltage == "5V" else b'\x00'  # 1 byte for out_voltage
        bias_v_fixed = int((bias_v * 1000) + 5000)  # Convert to fixed-point with 1mV steps and 5000mV offset
        bias_v_bytes = struct.pack("<H", bias_v_fixed)  # 2 bytes for BiasV
        running_byte = b'\x01' if running == "ON" else b'\x00'  # 1 byte for running

        bytestream = freq_bytes + on_time_byte + out_voltage_byte + bias_v_bytes + running_byte

        print("Freq:", freq, "On Time:", on_time, "Out Voltage:", out_voltage, "Bias V:", bias_v, "Running:", running)
        print("Bytestream:", bytestream.hex())
        msg = escape_data(bytestream)
        ser.write(msg)

app = tk.Tk()
app.title("MTJ Function Generator Control")

def update_freq_spinbox(event):
    try:
        new_value = int(freq_spinbox.get())
        clamped_value = min(max(new_value, 0), 1000000)
        if new_value != clamped_value:
            freq_spinbox.delete(0, tk.END)
            freq_spinbox.insert(0, clamped_value)
        on_value_change()
    except ValueError:
        pass

# Freq
freq_label = ttk.Label(app, text="Freq (0-125000):")
freq_label.grid(column=0, row=0)
freq_var = tk.IntVar(value=initFreq)
# Uncomment this line if you want it to update as soon as you type stuff in
# freq_var.trace_add('write', on_value_change)
freq_spinbox = ttk.Spinbox(app, from_=0, to=125000, textvariable=freq_var)
freq_spinbox.grid(column=1, row=0)
freq_spinbox.bind('<Return>', update_freq_spinbox)


# On Time
on_time_label = ttk.Label(app, text="On Time:")
on_time_label.grid(column=0, row=1)
on_time_var = tk.StringVar(value=initOnTime)
on_time_var.trace_add('write', on_value_change)
on_time_long = ttk.Radiobutton(app, text="Long", variable=on_time_var, value="Long", command=on_value_change)
on_time_short = ttk.Radiobutton(app, text="Short", variable=on_time_var, value="Short", command=on_value_change)
on_time_long.grid(column=1, row=1)
on_time_short.grid(column=2, row=1)

# Out Voltage
out_voltage_label = ttk.Label(app, text="Out Voltage:")
out_voltage_label.grid(column=0, row=2)
out_voltage_var = tk.StringVar(value=initOutVoltage)
out_voltage_var.trace_add('write', on_value_change)
out_voltage_045 = ttk.Radiobutton(app, text="0.45V", variable=out_voltage_var, value="0.45V", command=on_value_change)
out_voltage_5 = ttk.Radiobutton(app, text="5V", variable=out_voltage_var, value="5V", command=on_value_change)
out_voltage_045.grid(column=1, row=2)
out_voltage_5.grid(column=2, row=2)


def update_bias_v_entry(event):
    try:
        new_value = float(bias_v_entry.get())
        clamped_value = min(max(new_value, -5), 5)
        if new_value != clamped_value:
            bias_v_entry.delete(0, tk.END)
            bias_v_entry.insert(0, clamped_value)
        on_value_change()
    except ValueError:
        pass

# Bias V
bias_v_label = ttk.Label(app, text="Bias V (-5V to 5V):")
bias_v_label.grid(column=0, row=3)
bias_v_entry = ttk.Entry(app, width=8)
bias_v_entry.grid(column=2, row=3)
bias_v_entry.insert(0, initBiasV)
bias_v_entry.bind('<Return>', update_bias_v_entry)


# Running
running_label = ttk.Label(app, text="Running:")
running_label.grid(column=0, row=4)
running_var = tk.StringVar(value=initRunning)
running_var.trace_add('write', on_value_change)
running_off = ttk.Radiobutton(app, text="OFF", variable=running_var, value="OFF", command=on_value_change)
running_on = ttk.Radiobutton(app, text="ON", variable=running_var, value="ON", command=on_value_change)
running_off.grid(column=1, row=4)
running_on.grid(column=2, row=4)




# app.mainloop()


# Function to run in a separate thread
def read_serial_data():
    buffer = bytearray()
    frame_delimiter = 0x7E

    while True:
        if ser.inWaiting() > 0:
            # Read one byte
            byte = ser.read(1)

            # Check for frame delimiter
            if byte[0] == frame_delimiter:
                if len(buffer) > 0:
                    # Unpack the bytestream
                    freq, on_time, out_voltage, bias_v, running = unpack_bytestream(buffer)

                    # Update the GUI
                    app.after(0, update_gui, freq, on_time, out_voltage, bias_v, running)

                    # Clear the buffer
                    buffer.clear()
            else:
                # Add the byte to the buffer
                buffer.extend(byte)


# Function to update the GUI
def update_gui(freq, on_time, out_voltage, bias_v, running):
    global time_last_updated
    freq_var.set(freq)
    on_time_var.set(on_time)
    out_voltage_var.set(out_voltage)
    bias_v_entry.delete(0, tk.END)
    bias_v_entry.insert(0, round(bias_v, 3))
    running_var.set(running)

    # Set the time when the GUI was last updated
    time_last_updated = time.time()

# Start the thread
thread = threading.Thread(target=read_serial_data, daemon=True)
thread.start()

# Start the main loop
app.mainloop()
