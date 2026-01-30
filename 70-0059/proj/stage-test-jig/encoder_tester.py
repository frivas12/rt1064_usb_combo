import comms
import serial
import serial.tools.list_ports
import time
import datetime
import options



class auto_options:
    port_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    slot_scan_period : datetime.timedelta = datetime.timedelta(seconds=0.5)
    slots_to_test : set[int] = {5, 6, 7}
    move_timeout : datetime.timedelta = datetime.timedelta(seconds=1)
    encoder_count_delta : int = 1000
    encoder_count_test_deadband : int = 2 

    verbose_output : bool = False
    post_mortem_top_level_exceptions : bool = False
    

class auto_state:
    controller : comms.pnp_mcm_controller

OPTIONS=[
    options.option(short_name="g", long_name="debug", description="enables breaking on exception"),
    options.option(short_name="v", long_name="verbose", description="enables verbose output")
]    



def step(state : auto_state, options : auto_options):
    print("Waiting for a single COM port connection...")
    port = None
    ports = serial.tools.list_ports.comports()
    while len(ports) != 1:
        time.sleep(options.port_scan_period.total_seconds())
        ports = serial.tools.list_ports.comports()

    time.sleep(0.5)

    port_name = ports[0].name
    port = serial.Serial(port_name)

    state.controller = comms.pnp_mcm_controller(port)
    

    try:
        # Wait for all the slots to be available.
        def scan_slots() -> set[int]:
            current_slots = set()
            for slot in options.slots_to_test:
                if state.controller.is_device_connected(slot):
                    current_slots.add(slot)
            return current_slots

        available_slots = None
        current_slots = scan_slots()
        while current_slots.intersection(options.slots_to_test) != opt.slots_to_test:
            if current_slots != available_slots:
                print("Waiting for stages to connect on the following ports: [%s]" %
                    ", ".join([str(obj) for obj in options.slots_to_test.difference(current_slots)]))
            available_slots = current_slots
            time.sleep(options.port_scan_period.total_seconds())
            current_slots = scan_slots()

        print("Starting test...")
        # Set the starting positions to 0.
        for slot in options.slots_to_test:
            state.controller.set_position(slot, 0)

        # Calculate their target positions.
        conversion_factor : map[int, float] = {}
        target_position : map[int, int] = {}
        for slot in options.slots_to_test:
            conversion_factor[slot] = state.controller.get_nm_per_encoder_count(slot)
            target_position[slot] = options.encoder_count_delta
        
        # Command them to move.
        for slot in options.slots_to_test:
            state.controller.move_to(slot, target_position[slot])
        
        # Wait for the timeout
        time.sleep(options.move_timeout.total_seconds())

        # Record their final positions
        ending_position : map[int, int] = {}
        for slot in options.slots_to_test:
            ending_position[slot] = state.controller.get_position(slot)

        # Check for any issues
        successful = True
        for slot in options.slots_to_test:
            delta = abs(ending_position[slot] - target_position[slot])
            max_delta = options.encoder_count_test_deadband
            if options.verbose_output:
                target = target_position[slot] * conversion_factor[slot]
                end = ending_position[slot] * conversion_factor[slot]
                print("Slot %d\n\tStarting Position (nm):  %3f\n\tTarget Position (nm):  %3f\n\tEnding Position (nm):  %3f\n\n\tCoefficient (nm/encoder_count):  %3f" %
                      (slot, 0, target, end, conversion_factor[slot]))
            if delta > max_delta:
                successful = False
                print("FAILURE:  slot %d had a delta of %3f (more than max error of %3f)"
                      % (slot, delta * conversion_factor[slot], max_delta * conversion_factor[slot]))
            elif options.verbose_output:
                print("SUCCESS:  slot %d had a delta of %3f (less than max error of %3f)"
                      % (slot, delta * conversion_factor[slot], max_delta * conversion_factor[slot]))

        # Move back to starting poistion
        for slot in options.slots_to_test:
            state.controller.move_to(slot, 0)
        
        # Wait for the timeout
        time.sleep(options.move_timeout.total_seconds())

        
        if successful:
            print("SUCCESS")

    finally:
        state.controller.close()

    print("Waiting for the controller to be disconnected...")
    ports = serial.tools.list_ports.comports()
    while port_name in [port.name for port in ports]:
        time.sleep(options.port_scan_period.total_seconds())
        ports = serial.tools.list_ports.comports()




if __name__ == "__main__":
    opt = auto_options()
    state = auto_state()

    result = options.parse_arguments(OPTIONS)

    for option in result.flags:
        if option.short_name == 'v':
            opt.verbose_output = True
        elif option.short_name == 'g':
            opt.post_mortem_top_level_exceptions = True

    running = True
    while running:
        try:
            step(state, opt)
        except KeyboardInterrupt:
            print("interrupted - exiting program...")
            running = False
        except Exception as e:
            if opt.post_mortem_top_level_exceptions:
                import pdb; pdb.post_mortem()