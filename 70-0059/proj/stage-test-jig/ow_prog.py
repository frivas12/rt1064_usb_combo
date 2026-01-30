import comms
from typing import Literal
import datetime
import time

class one_wire_programming_options:
    data_to_program : bytearray = bytearray()
    slot_to_program : int = 5
    change_programming_delay : datetime.timedelta = datetime.timedelta(milliseconds=100)

class one_wire_programming_state:
    state_t = Literal['not started', 'programming', 'finished']

    chunk_size : int
    data_to_program : bytearray
    last_error : int = 0
    state : state_t = 'not started'


def step(com : comms.pnp_mcm_controller, state : one_wire_programming_state, options : one_wire_programming_options = one_wire_programming_options()):
    if state.state == 'not started':
        state.chunk_size = com.get_one_wire_chunk_size()
        success = com.set_one_wire_programming(options.slot_to_program, True)
        time.sleep(options.change_programming_delay.total_seconds())
        if success:
            state.data_to_program = options.data_to_program
            state.state = 'programming'
        else:
            state.last_error = -1
            state.state = 'finished'
    elif state.state == 'programming':
        bytes_to_program = len(state.data_to_program) > state.chunk_size and state.chunk_size or len(state.data_to_program)
        if bytes_to_program == 0:
            com.set_one_wire_programming(options.slot_to_program, False)
            time.sleep(options.change_programming_delay.total_seconds())
            state.state = 'finished'
        else:
            writing = state.data_to_program[:bytes_to_program]
            state.data_to_program = state.data_to_program[bytes_to_program:]

            error = com.program_one_wire(options.slot_to_program, writing)
            if error != 0:
                state.last_error = error
                com.set_one_wire_programming(options.slot_to_program, False)
                time.sleep(options.change_programming_delay.total_seconds())
                state.state = 'finished'
    