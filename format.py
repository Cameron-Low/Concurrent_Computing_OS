disk = open("disk.bin", 'r+b')
# Set the bitmap
disk.seek(0)
disk.write(bytearray([1]))
disk.seek(16)
disk.write(bytearray([1]))

# Initialise the root directory inode
disk.seek(32*64 + 4)
disk.write(bytearray([0, 0, 0, 0]))
disk.seek(32*64 + 4 + 4)
disk.write(bytearray([0, 0, 0, 0]))
for i in range(2,12):
    disk.seek(32*64 + 4 + i*4)
    disk.write(bytearray([255, 255, 255, 255]))

# Initialise the root directory entry
disk.seek(10240);
disk.write(bytearray([0, 0, 0, 0, 0, 47]))
disk.close()
