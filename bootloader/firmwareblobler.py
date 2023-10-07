import sys
import re
import struct

if len(sys.argv) < 3:
    print('Usage: python firmwareblober.py <input_file> <output_file>')
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

# Open the input file
with open(input_file, 'r') as f:
    header = f.read()

# Find the firmware data in the header file
match = re.search(r'{([^}]+)}', header, re.DOTALL)
if match:
    firmware_data = match.group(1).replace('\n', '').replace(' ', '').split(',')
    #print("Found firmware data in header file: ")
    for byte in firmware_data:
        #print(byte)

    # Convert the firmware data array to binary
    firmware_binary = b''
    for byte in firmware_data:
        firmware_binary += struct.pack('B', int(byte, 16))


    # Write the binary data to a file
    with open(output_file, 'wb') as f:
        f.write(firmware_binary)
else:
    print("Error: Could not find firmware data in header file")