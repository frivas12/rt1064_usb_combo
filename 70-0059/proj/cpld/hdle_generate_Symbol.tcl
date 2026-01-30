lappend auto_path "C:/lscc/diamond/3.10_x64/data/script"
package require symbol_generation

set ::bali::Para(MODNAME) quad_decoder
set ::bali::Para(PACKAGE) {"C:/lscc/diamond/3.10_x64/cae_library/vhdl_packages/vdbs"}
set ::bali::Para(PRIMITIVEFILE) {"C:/lscc/diamond/3.10_x64/cae_library/synthesis/verilog/machxo2.v"}
set ::bali::Para(FILELIST) {"C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/spi_slave_top.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/wb_ctrl.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/ip/spi_slave_efb.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/commands.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/spi_ctrl.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/mcm_top.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/quad_decoder.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/pwm_controller.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/cs_decoder.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/intrpt_ctrl.v=work" "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/set_pins.v=work" }
set ::bali::Para(INCLUDEPATH) {}
puts "set parameters done"
::bali::GenerateSymbol
