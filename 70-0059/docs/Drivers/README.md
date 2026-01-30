To build the `pnp-mcm-ftdi-port.inf` and `pnp-mcm-ftdibus.inf`, the `amd64` and `i386` folders need to be present with their contents in the same directory as the `.inf` file.  For the `create-cat.sh` script, use the following arguments:
```shell
create-cat.sh *.inf -Iamd64 -Ii386
```
