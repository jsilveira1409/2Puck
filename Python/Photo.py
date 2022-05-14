from threading import Thread
import sys			# System library
import time			# Used for image capture process
import struct
from tkinter import N 		# Used for Big-Endian messages
from PIL import Image  		# Used for the pictures of the camera
import serial
from Python.Function import*
#import yagmail

width = 250
height = 350

size = width * height

score1 = 0
score2 = 0


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
            #self.port = serial.Serial(port, timeout=None, baudrate=115200)
            self.port = serial.Serial(port, timeout=None)
        except:
            print('Cannot connect to the e-puck2')
            sys.exit(0)
        
    #function called after the init
    def run(self):      
        chosen_song = 0
        
        winner = 0
        option = input("Enter option: \n")   
        
        if(option == 'w'):
            self.port.write(b'w')         
            self.send_start_game()
            chosen_song = self.rec_chose_song()
            time.sleep(1)
            print(chosen_song, songs[chosen_song])
            self.send_player1_play() 
            score1 = self.rec_score1()
            print("score 1",score1)
            self.send_player2_play()              
            score2 = self.rec_score2()
            print("score 2",score2)    
            self.rec_picture()
            self.stop()
                               
    def send_start_game(self):
        print("Start Game")
        self.port.write(b'w')

    def send_player1_play(self):
        print("Player 1 play")
        self.port.write(b'START')
        self.port.write(struct.pack('B',2))
        self.port.flush()
    
    def send_player2_play(self):
        print("Player 2 play")
        self.port.write(b'START')
        self.port.write(struct.pack('B',3))
        self.port.flush()

    def rec_score1(self):
        data = []
        s1 = 0
        readUint8Serial(self.port, data)
        s1 = data [0][0]
        return s1

    def rec_score2(self):
        data = []
        s2 = 0
        readUint8Serial(self.port, data)
        s2 = data [0][0]
        return s2

    def rec_chose_song(self):
        data = []
        readUint8Serial(self.port, data)
        song = data [0][0]
        return song
    
    def rec_winner(self):
        data = []
        print("Winner")
        readUint8Serial(self.port, data)
        winner = data[0][0]
        return winner

    def rec_picture(self):
        print('Taking Photo...')
        data = []
        line = 0
        while (line < height):
            if(readUint8Serial(self.port, data)):
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
        #self.send_mail()

    def stop(self):
        if(self.port.isOpen()):
            self.port.close()
        print('Goodbye')

    def send_mail():
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
            attachments=filename,
        )

if __name__ == '__main__':
    reader_thd = serial_thread('/dev/cu.usbmodemEPUCK3')
    # reader_thd = serial_thread('com3')
    reader_thd.start()
