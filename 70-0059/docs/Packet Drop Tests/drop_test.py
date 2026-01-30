import serial
import sys
import getopt

error = False

def print_err(out : str):
    global error
    error = True
    print(out)

def print_usage():
    print_err(
        """Usage:\tpython drop_test.py [-b Baud Rate] [-t Timeout In MS] [-n Communication Attempts] [-m Hex Message] [-r Response Command] [-q] [-d] [--data-only] <Serial Port>
        -b\t\tSets the baudrate for the serial connection.  May cause an error if it is not a standard value.
        -t\t\tSets the response timeout in milliseconds.
        -n\t\tSets the number of response attempts tired.
        -m\t\tHex message to send.  Example:  -m 1140001101
        -r\t\tHex command to receive.
        -q\t\tResults of each test are suppressed.
        -d\t\tOutputs timing data for responses.
        --data-only\tConclusive result is suppressed."""
    )

opts = None
args = []

try:
    opts, args = getopt.getopt(sys.argv[1:], "b:t:n:m:r:qd", ["data-only"])
except:
    print_usage()

baudrate = 9600
timeout = 15
attempts = 10
serial_path = ""
message = bytes([0x10 , 0x40, 0x00, 0x00, 0x11, 0x01])
message_response = None
quiet = False
output_time_data = False
data_only = False

if opts is not None:
    for opt, arg in opts:
        if opt in ['-b']:
            val = -1
            try:
                val = int(arg)
                if val > 0:
                    baudrate = val
                else:
                    print_err("Baudrate must be positive.")
            except:
                print_err("Could not parse baudrate to an integer.")
        elif opt in ['-t']:
            val = -1
            try:
                val = int(arg)
                if val > 0:
                    timeout = val
                else:
                    print_err("Timeout must be positive.")
            except:
                print_err("Could not parse timeout to an integer.")
        elif opt in ['-n']:
            val = -1
            try:
                val = int(arg)
                if val > 0:
                    attempts = val
                else:
                    print_err("Communication attempts must be positive.")
            except:
                print_err("Could not parse communication attempts to an integer.")
        elif opt in ['-m']:
            if (len(arg) % 2 == 0):
                parts : list[bytes] = []
                for i in range(len(arg) // 2):
                    hex_str = "0x" + arg[2*i : 2*i + 2]
                    try:
                        val = int(hex_str, 0)
                        parts.append(val)
                    except:
                        print_err("Could not parse %s as a hexadecimal." % hex_str)
                message = bytes(parts)
            else:
                print_err("Message string must use two characters per byte.")

        elif opt in ['-r']:
            val = -1
            try:
                val = int(arg)
                if val > 0:
                    try:
                        val = int(hex_str, 0)
                        message_response = val
                    except:
                        print_err("Could not parse %s as a hexadecimal." % hex_str)
                else:
                    print_err("Response size attempts must be positive.")
            except:
                print_err("Could not parse response size to an integer.")
        elif opt in ['-q']:
            quiet = True
        elif opt in ['-d']:
            output_time_data = True
        elif opt in ['--data-only']:
            output_time_data = True
            data_only = True

if message_response is None:
    message_response = int.from_bytes(message[0:2], 'little') + 1

if not error:
    time_data = []

    if len(args) > 0:
        serial_path = args[0]

    if serial_path != "":
        ser = None
        try:
            ser = serial.Serial(serial_path, baudrate)
        except:
            print_err("Could not open serial device at path %s at baudrate %d." % (serial_path, baudrate))

        if ser != None:
            success = 0

            for _ in range(attempts):
                # Spin out any existing data
                data = [1]
                ser.timeout = 0
                while len(data) > 0:
                    data = ser.read()
                ser.timeout = timeout / 1000.0
                import time
                ser.write(message)
                start_time = time.perf_counter_ns()
                data = ser.read(6)
                end_time = time.perf_counter_ns()

                time_delta = end_time - start_time
                correct_packet = int.from_bytes(data[0:2], 'little') == message_response

                good_data = False

                if (correct_packet):
                    bytes_to_read = int.from_bytes(data[2:4], 'little')
                    extended_data = (data[4] & 0x80) > 0
                    if extended_data:
                        start_time = time.perf_counter_ns()
                        data = ser.read(bytes_to_read)
                        end_time = time.perf_counter_ns()

                        time_delta += end_time - start_time

                        good_data = True
                    else:
                        good_data = True
                        


                if (good_data):
                    success += 1
                    time_data.append(time_delta)
                if not quiet:
                    time_adjust = time_delta / 1E6
                    if (correct_packet):
                        print("%3.00dms\tResponse from controller." % time_adjust)
                    elif (len(data) != 0):
                        print("%3.00dms\tUnexpected response from controller." % time_adjust)
                    else:
                        print("%3.00dms\tNo response from the controller." % time_adjust)

            ser.close()
            if not data_only:
                print("%d Responses from %d messages with %dms timeouts (%3.2f)%%." % (success, attempts, timeout, (float)(success) / attempts * 100) )
            if output_time_data:
                print(','.join([str(a) for a in time_data]))
                print("ns,%d,%d" % (success, attempts))
    else:
        print_err("No serial path was provided.")