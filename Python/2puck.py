from threading import Thread
import sys			# System library
import time			# Used for image capture process
import struct
from tkinter import N 		# Used for Big-Endian messages
from PIL import Image  		# Used for the pictures of the camera
import serial
import yagmail
from Function import *

width = 250
height = 350

size = width * height

def rec_picture():
    data = []
    line = 0
    while (line < height):
        if(readUint8Serial(port, data)):
            line = line + 1
            if (line%50 == 0):
                print(line)
    print('done')
    im = []
    for x in data:            
        im.append(x[1])
        im.append(x[0])
    img = bytes(im)
    image = Image.frombuffer("RGB", (width, height), img, "raw",  "BGR;16", 0, 1)
    image.save('capture.png')

def stop():
    if(port.isOpen()):
        port.close()
    print('Goodbye')

def send_mail():
    print("Sending Email")
    yagmail.register('2puckshakour@gmail.com', 'HsmTD2k@15Sr')
    receiver = "joaquim.silveira@epfl.ch"
    body = "Dear Prof. Mondada,\n\n \
            Please find attached the Winner of the amazing music competition. \n\n \
            Kind regards,\n \
            2Puck"
    filename = "capture.png"

    yag = yagmail.SMTP("2puckshakour@gmail.com")
    yag.send(
        to=receiver,
        subject="2Puck Competition",
        contents=body, 
        attachments=filename
        )
    print("Email sent")

if __name__ == '__main__':
    # port = serial.Serial('/dev/cu.e-puck2_04076-UART')
    port = serial.Serial('/dev/cu.usbmodemEPUCK3')
    # port.write(b'w')
    while True:
        line = port.readline()
        line = line.rstrip().decode()
        line_test = line.partition(': ')[2]
        if line_test == 'Input':
            user_input = input(port.readline().rstrip().decode()+ ": ")
            port.write(user_input.encode('ascii'))
        elif line_test == 'Taking Photo':
            print(line)
            rec_picture()
            send_mail()
        else:
            print(line)
        port.flush()