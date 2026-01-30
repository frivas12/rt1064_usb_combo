set PROJ_DIR "C:\Users\zshi\dev\mcm\mcm_cpld_firmware\mcm_top_02"

cd $PROJ_DIR\sim

if {![file exists work]} {
    vlib work 
}
endif

design create work .
design open work
adel -all

cd $PROJ_DIR\


vlog ../mcm_top_02/sources/commands.v
vlog ../mcm_top_02/sources/spi_slave_top.v
vlog ../mcm_top_02/sources/wb_ctrl.v
vlog ../mcm_top_02/sources/ip/spi_slave_efb.v
                                                   
vlog ../mcm_top_02/sim/tb_mcm_top.v

vsim +access +r tb_mcm_top -L ovi_machxo2

add wave dut_spi_slave_top/*

run 1ms