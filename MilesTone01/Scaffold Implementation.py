import time
import random

timer1 = 0
timer2 = 0

POLLING_INTERVAL = 1
UPLOAD_INTERVAL = 15

timer1ready = True
timer2ready = False
MAX_BUFFER = 8

current_buffer = []
buffer_not_full = True
buffer_full = False 
idle = True

def polling():
    voltage = random.randint(220, 240)
    current = random.randint(2,4)
    power = random.randint(2,10)
    print(f"Reading {voltage}V, {current}A and {power}W")
    return [voltage, current, power]


def buffering(current:list, data: list[int]):
    print("buffering")
    global buffer_full
    global current_buffer
    current.append(data)
    if len(current) >= MAX_BUFFER:
        buffer_full = True
    return current


def uploading(current_buffer):
    print(f"{len(current_buffer)} uploading {current_buffer}")

while True:
    timer1 += 1
    timer2 += 1

    time.sleep(1)  # wait for POLLING_INTERVAL
    if timer2 % UPLOAD_INTERVAL == 0:
        timer2ready = True
    

    if timer2ready or buffer_full:
        uploading(current_buffer)

        timer2ready = False
        timer2 = 0
        current_buffer = []
        buffer_full = False

    else:
        if buffer_not_full and idle:
            new_data = polling()
            buffering(current_buffer, new_data)
            timer1ready = False
            timer1 = 0
