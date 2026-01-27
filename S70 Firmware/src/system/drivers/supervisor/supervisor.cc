/**
 * @file supervisor.c
 *
 * @brief The supervisor task handles the blinking LED, reading the CPU temperature, enclosure
 * temperature, and power in voltage.  The task runs at 10ms but only the power check runs at
 * 10ms all the rest of the services run at 500ms.
 *
 *  *@dot
 digraph G {
 1[label="Read power voltage"]
 2[shape=diamond, label="Power good"]
 3[label="Power BAD"]
 4[label="Read eeprom flag"]
 5[label="Not restarted"]
 6[label="Write eeprom flag"]
 7[label="Restart"]
 8[label="Already restarted"]
 9[label="Power GOOD"]
 10[label="Reset EEPROM flag if set"]

 1->2->3->4->5->6->7->1
 4->8->1
 2->9->10->1
 }
 * @enddot
 */

#include "supervisor.h"

//#include <asf.h>
#include "Debugging.h"
#include <string.h>
#include "sys_task.h"
#include "user_spi.h"
#include <cpld.h>
#include "user_adc.h"
#include "usb_host.h"
#include "25lc1024.h"
#include "math.h"
#include "../device_detect/device_detect.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if TASK_HEARTBEAT_WATCHDOGS > 0
static const heartbeat_watchdog * g_heartbeat_watchdogs[TASK_HEARTBEAT_WATCHDOGS];
static size_t g_heartbeat_watchdogs_size = 0;
#endif
#if TASK_EVENT_WATCHDOGS > 0
static const event_watchdog * g_event_watchdogs[TASK_EVENT_WATCHDOGS];
static size_t g_event_watchdogs_size = 0;
#endif

constexpr TickType_t IDENTIFY_PERIOD = 150;
constexpr TickType_t IDENTIFY_BLINKS = 2;
constexpr TickType_t IDENTIFY_COUNTER_MAX = IDENTIFY_PERIOD * IDENTIFY_BLINKS;
static TickType_t s_identify_counter;
static slot_nums s_identify_slot;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void set_led_duty(uint32_t duty);
static void check_power_good(void);
static void update_fade_val(void);

/**
 * Check that all watchdogs are valid.
 * \return true All watchdogs are valid.
 * \return false At least one watchdog failed.  Initiate reboot.
 */
static bool check_watchdogs(void);

bool global_500ms_tick;
uint8_t fade_val;

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

// MARK:  SPI Mutex Required
static void set_led_duty(uint32_t duty)
{
	set_reg(C_SET_PWM_DUTY, LED_ADDRESS, 0, duty);
}

// MARK:  SPI Mutex Required
static void check_power_good(void)
{
	static uint8_t restart_check_val;
	static bool restart_eeprom_cleared = false;

	if (board.vin_monitor_val >= MIN_PWR_VOLTAGE)
	{
		board.power_good = POWER_GOOD;
		if (!restart_eeprom_cleared)
		{
			restart_check_val = 0;
			restart_eeprom_cleared = true;
			eeprom_25LC1024_write(POWER_BAD_RESTART_CHECK_EEPROM_START, 1,
					&restart_check_val);
		}
	}
	else
	{
		debug_print("ERROR: Power failure\n");
		board.power_good = POWER_NOT_GOOD;
		/*Restart board if power not good but only once.Power in checks the high voltage, if it
		 * is low it will restart the system.  To avoid multiple restarts while power is not
		 * good (ie when the emergency stop is hit) we can set a value in eeprom to indicate we have
		 * already restarted and just wait until power is good to continue with task*/

		eeprom_25LC1024_read(POWER_BAD_RESTART_CHECK_EEPROM_START, 1,
				&restart_check_val);
		if (restart_check_val != 100)
		{
			restart_check_val = 100;
			eeprom_25LC1024_write(POWER_BAD_RESTART_CHECK_EEPROM_START, 1,
					&restart_check_val);
			xSemaphoreGive(xSPI_Semaphore);
			vTaskDelay(pdMS_TO_TICKS(50));
			xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
			restart_board();
		}
	}
}

static void update_fade_val(void)
{
	static bool fade_dir = 1;
    static constexpr int FADE_RATE = 10;

	/*Used for a global variable that increases and decreases 0 to 255
	 * Used for HID joystick fade led*/
	if (fade_val >= 255 - FADE_RATE)
	{
		fade_dir = 0;
	}
	else if (fade_val <= FADE_RATE)
	{
		fade_dir = 1;
	}

	if (fade_dir)
	{
		fade_val = fade_val + FADE_RATE;
	}
	else
	{
		fade_val = fade_val - FADE_RATE;
	}
}

static bool check_watchdogs(void)
{
	bool rt = true;
	TickType_t now = xTaskGetTickCount();
#if TASK_HEARTBEAT_WATCHDOGS > 0
	for (size_t i = 0; i < g_heartbeat_watchdogs_size && rt; ++i)
	{
		rt = g_heartbeat_watchdogs[i]->valid(now);
	}
#endif
#if TASK_EVENT_WATCHDOGS > 0
	for (size_t i = 0; i < g_event_watchdogs_size && rt; ++i)
	{
		rt = g_event_watchdogs[i]->valid(now);
	}
#endif
	return rt;
}

/****************************************************************************
 * Task
 ****************************************************************************/
/**
 *
 * Takes xSPI_Semaphore - reserving SPI bus
 *
 * @param pvParameters : not used
 */
static void task_supervisor_check(void *pvParameters)
{
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = SUPERVISOR_TIMEOUT; /* 10 ms*/

	/*Task runs at 10ms for the power good check but other services like led blink need to run
	 * at 500ms so this counter is used to run those at a slower pace*/
	uint8_t task_counter = 0;

#define LED_BLINK_RATE	50 // in 10's ms
	uint8_t led_blink_counter = 0;

	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	UNUSED(pvParameters);

	board.power_good = POWER_NOT_GOOD;

	for (;;)
	{
		//block until we can send the message on the SPI port
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

		if (cpld_read_em_stop_flag())
		{
			board.clpd_em_stop = 0xff;
			debug_print("em /r/n");
		}

		/*500 ms task go here*/
		if (task_counter >= 50)
		{
			board.temperature_sensor_val = read_analog(AFEC0, TEMP_SENSOR);
			board.cpu_temp_val = read_analog(AFEC0, AFEC_TEMPERATURE_SENSOR);
			task_counter = 0;

			/* reset the watchdog timeer RSWDT_CR.WDRSTT , default set to 15 sec and will reset the
			 * microcontroller if not reset here*/
			wdt_restart(WDT);

			global_500ms_tick = !global_500ms_tick;
		}

		board.vin_monitor_val = read_analog(AFEC1, VIN_MONITOR);
		check_power_good();
		update_fade_val();

		// Marking as unsafe because it is not guaranteed to be synched.
		bool unsafe_pnp_error_on_any_slot;
		{
			pnp_status_t pnp_error_set = 0;

			// Allowing for unsafe 
			for (int i = 0; i < NUMBER_OF_BOARD_SLOTS; ++i)
			{
				pnp_error_set |= slots[i].pnp_status;
			}

			// Do not raise a pnp error when a slot does not have a device connected (bit 0).
			unsafe_pnp_error_on_any_slot = (pnp_error_set & 0xFFFFFFFE) > 0;
		}

		/*This is the status LED on the front panel power switch.  The set_led_duty
		 * must be called every 10ms or the CPLD will flash this LED because it thinks
		 * the uC is locked up.  We can not send this to report errors making the LED blink
		 * and also shutting down slot cards*/

		// blink led if there is an error
		if (s_identify_counter < IDENTIFY_COUNTER_MAX)
		{
			constexpr TickType_t FIRST_PULSE_END = IDENTIFY_PERIOD * 10 / 100;
			constexpr TickType_t SECOND_PULSE_BEGIN = IDENTIFY_PERIOD * 20 / 100;
			constexpr TickType_t SECOND_PULSE_END = IDENTIFY_PERIOD * 50 / 100;
			// LED Identifying
			const TickType_t period_counter = s_identify_counter % IDENTIFY_PERIOD;
			
			if (period_counter < FIRST_PULSE_END ||
				(SECOND_PULSE_BEGIN <= period_counter && period_counter < SECOND_PULSE_END)
			) {
				board.dim_value = 1;
			} else {
				board.dim_value = board.dim_bound;
			}

			if (++s_identify_counter == IDENTIFY_COUNTER_MAX)
			{
				s_identify_slot = NO_SLOT;
			}
		}
		else if (board.error > 0 || unsafe_pnp_error_on_any_slot)
		{
			if (led_blink_counter >= LED_BLINK_RATE)
			{
				set_led_duty((1900 * 1) / 100);
				board.dim_value = 1;
			}
			else
			{
				board.dim_value = 100;
			}
			led_blink_counter++;
			if (led_blink_counter > (LED_BLINK_RATE * 2))
				led_blink_counter = 0;
		}
		else // make sure we update, this is the handshake to let the
			// CPLD know the uC is not locked up
		{
			board.dim_value = board.dim_bound;
		}

		uint8_t SLOTS[8];
		memset(SLOTS, 0xFF, 8);

		// Slot LED control
		for (slot_nums slot = SLOT_1; slot <= SLOT_8; ++slot)
		{
            const bool IDENTIFYING = s_identify_counter != IDENTIFY_COUNTER_MAX;
            const bool ERROR_CONDITION = 
                (slots[slot].pnp_status & 0xFFFFFFFE) != 0
                || (board.error & (1 << (uint8_t)slot)) != 0;
            if ((IDENTIFYING && (s_identify_slot == slot || s_identify_slot == NO_SLOT)) ||
                (!IDENTIFYING && ERROR_CONDITION))
            {
				slots[slot].slot_led_brightness = board.dim_value;
			} else {
				slots[slot].slot_led_brightness = 0xFF;
			}
		}

		set_led_duty((1900 * board.dim_value) / 100);

		xSemaphoreGive(xSPI_Semaphore);

		if (!check_watchdogs())
		{
			restart_board();
		}

		task_counter++;

		// Wait for the next cycle.
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

static void task_supervisor_unprogrammed(void *pvParameters)
{
        TickType_t xLastWakeTime;
        const TickType_t xFrequency = SUPERVISOR_TIMEOUT; /* 10 ms*/

        /*Task runs at 10ms for the power good check but other services like led blink need to run
         * at 500ms so this counter is used to run those at a slower pace*/
        uint8_t task_counter = 0;


        // Initialize the xLastWakeTime variable with the current time.
        xLastWakeTime = xTaskGetTickCount();

        UNUSED(pvParameters);

        for (;;)
        {
                /*500 ms task go here*/
                if (task_counter >= 50)
                {
                        board.temperature_sensor_val = read_analog(AFEC0, TEMP_SENSOR);
                        board.cpu_temp_val = read_analog(AFEC0, AFEC_TEMPERATURE_SENSOR);
                        task_counter = 0;

                        /* reset the watchdog timeer RSWDT_CR.WDRSTT , default set to 15 sec and will reset the
                         * microcontroller if not reset here*/
                        wdt_restart(WDT);

                        global_500ms_tick = !global_500ms_tick;
                }

                task_counter++;

                // Wait for the next cycle.
                vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void supervisor_init(const bool programmed)
{
	// Initialize identifying feature.
	s_identify_counter = IDENTIFY_COUNTER_MAX;
	s_identify_slot = NO_SLOT;

        const TaskFunction_t TASK = (programmed) ? (&task_supervisor_check)
                : (&task_supervisor_unprogrammed);

	/* Create task to make led blink */
	if (xTaskCreate(TASK, "supervisor",
	TASK_SUPERVISOR_CHECK_STACK_SIZE, NULL,
	TASK_SUPERVISOR_CHECK_STACK_PRIORITY, &xSupervisorHandle) != pdPASS)
	{
		debug_print("Failed to create supervisor task\r\n");
	}
}

void supervisor_identify(const slot_nums slot)
{
	s_identify_counter = 0;
	s_identify_slot = slot;
}


void supervisor_add_watchdog(event_watchdog * const p_watchdog)
{
#if TASK_EVENT_WATCHDOGS > 0
	if (g_event_watchdogs_size == TASK_EVENT_WATCHDOGS)
	{
		// TODO:  error
		for (;;) {} // Spin
	}

	g_event_watchdogs[g_event_watchdogs_size++] = p_watchdog;
#endif
}


void supervisor_add_watchdog(heartbeat_watchdog * const p_watchdog)
{
#if TASK_HEARTBEAT_WATCHDOGS > 0
	if (g_heartbeat_watchdogs_size == TASK_HEARTBEAT_WATCHDOGS)
	{
		// TODO:  error
		for (;;) {} // Spin
	}

	g_heartbeat_watchdogs[g_heartbeat_watchdogs_size++] = p_watchdog;
#endif
}

//EOF
