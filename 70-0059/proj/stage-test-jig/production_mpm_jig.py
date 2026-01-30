import comms
import pls_jig
import serial
import serial.tools.list_ports
import time
import datetime
import platform
import csv
import os
import os.path
from collections.abc import Callable
import statistics
import sys
import json

class auto_options:
    port_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    slot_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    use_name_checking : bool = True
    # If defined, only that string name will be used.
    permitted_name : str | None = None
    scanned_slots : list[int] = [5, 6]
    post_mortem_top_level_exceptions : bool = False
    tests_to_capture : int = 10
    export_dir : str = "V:\\Operations\\Quality\\Production QC Reports\\PnP Stage Test Jig"

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
    failure_delta : float
    active_slot : int
    output_file = None

def clear_screen():
    if platform.system() == 'Windows':
        os.system('cls')
    else:
        os.system('clear')


class serialized_test:
    stage_name : str
    backlash_units : str 
    backlash_tests : list[float]

    def __init__(self):
        self.backlash_units = "nm"


def step(state : auto_state, options : auto_options):
    clear_screen()
    def slot_test(list : list[int]):
        if len(list) == 1:
            return f"{list[0]:d}"
        elif len(list) == 2:
            return f"{list[0]:d} or {list[1]:d}"
        else:
            return "".join([f"{value:d}, " for value in list[0:-1]]) + f"or {list[-1]:d}"


    print("connect a stage to slot %s to begin test\nturn off the controller to end testing" % slot_test(options.scanned_slots))
    device_connection_status : dict[int, bool] = {}
    def update_connection_status():
        for slot in options.scanned_slots:
            device_connection_status[slot] = state.controller.is_device_connected(slot)
    def active_slot_available():
        set = False
        for slot in device_connection_status:
            connected = device_connection_status[slot]
            if connected:
                if set:
                    return False
                state.active_slot = slot
                set = True
        return set
        
    def wait_for_stage_disconnect():
        while (state.controller.is_device_connected(state.active_slot)):
            time.sleep(options.slot_scan_period.total_seconds())
    def output_file_name(stage_name : str) -> str:
        return f"backlash-record-{stage_name}.json"
    def serialize_test_results(name : str, test_results : list[float]):
        ser = serialized_test()
        ser.stage_name = name
        ser.backlash_tests = test_results

        json.dump(ser, state.output_file, default=lambda x: x.__dict__)

    
    update_connection_status()
    while (not active_slot_available()):
        update_connection_status()
        time.sleep(options.slot_scan_period.total_seconds())
    clear_screen()

    def set_options_slots(jig : pls_jig.jig_options) -> pls_jig.jig_options:
        jig.encoder_slot = 7
        jig.stage_slot = state.active_slot
        return jig
    options.append_jig_transform(set_options_slots)
    
    def print_table_bar():
        print('| ---- | ------------- |')
    def print_table_header():
        print('| Test | Backlash (µm) |')
    def print_table_test(test : int, backlash_nm : float):
        print(f'| {test: >4d} | {(backlash_nm / 1000): >13.3f} |')

    name = state.controller.get_device_name(state.active_slot)
    split_name = name[0:name.find('-')]
    name_used = options.use_name_checking and split_name or options.permitted_name

    path = os.path.join(options.export_dir, output_file_name(name))
    if os.path.exists(path):
        print(f"error: a report file for {name} was already found.")
        print("       if this is a mistake, seek support")
        print("       otherwise, disconnect the stage")
        wait_for_stage_disconnect()
        return
    state.output_file = open(path, 'wt')

    jig_options = create_options_from_name(name_used)
    if jig_options is None:
        print("could not find jig options for device type \"%s\"" % name_used)
        print("please, disconnect the stage")
        while (state.controller.is_device_connected(state.active_slot)):
            time.sleep(options.slot_scan_period.total_seconds())
        return
    
    jig_options = options.apply_jig_options(jig_options)
    state.failure_delta = create_failure_delta_from_name(name_used)



    print_table_bar()
    print_table_header()
    print_table_bar()

    backlash_tests : float = []
    jstate = pls_jig.jig_state()
    while state.controller.is_open() and jstate.step != 'finished':
        pls_jig.jig_step_backlash(state.controller, jstate, jig_options)
        for item in jstate.test_progress_queue:
            running_state, encoder_count, stage_count = item
            if running_state == 'back to zero cooldown':
                stage_position = jstate.to_stage_position(stage_count)
                encoder_position = jstate.to_encoder_position(encoder_count)
                backlash = encoder_position - stage_position
                backlash_tests.append(backlash)

                print_table_test(jstate.test_counter, backlash)
        jstate.test_progress_queue = []
        time.sleep(jig_options.poll_frequency.total_seconds())
    
    state.controller.home(state.active_slot)

    print_table_bar()

    print(f"\nAverage Backlash (µm):  {statistics.mean(backlash_tests)/1000:.3f}")
    tests_failing = sum(map(lambda x: x > state.failure_delta and 1 or 0, backlash_tests))
    if tests_failing > 0:
        for i in range(5):
            print("!!!FAILED!!!")
        print(f"\t{tests_failing:d}/{options.tests_to_capture:d} had a backlash over the limit ({state.failure_delta:.3f})")
    else:
        print("PASSED")

    serialize_test_results(name, backlash_tests)
    state.output_file.close()
    state.output_file = None
    
    print("\nMOVING! Do NOT disconnect the stage.")
    while (state.controller.is_moving(state.active_slot)):
        time.sleep(options.slot_scan_period.total_seconds())

    print("You may now disconnect the stage.")
    wait_for_stage_disconnect()

def import_captures(state : auto_state, file):
    reader = csv.reader(file)
    for row in reader:
        state.captured_device_names.add(row[0])
    
def create_mpm_options() -> pls_jig.jig_options:
    rt = pls_jig.jig_options()

    rt.tests = 1 
    rt.zero_offset_from_home = 12500000
    rt.bidirectional_movement = 5 * 1000000
    rt.warmup_movement = 9000000
    rt.warmup_time = datetime.timedelta(seconds=30)
    rt.movement_cooldown = datetime.timedelta(seconds=5)

    return rt

def create_options_from_name(name : str) -> pls_jig.jig_options | None:
    name = name.lower()
    if name == "mpm":
        return create_mpm_options()
    else:
        return None

def create_failure_delta_from_name(name : str) -> float:
    name = name.lower()
    if name == "mpm":
        return 15000
    else:
        return 0

def query_com_ports(invalid_ports : list[str]) -> list[serial.Serial]:
    untested_ports = []

    current_ports = serial.tools.list_ports.comports()
    for port in current_ports:
        if port.name not in invalid_ports:
            untested_ports.append(port)

    rt : list[serial.Serial] = []
    for port_info in untested_ports:
        port : serial.Serial | None = None
        try:
            port = serial.Serial(port_info.name)
            if comms.pnp_mcm_controller.test_port_is_valid(port):
                rt.append(port)
                port = None
            else:
                invalid_ports.append(port_info.name)
        except:
            invalid_ports.append(port_info.name)
        finally:
            if port is not None:
                port.close()
                port = None
            else:
                print(f"{port_info.name} is not a controller port. ignoring...")

    return rt
    
def check_export_path(opt : auto_options) -> bool:
    if not os.path.isdir(opt.export_dir):
        return False
    return True

if __name__ == "__main__":
    opt = auto_options()
    state = auto_state()
    def remove_header(option):
        option.include_header = False
        return option
    opt.append_jig_transform(lambda option: remove_header(option))

    if not check_export_path(opt):
        print(f"Output directory \"{opt.export_dir}\" does not exist. Exiting...")
        sys.exit(-1)

    

    print("Waiting for a COM port connection...")
    port = None
    invalid_ports : list[str] = []
    valid_ports : list[serial.Serial]
    def scan_ports() -> serial.Serial | None:
        valid_ports = query_com_ports(invalid_ports)
        # Assumption:  The invalid ports is *always* added, never removed.
        if len(valid_ports) > 1:
            for p in valid_ports:
                p.close()
            print("error: found more than one valid port")
            sys.exit(-1)
        elif len(valid_ports) == 1:
            return valid_ports[0]

        return None

    port = scan_ports()
    while port is None:
        time.sleep(opt.port_scan_period.total_seconds())
        port = scan_ports()

    state.controller = comms.pnp_mcm_controller(port)

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
        finally:
            if state.output_file is not None:
                # Assuming the file is invalid
                state.output_file.close()
