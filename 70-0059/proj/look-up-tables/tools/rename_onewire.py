# usage: python3 rename_onewire.py <source-bin> <dest-bin> <name>
# Allows the name string in a compiled one-wire object to be changed.
import sys

if len(sys.argv) != 4:
    print("usage: python3 rename_onewire <source-bin> <dest-bin> <name>")
    exit(1)

src_bin = sys.argv[1]
dest_bin = sys.argv[2]
name = sys.argv[3]

enc_name = bytes(name, 'utf8')
padding_bytes = 15 - len(enc_name)
if padding_bytes <= 0:
    print(f'error (2): name "{name}" exceeds 15-byte buffer')
    exit(2)
enc_name += bytes([0 for x in range(padding_bytes)])
enc_name_checksum = sum(map(lambda x: int(x), enc_name))

with open(dest_bin, 'wb') as dest:
    with open(src_bin, 'rb') as src:
        dest.write(src.read(1))
        src_checksum = int.from_bytes(src.read(1))
        signature = src.read(4)
        src_part_number = src.read(15)
        src_pn_checksum = sum(map(lambda x: int(x), src_part_number))

        dest_checksum = (src_checksum + enc_name_checksum -
                         src_pn_checksum) % 256

        dest.write(bytes([dest_checksum]))
        dest.write(signature)
        dest.write(enc_name)
        dest.write(src.read())
