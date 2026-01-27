/**
 * @file hid_report.c
 *
 * @brief Functions for gathering all the information from the HID report to populate the
 * structure describing the device.
 *
 */

#include "Debugging.h"
#include <asf.h>
#include "hid_report.h"
#include "usb_device.h"
#include "string.h"
#include "usb_host.h"
#include "hid.h"
#include "max3421e_usb.h"

#include "hid_item_parser.hh"

/****************************************************************************
 * Private Data
 ****************************************************************************/
extern SemaphoreHandle_t xSPI_Semaphore;

struct global_item_state_t
{
	uint16_t usage_page;
	int32_t logical_minimum;
	int32_t logical_maximum;
	int32_t physical_minimum;
	int32_t physical_maximum;
	int32_t unit_exponent;
	int32_t unit;
	uint32_t report_size;
	uint32_t report_id;
	uint32_t report_count;
};

struct local_item_state_t
{
	uint8_t usage_queue_length;
	uint32_t usage_queue[MAX_NUM_CONTROLS];

	// uint32_t usage;
	uint32_t usage_minimum;
	uint32_t usage_maximum;
	uint32_t designator_index;
	uint32_t designator_minimum;
	uint32_t designator_maximum;
	uint32_t string_index;
	uint32_t string_minimum;
	uint32_t string_maximum;
};

struct item_state_t
{
	global_item_state_t global;
	local_item_state_t local;	
};

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void get_usage_page(const hid_item_parser& r_parser, item_state_t& r_item_state);
static bool get_usage(const hid_item_parser& r_parser, item_state_t& r_item_state);

/**
 * Populates the usage queue with the usage range specified (if any).
 * \param r_local_state The local state whose usages will be extended
 * \return true Parsing was successful.
 * \return false The range is too large.  Do not enumerate the device.
 */
static bool expand_usage_range(local_item_state_t& r_local_state);
static void parse_item_bits(const uint32_t item_bits, const hid_tag_e tag);
static void get_collection_type(uint8_t second_byte_toparse);
static void create_control(Hid_Control_info * const p_control, const item_state_t& r_item_state, const uint32_t item_bits, const uint32_t usage, const uint32_t bit_position);
static bool create_input_controls(const uint8_t address, const item_state_t& r_item_state, const uint32_t item_bits);
static bool create_output_controls(const uint8_t address, const item_state_t& r_item_state, const uint32_t item_bits);

/**
 * Parses the HID report.
 * \param address The address of the HID device being parsed.
 * \param r_parser A reference to the hid item parser.
 * \param r_global_items A reference to the global items structure.
 * \return true The report was successfully parsed
 * \return false The report had some issue.  Do not support the device.
 */
static bool parse_report(uint8_t address, hid_item_parser& r_parser, item_state_t& r_item_state);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void get_usage_page(const hid_item_parser& r_parser, item_state_t& r_item_state)
{
	r_item_state.global.usage_page = r_parser.get_data();

#if DEBUG && DEBUG_HID_REPORT
	switch (r_global_items.usage_page)
	{
	case HID_USAGE_PAGE_GENERIC_DESKTOP:
		hid_report_print(" (Generic_Desktop_Controls)		");
		break;

	case HID_USAGE_PAGE_SIMULATION:
		hid_report_print(" (Simulation_Controls)			");
		break;

	case HID_USAGE_PAGE_VR:
		hid_report_print(" (Game VR_Controls)		");
		break;

	case HID_USAGE_PAGE_SPORT:
		hid_report_print(" (Sport_Controls)		");
		break;

	case HID_USAGE_PAGE_GAME:
		hid_report_print(" (Game_Controls)			");
		break;

	case HID_USAGE_PAGE_GENERIC_DEVICE:
		hid_report_print(" (Generic_Device_Controls)				");
		break;

	case HID_USAGE_PAGE_KEYBOARD_KEYPAD:
		hid_report_print(" (Keyboard_Keypad)				");
		break;

	case HID_USAGE_PAGE_LED:
		hid_report_print(" (LED)				");
		break;

	case HID_USAGE_PAGE_BUTTON:
		hid_report_print(" (Button)				");
		break;

	default:
		hid_report_print("Vendor-Defined\n");
		break;
	}
#endif
}
static bool get_usage(const hid_item_parser& r_parser, item_state_t& r_item_state)
{
	const std::size_t ITEM_SIZE = r_parser.get_item_size();
	uint32_t usage = r_parser.get_data();

	switch(ITEM_SIZE)
	{
	case 1:		// 1 byte
	case 2:		// 2 bytes
		usage |= (r_item_state.global.usage_page << 16);
	break;
	}


#if DEBUG_ENABLE && DEBUG_HID_REPORT
// This code is broken.  It needs to parse the last usage defined in the usage array.
	if(ri->usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP)
	{

		switch (usage)
		{
		case HID_USAGE_UNDEFINED:
			hid_report_print(" (Undefined)		");
			break;

		case HID_USAGE_POINTER:
			hid_report_print(" (Pointer)			");
			break;

		case HID_USAGE_MOUSE:
			hid_report_print(" (Mouse)		");
			break;

		case HID_USAGE_RESERVED:
			hid_report_print(" (Reserved)		");
			break;

		case HID_USAGE_JOYSTICK:
			hid_report_print(" (Joystick)			");
			break;

		case HID_USAGE_GAME_PAD:
			hid_report_print(" (Game_Pad)				");
			break;

		case HID_USAGE_KEYBOARD:
			hid_report_print(" (Keyboard)				");
			break;

		case HID_USAGE_KEYPAD:
			hid_report_print(" (Keypad)				");
			break;

		case HID_USAGE_MULTIAXIS_CONTROLLER:
			hid_report_print(" (Multiaxis_Controller)				");
			break;

		case HID_USAGE_TABLET_PC_SYSTEM:
			hid_report_print(" (Tablet_PC_System)				");
			break;

		case HID_USAGE_X:
			hid_report_print(" (X)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_Y:
			hid_report_print(" (Y)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_Z:
			hid_report_print(" (Z)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_RX:
			hid_report_print(" (Rx)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_RY:
			hid_report_print(" (Ry)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_RZ:
			hid_report_print(" (Rz)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_SLIDER:
			hid_report_print(" (Slider)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_DIAL:
			hid_report_print(" (Dial)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_WHEEL:
			hid_report_print(" (Wheel)				");
			add_control = TYPE_INPUT;
			break;

		case HID_USAGE_VX:
			hid_report_print(" (Vx)				");
			break;

		case HID_USAGE_VY:
			hid_report_print(" (Vy)				");
			break;

		default:
			hid_report_print("Vendor-Defined\n");
			break;
		}
	}
	else if (ri->usage_page == HID_USAGE_PAGE_LED)
	{
		switch (ri->second_byte)
		{
			case HID_USAGE_INDICATOR_RED:
				break;

			case HID_USAGE_INDICATOR_GREEN:
				break;

			case HID_USAGE_INDICATOR_AMBER:
				break;

			case HID_USAGE_GENERIC_INDICATOR:
				hid_report_print(" (Generic Indicator)		");
				break;

			default:
				hid_report_print("Vendor-Defined\n");
				break;
		}
	}
#endif


	const bool CAN_ADD = r_item_state.local.usage_queue_length < MAX_NUM_CONTROLS;
	const bool SHOULD_ADD = ITEM_SIZE == 3 || // Explicit
		r_item_state.local.usage_queue_length == 0 || // No data in queue
		r_item_state.local.usage_queue[r_item_state.local.usage_queue_length] != usage; // Last data item was the same.

	if (CAN_ADD && SHOULD_ADD)
	{
		r_item_state.local.usage_queue[r_item_state.local.usage_queue_length++] = usage;
	}

	// XAND:
	// 1. Normal Insertion
	// 2. At capacity but previous usage is the same as the current usage.
	return CAN_ADD == SHOULD_ADD;
}

static bool expand_usage_range(local_item_state_t& r_local_state)
{
        // If the minimum is not set, do not expand.
        if (r_local_state.usage_minimum == 0)
        {
            return true;
        }
	for (uint32_t usage = r_local_state.usage_minimum; usage <= r_local_state.usage_maximum; ++usage)
	{
		if (r_local_state.usage_queue_length == MAX_NUM_CONTROLS)
		{
			return false;
		}

		r_local_state.usage_queue[r_local_state.usage_queue_length++] = usage;
	}

	return true;
}

static void parse_item_bits(const uint32_t item_bits, const hid_tag_e tag)
{
#if DEBUG && DEBUG_HID_REPORT
	// Bit 0
	if (item_bits & 0x01)
	{
		hid_report_print("Const,");
	}
	else
	{
		hid_report_print("Data,");
	}

	// Bit 1
	if (item_bits & 0x02)
	{
		hid_report_print("Variable,");
	}
	else
	{
		hid_report_print("Array,");
	}

	// Bit 2
	if (item_bits & 0x04)
	{
		hid_report_print("Relative,");
	}
	else
	{
		hid_report_print("Absolute,");
	}

	// Bit 3
	if (item_bits & 0x08)
	{
		hid_report_print("Wrap,");
	}
	else
	{
		hid_report_print("No Wrap,");
	}

	// Bit 4
	if (item_bits & 0x10)
	{
		hid_report_print("Nonlinear,");
	}
	else
	{
		hid_report_print("Linear,");
	}

	// Bit 5
	if (item_bits & 0x20)
	{
		hid_report_print("No Preferred,");
	}
	else
	{
		hid_report_print("Preferred State,");
	}

	// Bit 6
	if (item_bits & 0x40)
	{
		hid_report_print("No Null Position,");
	}
	else
	{
		hid_report_print("Null State,");
	}

	// Bit 7 Reserved
	if (tag != hid_tag_e::INPUT)
	{
		if (item_bits & 0x80)
		{
			hid_report_print("Volatile,");
		}
		else
		{
			hid_report_print("Non Volatile,");
		}
	}

	// Bit 8
	if (item_bits & 0x0100)
	{
		hid_report_print("Buffered Bytes,");
	}
	else
	{
		hid_report_print("Bit Field,");
	}

	hid_report_print(")	");
#endif
}

static void get_collection_type(uint8_t second_byte_toparse)
{
	switch (second_byte_toparse)
	{
	case PHYSICAL:
		hid_report_print(" (Physical)			");
		break;

	case APPLICATION:
		hid_report_print(" (Application)			");
		break;

	case LOGICAL:
		hid_report_print(" (Logical)			");
		break;

	case REPORT:
		hid_report_print(" (Report)		");
		break;

	case NAMED_ARRAY:
		hid_report_print(" (Array)		");
		break;

	case USAGE_SWITCH:
		hid_report_print(" (Switch)		");
		break;

	case USAGE_MODIFIER:
		hid_report_print(" (Modifier)		");
		break;

	case USAGE_RESERVED:
		hid_report_print(" (Reserved)		");
		break;

	default:
		error_print(" ERROR3: HID Report, Collection type NA\n");
		break;
	}
}

static void create_control(Hid_Control_info * const p_control, const item_state_t& r_item_state, const uint32_t item_bits, const uint32_t usage, const uint32_t bit_position)
{
	/*Set the usage id for this control.*/
	p_control->usage = usage;

	/*Set the item bits for this control*/
	p_control->item_bits = item_bits;

	/*Set this controls report size*/
	p_control->report_size = r_item_state.global.report_size;

	/*Set the bit position for this control from the number of bits counter*/
	p_control->bit_position = bit_position;

	/*Set the byte position for this control*/
	p_control->byte_position =
			(p_control->bit_position / 8);

	/*Set the logical_min for this control*/
	p_control->logical_min = r_item_state.global.logical_minimum;

	/*Set the logical_max for this control*/
	p_control->logical_max = r_item_state.global.logical_maximum;

	// first_time is not set up
	// prev_value is not set up
}

static bool create_input_controls(const uint8_t address, const item_state_t& r_item_state, const uint32_t item_bits)
{
	for (std::size_t report_num = 0; report_num < r_item_state.global.report_count; report_num++)
	{
		const uint8_t CONTROL_NUM = usb_device[address].hid_data.num_of_input_controls;

		/*Skip Padding*/
		// * We currenly only support array controls.
		if(!Tst_bits(item_bits, 0x01))
		{
			if (CONTROL_NUM == MAX_NUM_CONTROLS)
			{
				error_print("ERROR: To many HID input controls.");
				return false;
			}

			// A usage is dequeued.
			const uint32_t USAGE = r_item_state.local.usage_queue[
				(report_num < r_item_state.local.usage_queue_length) ? report_num : (r_item_state.local.usage_queue_length - 1)
			];

			create_control(
				&usb_device[address].hid_data.hid_in_controls[CONTROL_NUM],
				r_item_state,
				item_bits,
				USAGE,
				usb_device[address].hid_data.num_of_input_bits
			);

			/*Increment the number of input controls*/
			usb_device[address].hid_data.num_of_input_controls++;
		}
		/*Add the report size to the overall number of input bits*/
		usb_device[address].hid_data.num_of_input_bits += r_item_state.global.report_size;
	}

	return true;
}

static bool create_output_controls(const uint8_t address, const item_state_t& r_item_state, const uint32_t item_bits)
{
	for (std::size_t report_num = 0; report_num < r_item_state.global.report_count; report_num++)
	{
		const uint8_t CONTROL_NUM = usb_device[address].hid_data.num_of_output_controls;
		/*Skip Padding*/
		// * We only support array types.
		if(!Tst_bits(item_bits, 0x01))
		{
			if (CONTROL_NUM == MAX_NUM_CONTROLS)
			{
				error_print("ERROR: To many HID output controls.");
				return false;
			}

			// A usage is dequeued.
			const uint32_t USAGE = r_item_state.local.usage_queue[
				(report_num < r_item_state.local.usage_queue_length) ? report_num : (r_item_state.local.usage_queue_length - 1)
			];

			create_control(
				&usb_device[address].hid_data.hid_out_controls[CONTROL_NUM],
				r_item_state,
				item_bits,
				USAGE,
				usb_device[address].hid_data.num_of_output_bits
			);

			/*Increment the number of input controls*/
			usb_device[address].hid_data.num_of_output_controls++;
		}

		/*Add the report size to the overall number of output bits*/
		usb_device[address].hid_data.num_of_output_bits += r_item_state.global.report_size;
	}

	return true;
}


static bool parse_report(uint8_t address, hid_item_parser& r_parser, item_state_t& r_item_state)
{
	// * Assumption:	Item state is set up before hand
	// *				For first call, this means both the local and global states are zeroed.

	// Prevents the do loop from running if at the end of the parser.
	if (r_parser.is_eof())
	{
		return true;
	}

	bool in_delimiter = false;
	bool in_first_delimiter = true;
	
	hid_tag_e tag;
	do
	{
		tag = r_parser.get_item_tag();

		// Skip item parsing when in a delimiter past the first EXCEPT when a close delimiter is reached.
		if (in_delimiter && !in_first_delimiter && tag != hid_tag_e::DELIMITER)
		{
			continue;
		}

		switch(tag)
		{
		case hid_tag_e::INPUT:
			hid_report_print("Input (");
			parse_item_bits(r_parser.get_data(), tag);
			if (!expand_usage_range(r_item_state.local) || !create_input_controls(address, r_item_state, r_parser.get_data()))
			{
				return false;
			}
			memset(&r_item_state.local, 0, sizeof(r_item_state.local));	// Flush local
		break;
		case hid_tag_e::OUTPUT:
			hid_report_print("Output (");
			parse_item_bits(r_parser.get_data(), tag);
			if (!expand_usage_range(r_item_state.local) || !create_output_controls(address, r_item_state, r_parser.get_data()))
			{
				return false;
			}
			memset(&r_item_state.local, 0, sizeof(r_item_state.local));	// Flush local
		break;
		case hid_tag_e::FEATURE:
			hid_report_print("Feature (");
			parse_item_bits(r_parser.get_data(), tag);
			// * Features are currently not supported in our parser.
			memset(&r_item_state.local, 0, sizeof(r_item_state.local));	// Flush local
		break;
		case hid_tag_e::COLLECTION:
			hid_report_print("Collection ");
			get_collection_type(r_parser.get_data_as_uint8());
			memset(&r_item_state.local, 0, sizeof(r_item_state.local));	// Flush local
		break;
		case hid_tag_e::END_COLLECTION:
			hid_report_print("End Collection 				");
			memset(&r_item_state.local, 0, sizeof(r_item_state.local));	// Flush local
		break;

		case hid_tag_e::USAGE_PAGE:
			hid_report_print("Usage Page ");
			get_usage_page(r_parser, r_item_state);
		break;
		case hid_tag_e::LOGICAL_MINIMUM:
			r_item_state.global.logical_minimum = static_cast<int32_t>(r_parser.get_data());
			hid_report_print("Logical Minimum (%d)			", r_item_state.global.logical_minimum);
		break;
		case hid_tag_e::LOGICAL_MAXIMUM:
			r_item_state.global.logical_maximum = static_cast<int32_t>(r_parser.get_data());
			hid_report_print("Logical Maximum (%d)			", r_item_state.global.logical_maximum);
		break;
		case hid_tag_e::PHYSICAL_MINIMUM:
			r_item_state.global.physical_minimum = static_cast<int32_t>(r_parser.get_data());
			hid_report_print("Physical Minimum (%d)			", r_item_state.global.physical_minimum);
		break;
		case hid_tag_e::PHYSICAL_MAXIMUM:
			r_item_state.global.physical_maximum = static_cast<int32_t>(r_parser.get_data());
			hid_report_print("Physical Maximum (%d)			", r_item_state.global.physical_maximum);
		break;
		case hid_tag_e::UNIT_EXPONENT:
			r_item_state.global.unit_exponent = static_cast<int32_t>(r_parser.get_data());
			hid_report_print("Unit Exponent (%d)			", r_item_state.global.unit_exponent);
		break;
		case hid_tag_e::UNIT:
			r_item_state.global.unit = static_cast<int32_t>(r_parser.get_data());
			hid_report_print("Unit (%d)			", r_item_state.global.unit);
		break;
		case hid_tag_e::REPORT_SIZE:
			r_item_state.global.report_size = r_parser.get_data();
			hid_report_print("Report Size (%d)			", r_item_state.global.report_size);
		break;
		case hid_tag_e::REPORT_ID:
			r_item_state.global.report_id = r_parser.get_data();
			hid_report_print("Report ID (%d)			", r_item_state.global.report_id);
		break;
		case hid_tag_e::REPORT_COUNT:
			r_item_state.global.report_count = r_parser.get_data();
			hid_report_print("Report Count (%d)			", r_item_state.global.report_count);
		break;
		case hid_tag_e::PUSH:
			hid_report_print("Push			");

			// Recursive call to push the new state onto the stack.
			{
				item_state_t clone;
				memcpy(&clone, &r_item_state, sizeof(clone));
				if (!parse_report(address, r_parser, clone))
				{
					return false;
				}
			}
		break;
		case hid_tag_e::POP:
			hid_report_print("Pop			");

			// Exits the recursive call.
			return true;
		break;

		case hid_tag_e::USAGE:
			hid_report_print("Usage 	");
			get_usage(r_parser, r_item_state);
		break;
		case hid_tag_e::USAGE_MINIMUM:
			r_item_state.local.usage_minimum = r_parser.get_data();
			if (r_parser.get_item_size() != 4)
			{
				r_item_state.local.usage_minimum |= (r_item_state.global.usage_page << 16);
			}
			hid_report_print("Usage Minimum  (%d)			", r_item_state.local.usage_minimum);
		break;
		case hid_tag_e::USAGE_MAXIMUM:
			r_item_state.local.usage_maximum = r_parser.get_data();
			if (r_parser.get_item_size() != 4)
			{
				r_item_state.local.usage_maximum |= (r_item_state.global.usage_page << 16);
			}
			hid_report_print("Usage Maximum  (%d)			", r_item_state.local.usage_maximum);
		break;
		case hid_tag_e::DESIGNATOR_INDEX:
			r_item_state.local.designator_index = r_parser.get_data();
			hid_report_print("Designator Index  (%d)			", r_item_state.local.designator_index);
		break;
		case hid_tag_e::DESIGNATOR_MINIMUM:
			r_item_state.local.designator_minimum = r_parser.get_data();
			hid_report_print("Designator Maximum  (%d)			", r_item_state.local.designator_minimum);
		break;
		case hid_tag_e::DESIGNATOR_MAXIMUM:
			r_item_state.local.designator_maximum = r_parser.get_data();
			hid_report_print("Designator Maximum  (%d)			", r_item_state.local.designator_maximum);
		break;
		case hid_tag_e::STRING_INDEX:
			r_item_state.local.string_index = r_parser.get_data();
			hid_report_print("String Index  (%d)			", r_item_state.local.string_index);
		break;
		case hid_tag_e::STRING_MINIMUM:
			r_item_state.local.string_minimum = r_parser.get_data();
			hid_report_print("String Maximum  (%d)			", r_item_state.local.string_minimum);
		break;
		case hid_tag_e::STRING_MAXIMUM:
			r_item_state.local.string_maximum = r_parser.get_data();
			hid_report_print("String Maximum  (%d)			", r_item_state.local.string_maximum);
		break;
		case hid_tag_e::DELIMITER:
			if (r_parser.get_data() > 0)
			{
				in_delimiter = true;
				hid_report_print("Delimiter (open)			");
			}
			else
			{
				// Close set.
				in_delimiter = false;
				in_first_delimiter = false;
				hid_report_print("Delimiter (close)			");
			}
		break;
		default:
		break;
		}

#if DEBUG && DEBUG_HID_REPORT
		const uint32_t DATA = r_parser.get_data();
		switch(r_parser.get_item_size())
		{
		case 0:
			hid_report_print("%x\r\n", r_parser.get_header());
		break;
		case 1:
			hid_report_print("%x	%x\r\n", r_parser.get_header(),
				reinterpret_cast<uint8_t*>(&DATA)[0]
			);
		break;
		case 2:
			hid_report_print("%x	%x	%x\r\n", r_parser.get_header(),
				reinterpret_cast<uint8_t*>(&DATA)[0],
				reinterpret_cast<uint8_t*>(&DATA)[1]
			);
		break;
		case 4:
			hid_report_print("%x	%x	%x	%x\r\n", r_parser.get_header(),
				reinterpret_cast<uint8_t*>(&DATA)[0],
				reinterpret_cast<uint8_t*>(&DATA)[1],
				reinterpret_cast<uint8_t*>(&DATA)[2],
				reinterpret_cast<uint8_t*>(&DATA)[3]
			);
		break;
		}
#endif
	} while (r_parser.next_item());
	// ! Cannot have code after loop due to POP returning.
	// If needed, change POP to use a GOTO.

	return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief The HID parser is used to analyze items found in the Report descriptor.
 * The parser extracts information from the descriptor in a linear fashion.
 * The parser collects the state of each known item as it walks through the descriptor,
 * and stores them in an item state table. The item state table contains the state of
 * individual items.
 *
 *
 * @param address : Address of device we are getting the HID report for.
 * @param lengthOfDescriptor : This is the length we got from the HID descriptor
 * report
 * @return
 */
uint8_t HID_get_report(uint8_t address, uint16_t lengthOfDescriptor)
{
	uint8_t rcode = 0;
	uint8_t buf[lengthOfDescriptor];  // ? Isn't this unsafe (stack overflow possibly)

	/*Grab the HID report data*/
	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
	uint8_t Get_HID_Report[8] =
	{ 0x81, USB_REQUEST_GET_DESCRIPTOR, 0x00,
	HID_DESCRIPTOR_REPORT, 0x00, 0x00, static_cast<uint8_t>(lengthOfDescriptor), 0x00 };
	rcode = control_read(address, Get_HID_Report, (uint8_t*) &buf);
	xSemaphoreGive(xSPI_Semaphore);

	if (rcode)
	{
		error_print("ERROR: Can't get USB HID Report\r\n");
		return rcode;
	}
	hid_report_print("HID Report size = %d\r\n\n", lengthOfDescriptor);
	hid_report_print("********* HID Report *********\r\n");

	hid_item_parser parser(&buf, lengthOfDescriptor);
	item_state_t item_state = { 0 };
	if (parse_report(address, parser, item_state))
	{
		usb_device[address].hid_data.num_of_input_bytes =
				usb_device[address].hid_data.num_of_input_bits / 8;
		usb_device[address].hid_data.num_of_output_bytes =
				usb_device[address].hid_data.num_of_output_bits / 8;
		hid_report_print("Finished HID report\r\n");
		return rcode;
	}
	else
	{
		return 1;
	}
}

uint16_t HID_get_usage_page(const Hid_Control_info * const p_control)
{
	return p_control->usage >> 16;
}

uint16_t HID_get_usage_id(const Hid_Control_info * const p_control)
{
	return p_control->usage & 0x0000FFFF;
}
