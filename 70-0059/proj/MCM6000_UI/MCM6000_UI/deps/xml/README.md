# XML File Structure
This document details the file structure of the XML directory.  The sections below will detail each folder and what they contain.

- [XML File Structure](#xml-file-structure)
  - [system/](#system)
    - [system/configs/](#systemconfigs)
    - [system/configs/ow-only/](#systemconfigsow-only)
    - [system/device/](#systemdevice)
    - [system/device/full/](#systemdevicefull)
    - [system/device/one-wire/](#systemdeviceone-wire)
    - [system/joysticks/](#systemjoysticks)
  - [system.archive.zip](#systemarchivezip)

## system/
All new XMLs fall under this directory.
Developers will populate XMLs in this directory, which production will then use to configure systems.

XMLs existing at this top-level are system-level XMLs.
These encapsulate all the configuration information for a controller.
Top-level XMLs indirectly reference files in the subdirectories.

### system/configs/
This directory contains all XMLs for the config LUT's structures.
Typically, file names are in the format "[STRUCT_ID]-[CONFIG_ID].xml".
Developers can add files to this directory, but production will not directly access them.

Because structure XMLs are compiled, a *bin/* folder exists as well, containing the raw binary that will be passed.
No files in the *bin/* directory are saved by GIT.

### system/configs/ow-only/
The "one-wire only" directory contains the XMLs for config entries stored on one-wire chips.
Typically, file names are in the format "[DEFAULT_SLOT_TYPE]-[DEVICE_ID]_[STRUCT_TYPE].xml".
Like its parent directory, this directory will not be accessed directly by production.

Because structure XMLs are compiled, a *bin/* folder exists as well, containing the raw binary that will be passed.
No files in the *bin/* directory are saved by GIT.

### system/device/
The device directory contains information about a given device.
Each XML in this directory represents a single one-wire device, what its signature is, and what slot cards it is compatible with.
Typically, file names are in the format "[DEFAULT_SLOT_TYPE]-[DEVICE_ID].xml".

Subdirectories for slot card IDs contain the device configuration set for the given slot card.
The XMLs in the *device/* directory reference XMLs in these subdirectories.
For example, a MCM_Stepper_LC_HD_DB15 slot card (ID 19) will define the configuration set for all devices it can run.
Typically, file names are in the format "[DIRECTORY_NAME]-[DEFAULT_SLOT_TYPE]-[DEVICE_ID].xml".

### system/device/full/
For configuring slots manually, the *full/* directory contains self-contained XMLs for configuring a stage.
XMLs in this directory are sorted into subdirectories based on the type of card (stepper, servo, piezo, etc.).
In the UI, these XMLs are accepted by the "Load XML" button for slot-card configuration tabs (stepper tab, servo tab, etc.).

Although not required, XMLs here may reference other XMLs in other directories.

### system/device/one-wire/
The "one-wire" directory contains the XMLs needed to fully program the one-wire chips on devices.
In the UI, these XMLs are accepted by the "Load XML" button on the One-Wire tab.
Typically, file names are in the format "[DEFAULT_SLOT_TYPE]-[DEVICE_ID].xml".

Because one-wire XMLs contain structure XMLs, a *bin/* directory exists to save the last compilation.
No files in the *bin/* directory are saved by GIT.

### system/joysticks/
The joysticks directory saves the IN and OUT mappings for a joystick.
File names have no formatting standard, but the type of joystick and its usage is recommended for clarity.

## system.archive.zip
The "system.archive" zip file contains all of the previous XMLs that do not follow the new XML format.
Developers can extract files from this archive for porting.
If XMLs for previous firmware versions are needed, they will also be in this archive.