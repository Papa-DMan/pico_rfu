import sys
import re
import struct

def hex_to_bin(hex_values, bin_file_path):
    bin_data = b''
    with open(bin_file_path, 'wb') as bin_file:
        #for every 4 bytes in the hex string, decode the bytes and write them to the binary file
        for i in range(0, len(hex_values), 4):
            #convert from string to int, then to bytes
            bytes = struct.pack('4B', *[int(hex_values[i+j], 16) for j in range(4)])
            bin_data += bytes
        bin_file.write(bin_data)
            

           
            


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
    firmware_data = match.group(1).replace('\n', '').replace(' ', '').replace('\\', '').split(',')
    hex_to_bin(firmware_data, output_file)
else:
    print("Error: Could not find firmware data in header file")