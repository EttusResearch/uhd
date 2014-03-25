add_files ../rtl
set_property target_constrs_file example_top.xdc [current_fileset -constrset]

import_ip -file {ddr_icon_cg.xco} -name ddr_icon 
import_ip -file {ddr_ila_basic_cg.xco} -name ddr_ila_basic  
import_ip -file {ddr_ila_wrpath_cg.xco} -name ddr_ila_wrpath 
import_ip -file {ddr_ila_rdpath_cg.xco} -name ddr_ila_rdpath 
import_ip -file {ddr_vio_sync_async_out72_cg.xco} -name ddr_vio_sync_async_out72 
import_ip -file {ddr_vio_async_in_sync_out_cg.xco} -name ddr_vio_async_in_sync_out 

generate_target  All [get_files ddr_icon.xco -of_objects [get_filesets sources_1]]
generate_target  All [get_files ddr_ila_basic.xco -of_objects [get_filesets sources_1]]
generate_target  All [get_files ddr_ila_wrpath.xco -of_objects [get_filesets sources_1]]
generate_target  All [get_files ddr_ila_rdpath.xco -of_objects [get_filesets sources_1]]
generate_target  All [get_files ddr_vio_sync_async_out72.xco -of_objects [get_filesets sources_1]]
generate_target  All [get_files ddr_vio_async_in_sync_out.xco -of_objects [get_filesets sources_1]]

