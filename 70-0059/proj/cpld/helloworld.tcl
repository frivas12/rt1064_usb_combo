prj_project open "mcm_top.ldf"
prj_run Export -impl impl_mcm -task Jedecgen -forceAll
prj_project close

prj_project open "hex_top.ldf"
prj_run Export -impl impl_hex -task Jedecgen -forceAll
prj_project close