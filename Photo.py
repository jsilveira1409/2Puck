from threading import Thread
import sys			# System library
import time			# Used for image capture process
import struct 		# Used for Big-Endian messages
from PIL import Image  		# Used for the pictures of the camera
import serial
# You have to use the keys of this dictionary for indicate the operating 
# mode of the camera


width = 500
height = 500
# Two times the width as each pixel is two bytes

size = width * height

#reads the data in uint8 from the serial
def readUint8Serial(port, data):

    state = 0

    while(state != 5):

        #reads 1 byte
        c1 = port.read(1)
        #timeout condition
        if(c1 == b''):
            return [];

        if(state == 0):
            if(c1 == b'S'):
                state = 1
            else:
                state = 0
        elif(state == 1):
            if(c1 == b'T'):
                state = 2
            elif(c1 == b'S'):
                state = 1
            else:
                state = 0
        elif(state == 2):
            if(c1 == b'A'):
                state = 3
            elif(c1 == b'S'):
                state = 1
            else:
                state = 0
        elif(state == 3):
            if(c1 == b'R'):
                state = 4
            elif (c1 == b'S'):
                state = 1
            else:
                state = 0
        elif(state == 4):
            if(c1 == b'T'):
                state = 5
            elif (c1 == b'S'):
                state = 1
            else:
                state = 0

    #reads the size
    #converts as short int in little endian the two bytes read
    size = struct.unpack('<h',port.read(2)) 
    
    #removes the second element which is void
    size = size[0]  

    #reads the data
    rcv_buffer = port.read(size)

    #if we receive the good amount of data, we convert them in float32
    if(len(rcv_buffer) == size):
        i = 0
        while(i < size):
            #data.append(struct.pack(">bb", - ord("I"), 0))
            data.append(struct.unpack_from('<BB',rcv_buffer, i))
            i = i+2
        return True
    else:
        return False



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
        data = []
        while(self.alive):
            line = 0
            while (line < height):
                if(readUint8Serial(self.port, data)):
                    line = line + 1
                    print(line)
            print('done')
            im = []
            #img = struct.pack('B' * len(data) , *s)
            for x in data:            
                im.append(x[1])
                im.append(x[0])
            img = bytes(im)
            image = Image.frombuffer("RGB", (width, height), img, "raw",  "BGR;16", 0, 1)
            image.save('capture.png')
            

    #clean exit of the thread if we need to stop it
    def stop(self):
        if(self.port.isOpen()):
            self.port.close()
        print('Goodbye')
            
reader_thd = serial_thread('com10')
reader_thd.start()

            


