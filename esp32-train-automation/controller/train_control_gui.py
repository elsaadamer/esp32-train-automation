"""
Train Control Panel — GUI for Brio Train Network
Sends MQTT commands to ESP32 controllers over local WiFi.

How to run:
  1. Make sure Mosquitto broker is running on your laptop
  2. pip install paho-mqtt
  3. python train_control_gui.py
"""

import tkinter as tk
from tkinter import messagebox
import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion

# ========== SETTINGS ==========
BROKER_IP = "YOUR_LAPTOP_IP"
BROKER_PORT = 1883


# ========== MQTT CONNECTION ==========

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        status_label.config(text="Connected to broker", fg="#2ecc71")
    else:
        status_label.config(text=f"Connection failed (code {rc})", fg="#e74c3c")


def on_disconnect(client, userdata, rc):
    status_label.config(text="Disconnected", fg="#e74c3c")


def send(topic, message):
    client.publish(topic, message)
    log_text.insert(tk.END, f"[{topic}] → {message}\n")
    log_text.see(tk.END)


# ========== GUI SETUP ==========

root = tk.Tk()
root.title("Brio Train Control Panel")
root.configure(bg="#1e1e2e")
root.resizable(False, False)

BG_COLOR = "#1e1e2e"
CARD_COLOR = "#2a2a3d"
TEXT_COLOR = "#e0e0e0"
ACCENT_BLUE = "#5b9bd5"
ACCENT_GREEN = "#2ecc71"
ACCENT_RED = "#e74c3c"
ACCENT_ORANGE = "#f39c12"
ACCENT_PURPLE = "#9b59b6"
ACCENT_CYAN = "#1abc9c"
BUTTON_FONT = ("Segoe UI", 11, "bold")
LABEL_FONT = ("Segoe UI", 13, "bold")
SMALL_FONT = ("Segoe UI", 9)


def make_section(parent, title, row, col, color, colspan=1):
    frame = tk.Frame(parent, bg=CARD_COLOR, bd=0, highlightthickness=2,
                     highlightbackground=color, padx=15, pady=10)
    frame.grid(row=row, column=col, columnspan=colspan,
               padx=8, pady=8, sticky="nsew")
    label = tk.Label(frame, text=title, font=LABEL_FONT,
                     fg=color, bg=CARD_COLOR)
    label.pack(pady=(0, 8))
    return frame


def make_button(parent, text, command, color, width=12):
    btn = tk.Button(parent, text=text, command=command,
                    font=BUTTON_FONT, fg="white", bg=color,
                    activebackground=color, activeforeground="white",
                    bd=0, padx=10, pady=6, width=width, cursor="hand2")
    btn.pack(pady=3)
    return btn


def make_button_side(parent, text, command, color, width=10):
    """Same as make_button but packs to the left — used for horizontal rows."""
    btn = tk.Button(parent, text=text, command=command,
                    font=BUTTON_FONT, fg="white", bg=color,
                    activebackground=color, activeforeground="white",
                    bd=0, padx=10, pady=6, width=width, cursor="hand2")
    btn.pack(side=tk.LEFT, padx=4, pady=3)
    return btn


# ========== SPEED CONTROL STATE ==========
# Train speed goes from 0 to 255
# We use steps of 50 so the user can increase/decrease easily
train_speed = tk.IntVar(value=150)
train_direction = tk.StringVar(value="stopped")


def train_forward():
    train_direction.set("forward")
    spd = train_speed.get()
    send("train/1", f"speed:{spd}")
    direction_label.config(text=f"FORWARD  {spd}", fg="#1abc9c")


def train_backward():
    train_direction.set("backward")
    spd = train_speed.get()
    send("train/1", f"speed:-{spd}")
    direction_label.config(text=f"BACKWARD  {spd}", fg="#f39c12")


def train_stop():
    train_direction.set("stopped")
    send("train/1", "stop")
    direction_label.config(text="STOPPED", fg="#e74c3c")


def speed_up():
    current = train_speed.get()
    new_speed = min(255, current + 25)
    train_speed.set(new_speed)
    speed_label.config(text=f"Speed: {new_speed}")
    # If already moving, update immediately
    if train_direction.get() == "forward":
        train_forward()
    elif train_direction.get() == "backward":
        train_backward()


def speed_down():
    current = train_speed.get()
    new_speed = max(50, current - 25)
    train_speed.set(new_speed)
    speed_label.config(text=f"Speed: {new_speed}")
    if train_direction.get() == "forward":
        train_forward()
    elif train_direction.get() == "backward":
        train_backward()


# ========== TITLE ==========
title_label = tk.Label(root, text="BRIO TRAIN CONTROL",
                        font=("Segoe UI", 18, "bold"),
                        fg=ACCENT_BLUE, bg=BG_COLOR)
title_label.grid(row=0, column=0, columnspan=3, pady=(15, 5))

status_label = tk.Label(root, text="Connecting...", font=SMALL_FONT,
                         fg=ACCENT_ORANGE, bg=BG_COLOR)
status_label.grid(row=1, column=0, columnspan=3, pady=(0, 10))

content = tk.Frame(root, bg=BG_COLOR)
content.grid(row=2, column=0, columnspan=3, padx=10)

# ========== ROW 0: BRIDGE + DIVERSION 1 + DIVERSION 2 ==========

bridge_frame = make_section(content, "Bridge", 0, 0, ACCENT_BLUE)
make_button(bridge_frame, "UP",   lambda: send("bridge/1", "up"),   "#2980b9")
make_button(bridge_frame, "DOWN", lambda: send("bridge/1", "down"), "#34495e")

div1_frame = make_section(content, "Diversion 1", 0, 1, ACCENT_GREEN)
make_button(div1_frame, "STRAIGHT", lambda: send("diversion/1", "straight"), "#27ae60")
make_button(div1_frame, "RIGHT",    lambda: send("diversion/1", "right"),    "#16a085")

div2_frame = make_section(content, "Diversion 2", 0, 2, ACCENT_PURPLE)
make_button(div2_frame, "STRAIGHT", lambda: send("diversion/2", "straight"), "#8e44ad")
make_button(div2_frame, "RIGHT",    lambda: send("diversion/2", "right"),    "#6c3483")

# ========== ROW 1: BARRIER 1 + BARRIER 2 + LOG ==========

bar1_frame = make_section(content, "Barrier 1", 1, 0, ACCENT_ORANGE)
make_button(bar1_frame, "OPEN",  lambda: send("barrier/1", "open"),  "#e67e22")
make_button(bar1_frame, "CLOSE", lambda: send("barrier/1", "close"), "#d35400")

bar2_frame = make_section(content, "Barrier 2", 1, 1, ACCENT_RED)
make_button(bar2_frame, "OPEN",  lambda: send("barrier/2", "open"),  "#c0392b")
make_button(bar2_frame, "CLOSE", lambda: send("barrier/2", "close"), "#922b21")

log_frame = tk.Frame(content, bg=CARD_COLOR, bd=0, highlightthickness=2,
                     highlightbackground="#555", padx=10, pady=10)
log_frame.grid(row=1, column=2, padx=8, pady=8, sticky="nsew")
log_label = tk.Label(log_frame, text="Command Log", font=LABEL_FONT,
                      fg=TEXT_COLOR, bg=CARD_COLOR)
log_label.pack(pady=(0, 5))
log_text = tk.Text(log_frame, height=4, width=22, font=("Consolas", 9),
                    bg="#1a1a2e", fg="#aaa", bd=0, insertbackground="#aaa")
log_text.pack()

# ========== ROW 2: TRAIN (spans full width) ==========

train_frame = make_section(content, "Train 1", 2, 0, ACCENT_CYAN, colspan=3)

# Direction status
direction_label = tk.Label(train_frame, text="STOPPED",
                            font=("Segoe UI", 13, "bold"),
                            fg="#e74c3c", bg=CARD_COLOR)
direction_label.pack(pady=(0, 6))

# Direction buttons row
dir_row = tk.Frame(train_frame, bg=CARD_COLOR)
dir_row.pack()
make_button_side(dir_row, "BACKWARD", train_backward, "#e67e22", width=10)
make_button_side(dir_row, "STOP",     train_stop,     "#c0392b", width=8)
make_button_side(dir_row, "FORWARD",  train_forward,  "#27ae60", width=10)

# Speed control row
speed_row = tk.Frame(train_frame, bg=CARD_COLOR)
speed_row.pack(pady=(8, 0))

tk.Button(speed_row, text="  −  ", command=speed_down,
          font=BUTTON_FONT, fg="white", bg="#555",
          activebackground="#444", bd=0, padx=8, pady=4,
          cursor="hand2").pack(side=tk.LEFT, padx=4)

speed_label = tk.Label(speed_row,
                        text=f"Speed: {train_speed.get()}",
                        font=("Segoe UI", 11), fg=TEXT_COLOR, bg=CARD_COLOR)
speed_label.pack(side=tk.LEFT, padx=10)

tk.Button(speed_row, text="  +  ", command=speed_up,
          font=BUTTON_FONT, fg="white", bg="#555",
          activebackground="#444", bd=0, padx=8, pady=4,
          cursor="hand2").pack(side=tk.LEFT, padx=4)


# ========== START MQTT ==========

client = mqtt.Client(callback_api_version=CallbackAPIVersion.VERSION1,
                     client_id="TrainGUI")
client.on_connect = on_connect
client.on_disconnect = on_disconnect

try:
    client.connect(BROKER_IP, BROKER_PORT)
    client.loop_start()
except Exception as e:
    status_label.config(text="Cannot reach broker", fg="#e74c3c")


# ========== RUN ==========

def on_closing():
    train_stop()
    client.loop_stop()
    client.disconnect()
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()

client.loop_stop()
client.disconnect()