import struct

def readSyncMsg(port):
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
    return

#reads the data in uint8 from the serial
def readUint8Serial(port, data):
    #reads the size
    readSyncMsg(port)
    #converts as short int in little endian the two bytes read
    size = struct.unpack('<h',port.read(2)) 
    #removes the second element which is void
    size = size[0]  

    #reads the data
    
    rcv_buffer = port.read(size)

    #if we receive the good amount of data, we convert them in float32
    if(len(rcv_buffer) == size):
        i = 0
        if(size > 1):
            while(i < size):
                #data.append(struct.pack(">bb", - ord("I"), 0))
                data.append(struct.unpack_from('<BB',rcv_buffer, i))
                i = i+2
            return True
        else:    
            data.append(struct.unpack_from('<B',rcv_buffer))
            return True

    else:
        return False

