import serial
import time # Optional (required if using time.sleep() below)
import asyncio
import aiohttp

ser = serial.Serial(port='COM9', baudrate=115200)

while (True):
    # Check if incoming bytes are waiting to be read from the serial input 
    # buffer.
    # NB: for PySerial v3.0 or later, use property `in_waiting` instead of
    # function `inWaiting()` below!
    if (ser.inWaiting() > 0):
        # read the bytes and convert from binary array to ASCII
        data_str = ser.read(ser.inWaiting()).decode('ascii') 
        # print the incoming string without putting a new-line
        # ('\n') automatically after every print()
        print(data_str, end='\n')

        if "Found I2C" in data_str:
            print("it's in there")
        
        # TODO: listen for a specially formatted output

    # Put the rest of your code you want here
    
    # Optional, but recommended: sleep 10 ms (0.01 sec) once per loop to let 
    # other threads on your PC run during this time. 
    time.sleep(0.01)

def acknowledge(request_code):
    # TODO: implement acknowledgement
    return

async def handle_request(request_body):
    # TODO: implement async aiohttp request,
    #       or a 
    return

def return_response(response_body):
    # TODO: serial.write the response back
    return