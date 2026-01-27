# Changelog
## Latest (in-`dev`)
### Changes
### Added
### Removed
### Fixed

## 7.1.1 (2025-06-13)
### Changes
- Building has been simplified to the targets `all`, `MCM301`, `MCM6101`, `MCM6201`, and `clean`.
- Values previously declared in the ["config.mk"](./config.mk) file are now in "target-config.h" files and ["version.h"](./src/version.h).
- Moved versioning to its own header file as this was a large leader in recompilation.
### Fixes
- `MGMSG_MCM_GET_STAGEPARAMS` had its return length corrected to remove invalid bytes at the tail.

## 7.1.0 (2025-06-11)
### Added
- Steppers now use "speed channels" to calculate the maximum velocity for the `cmd_vel` field.
    - Each movement type has speed.
    - The joystick and synchronous motion channels allow for independent movement speeds for whatever movement they use.
- The `MGMSG_MCM_[SET/REQ/GET]_SPEED_LIMIT` command can be used to interact with stepper speed channels.
    - Requests use a bitset to identify a channel.
    - Sets can mutate one or more channel to a value at the same time.
    - Joystick and synchronous motion speed channels are not exposed.
    - Values of 0 are not allowed--they will be set to the min value.
### Removed
- Non-PID GOTO movements for steppers.
    - They are forced to become PID controls if unset.
- Non-PID JOG movements for steppers.
    - They are forced to become PID controls if unset.

## 7.0.2 - 2025-03-21
- Release of the MCM6101-era firmware
- Start of version record keeping.
