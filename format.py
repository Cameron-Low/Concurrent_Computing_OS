disk = open("disk.bin", 'r+b')
data = []
with open("P1", 'rb') as prog:
    byte = prog.read(1)
    while byte != "":
        data.append(ord(byte))
        byte = prog.read(1)
print(data)

# Set the bitmap
disk.seek(0)
disk.write(bytearray([3]))
disk.seek(16)
disk.write(bytearray([7]))

# Initialise the root directory inode
disk.seek(32*64 + 4)
disk.write(bytearray([0, 0, 0, 0]))
disk.seek(32*64 + 4 + 4)
disk.write(bytearray([0, 0, 0, 0]))
disk.seek(32*64 + 4 + 8)
disk.write(bytearray([1, 0, 0, 0]))
for i in range(3,12):
    disk.seek(32*64 + 4 + i*4)
    disk.write(bytearray([255, 255, 255, 255]))

disk.seek(33*64)
disk.write(bytearray([1]))
disk.seek(33*64 + 4)
disk.write(bytearray([2]))
for i in range(1,12):
    disk.seek(33*64 + 4 + i*4)
    disk.write(bytearray([255, 255, 255, 255]))

# Initialise the root directory entry
disk.seek(10240);
disk.write(bytearray([0, 0, 0, 0, 0, 47]))

# Initialise the P1 directory entry
disk.seek(10304);
disk.write(bytearray([1, 0, 0, 0, 1, 80, 49]))
# Initialise the P1 data
disk.seek(10368);
disk.write(bytearray(data))
disk.close()
