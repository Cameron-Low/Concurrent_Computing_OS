disk = open("disk.bin", 'r+b')

# Set the bitmap
disk.seek(0)
disk.write(bytearray([1]))
disk.seek(16)
disk.write(bytearray([1]))

# Initialise the root directory inode
for i in range(3,13):
    disk.seek(32*64 + i*4)
    disk.write(bytearray([255, 255, 255, 255]))

# Initialise the root directory entry
disk.seek(160*64);
disk.write(bytearray([0, 0, 0, 0, 0, 47]))
