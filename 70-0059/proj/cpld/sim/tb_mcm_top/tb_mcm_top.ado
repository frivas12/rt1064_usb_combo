setenv SIM_WORKING_FOLDER .
set newDesign 0
if {![file exists "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim/tb_mcm_top/tb_mcm_top.adf"]} { 
	design create tb_mcm_top "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim"
  set newDesign 1
}
design open "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim/tb_mcm_top"
cd "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim"
designverincludedir -clear
designverlibrarysim -PL -clear
designverlibrarysim -L -clear
designverlibrarysim -PL pmi_work
designverlibrarysim ovi_machxo2
designverdefinemacro -clear
if {$newDesign == 0} { 
  removefile -Y -D *
}
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/intrpt_ctrl.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/quad_decoder.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/pwm_controller.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/cs_decoder.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/set_pins.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/spi_ctrl.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/wb_ctrl.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/ip/spi_slave_efb.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/spi_slave_top.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/mcm_top.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim/tb_mcm_top.v"
addfile "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/commands.v"
vlib "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim/tb_mcm_top/work"
set worklib work
adel -all
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/intrpt_ctrl.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/quad_decoder.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/pwm_controller.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/cs_decoder.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/set_pins.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/spi_ctrl.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/wb_ctrl.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/ip/spi_slave_efb.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/spi_slave_top.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/mcm_top.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sim/tb_mcm_top.v"
vlog -dbg -work work "C:/Users/frivas/Documents/Panchy/git/70-0059/mcm_cpld_firmware_zs/sources/commands.v"
module tb_mcm_top
vsim  +access +r tb_mcm_top   -PL pmi_work -L ovi_machxo2
add wave *
run 1000ns
