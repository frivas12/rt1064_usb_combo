import comms
import ow_prog
import datetime
import time
import sys
import serial
import serial.tools.list_ports
import os.path

class auto_options:
    port_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    slot_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    post_program_delay : datetime.timedelta = datetime.timedelta(seconds=0.5)
    scanned_slot : int = ow_prog.one_wire_programming_options().slot_to_program
    post_mortem_top_level_exceptions : bool = True

class auto_state:
    controller : comms.pnp_mcm_controller
    file_to_flash : str


def step(state : auto_state, options : auto_options):
    print("connect a cable to slot %d to begin test\nturn off the controlelr to end testing" % options.scanned_slot)
    while not state.controller.is_cable_connected(options.scanned_slot):
        time.sleep(options.slot_scan_period.total_seconds())

    print('cable detected.  flashing file...')
    ow_options = ow_prog.one_wire_programming_options()
    ow_options.slot_to_program = options.scanned_slot
    with open(state.file_to_flash, 'rb') as file:
        ow_options.data_to_program = file.read()
    
    ow_state = ow_prog.one_wire_programming_state()

    while ow_state.state != 'finished':
        ow_prog.step(state.controller, ow_state, ow_options)

    if ow_state.last_error != 0:
        print('experienced error %d: %s' % (ow_state.last_error, comms.pnp_mcm_controller.parse_one_wire_code(ow_state.last_error)))
    else:
        time.sleep(options.post_program_delay.total_seconds())
        pnp_status = state.controller.get_pnp_status(options.scanned_slot)

        if pnp_status == 0:
            print('TEST PASSED')
        else:
            print('TEST FAILED (pnp error 0x%08X:  %s)' % (pnp_status, comms.pnp_mcm_controller.parse_pnp_status(pnp_status)))

    print('disconnect the cable')
    while state.controller.is_cable_connected(options.scanned_slot):
        time.sleep(options.slot_scan_period.total_seconds())

if __name__ == "__main__":
    opt = auto_options()
    state = auto_state()
    print("Enter slot to test (default: %d)" % opt.scanned_slot)
    line = sys.stdin.readline().replace('\n', '')
    if line != "":
        opt.scanned_slot = int(line)
    
    if len(sys.argv) >= 2:
        state.file_to_flash = sys.argv[1]
    else:
        print("Enter file to flash:")
        line = sys.stdin.readline().replace('\n', '')

    if not os.path.isfile(state.file_to_flash):
        print("error:  could not find file \"%s\"" % state.file_to_flash)
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
    while state.controller.is_open():
        try:
            step(state, opt)
        except KeyboardInterrupt:
            print("interrupted - exiting program...")
            state.controller.close()
        except Exception as e:
            if opt.post_mortem_top_level_exceptions:
                import pdb; pdb.post_mortem()

    print("finished")
