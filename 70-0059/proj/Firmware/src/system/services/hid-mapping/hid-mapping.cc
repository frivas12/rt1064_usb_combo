#include "hid-mapping.hh"

#include <algorithm>
#include <cstring>

#include "25lc1024.h"
#include "FreeRTOSConfig.h"
#include "UsbCore.h"
#include "hid_in.h"
#include "hid_out.h"
#include "hid_report.h"
#include "lock_guard.hh"
#include "portmacro.h"
#include "projdefs.h"
#include "service-inits.h"
#include "spi-transfer-handle.hh"
#include "static-lifetime-promise.hh"
#include "task.h"
#include "user-eeprom.hh"
#include "user_spi.h"

using namespace service::hid_mapping;

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
struct in_mutations {
    Hid_Mapping_control_in* src_selection = nullptr;
    Hid_Mapping_control_in* src_speed     = nullptr;
    Hid_Mapping_control_in* src_enabled   = nullptr;

    bool operator==(const in_mutations&) const = default;
};

struct out_mutables_routing {
    /// \brief If set, then the slot of the associated control
    ///        will be changed.
    const Hid_Mapping_control_in* parent_ctrl = nullptr;
};

struct address_bundle {
    std::array<Hid_Mapping_control_in, MAX_NUM_CONTROLS> in_mapping_apt;
    std::array<Hid_Mapping_control_in, MAX_NUM_CONTROLS> in_mapping_live;
    std::array<Hid_Mapping_control_out, MAX_NUM_CONTROLS> out_mapping_apt;
    std::array<Hid_Mapping_control_out, MAX_NUM_CONTROLS> out_mapping_live;

    std::array<in_mutations, MAX_NUM_CONTROLS> mutations;

    SemaphoreHandle_t lock   = nullptr;
    bool must_calculate_live = true;

    const Hid_Mapping_control_in* apply_out_routing(
        Hid_Mapping_control_out& mapping) noexcept {
        for (std::size_t ctrl = 0; ctrl < in_mapping_apt.size(); ++ctrl) {
            auto& apt_in  = in_mapping_apt[ctrl];
            auto& live_in = in_mapping_live[ctrl];
            if (apt_in.destination_slot == mapping.source_slots) {
                mapping.source_slots = live_in.destination_slot;
                return &live_in;
            }
        }

        return nullptr;
    }

    address_bundle& calculate_live_if_needed() {
        if (!must_calculate_live) {
            return *this;
        }

        must_calculate_live = false;

        // Reset the live mappings to the APT mappings.
        std::ranges::copy(in_mapping_apt, in_mapping_live.begin());
        std::ranges::copy(out_mapping_apt, out_mapping_live.begin());

        // Applies the mutations then routing.
        for (std::size_t i = 0; i < in_mapping_live.size(); ++i) {
            auto& dest = in_mapping_live[i];
            if (!control_mode_is_mutable(dest.mode)) {
                continue;
            }

            if (const auto selection = mutations[i].src_selection; selection) {
                dest.destination_slot    = selection->destination_slot;
                dest.destination_bit     = selection->destination_bit;
                dest.destination_virtual = selection->destination_virtual;
                dest.destination_port    = selection->destination_port;
                dest.dead_band           = selection->dead_band;
            }

            if (const auto speed = mutations[i].src_speed; speed) {
                dest.speed_modifier = speed->speed_modifier;
                dest.reverse_dir    = speed->reverse_dir;
            }

            dest.control_disable = mutations[i].src_enabled != nullptr;
        }

        // Out mapping routings are derived from source mappings, so it needs to
        // be done after all IN maps are calculated.
        std::ranges::for_each(out_mapping_live,
                              [this](auto& x) { apply_out_routing(x); });

        return *this;
    }
};

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void load_in_mappings_to_apt(drivers::spi::handle_factory& spi,
                                    uint8_t address);
static void load_out_mappings_to_apt(drivers::spi::handle_factory& spi,
                                     uint8_t address);
static void store_in_mappings_from_apt(drivers::spi::handle_factory& spi,
                                       uint8_t address);
static void store_out_mappings_from_apt(drivers::spi::handle_factory& spi,
                                        uint8_t address);
static void set_overload(int8_t addr, uint16_t dest,
                         Hid_Mapping_control_in* const value,
                         Hid_Mapping_control_in* in_mutations::*member);
static void check_mutation_reset(address_bundle& bundle,
                                 Hid_Mapping_control_in& dest,
                                 const Hid_Mapping_control_in& src);

/*****************************************************************************
 * Static Data
 *****************************************************************************/
static std::array<address_bundle, USB_NUMDEVICES> global_bundles;

static SemaphoreHandle_t cache_lock = nullptr;
static code_promise::startup_initialized<drivers::eeprom::page_cache>
    eeprom_cache;

static constexpr address_bundle& get_bundle(uint8_t address) {
    return global_bundles[address - 1];
}

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
address_handle::address_handle(address_handle&& other)
    : address_handle(other._addr) {
    this->_owned = other._owned;
    other._owned = false;
}
address_handle& address_handle::operator=(address_handle&& other) {
    if (_owned) {
        xSemaphoreGive(get_bundle(_addr).lock);
    }
    _owned       = other._owned;
    _addr        = other._addr;
    other._owned = false;

    return *this;
}

address_handle::~address_handle() {
    if (_owned) {
        xSemaphoreGive(get_bundle(_addr).lock);
    }
}

address_handle address_handle::create(uint8_t address) {
    xSemaphoreTake(get_bundle(address).lock, portMAX_DELAY);
    return address_handle(address);
}

std::optional<address_handle> address_handle::try_create(uint8_t address,
                                                         TickType_t timeout) {
    if (0 == address || address > USB_NUMDEVICES) {
        return {};
    }

    if (xSemaphoreTake(get_bundle(address).lock, timeout) != pdTRUE) {
        return {};
    }

    return std::optional(address_handle(address));
}

std::size_t address_handle::out_map_size() const noexcept {
    return get_bundle(_addr).out_mapping_apt.size();
}

std::size_t address_handle::in_map_size() const noexcept {
    return get_bundle(_addr).in_mapping_apt.size();
}

const Hid_Mapping_control_out& address_handle::out_map(
    uint8_t control_idx) const noexcept {
    return get_bundle(_addr)
        .calculate_live_if_needed()
        .out_mapping_live[control_idx];
}

const Hid_Mapping_control_in& address_handle::in_map(
    uint8_t control_idx) const noexcept {
    return get_bundle(_addr)
        .calculate_live_if_needed()
        .in_mapping_live[control_idx];
}

const Hid_Mapping_control_in& address_handle::get_apt_in(
    uint16_t control) const {
    return get_bundle(_addr).in_mapping_apt[control];
}

const Hid_Mapping_control_out& address_handle::get_apt_out(
    uint16_t control) const {
    return get_bundle(_addr).out_mapping_apt[control];
}

void address_handle::set_apt(const Hid_Mapping_control_out& src,
                             uint16_t control) {
    auto& bundle                    = get_bundle(_addr);
    bundle.out_mapping_apt[control] = src;
    bundle.must_calculate_live      = true;
}

void address_handle::set_apt(const Hid_Mapping_control_in& src,
                             uint16_t control) {
    auto& bundle = get_bundle(_addr);

    check_mutation_reset(bundle, bundle.in_mapping_apt[control], src);

    bundle.in_mapping_apt[control] = src;
    bundle.must_calculate_live     = true;
}

const Hid_Mapping_control_in* address_handle::get_overload(
    selection_overload_t, uint16_t dest) const {
    return get_bundle(_addr).mutations[dest].src_selection;
}

const Hid_Mapping_control_in* address_handle::get_overload(
    speed_overload_t, uint16_t dest) const {
    return get_bundle(_addr).mutations[dest].src_speed;
}

const Hid_Mapping_control_in* address_handle::get_overload(
    enable_overload_t, uint16_t dest) const {
    return get_bundle(_addr).mutations[dest].src_enabled;
}

void address_handle::set_overload(selection_overload_t, uint16_t dest,
                                  uint16_t src) {
    ::set_overload(_addr, dest, &get_bundle(_addr).in_mapping_apt[src],
                   &in_mutations::src_selection);
}
void address_handle::set_overload(speed_overload_t, uint16_t dest,
                                  uint16_t src) {
    ::set_overload(_addr, dest, &get_bundle(_addr).in_mapping_apt[src],
                   &in_mutations::src_speed);
}
void address_handle::set_overload(enable_overload_t, uint16_t dest,
                                  uint16_t src) {
    ::set_overload(_addr, dest, &get_bundle(_addr).in_mapping_apt[src],
                   &in_mutations::src_enabled);
}

void address_handle::clear_overloads() {
    for (std::size_t i = 0; i < get_bundle(_addr).mutations.size(); ++i) {
        clear_overloads(i);
    }
}

void address_handle::clear_overloads(uint16_t dest) {
    auto& bundle               = get_bundle(_addr);
    auto& mutations            = bundle.mutations[dest];
    mutations.src_enabled      = nullptr;
    mutations.src_speed        = nullptr;
    mutations.src_selection    = nullptr;
    bundle.must_calculate_live = true;
}

void address_handle::clear_overload(selection_overload_t, uint16_t dest) {
    ::set_overload(_addr, dest, nullptr, &in_mutations::src_selection);
}
void address_handle::clear_overload(speed_overload_t, uint16_t dest) {
    ::set_overload(_addr, dest, nullptr, &in_mutations::src_speed);
}
void address_handle::clear_overload(enable_overload_t, uint16_t dest) {
    ::set_overload(_addr, dest, nullptr, &in_mutations::src_enabled);
}

void address_handle::load_in_from_eeprom(drivers::spi::handle_factory& spi) {
    auto _       = lock_guard(cache_lock);
    auto& bundle = get_bundle(_addr);
    load_in_mappings_to_apt(spi, _addr);
    clear_overloads();
    bundle.must_calculate_live = true;
}
void address_handle::load_out_from_eeprom(drivers::spi::handle_factory& spi) {
    auto _       = lock_guard(cache_lock);
    auto& bundle = get_bundle(_addr);
    load_out_mappings_to_apt(spi, _addr);

    bundle.must_calculate_live = true;
}

void address_handle::store_in_to_eeprom(drivers::spi::handle_factory& spi) {
    auto _ = lock_guard(cache_lock);
    store_in_mappings_from_apt(spi, _addr);
}

void address_handle::store_out_to_eeprom(drivers::spi::handle_factory& spi) {
    auto _ = lock_guard(cache_lock);
    store_out_mappings_from_apt(spi, _addr);
}

address_handle::address_handle(uint8_t address) : _addr(address) {}

void service::hid_mapping::init() {
    if (cache_lock == nullptr) {
        // Initialize cache.
        cache_lock = xSemaphoreCreateMutex();
        configASSERT(cache_lock);
    }

    // Adopt lock to prevent OS operation before OS startup.
    drivers::spi::handle_factory spi(std::adopt_lock);

    // Loads data into RAM.
    // Note that the cache will be default-initialized to be non-dirty,
    // so any recache operation will not do anything.
    for (std::size_t i = 1; i < USB_NUMDEVICES; ++i) {
        auto& bundle               = get_bundle(i);
        bundle.lock                = xSemaphoreCreateMutex();
        bundle.must_calculate_live = true;
        load_in_mappings_to_apt(spi, i);
        load_out_mappings_to_apt(spi, i);
    }
}

bool service::hid_mapping::is_address_valid(uint8_t address) {
    return address > 0 && address <= USB_NUMDEVICES;
}
bool service::hid_mapping::is_control_valid(uint8_t address,
                                            uint16_t control_number) {
    return is_address_valid(address) && control_number < MAX_NUM_CONTROLS;
}

extern "C" void hid_mapping_service_init(void) { service::hid_mapping::init(); }

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
template <typename T, typename FSerialize, std::size_t TSize,
          std::size_t Extent>
static void load_mappings(drivers::spi::handle_factory& spi,
                          drivers::eeprom::stream_descriptor& descriptor,
                          std::span<std::byte, Extent> buffer,
                          std::array<T, TSize>& ram, std::array<T, TSize>& live,
                          FSerialize&& deserialize) {
    drivers::eeprom::cache_backend_eeprom_stream stream(spi, descriptor,
                                                        eeprom_cache);
// Check the data type byte.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    int data_type = std::to_integer<int>(*stream.read());
#pragma GCC diagnostic pop
    // configASSERT(data_type == USB_HID_DATA_TYPE || data_type == 0xFF);

    for (std::size_t i = 0; i < TSize; ++i) {
        // Read in the dataa.
        std::size_t read = stream.read(buffer);
        configASSERT(read == buffer.size_bytes());
        const void* const rt =
            deserialize(&ram[i], buffer.data(), buffer.size());
        configASSERT(rt != buffer.data());

        // Update the live mappings.
        live[i] = ram[i];
    }
}

template <typename T, typename FSerialize, std::size_t TSize,
          std::size_t Extent>
static void store_mappings(drivers::spi::handle_factory& spi,
                           drivers::eeprom::stream_descriptor& descriptor,
                           std::span<std::byte, Extent> buffer,
                           std::array<T, TSize>& ram, FSerialize&& serialize) {
    drivers::eeprom::cache_backend_eeprom_stream stream(spi, descriptor,
                                                        eeprom_cache);

    // Write data type.
    stream.write((std::byte)USB_HID_DATA_TYPE);

    // Write all the mappings.
    for (T& mapping : ram) {
        const void* rt = serialize(&mapping, buffer.data(), buffer.size());
        configASSERT(rt != buffer.data());
        stream.write(buffer);
    }
}

static void load_in_mappings_to_apt(drivers::spi::handle_factory& spi,
                                    uint8_t address) {
    auto& bundle    = get_bundle(address);
    auto descriptor = *drivers::eeprom::stream_descriptor::from_address_range(
        drivers::eeprom::regions::hid_in(address - 1));
    std::array<std::byte, 16> mapping_buffer;

    load_mappings(spi, descriptor, std::span(mapping_buffer),
                  bundle.in_mapping_apt, bundle.in_mapping_live,
                  hid_mapping_control_in_deserialize_eeprom);
}

static void load_out_mappings_to_apt(drivers::spi::handle_factory& spi,
                                     uint8_t address) {
    auto& bundle    = get_bundle(address);
    auto descriptor = *drivers::eeprom::stream_descriptor::from_address_range(
        drivers::eeprom::regions::hid_out(address - 1));
    std::array<std::byte, 13> mapping_buffer;

    load_mappings(spi, descriptor, std::span(mapping_buffer),
                  bundle.out_mapping_apt, bundle.out_mapping_live,
                  hid_mapping_control_out_deserialize);
}

static void store_in_mappings_from_apt(drivers::spi::handle_factory& spi,
                                       uint8_t address) {
    auto& bundle    = get_bundle(address);
    auto descriptor = *drivers::eeprom::stream_descriptor::from_address_range(
        drivers::eeprom::regions::hid_in(address - 1));
    std::array<std::byte, 16> mapping_buffer;

    store_mappings(spi, descriptor, std::span(mapping_buffer),
                   bundle.in_mapping_apt,
                   hid_mapping_control_in_serialize_eeprom);
}

static void store_out_mappings_from_apt(drivers::spi::handle_factory& spi,
                                        uint8_t address) {
    auto& bundle    = get_bundle(address);
    auto descriptor = *drivers::eeprom::stream_descriptor::from_address_range(
        drivers::eeprom::regions::hid_out(address - 1));
    std::array<std::byte, 13> mapping_buffer;

    store_mappings(spi, descriptor, std::span(mapping_buffer),
                   bundle.out_mapping_apt, hid_mapping_control_out_serialize);
}

static void set_overload(int8_t addr, uint16_t dest,
                         Hid_Mapping_control_in* const value,
                         Hid_Mapping_control_in* in_mutations::*member) {
    auto& bundle               = get_bundle(addr);
    auto& mutations            = bundle.mutations[dest];
    bundle.must_calculate_live = mutations.*member != value;
    mutations.*member          = value;
}

static void check_mutation_reset(address_bundle& bundle,
                                 Hid_Mapping_control_in& dest,
                                 const Hid_Mapping_control_in& src) {
    if (!control_mode_is_mutating(dest.mode) ||
        (dest.mode == src.mode &&
         dest.modify_control_port == src.modify_control_port &&
         dest.modify_control_ctl_num == src.modify_control_ctl_num &&
         !src.control_disable)) {
        return;
    }

    // Clear any mutations referencing this
    for (auto& mutation : bundle.mutations) {
        if (mutation.src_enabled == &dest) {
            mutation.src_enabled = nullptr;
        }
        if (mutation.src_selection == &dest) {
            mutation.src_selection = nullptr;
        }
        if (mutation.src_speed == &dest) {
            mutation.src_speed = nullptr;
        }
    }
    // bundle.must_calculate_live = true;
    // This is handled in calling code.
}
