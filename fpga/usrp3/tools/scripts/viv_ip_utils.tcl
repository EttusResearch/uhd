#
# Copyright 2015 Ettus Research
#

if [expr $argc < 2] {
    error "ERROR: Invalid number of arguments"
    exit
}

set cmd       [lindex $argv 0]
set part_name [lindex $argv 1]

# Only create an in-memory roject when not using bdtcl commands.
if [expr [string first "_bdtcl" $cmd] == -1] {
    create_project -in_memory -ip -name inmem_ip_proj -part $part_name
# Otherwise, set the system's TMP directory.
} else {
    set sys_tmpdir [pwd]
    if {[file exists "/tmp"]} {set sys_tmpdir "/tmp"}
    catch {set sys_tmpdir $::env(TRASH_FOLDER)} ;# very old Macintosh. Mac OS X doesn't have this.
    catch {set sys_tmpdir $::env(TMP)}
    catch {set sys_tmpdir $::env(TEMP)}
}

if { [string compare $cmd "create"] == 0 } {
    if [expr $argc < 5] {
        error "ERROR: Invalid number of arguments for the create operation"
        exit
    }
    set ip_name [lindex $argv 2]
    set ip_dir  [lindex $argv 3]
    set ip_vlnv [lindex $argv 4]
    create_ip -vlnv $ip_vlnv -module_name $ip_name -dir $ip_dir

} elseif { [string compare $cmd "modify"] == 0 } {
    if [expr $argc < 3] {
        error "ERROR: Invalid number of arguments for the modify operation"
        exit
    }

    set src_file [lindex $argv 2]
    set src_ext [file extension $src_file ]
    if [expr [lsearch {.xci} $src_ext] >= 0] {
        read_ip $src_file
    } elseif [expr [lsearch {.bd} $src_ext] >= 0] {
        add_files -norecurse $src_file
        export_ip_user_files -of_objects  [get_files $src_file] -force -quiet
        open_bd_design $src_file
    } else {
        puts "ERROR: Invalid file extension: $src_ext"
    }

} elseif { [string compare $cmd "list"] == 0 } {
    puts "Supported IP for device ${part_name}:"
    foreach ip [lsort [get_ipdefs]] {
        puts "- $ip"
    }

} elseif { [string compare $cmd "upgrade"] == 0 } {
    if [expr $argc < 3] {
        error "ERROR: Invalid number of arguments for the upgrade operation"
        exit
    }
    set src_file [lindex $argv 2]
    read_ip $src_file
    upgrade_ip [get_ips *]

} elseif { [string compare $cmd "modify_bdtcl"] == 0 } {
    if [expr $argc < 4] {
        error "ERROR: Invalid number of arguments for the modify operation"
        exit
    }

    set src_file [lindex $argv 2]
    set src_rootname [file rootname [file tail $src_file]]
    set src_ext [file extension $src_file ]
    set ip_repos [lindex $argv 3]
    set hdl_sources "[file dirname $src_file]/hdl_sources.tcl"
    if [expr [lsearch {.tcl} $src_ext] >= 0] {
        # Create a temporary project to work on.
        set tmp_bddir "${sys_tmpdir}/.viv_${src_rootname}"
        file mkdir $tmp_bddir
        cd $tmp_bddir
        # Create temporary project to store user changes.
        create_project tmp_bd $tmp_bddir -part $part_name -force
        set_property ip_repo_paths "{$ip_repos}" [current_project]
        update_ip_catalog
        # Add any supporting HDL first
        if {[file exists $hdl_sources] == 1} {
          source $hdl_sources
        } else {
          puts "hdl_sources.tcl not found in IP directory. Skipping HDL import for BD design"
        }
        # Recreate BD design from source file (.tcl)
        source $src_file
        regenerate_bd_layout
        validate_bd_design
        save_bd_design
    } else {
        puts "ERROR: Invalid file extension: $src_ext"
    }

} elseif { [string compare $cmd "write_bdtcl"] == 0 } {
    if [expr $argc < 3] {
        error "ERROR: Invalid number of arguments for the create operation"
        exit
    }
    # When regenerating a TCL file from a BD design, there should be a tmp project
    # created by this tool ($cmd = modify_bdtcl).
    set src_file [lindex $argv 2]
    set src_rootname [file rootname [file tail $src_file]]
    set src_ext [file extension $src_file ]
    set src_dir [file dirname $src_file]
    # Make sure a BD or TCL files is passed
    if [expr [lsearch {.tcl} $src_ext] >= 0] {
        # Validate that a previously created BD project exists.
        set tmp_bddir "${sys_tmpdir}/.viv_${src_rootname}"
        if {[file exists "$tmp_bddir/tmp_bd.xpr"] == 1} {
            puts "INFO: Generating TCL file from BD design..."
            # Open project and BD design
            open_project "$tmp_bddir/tmp_bd.xpr"
            open_bd_design [get_files "$src_rootname.bd"]
            if [expr $argc > 3 && [string equal [lindex $argv 3] "upgrade"]] {
                puts "INFO: Upgrading IP"
                upgrade_ip [get_ips *]
                validate_bd_design
            }
            # Rewrite TCL BD file
            write_bd_tcl -make_local -force "$src_dir/$src_rootname.tcl"
            puts "INFO: BD TCL source updated: $src_dir/$src_rootname.tcl"
            # Close and delete tmp_bd project, not needed anymore.
            close_project
            puts "INFO: Deleting temp Vivado BD project..."
            file delete -force -- $tmp_bddir
            exit
        } else {
            puts "ERROR: No BD temp project found in: $tmp_bddir"
            exit
        }
    } else {
        puts "ERROR: Invalid file extension: $src_ext"
        exit
    }

} else {
    error "ERROR: Invalid command: $cmd"
}