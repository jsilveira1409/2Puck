import struct

msg = 'c'
print(struct.pack('!c', msg.encode('ascii')))
print(msg)

