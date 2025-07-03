##
## IP details
##
set_module_property DESCRIPTION "MEM Interface (Avalon-MM slave port)"
set_module_property NAME mem_interface
set_module_property VERSION 1.0
set_module_property GROUP "Templates"
set_module_property AUTHOR Belal
set_module_property DISPLAY_NAME "mem_interface"
set_module_property TOP_LEVEL_HDL_FILE mem_interface_top.v
set_module_property TOP_LEVEL_HDL_MODULE mem_interface_top
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property SIMULATION_MODEL_IN_VERILOG false
set_module_property SIMULATION_MODEL_IN_VHDL false
set_module_property SIMULATION_MODEL_HAS_TULIPS false
set_module_property SIMULATION_MODEL_IS_OBFUSCATED false

##
## Link to elaboration/validation
##
set_module_property ELABORATION_CALLBACK elaborate_me
set_module_property VALIDATION_CALLBACK validate_me

##
## Files
##
add_file mem_interface_top.v {SYNTHESIS SIMULATION}
add_file mem_interface.v {SYNTHESIS SIMULATION}

##
## IP Parameters
##
add_parameter DATA_WIDTH int 32 "Data width for Avalon-MM and registers"
set_parameter_property DATA_WIDTH DISPLAY_NAME "Word Size"
set_parameter_property DATA_WIDTH GROUP "Register Properties"
set_parameter_property DATA_WIDTH AFFECTS_PORT_WIDTHS true
set_parameter_property DATA_WIDTH ALLOWED_RANGES {8 16 32}

##
## Interfaces
##

# Clock/reset
add_interface clock_reset clock end
set_interface_property clock_reset ptfSchematicName ""

add_interface_port clock_reset clk   clk   Input 1
add_interface_port clock_reset reset reset Input 1

# Avalon-MM slave
add_interface s0 avalon end
set_interface_property s0 holdTime 0
set_interface_property s0 linewrapBursts false
set_interface_property s0 minimumUninterruptedRunLength 1
set_interface_property s0 bridgesToMaster ""
set_interface_property s0 isMemoryDevice false
set_interface_property s0 burstOnBurstBoundariesOnly false
set_interface_property s0 addressSpan 512
set_interface_property s0 timingUnits Cycles
set_interface_property s0 setupTime 0
set_interface_property s0 writeWaitTime 0
set_interface_property s0 isNonVolatileStorage false
set_interface_property s0 addressAlignment DYNAMIC
set_interface_property s0 maximumPendingReadTransactions 0
set_interface_property s0 readWaitTime 0
set_interface_property s0 readLatency 3
set_interface_property s0 printableDevice false
set_interface_property s0 ASSOCIATED_CLOCK clock_reset

add_interface_port s0 slave_address address Input 9
add_interface_port s0 slave_read    read    Input 1
add_interface_port s0 slave_write   write   Input 1
add_interface_port s0 slave_readdata  readdata Output -1
add_interface_port s0 slave_writedata writedata Input -1



# User I/O
add_interface user_interface conduit end
add_interface_port user_interface pwm_vals      export Output -1
add_interface_port user_interface reset_control export Output -1
add_interface_port user_interface encoder_vals  export Input  -1

##
## Elaboration/Validation
##

proc validate_me {} {
}

proc elaborate_me {} {
  set dw [get_parameter_value DATA_WIDTH]

  set_port_property slave_readdata  WIDTH $dw
  set_port_property slave_writedata WIDTH $dw
  set_port_property pwm_vals        WIDTH $dw
  set_port_property encoder_vals    WIDTH $dw
  set_port_property reset_control   WIDTH $dw
  

  if { $dw != 8 } {
    add_interface_port s0 slave_byteenable byteenable Input [expr {$dw / 8} ]
  }
}
