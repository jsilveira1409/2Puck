from threading import Thread
import sys			# System library
import time			# Used for image capture process
import struct 		# Used for Big-Endian messages
from PIL import Image  		# Used for the pictures of the camera
import serial
from Function import*

width = 500
height = 500
size = width * height

#thread used to control the communication part
class serial_thread(Thread):

    #init function called when the thread begins
    def __init__(self, port):
        Thread.__init__(self)
        self.contReceive = False
        self.alive = True
        self.need_to_update = False

        print('Connecting to port {}'.format(port))

        try:
            self.port = serial.Serial(port, timeout=0.5)
        except:
            print('Cannot connect to the e-puck2')
            sys.exit(0)
    #function called after the init
    def run(self):
        #data = bytes(size)
        option = input("Enter option: \n")
        if(option == 'n')

        
            

    #clean exit of the thread if we need to stop it
    def stop(self):
        if(self.port.isOpen()):
            self.port.close()
        print('Goodbye')
    def take_picture(self):
        data = []
        line = 0
        while (line < height):
            if(readUint8Serial(self.port, data)):
                line = line + 1
                print(line)
        print('done')
        im = []
        for x in data:            
            im.append(x[1])
            im.append(x[0])
        img = bytes(im)
        image = Image.frombuffer("RGB", (width, height), img, "raw",  "BGR;16", 0, 1)
        image.save('capture.png')
        
            
reader_thd = serial_thread('com10')
reader_thd.start()

