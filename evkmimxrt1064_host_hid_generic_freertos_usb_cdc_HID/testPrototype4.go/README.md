# testPrototype4 (no external clock)

This folder provides a minimal bring-up firmware skeleton for an RT1064 board
that does **not** have an external crystal/oscillator. The example uses the
internal 24 MHz RC oscillator and writes `Hello World` over SEGGER RTT so you
can verify execution immediately.

## Files

- `source/testPrototype4.c`: Minimal `main()` that configures internal RC clocks
  and prints a startup message via RTT.

## Notes

- The example expects the project include paths to provide:
  - `drivers/` headers (for `fsl_clock.h` and `fsl_device_registers.h`)
  - `utilities/segger_rtt` headers (for `SEGGER_RTT.h`)
- You can integrate this file into an existing MCUXpresso SDK project by adding
  it to your build sources and ensuring the include paths above are configured.
