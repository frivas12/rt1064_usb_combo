# End-to-End Testing of the HID Service
Based on commit 14074d3d.

The main things to be aware with the mapping service are...
1. The LIVE state based on the APT layer and mutations for IN mappings.
2. The LIVE state based on the APT layer and routing for OUT mappings.

## T.1: Code Review
1. The `address_handle` is a move-only data type.
2. The `address_handle` has ownership of its associated mutex until destroyed or moved.
3. The `in_map(uint8_t)` and `out_map(uint8_t)` functions will check if dirty data needs to be calculated.
4. Function `set_apt(const Hid_Mapping_control_out&, uint16_t)` will recalculate routing.
5. Function `set_apt(const Hid_Mapping_control_in&, uint16_t)` will recalulate IN mutations if applicable.
6. Function `set_apt(const Hid_Mapping_control_in&, uint16_t)` will recalulate routing if an OUT map targets it.
7. OUT routings may only target mutable controls.
8. `load_in_from_eeprom(drivers::spi::handle_factory&)` will reset overloads and recalculate OUT routings.
9. `load_out_from_eeprom(drivers::spi::handle_factory&)` will recalculate OUT routings.
10. At startup, all maps are loaded from EEPROM; overloads are reset; and routings are calculated.

## T.2: void services::hid_mapping::address_handle::set_apt(const Hid_Mapping_control_out&, uint16_t)
1. No Routing -> Routing
    1. Have 2 input controls and 1 output controls.
       Tie input control 1 to axis 1.
       Tie input control 2 to toggle to axis 3.
       Tie output control 1 to axis 2.
    2. Check: Input 1 toggles axis 1.  Output 1 is steady.
    3. Check: Input 3 changes input 1 to axis 3.  Output 1 is steady.
    4. Check: Input 1 now toggles axis 3. Output 1 is steady.
    5. Tie output 1 to axis 1.
    6. Check: Input 1 toggles axis 2.  Output 1 changes.
    7. Check: Input 3 changes input 1 to axis 3.  Output 1 reflects this.
2. Routing -> No Routing
    Do the previous test in reverse.

Here are some commands for the MCMK4 using the R knob and R LED:
Input 1: Axis S1 
0x14 0x40 0x15 0 0x91 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0xFF 0x00 0 0 0 0 0 0
Input 2: Toggle I1 to S3
0x14 0x40 0x15 0 0x91 1 0 4 0 0 0 0 1 1 0 4 0 0 0 0xFF 0x00 0 2 0 0 0 0
Output 1: Axis S2
0x17 0x40 0x0F 0 0x91 1 0 0 8 0 0 0 0 0 2 3 4 2 0 0 0

Output 1: Axis S1
0x17 0x40 0x0F 0 0x91 1 0 0 8 0 0 0 0 0 2 3 4 1 0 0 0

## T.3: void services::hid_mapping::address_handle::set_apt(const Hid_Mapping_control_out&, uint16_t)
1. Mutation Control -> Mutable Control
    1. Have 2 input controls.
        Tie input 1 to axis 1 and input 2 to toggle input 1 to axis 2.
    2. Check: Input 2 changes input 1 to axis 2.
    3. Tie input 2 to axis 2.
    4. Check: Input 1 affects axis 1.
2. Mutation Control is disabled.
    1. Have 2 input controls.
        Tie input 1 to axis 1 and input 2 to toggle input 1 to axis 2.
    2. Check: Input 2 changes input 1 to axis 2.
    3. Disable input 2.
    4. Check: Input 1 affects axis 1.
3. Mutation mode change.
    1. Have 2 input controls.
        Tie input 1 to axis 1 and input 2 to toggle input 1 to axis 2.
    2. Check: Input 2 changes input 1 to axis 2.
    3. Tie input 2 to set the input 1 to axis 2 (not toggle).
    4. Check: Input 1 affects axis 1.
4. Affects routing
    1. Have 2 input controls and 1 output control.
        Tie input 1 to axis 1 and input 2 to toggle input 1 to axis 2.
        Tie output 1 to axis 1.
    2. Check: Output 1 follows input 1.
    3. Tie input 1 to axis 3.
    4. Check: Output 1 no longer follows input 1.

Here are some commands for the MCMK4 using the R knob and LED button.
Input 1: Axis S1 
0x14 0x40 0x15 0 0x91 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0xFF 0x00 0 0 0 0 0 0
Input 1: Axis S3
0x14 0x40 0x15 0 0x91 1 0 0 0 0 0 0 0 0 0 4 0 0 0 0xFF 0x00 0 0 0 0 0 0
Input 2: Toggle I1 to S2
0x14 0x40 0x15 0 0x91 1 0 4 0 0 0 0 1 1 0 2 0 0 0 0xFF 0x00 0 2 0 0 0 0
Input 2: Toggle I1 to S2 (disabled)
0x14 0x40 0x15 0 0x91 1 0 4 0 0 0 0 1 1 0 2 0 0 0 0xFF 0x00 0 2 0 0 0 1
Input 2: Toggle Lock S2
0x14 0x40 0x15 0 0x91 1 0 4 0 0 0 0 0 0 0 2 0 0 0 0xFF 0x00 0 5 0 0 0 0
Input 2: Set I1 to S2
0x14 0x40 0x15 0 0x91 1 0 4 0 0 0 0 1 1 0 2 0 0 0 0xFF 0x00 0 1 0 0 0 0

Output 1: Axis S1
0x17 0x40 0x0F 0 0x91 1 0 0 8 0 0 0 0 0 2 3 4 1 0 0 0
Output 1: Axis S2
0x17 0x40 0x0F 0 0x91 1 0 0 8 0 0 0 0 0 2 3 4 2 0 0 0
