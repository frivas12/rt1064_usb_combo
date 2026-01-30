Look-Up Table (LUT) Generator
=============================
This project provides the human-readable XML files for entries in the device and
    config look-up tables (LUTs).
This project also provides tooling for selecting the devices
    (and dependent configs) to put into a LUTs, then building them into ".zip" packages.
Furthermore, the project builds one-wire files into a binary file package
    that programs can use to program devices.


## Requirements
- Python 3.11 or later
    - PIP package requirements are in ["requirements.txt"](./requirements.txt).
- GNU Make 4.4.1 or later

In addition, a SQLite3 viewer can be helpful for editting the database
    directly instead of the .sql files under ["database/"](./database/).
[SQLiteBrowser](https://sqlitebrowser.org/) is the recommended utility.

### Scoop Packages
For Windows environments, the following [Scoop](https://scoop.sh/) packages satisfy tooling requirements.
```shell
scoop install make sqlite3 sqlitebrowser
```

## Building
Building assumes the working directory is the project directory.
The makefile provides the standard build targets `all` and `clean`.
The default build target is `all`.
Furthermore, `all` will build the database, onewire package, and all look-up
    table packages.
```shell
make [all]
make clean
```

### Database
The LUT generator utilizes an SQLite3 database to host data.
The `db` target will build the database to `build/global.db`.
Since the database is a build target, changing the files
    in the ["database/"](./database/) directory will cause the database to be
    rebuilt.

**Note**: tooling requires the database to be present for
    most operations.
```shell
make db
```

### Database Export
To export database changes from `build/global.db`, run the
    `dbexport` build target.
```shell
make dbexport
```

### All LUT Packages
The `make lut` command will build all the LUT packages in the project.
```shell
make luts
```

### Specific LUT Packages
Running `make <package-name>` will build the package described by the
    file "packaging/\<package-name\>.toml".
```shell
make <package-name>
```

### One-Wire Package
Running `make onewire` will build the onewire package to "bin/onewire.zip".
```shell
make onewire
```

## Project APIs and Utilities
The ["tools/"](./tools/) directory contains both build tools as well as
    APIs used by the build tools.
In addition, some utility tools are provided for users.
A few of these are listed below:
| File Name                                       | Type | Description                                             |
| :---------------------------------------------- | :--- | :------------------------------------------------------ |
| [dbapi.py](./tools/globalapi.py)                | lib  | Library for database reading.                           |
| [tomlapi.py](./tools/tomlapi.py)                | lib  | Library for parsing LUT package TOML files.             |
| [xmlapi.py](./tools/xmlapi.py)                  | lib  | Library for parsing XML files.                          |
| [list\_devices.py](./tools/list_devices.py)     | tool | Utility to list devices in the database.                |
| [next\_config.py](./tools/next_config.py)       | tool | Utility for creating new configs XMLs.                  |
| [rename\_onewire.py](./tools/rename_onewire.py) | tool | Utility to rename the "Part Number" section of the api. |

## Creating a LUT Package
Different products may product different LUTs to restrict support of devices.
For this example, the package will be named "mydevice".

First, copy the ["packaging/pkg-name.toml.template"](./packaging/pkg-name.toml.template) file into
    "packaging/mydevice.toml".
Next, edit the TOML file to change the `name` key to "mydevice" and the version `key` to the desired version (version 1.2.3 in this example).
Then, add a `[[devices]]` table for each device to include.
For this example, the devices "0013:0001" (slot type 19 and device id 1) and "0013:0004" will be included.
By default, all avaiable configs matching to the device's signature will be provided.
The `slot_filter` key under `[[devices]]` can be used to restrict the device to only use configs for the provided slots.
This example will further restrict "0013:0004" to slot cards with IDs 3 and 19.
```toml
name = "mydevice"
version = "1.2.3"

[[devices]]
slot_type = 19
device_id = 1

[[devices]]
slot_type = 19
device_id = 4
slot_filter = [3, 19]
```

## XML Template Format
The device and config XMLs used for creating LUT entries have template
    files to facilitate the creation of new entries.
Placeholders in the template have the following syntax:
- `{type}`
- `{recommended_value:type}`
- `{[min_value, max_value]}`

Types are expressed using Rust-like notation (i.e. `i32` = `int32_t` =
    32-bit signed integer, `f32` = 32-bit (single-precision) float,
    `u3` = 3-bit unsigned integer).
The `[min_value, max_value]` expression explicitly requires the values
    be no less than `min_value` and no greater than `max_value`.

## Adding a Part Number to the Project
When adding a new part number, first start by determining the number
    of devices in the part (called the **usage count**).
This is typically 1 but rarely a greater value is needed.
For example, the part number "30-1303" is the Imperial Motorized-Z Red Post (FGPN MPM250).
Since this part number only has one device--the red post itself--it
    has a usage count of 1.
However,the part number "000-106-677" is the Bergamo III body including the X, Y, and Z axes.
Therefore, it has a usage count of 3--one for each axis.
Each usage will be mapped--usually uniquely--to a **device signature**, the primary key of the
    "Devices" table in the database.

Next, determine if the part is an interoperable clone of another part.
For example, the part number "30-1412" is the Metric Motorized-Z Red Post (FGPN MPM250/M).
Since any controller capable of driving 30-1303 should also drive 30-1412,
    these two will have the same device signature.
**Note**: sharing a device signature **prevents** separation of the two
    parts for identification purposes.
For example, a system-level device and a catalog device **must** be split into two
    different device signatures to control what platforms support the system-level
    device and the catalog-level device.

Now, the procedure below can be used to add the part number into the project:
1. Build (`make db`) and open the database file, [build/global.db](./build/global.db).
2. Open the database and navigate to the "PartNumbers" table.
3. Create as many rows as the part number has usages.
4. Fill the created rows' "PartNumber" column with the part number to input.
5. Number the "UsageID" column of the created rows sequentially, starting at 0.
6. Set the created rows' "IsActive" column to true (1).
7. Describe each usage in the row's "UsageDescription" column.

Next, device identification needs to be performed.
If any usage will reuse a pre-existing device signature,
    fill in the "Slot Type" and "Device ID" column
    for the applicable usages.
In addition, the "Description" of the row may need to be updated
    to reflect its shared nature.
Otherwise, new device signatures shall be created for each remaining
    row.
Use the instructions below to add a new device signature.
1. Build (`make db`) and open the database file, [build/global.db](./build/global.db).
1. Navigate to the "Devices" table, and create a new row.
3. Set the "IsActive" flag to true (1) in the new row.
4. Select the default slot type for the device--the slot type
    that the device should always work with--and fill in the
    "SlotType" column.
5. Find the next index to create and insert it into the "DeviceID" column.
6. Fill in the "Description" column of the device (typically the same as the "UsageDescription").

Now that a device has been created in the database,
    the files for the device need to be created.
First, the one-wire files will be created.
These files contain the binary information encoded onto
    each cable.
For each device signature created...
1. Copy the most applicable template file in ["onewire/"](./onewire/)
    and place it into an applicable folder.
2. Assign the "DefaultSlotType" and "DeviceID" XML elements with
    the device signature.
3. Fill the placeholders in the one-wire file.
4. Stage or commit the one-wire file to Git.

**Never edit a one-wire XML that has been released!**

Finally, the config list file (a.k.a device file) will be created every
    new device signature.
Follow the instructions for [Extending Device Support](#Extending-Device-Support)
    for each row created in the "Devices" table.
Use the value in the "Slot Type" column for the "Attached Slot" column.

## Extending Device Support
**Do NOT edit config list files unless absolutely necessary.**
**If a change is required, prepend the file with an XML comment with your company e-mail, date, the changes, and the reason for the changes.**
1. Build (`make db`) and open the database file, [build/global.db](./build/global.db).
2. Navigate to the "DeviceFiles" table, and enter the device signature into "SlotType" and "DeviceID"
    columns.
3. Fill the "AttachedSlot" column with the slot type that will
    be supported.
4. In the "devices/{attached-slot}" folder, find the "template.xml" file.
5. Copy that file to the same directory and give it a meaningful name.
6. Fill the "Path" column with the relative path from the database to the
    copied file (i.e. "./device/19/PLS-X.xml").
7. Open the copied file and fill in the "DefaultSlotType" and "DeviceID"
    XML elements with the device signature.
8. Fill the placeholders in the new device file.  This may require
    creating new configs (see [Creating a New Config](#Creating-a-New-Config)).
9. Commit changes to the database and close the file.
10. Run `make dbexport` to apply the changes in the database file to the ["database/"](./database/) directory.
11. Commit the ["database/"](./database/) directory and the new "Device" file to Git.

## Creating a New Config
**Never edit a config XML that has been released!**
1. Select the struct ID to use.
2. Invoke the command below.
You can also omit `--copy` to only emit the file name.
Otherwise, the template will be copied to the output path.
```shell
python3 tools/next_config.py <struct-id> --copy
```
Possible Output:
```text
./config/<struct-id>-10.xml
```

3. Fill the placeholders in the new config.
4. Commit the new file to Git.

## Deprecation of a Part Number
To deprecate a part number, set the associated "IsActive" column to 0 in the "PartNumbers" table.
Next, invoke the command `sqlite3 build/global.db < tools/ActiveConfigsWithoutActiveParts.sql`.
If the device signature of the newly-deprecated part number appears, then no other
    part number is associated with that device signature.
Therefore, that device signature should be deprecated as well.

## Deprecation of a Device Signature
To deprecate a device signature, set the associated "IsActive" column to 0 in the "Devices" table.
Next, invoke the command `sqlite3 build/global.db < tools/PartNumbersUseActiveDeviceConfigs.sql` to check
    if the deprecated device signature was referenced by any other active part number.
If any of those part numbers should not be deprecated, the device signature should not either.

Device deprecation allows the build tools to warn when deprecated devices
    are included in a package.
This allows developers to consider removing the device from the package's inclusion list and,
    thus, keeping the limited memory in the controllers to a minimum.
