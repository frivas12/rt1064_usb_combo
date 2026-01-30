import sys
import apt_header_validator

def convert_cs_line(item : tuple[str, int], indentation : str) -> str:
    return f"{indentation}public const ushort {item[0]} = 0x{item[1]:04X};"

if __name__ == "__main__":
    def print_usage(argv_0 : str):
        print(f"Usage: python {argv_0} <path to \"apt.h\">")

    if len(sys.argv) != 2:
        print_usage()
        exit(-1)

    state = apt_header_validator.parser_state(sys.argv[1])
    indent = "    "


    try:
        commands = state.get_commands()
    except FileNotFoundError:
        print(f"Error: cannot open \"{sys.argv[1]}\"")
        exit(-1)


    print(f"namespace Thorlabs\n{{\n{indent}public class APT\n{indent}{{")

    for command in commands:
        print(convert_cs_line(command, indent + indent))


    print("\n")
    print(f"{indent}{indent}public enum Address\n{indent}{indent}{{")
    print(f"{indent}{indent}{indent}HOST                    = 0x01,")
    print(f"{indent}{indent}{indent}MOTHERBOARD             = 0x11,")
    print(f"{indent}{indent}{indent}MOTHERBOARD_STANDALONE  = 0x50,")
    print(f"{indent}{indent}{indent}SLOT_1                  = 0x21,")
    print(f"{indent}{indent}{indent}SLOT_2                  = 0x22,")
    print(f"{indent}{indent}{indent}SLOT_3                  = 0x23,")
    print(f"{indent}{indent}{indent}SLOT_4                  = 0x24,")
    print(f"{indent}{indent}{indent}SLOT_5                  = 0x25,")
    print(f"{indent}{indent}{indent}SLOT_6                  = 0x26,")
    print(f"{indent}{indent}{indent}SLOT_7                  = 0x27,")
    print(f"{indent}{indent}{indent}SYNC_MOTION             = 0x28,")
    print(f"{indent}{indent}}}")
    # print(f"\n\n{indent}{indent}public implicit operator byte(Address addr)\n{indent}{indent}{{\n")
    # print(f"{indent}{indent}{indent}return (byte)addr;\n{indent}{indent}}}")
    print(f"{indent}}}\n}}")

    if state.error:
        exit(1)
