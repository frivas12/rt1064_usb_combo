import comms
import datetime
import time
from typing import Literal
import serial
import serial.tools.list_ports

class jig_options:
    warmup_time : datetime.timedelta = datetime.timedelta(minutes=1)
    tests : int = 2
    poll_frequency : datetime.timedelta = datetime.timedelta(milliseconds=250)
    encoder_slot : int = 6
    stage_slot : int = 7
    zero_offset_from_home : float = 14500000
    bidirectional_movement : float = 13500000
    warmup_movement : float = 10000000
    movement_cooldown : datetime.timedelta = datetime.timedelta(seconds=10)
    include_header : bool = True
    post_test_action = None

    def __init__(self, other = None):
        if other is None:
            return
        
        self.warmup_time = other.warmup_time
        self.tests = other.tests
        self.poll_frequency = other.poll_frequency
        self.encoder_slot = other.encoder_slot
        self.stage_slot = other.stage_slot
        self.zero_offset_from_home = other.zero_offset_from_home
        self.bidirectional_movement = other.bidirectional_movement
        self.warmup_movement = other.warmup_movement
        self.movement_cooldown = other.movement_cooldown
        self.include_header = other.include_header
        self.post_test_action = other.post_test_action


class jig_state:
    running_step_t = Literal['start', 'move forward', 'forward cooldown', 'move back to zero', 'back to zero cooldown', 'move backwards', 
                          'backwards cooldown', 'move forward to zero', 'forward to zero cooldown']
    step : Literal['not started', 'homing', 'warming up', 'running', 'finished'] = 'not started'
    running_step : running_step_t = 'start'
    virtual_zero_encoder : int
    virtual_zero_stage : int
    stage_serial : str
    encoder_coefficient : float
    stage_coefficient : float
    warmup_start : datetime.datetime
    test_counter : int = 0
    # (running step, true encoder, stage encoder)
    test_progress_queue : list[tuple[running_step_t, int, int]] = []

    def to_encoder_position(self, encoder_counts : int) -> float:
        return self.encoder_coefficient * (encoder_counts - self.virtual_zero_encoder)

    def to_stage_position(self, encoder_counts : int) -> float:
        return self.stage_coefficient * (encoder_counts - self.virtual_zero_stage)

    def to_encoder_encoder(self, position : float) -> int:
        return int(position / self.encoder_coefficient + self.virtual_zero_encoder)

    def to_stage_encoder(self, position : float) -> int:
        return int(position / self.stage_coefficient + self.virtual_zero_stage)

    def set_zero_encoder(self, encoder_counts : int):
        self.virtual_zero_encoder = encoder_counts

    def set_zero_stage(self, encoder_counts : int):
        self.virtual_zero_stage = encoder_counts

    def produce_progress(self, encoder_counts : int, stage_counts : int):
        self.test_progress_queue.append((self.running_step, encoder_counts, stage_counts))

    @staticmethod
    def output_header():
        print("Test Number,Forward Position - Encoder (nm),Forward Position - Stage (nm),"
              + "Backlash Zero Position - Encoder (nm),Backlash Zero Position - Stage (nm),"
              + "Reverse Position - Encoder (nm),Reverse Position - Stage (nm),"
              + "Two-Way Zero Position - Encoder (nm),Two-Way Zero Position - Stage (nm)"
        )


def jig_step_not_started(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if not comms.is_device_connected(options.stage_slot):
        raise Exception('stage not detected')

    stage_name = comms.get_device_name(options.stage_slot)
    dash_index = stage_name.find('-')
    if dash_index == -1:
        raise Exception('stage name in incorrect format')
    state.stage_serial = stage_name[(dash_index + 1):]

    state.encoder_coefficient = comms.get_nm_per_encoder_count(options.encoder_slot)
    state.stage_coefficient = comms.get_nm_per_encoder_count(options.stage_slot)        

    return True

def jig_step_homing(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if not comms.is_device_connected(options.encoder_slot):
        raise Exception('jig encoder not detected')
    if not comms.is_device_connected(options.stage_slot):
        raise Exception('stage not detected')

    if comms.is_homed(options.stage_slot):
        state.virtual_zero_encoder = comms.get_position(options.encoder_slot) + options.zero_offset_from_home // state.encoder_coefficient
        state.virtual_zero_stage = comms.get_position(options.stage_slot) + options.zero_offset_from_home // state.stage_coefficient
        return True
    return False

def jig_step_warming_up(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if not comms.is_device_connected(options.stage_slot):
        raise Exception('stage not detected')
    now = datetime.datetime.now()

    if state.running_step == 'start':
        comms.move_to(options.stage_slot, state.to_stage_encoder(options.bidirectional_movement))
        state.warmup_start = now
        state.running_step = 'move forward'
    elif state.running_step == 'move forward':
        if (now - state.warmup_start > options.warmup_time):
            comms.move_to(options.stage_slot, state.to_stage_encoder(-options.warmup_movement))
            state.running_step = 'move back to zero'
        elif state.to_stage_position(comms.get_position(options.stage_slot)) >= options.warmup_movement:
            comms.move_to(options.stage_slot, state.to_stage_encoder(-options.bidirectional_movement))
            state.running_step = 'move backwards'
    elif state.running_step == 'move backwards':
        if (now - state.warmup_start > options.warmup_time):
            comms.move_to(options.stage_slot, state.to_stage_encoder(-options.warmup_movement))
            state.running_step = 'move back to zero'
        elif state.to_stage_position(comms.get_position(options.stage_slot)) <= -options.warmup_movement:
            comms.move_to(options.stage_slot, state.to_stage_encoder(options.bidirectional_movement))
            state.running_step = 'move forward'
    elif state.running_step == 'move back to zero':
        # Moving down to remove backlash
        if not comms.is_moving(options.stage_slot):
            comms.move_to(options.stage_slot, state.to_stage_encoder(0))
            state.running_step = 'move forward to zero'
    elif state.running_step == 'move forward to zero':
        if not comms.is_moving(options.stage_slot):
            state.warmup_start = now
            state.running_step = 'forward to zero cooldown'
    elif state.running_step == 'forward to zero cooldown':
        if now - state.warmup_start > options.movement_cooldown:
            return True
    return False

def jig_step_running_backlash(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if not comms.is_device_connected(options.stage_slot):
        raise Exception('stage not detected')
    now = datetime.datetime.now()

    if state.running_step == 'start':
        state.test_counter += 1
        if state.test_counter > options.tests:
            return True
        else:
            state.set_zero_encoder(comms.get_position(options.encoder_slot))
            state.set_zero_stage(comms.get_position(options.stage_slot))

            state.produce_progress(state.virtual_zero_encoder, state.virtual_zero_stage)
            comms.move_to(options.stage_slot, state.to_stage_encoder(options.bidirectional_movement))
            state.running_step = 'move forward'
    elif state.running_step == 'move forward':
        if not comms.is_moving(options.stage_slot):
            state.running_step = 'forward cooldown'
            state.warmup_start = now
    elif state.running_step == 'forward cooldown':
        if now - state.warmup_start >= options.movement_cooldown:
            state.produce_progress(comms.get_position(options.encoder_slot), comms.get_position(options.stage_slot))
            comms.move_to(options.stage_slot, state.to_stage_encoder(0))
            state.running_step = 'move back to zero'
    elif state.running_step == 'move back to zero':
        if not comms.is_moving(options.stage_slot):
            state.warmup_start = now
            state.running_step = 'back to zero cooldown'
    elif state.running_step == 'back to zero cooldown':
        if now - state.warmup_start >= options.movement_cooldown:
            state.produce_progress(comms.get_position(options.encoder_slot), comms.get_position(options.stage_slot))
            return True
    return False

def jig_step_running_repeatability_part2(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if not comms.is_device_connected(options.stage_slot):
        raise Exception('stage not detected')
    now = datetime.datetime.now()

    if state.running_step == 'move backwards':
        if not comms.is_moving(options.stage_slot):
            state.warmup_start = now
            state.running_step = 'backwards cooldown'
    elif state.running_step == 'backwards cooldown':
        if now - state.warmup_start >= options.movement_cooldown:
            state.produce_progress(comms.get_position(options.encoder_slot), comms.get_position(options.stage_slot))
            comms.move_to(options.stage_slot, state.to_stage_encoder(0))
            state.running_step = 'move forward to zero' 
    elif state.running_step == 'move forward to zero':
        if not comms.is_moving(options.stage_slot):
            state.warmup_start = now
            state.running_step = 'forward to zero cooldown'
    elif state.running_step == 'forward to zero cooldown':
        if now - state.warmup_start >= options.movement_cooldown:
            state.produce_progress(comms.get_position(options.encoder_slot), comms.get_position(options.stage_slot))
            return True

    return False


def jig_step_reliability(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if state.step == 'not started':
        if jig_step_not_started(comms, state, options):
            comms.home(options.stage_slot)
            state.step = 'homing'

    elif state.step == 'homing':
        if jig_step_homing(comms, state, options):
            state.step = 'warming up'
            state.running_step = 'start'

    elif state.step == 'warming up':
        if jig_step_warming_up(comms, state, options):
            state.step = 'running'
            state.running_step = 'start'
            if options.include_header:
                state.output_header()

    elif state.step == 'running':
        def is_backlash_step(step : jig_state.running_step_t) -> bool:
            return state.running_step == "start" or \
                   state.running_step == "move forward" or \
                   state.running_step == "forward cooldown" or \
                   state.running_step == "move back to zero" or \
                   state.running_step == "back to zero cooldown"

        if is_backlash_step(state.running_step):
            if jig_step_running_backlash(comms, state, options):
                if state.running_step == "start":
                    state.step = "finished"
                else:
                    comms.move_to(options.stage_slot, state.to_stage_encoder(-options.bidirectional_movement))
                    state.running_step = 'move backwards'

        elif jig_step_running_repeatability_part2(comms, state, options):
            if options.post_test_action is not None:
                options.post_test_action()
            state.running_step = 'start'

    elif state.step == 'finished':
        return True

    return False

def jig_step_backlash(comms : comms.pnp_mcm_controller, state : jig_state, options : jig_options) -> bool:
    if state.step == 'not started':
        if jig_step_not_started(comms, state, options):
            comms.home(options.stage_slot)
            state.step = 'homing'

    elif state.step == 'homing':
        if jig_step_homing(comms, state, options):
            state.step = 'warming up'
            state.running_step = 'start'

    elif state.step == 'warming up':
        if jig_step_warming_up(comms, state, options):
            state.step = 'running'
            state.running_step = 'start'
            if options.include_header:
                state.output_header()

    elif state.step == 'running':
        if state.running_step == 'move backwards':
            if not comms.is_moving(options.stage_slot):
                comms.move_to(options.stage_slot, state.to_stage_encoder(0))
                state.running_step = 'move forward to zero'
        elif state.running_step == 'move forward to zero':
            if not comms.is_moving(options.stage_slot):
                state.warmup_start = datetime.datetime.now()
                state.running_step = 'forward to zero cooldown'
        elif state.running_step == 'forward to zero cooldown':
            if datetime.datetime.now() - state.warmup_start >= options.movement_cooldown:
                state.running_step = 'start'
        elif jig_step_running_backlash(comms, state, options):
            if state.running_step == "start":
                state.step = 'finished'
            else:
                comms.move_to(options.stage_slot, state.to_stage_encoder(-options.warmup_movement))
                state.running_step = 'move backwards'

    elif state.step == 'finished':
        return True

    return False



def run_jig(comms : comms.pnp_mcm_controller, options : jig_options = jig_options()):
    state = jig_state()
    while comms.is_open() and state.step != 'finished':
        jig_step_reliability(comms, state, options)
        for item in state.test_progress_queue:
            running_state, encoder_count, stage_count = item
            stage_position = state.to_stage_position(stage_count)
            encoder_position = state.to_encoder_position(encoder_count)

            if running_state == 'start':
                print(f"{state.test_counter:d},", end='', flush=True)
            elif running_state == 'forward to zero cooldown':
                print(f"{encoder_position:f},{stage_position:f}")
            else:
                print(f"{encoder_position:f},{stage_position:f},", end='', flush=True)
        state.test_progress_queue = []

        time.sleep(options.poll_frequency.total_seconds())
    
    if state.step != 'finished':
        raise Exception("comms closed before test was finished")

if __name__ == "__main__":

    ports = serial.tools.list_ports.comports()

    if len(ports) != 1:
        print(len(ports))
        exit(1)

    port = serial.Serial(ports[0].name)

    run_jig(comms.pnp_mcm_controller(port))

