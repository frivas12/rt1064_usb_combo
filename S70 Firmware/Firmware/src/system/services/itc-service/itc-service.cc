#include "./itc-service.hh"

#include <optional>

#include "FreeRTOSConfig.h"
#include "UsbCore.h"
#include "service-inits.h"
#include "slots.h"

using namespace service::itc;
/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/
std::optional<pipeline_hid_in_t> opt_hid_in   = {};
std::optional<pipeline_hid_out_t> opt_hid_out = {};
std::optional<pipeline_cdc_t> opt_cdc = {};

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
service::itc::pipeline_hid_in_t& service::itc::pipeline_hid_in() {
    configASSERT(opt_hid_in.has_value());
    return *opt_hid_in;
}

service::itc::pipeline_hid_out_t& service::itc::pipeline_hid_out() {
    configASSERT(opt_hid_out.has_value());
    return *opt_hid_out;
}

service::itc::pipeline_cdc_t& service::itc::pipeline_cdc() {
    configASSERT(opt_cdc.has_value());
    return *opt_cdc;
}

void service::itc::init() {
    opt_hid_in.emplace(NUMBER_OF_BOARD_SLOTS);
    opt_hid_out.emplace(USB_NUMDEVICES);
    opt_cdc.emplace(3); // 2 for each source, 1 extra for safety
}

extern "C" void itc_service_init(void) { service::itc::init(); }

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
