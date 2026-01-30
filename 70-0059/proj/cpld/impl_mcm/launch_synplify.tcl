#-- Lattice Semiconductor Corporation Ltd.
#-- Synplify OEM project file C:/Users/sbenish/repositories/70-0059/cpld/impl_mcm/launch_synplify.tcl
#-- Written on Thu Feb 17 12:28:45 2022

project -close
set filename "C:/Users/sbenish/repositories/70-0059/cpld/impl_mcm/impl_mcm_syn.prj"
if ([file exists "$filename"]) {
	project -load "$filename"
	project_file -remove *
} else {
	project -new "$filename"
}
set create_new 0

#device options
set_option -technology MACHXO2
set_option -part LCMXO2_7000HC
set_option -package TG144C
set_option -speed_grade -4

if {$create_new == 1} {
#-- add synthesis options
	set_option -symbolic_fsm_compiler true
	set_option -resource_sharing true
	set_option -vlog_std v2001
	set_option -frequency 100
	set_option -maxfan 1000
	set_option -auto_constrain_io 0
	set_option -disable_io_insertion false
	set_option -retiming false; set_option -pipe true
	set_option -force_gsr false
	set_option -compiler_compatible 0
	set_option -dup false
	
	set_option -default_enum_encoding default
	
	
	
	set_option -write_apr_constraint 1
	set_option -fix_gated_and_generated_clocks 1
	set_option -update_models_cp 0
	set_option -resolve_multiple_driver 0
	
	
	set_option -seqshift_no_replicate 0
	
}
#-- add_file options
set_option -include_path "C:/Users/sbenish/repositories/70-0059/cpld"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/mcm_top.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/spi_slave_top.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/wb_ctrl.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/commands.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/spi_ctrl.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/status_led.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/quad_decoder.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/pwm_controller.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/cs_decoder.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/intrpt_ctrl.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/otm_dac.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/uart_controller.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/rs232.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/config_mcm/config.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/slot_cards/servo.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/slot_cards/shutter_4.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/slot_cards/slider_io.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/slot_cards/stepper.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/slot_cards/peizo_elliptec.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/config_mcm/ip/spi_slave_efb.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/config_mcm/ip/pll.v"
add_file -verilog -vlog_std v2001 "C:/s_links/sources/slot_cards/piezo.v"
#-- top module name
set_option -top_module {}
project -result_file {C:/Users/sbenish/repositories/70-0059/cpld/impl_mcm/impl_mcm.edi}
project -save "$filename"
