#pragma once

#include <array>
#include <concepts>
#include <optional>

#include "hid_in.h"
#include "hid_out.h"
#include "spi-transfer-handle.hh"

/**
 * The HID Mapping service provides an application-level
 * interface for interacting with the HID mappings of the
 * controller.
 *
 * Mappings are stored in two categories--IN and OUT mappings.
 * Both mappings exist at three layers--EEPROM, APT, and LIVE.
 * Mappings in EEPROM are not directly readable or writable;
 * the APT layer must either store itself into EEPROM or load
 * from EEPROM.
 *
 * The APT layer provides the template values for the
 * LIVE layer.
 * The APT layer is loaded from the EEPROM layer at service
 * startup.
 * Commands to configure the mappings should target the APT layer.
 * Write commands to the APT layer will propagate into the LIVE layer.
 *
 * The LIVE layer is generated from the APT layer as well as overloads
 * for IN maps or routings for OUT maps.
 * Overloads allow another mapping of the same address to overwrite
 * certain fields of the mapping.
 * Current overloads are SELECTION, SPEED, and ENABLE.
 * Only mutable fields can be overloaded.
 * The OUT layer determines if there is a control that it should follow.
 * If so, the output control's source fields will track the LIVE values
 * of its routed input field.
 * Routing is generated automatically between the first IN field that
 * has the same destination fields as the OUT map's source fields.
 * This layer is also used when generating any messages related to
 * the translation of HID IN/OUT reports and system HID messages.
 */
namespace service::hid_mapping {

/**
 * RAII handle that ensures mutual exclusion
 * to the mappings of a given address.
 */
class address_handle {
   public:
    /// \brief Overload affecting the destination_ fields.
    struct selection_overload_t {};

    /// \brief Overload affecting the speed and direction fields.
    struct speed_overload_t {};

    /**
     * Overload affecting the control_disabled field.
     * Unlike the other overloads that take in data, this
     * overload will disable the control while set.
     * It still takes a pointer to its source to observe
     * changes.
     */
    struct enable_overload_t {};
    static constexpr selection_overload_t selection_overload = {};
    static constexpr speed_overload_t speed_overload         = {};
    static constexpr enable_overload_t enable_overload       = {};

    address_handle(const address_handle&) = delete;
    address_handle(address_handle&&);
    address_handle& operator=(address_handle&&);
    ~address_handle();

    constexpr uint8_t address() const { return _addr; }

    /// \brief Blocks until the address handle is obtained.
    /// \pre is_address_valid(address)
    static address_handle create(uint8_t address);

    // \brief Attempts to obtain the address handle within
    //        the given timeout.
    // \return Empty if the timeout occurs.
    static std::optional<address_handle> try_create(uint8_t address,
                                                    TickType_t timeout);

    std::size_t out_map_size() const noexcept;
    std::size_t in_map_size() const noexcept;

    /// A reference to the OUT map.
    /// \pre 0 <= control_idx <= MAX_NUM_CONTROLS
    const Hid_Mapping_control_out& out_map(uint8_t control_idx) const noexcept;

    /// A reference to the IN map.
    /// \pre 0 <= control_idx <= MAX_NUM_CONTROLS
    const Hid_Mapping_control_in& in_map(uint8_t control_idx) const noexcept;

    /**
     * Reads the OUT mapping for the given address and control.
     * \param[in]  control The control number to access.
     * \pre 0 <= control < MAX_NUM_CONTROLS
     */
    const Hid_Mapping_control_in& get_apt_in(uint16_t control) const;

    /**
     * Reads the IN mapping for the given address and control.
     * \param[in]  control The control number to access.
     * \pre 0 <= control < MAX_NUM_CONTROLS
     */
    const Hid_Mapping_control_out& get_apt_out(uint16_t control) const;

    /**
     * Writes the OUT mapping for the given address and control.
     * This will also update the mapping array.
     * \param[in]  src     The mapping to write to RAM.
     * \param[in]  control The control number to access.
     * \pre 0 <= control < MAX_NUM_CONTROLS
     */
    void set_apt(const Hid_Mapping_control_out& src, uint16_t control);
    void set_apt(const Hid_Mapping_control_in& src, uint16_t control);

    const Hid_Mapping_control_in* get_overload(selection_overload_t,
                                               uint16_t dest) const;
    const Hid_Mapping_control_in* get_overload(speed_overload_t,
                                               uint16_t dest) const;
    const Hid_Mapping_control_in* get_overload(enable_overload_t,
                                               uint16_t dest) const;

    void set_overload(selection_overload_t, uint16_t dest, uint16_t src);
    void set_overload(speed_overload_t, uint16_t dest, uint16_t src);
    void set_overload(enable_overload_t, uint16_t dest, uint16_t src);

    void clear_overloads();
    void clear_overloads(uint16_t dest);
    void clear_overload(selection_overload_t, uint16_t dest);
    void clear_overload(speed_overload_t, uint16_t dest);
    void clear_overload(enable_overload_t, uint16_t dest);

    /// \brief Loads all the IN mappings from EEPROM into RAM.
    void load_in_from_eeprom(drivers::spi::handle_factory& spi);

    /// \brief Loads all the OUT mappings from EEPROM into RAM.
    void load_out_from_eeprom(drivers::spi::handle_factory& spi);

    /// \brief Saves all the IN mappings from RAM into EEPROM.
    /// \note Does not change the mapping arrays.
    void store_in_to_eeprom(drivers::spi::handle_factory& spi);

    /// \brief Saves all the OUT mappings from RAM into EEPROM.
    /// \note Does not change the mapping arrays.
    void store_out_to_eeprom(drivers::spi::handle_factory& spi);

   private:
    uint8_t _addr;
    bool _owned = true;

    address_handle(uint8_t addr);
};

/// \brief Initializes the service.
void init();

bool is_address_valid(uint8_t address);
bool is_control_valid(uint8_t address, uint16_t control_number);

}  // namespace service::hid_mapping
