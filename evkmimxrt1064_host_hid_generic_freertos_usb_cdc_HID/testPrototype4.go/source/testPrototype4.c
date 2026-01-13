/*
 * Minimal RT1064 bring-up using internal RC oscillator and SEGGER RTT.
 */

#include "fsl_device_registers.h"
#include "fsl_clock.h"
#include "segger_rtt/SEGGER_RTT.h"

#define TESTPROTOTYPE4_RC_OSC_FREQ_HZ (24000000U)

static void TestPrototype4_InitInternalRcClocks(void)
{
    /* Keep the frequency set for clock APIs; we are using the internal RC. */
    CLOCK_SetXtalFreq(TESTPROTOTYPE4_RC_OSC_FREQ_HZ);
    CLOCK_InitRcOsc24M();
    CLOCK_SwitchOsc(kCLOCK_RcOsc);
}

int main(void)
{
    TestPrototype4_InitInternalRcClocks();
    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0U, "Hello World from testPrototype4 (internal RC + RTT)\r\n");

    while (1)
    {
        __NOP();
    }
}
