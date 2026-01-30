MCM-Series Controllers
======================
This repository contains firmware and its tightly-coupled projects
    for the MCM-series of microscope controllers (i.e. MCM6000, MCM301).

Projects
--------
| Project                                           | Description                                                    |
| :------------------------------------------------ | :------------------------------------------------------------- |
| cpld                                              | Firmware for the LATTICE LCMX02-7000HC.                        |
| Firmware                                          | Firmware for the Atmel SAMS70N21.                              |
| Firmware\_Testbench                               | Limited unit tests for the Firmware project.                   |
| [look-up-tables](./proj/look-up-tables/README.md) | Look-up tables (LUTs) data, tooling, and packaging.            |
| [LUT\_Builder](./proj/LUT_Builder/)               | XML compiler for LUT XMLs and one-wire XMLs into binary files. |
| MCM6000\_UI                                       | .NETFramework Desktop GUI for MCM-controller configuration.    |
| [packaging](./proj/packaging/README.md)           | .mcmpkg file tooling and packaging.                            |
| stage-test-jig                                    | Automatic test jig for select stages.                          |

Certain projects depend on build targets from other projects.
See the table below for dependencies.

| Project        | Depends on...  | Build Command/Instructions             |
| :------------- | :------------- | :------------------------------------- |
| look-up-tables | LUT\_Builder   | `make all`                             |
| MCM6000\_UI    | look-up-tables | `make onewire`                         |
| packaging      | Firmware       | `make <pkg-name>`                      |
| packaging      | cpld           | Build .JED output with Lattice Diamond |
| packaging      | look-up-tables | `make <pkg-name>`                      |

Packaging
---------
MCM-series controllers have multiple firmware and logic files that
    are shipped in a *firmware package*.
The [**packaging**](./proj/packaging/) project provides the files
    defining packages.
See the [README](./proj/packaging/README.md) for more details.
