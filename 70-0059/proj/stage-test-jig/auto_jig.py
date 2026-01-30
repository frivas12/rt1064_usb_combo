import comms
import pls_jig
import sys
import serial
import serial.tools.list_ports
import time
import datetime
import io
import csv
import os.path
from collections.abc import Callable

class output_mirror_stream:
    _base_stream : io.TextIOBase
    _other_streams : list[io.TextIOBase]

    def __init__(self, base_stream, mirrored_streams):
        self._base_stream = base_stream
        self._other_streams = mirrored_streams

    def read(self, size=-1):
        return self._base_stream.read(size)

    def readline(self, size=-1):
        return self._base_stream.readline(size)
    
    def write(self, str):
        self._base_stream.write(str)

        for other in self._other_streams:
            other.write(str)

    def flush(self):
        self._base_stream.flush()

        for other in self._other_streams:
            other.flush()
    


class auto_options:
    port_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    slot_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    output_file_path : str = "out.csv"
    mirror_output : bool = True
    use_name_checking : bool = True
    # If defined, only that string name will be used.
    permitted_name : str | None = None
    scanned_slot : int = 6
    post_mortem_top_level_exceptions : bool = True

    def apply_jig_options(self, options : pls_jig.jig_options):
        for transform in self._transforms:
            options = transform(options)
        
        return options

    def set_jig_transforms(self, tfs : list[Callable[[pls_jig.jig_options], pls_jig.jig_options]]):
        self._transforms = tfs

    def get_jig_transforms(self):
        return self._transforms

    def set_jig_transform(self, transform : Callable[[pls_jig.jig_options], pls_jig.jig_options]):
        self._transforms = [ transform ]

    def prepend_jig_transform(self, transform : Callable[[pls_jig.jig_options], pls_jig.jig_options]):
        self._transforms.insert(0, transform)

    def append_jig_transform(self, transform : Callable[[pls_jig.jig_options], pls_jig.jig_options]):
        self._transforms.append(transform)
    

    _transforms : list[Callable[[pls_jig.jig_options], pls_jig.jig_options]] = []
    

class auto_state:
    controller : comms.pnp_mcm_controller

    captured_device_names : set[str] = set()

    def get_output_stream(self) -> io.TextIOBase:
        return self._redirected_sysout

    def get_std_out(self) -> io.TextIOBase:
        return self._startup_sysout

    def reallocate_std_out(self, mirror_to_stdout : bool) -> str | None:
        """Reallocates the std out redirection.

        If there was a previous buffer, everything in the buffer will be returned.
        """
        rt = None
        if self._output_buffer is not None:
            rt = self._output_buffer.getvalue()
        self._output_buffer = io.StringIO()
        self._redirected_sysout = mirror_to_stdout \
                              and output_mirror_stream(self._output_buffer, [self._startup_sysout]) \
                              or  mirror_to_stdout
        return rt

    _output_buffer : io.StringIO | None = None
    _redirected_sysout : io.TextIOBase
    _startup_sysout : io.TextIOBase

    def __init__(self):
        self._startup_sysout = sys.stdout
        self._redirected_sysout = self._startup_sysout



def step(state : auto_state, options : auto_options):
    print("connect a stage to slot %d to begin test\nturn off the controller to end testing" % options.scanned_slot)
    while (not state.controller.is_device_connected(options.scanned_slot)):
        time.sleep(options.slot_scan_period.total_seconds())

    name = state.controller.get_device_name(options.scanned_slot)
    split_name = name[0:name.find('-')]
    print('stage "%s" detected...' % name)

    name_used = options.use_name_checking and split_name or options.permitted_name
    jig_options = create_options_from_name(name_used)
    if jig_options is None:
        print("could not find jig options for device type \"%s\"" % name_used)
        print("please, disconnect the stage")
        while (state.controller.is_device_connected(options.scanned_slot)):
            time.sleep(options.slot_scan_period.total_seconds())
        return
    
    jig_options = options.apply_jig_options(jig_options)
    jig_options.stage_slot = options.scanned_slot

    if name in state.captured_device_names:
        print("already captured this stage.  please, disconnect the stage")
        while (state.controller.is_device_connected(options.scanned_slot)):
            time.sleep(options.slot_scan_period.total_seconds())
        return

    def post_test_action():
        # Flushes the test result
        # Pre:  "state" must have had an existing allocation
        sys.stdout = state.get_std_out()

        value : str | None = state.reallocate_std_out(options.mirror_output)
        if (value is None):
            raise Exception("precondition failed")
        with open(options.output_file_path, 'a') as file:
            file.writelines([ "%s,%s\n" % (name,line) for line in value.splitlines()])

        sys.stdout = state.get_output_stream()
    jig_options.post_test_action = post_test_action

        
    state.reallocate_std_out(options.mirror_output)

    print('beginning test...')
    sys.stdout = state.get_output_stream()
    try:
        pls_jig.run_jig(state.controller, jig_options)
    finally:
        sys.stdout = state.get_std_out()

    # Flush output into the file
    post_test_action()

    state.captured_device_names.add(name)
    print("test complete. ", end = '')

    print("please, disconnect the stage")
    while (state.controller.is_device_connected(options.scanned_slot)):
        time.sleep(options.slot_scan_period.total_seconds())

def import_captures(state : auto_state, file):
    reader = csv.reader(file)
    for row in reader:
        state.captured_device_names.add(row[0])
    
def create_pls_options() -> pls_jig.jig_options:
    rt = pls_jig.jig_options()

    rt.tests = 3
    rt.zero_offset_from_home = 15000000
    rt.bidirectional_movement = 11500000
    rt.warmup_movement = 9000000

    return rt

def create_mpm_options() -> pls_jig.jig_options:
    rt = pls_jig.jig_options()

    rt.tests = 3
    rt.zero_offset_from_home = 12500000
    rt.bidirectional_movement = 11000000
    rt.warmup_movement = 9000000

    return rt

def create_options_from_name(name : str) -> pls_jig.jig_options | None:
    name = name.lower()
    if name == "pls":
        return create_pls_options()
    elif name == "mpm":
        return create_mpm_options()
    else:
        return None


def interactive_jig_options() -> list[Callable[[pls_jig.jig_options], pls_jig.jig_options]]:
    line : str = ""
    rt = []
    
    print("Tests:")
    line = sys.stdin.readline().replace('\n', '')
    while line == "":
        line = sys.stdin.readline().replace('\n', '')
    test = int(line)
    def set_test_count(option : pls_jig.jig_options):
        option.tests = test
        return option
    rt.append(lambda option: set_test_count(option))

    return rt



# print("Enter text below to change the value.  Leave blank to use default value\n")
def interactive_auto_options(opt : auto_options) -> auto_options:
    line : str
    
    
    print("CSV Output File (default: %s)" % opt.output_file_path)
    line = sys.stdin.readline().replace('\n', '')
    if line != "":
        opt.output_file_path = line
    
    print("Use Name Checking (yes/no, default: %s)" % (opt.use_name_checking and 'yes' or 'no'))
    line = sys.stdin.readline().replace('\n', '')
    line = line.lower()
    if line == 'yes':
        opt.use_name_checking = True
    elif line == 'no':
        opt.use_name_checking = False

    print("Force Name Checking (yes/no, default: %s)" % (opt.permitted_name is None and 'no' or 'yes'))
    line = sys.stdin.readline().replace('\n', '')
    line = line.lower()
    if line == 'yes':
        opt.use_name_checking = True
        print ("Forced Name:")
        line = sys.stdin.readline().replace('\n', '')
        opt.permitted_name = line
    elif line == 'no':
        opt.use_name_checking = False

    print("Break on exception (yes/no, default:  %s)" % (opt.post_mortem_top_level_exceptions and 'yes' or 'no'))
    line = sys.stdin.readline().replace('\n','').lower()
    if line == 'yes':
        opt.post_mortem_top_level_exceptions = True
    elif line == 'no':
        opt.post_mortem_top_level_exceptions = False


    print("Configure Jig Options, (yes/no, default: no)")
    line = sys.stdin.readline().replace('\n', '')
    if line == "yes":
        transforms = interactive_jig_options()
        transforms.reverse()
        for tf in transforms:
            opt.prepend_jig_transform(tf)

    return opt

if __name__ == "__main__":
    opt = auto_options()
    state = auto_state()
    def remove_header(option):
        option.include_header = False
        return option
    def set_encoder_slot(option : pls_jig.jig_options):
        option.encoder_slot = 7
        return option
    opt.append_jig_transform(lambda option: remove_header(option))
    opt.append_jig_transform(lambda option: set_encoder_slot(option))

    print("Enter interactive configuration (yes/no, default: no)")
    line = sys.stdin.readline().replace('\n', '')
    if line != "":
        opt = interactive_auto_options(opt)
    if os.path.isfile(opt.output_file_path):
        print("A file already exists in that location.  Importing previous tests...")
        with open(opt.output_file_path, 'r') as file:
            import_captures(state, file)
    else:
        with open(opt.output_file_path, 'w') as file:
            old_sysout = sys.stdout
            sys.stdout = file
            try:
                print("Stage Name,", end='')
                pls_jig.jig_state.output_header()
            finally:
                sys.stdout = old_sysout
    
    if not opt.use_name_checking:
        base = create_options_from_name(opt.permitted_name)
        if base is None:
            print("could not find options for name \"%s\"" % opt.permitted_name)
            exit(-1)

            
    print("Waiting for a COM port connection...")
    port = None
    ports = serial.tools.list_ports.comports()
    while len(ports) == 0:
        time.sleep(opt.port_scan_period.total_seconds())
        ports = serial.tools.list_ports.comports()
    
    if len(ports) != 1:
        print("error:  found more than one port")
        exit(-1)
    
    port = serial.Serial(ports[0].name)

    state.controller = comms.pnp_mcm_controller(port)

    print("connected. turn off the controller between tests to exit")
    base_transforms = opt.get_jig_transforms()
    while state.controller.is_open():
        opt.set_jig_transforms(base_transforms)
        try:
            step(state, opt)
        except KeyboardInterrupt:
            print("interrupted - exiting program...")
            state.controller.close()
        except Exception as e:
            if opt.post_mortem_top_level_exceptions:
                import pdb; pdb.post_mortem()

    print("finished")
