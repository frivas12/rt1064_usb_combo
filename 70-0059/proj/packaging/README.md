MCM Packaging Pipeline
======================
This project integrates the build targets from other projects
    to create an MCMPKG file.

Requirements
------------
- GNU Make (>=4.4.1)
- NuGet package `thorlabs.mcmpack.cli` (>=1.1.2)

Building
--------
To maximize control, this project does not build any other
    build targets.
Therefore, the Firmware, CPLD, and LUT projects need to be built
    for the target package.
Refer to the other project's documentation to see how to build.

Packages are defined in the ["packaging/"](./packaging/) directory.
By default, the `make list` command will list the names of the packages
    that will be built.
The package will be output to the path "bin/\<fgpn\>.\<version\>.mcmpkg".
Assuming the firmware, CPLD, and LUT files have been built from the
    latest source, run the command below to output the package.
The package version will auto-deduce from the latest version of the
    firmware, CPLD, and LUT files.
```shell
make <FGPN>
```

It is possible to create a customized package with overwritten
    firmware, CPLD, and/or LUT files.
To overwrite the firmware, append the pattern
    `FIRMWARE_FILE=<path/to/file> FIRMWARE_VERSION=<major.minor.path>`
    to the make command.
To overwrite the CPLD, append the pattern
    `CPLD_FILE=<path/to/file> CPLD_VERSION=<major.minor.path>`
    to the make command.
To overwrite the LUT, append the pattern
    `LUT_FILE=<path/to/file> LUT_VERSION=<major.minor.path>`
    to the make command.
If a custom output path is required, append `OUTPUT=<path/to/output>`
    to the make command.

Creation
--------
When a new MCM controller shall be released, a new package
    target needs to be created.
1. Create/Get the finished-goods part number (FGPN) for the
    controller (typically on Brainwave).
2. Create/Get the USB Product ID for the controller (see
    "thorlabs\_pids.h" in the "thorlabs\_software\_lib" repository).
3. If not created, create a new build target in the firmware's
    [config.mk](../Firmware/config.mk).
    - Include the FGPN and the USB Product ID for the controller.
4. If not created, create a new LUT package.  Follow instructions in
    the LUT project's [README](../look-up-tables/README.md).
    - This step can be skipped if an existing LUT package will be reused.
5. Copy the [package template](./packaging/FGPN.mk.template) file
    to "\<FGPN\>.mk" (i.e. "MCM301.mk", "MCM6101.mk", etc.).
6. Fill in the `FGPN` variable in the ".mk" file with the FGPN.
7. Fill in the `PRODUCT_ID` variable in the ".mk" file with the USB
    Product ID as a 4-character hex (i.e. 0x21AB -> 21AB).
8. Enter the name of the LUT package in the ".mk" file's `LUT_PACKAGE`
    variable.
9. Enter the name of the schema file under ["schema/"](./schema/) in
    the `SCHEMA`) variable that best fits the controller.
    - This will typically be the latest rev.
