import time

timer1 = 0
timer2 = 0

timer1ready = False
timer2ready = False

buffer_not_full = True
buffer_full = False  # (unused here; consider deriving as: buffer_full = not buffer_not_full)
idle = True

def polling():
    print("Reading")

def buffering():
    print("buffering")

def uploading():
    print("uploading")

while True:
    timer1 += 1
    timer2 += 1

    time.sleep(1)  # wait for 2 seconds
    if timer2%5==0:
        timer2ready = True

    elif timer1%2==0:
        timer1ready = True
    

    if timer2ready:
        uploading()
        timer2ready = False
        timer2 = 0

    elif timer1ready:
        if buffer_not_full and idle:
            polling()
            buffering()
            timer1ready = False
            timer1 = 0
