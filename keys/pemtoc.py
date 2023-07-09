from cryptography.hazmat.primitives import serialization

def convert_private_key_to_c_header(key_path, header_path, variable_name):
    with open(key_path, "rb") as key_file:
        private_key = serialization.load_pem_private_key(
            key_file.read(),
            password=None
        )

    pem_data = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )

    header_content = f"const unsigned char {variable_name}[] = {{\n"
    for byte in pem_data:
        header_content += f"0x{byte:02x}, "
    header_content += "0x00};\n"

    with open(header_path, "w") as header_file:
        header_file.write(header_content)

if __name__ == "__main__":
    import sys

    if len(sys.argv) != 4:
        print("Usage: python script.py <key_path> <header_path> <variable_name>")
        sys.exit(1)

    key_path = sys.argv[1]
    header_path = sys.argv[2]
    variable_name = sys.argv[3]

    convert_private_key_to_c_header(key_path, header_path, variable_name)
