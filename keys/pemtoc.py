from cryptography.hazmat.primitives import serialization

def convert_private_key_to_c_header(key_path : str, header_path : str):
    with open(key_path, "rb") as key_file:
        private_key = serialization.load_pem_private_key(
            key_file.read(),
            password=None
        )

    private_pem_data = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )

    public_pem_data = private_key.public_key().public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )

    header_content = f"static const unsigned char private_key[] = {{\n"
    for byte in private_pem_data:
        header_content += f"0x{byte:02x}, "
    header_content += "0x00};\n"

    header_content += f"\n\nstatic const unsigned char public_key[] = {{\n"
    for byte in public_pem_data:
        header_content += f"0x{byte:02x}, "
    header_content += "0x00};\n"


    with open(header_path, "w") as header_file:
        header_file.write(header_content)

if __name__ == "__main__":
    import sys

    if len(sys.argv) != 3:
        print("Usage: python script.py <key_path> <header_path>")
        sys.exit(1)

    key_path = sys.argv[1]
    header_path = sys.argv[2]
    print(f"Converting {key_path} to {header_path}")
    convert_private_key_to_c_header(key_path, header_path)
