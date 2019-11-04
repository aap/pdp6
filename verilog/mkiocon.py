#!/usr/bin/python3

import sys;


start = '''// AUTOGEN
module iobus_{num}_connect(
	// unused
	input wire clk,
	input wire reset,

	// Master
	input wire m_iob_poweron,
	input wire m_iob_reset,
	input wire m_datao_clear,
	input wire m_datao_set,
	input wire m_cono_clear,
	input wire m_cono_set,
	input wire m_iob_fm_datai,
	input wire m_iob_fm_status,
	input wire m_rdi_pulse,
	input wire [3:9] m_ios,
	input wire [0:35] m_iob_write,
	output wire [1:7] m_pi_req,
	output wire [0:35] m_iob_read,
	output wire m_dr_split,
	output wire m_rdi_data'''

slv = ''',

	// Slave {i}
	output wire s{i}_iob_poweron,
	output wire s{i}_iob_reset,
	output wire s{i}_datao_clear,
	output wire s{i}_datao_set,
	output wire s{i}_cono_clear,
	output wire s{i}_cono_set,
	output wire s{i}_iob_fm_datai,
	output wire s{i}_iob_fm_status,
	output wire s{i}_rdi_pulse,
	output wire [3:9] s{i}_ios,
	output wire [0:35] s{i}_iob_write,
	input wire [1:7] s{i}_pi_req,
	input wire [0:35] s{i}_iob_read,
	input wire s{i}_dr_split,
	input wire s{i}_rdi_data'''

mas = '''
);
	assign m_pi_req = {pireq};
	assign m_iob_read = {read};
	assign m_dr_split = {split};
	assign m_rdi_data = {rdidata};
'''

sas = '''
	assign s{i}_iob_poweron = m_iob_poweron;
	assign s{i}_iob_reset = m_iob_reset;
	assign s{i}_datao_clear = m_datao_clear;
	assign s{i}_datao_set = m_datao_set;
	assign s{i}_cono_clear = m_cono_clear;
	assign s{i}_cono_set = m_cono_set;
	assign s{i}_iob_fm_datai = m_iob_fm_datai;
	assign s{i}_iob_fm_status = m_iob_fm_status;
	assign s{i}_rdi_pulse = m_rdi_pulse;
	assign s{i}_ios = m_ios;
	assign s{i}_iob_write = m_iob_write;'''

n = int(sys.argv[1])

vf = open("iobus_%d_connect.v" % n, "w+")
tf = open("iobus_%d_connect_hw.tcl" % n, "w+")

sys.stdout = vf

print(start.format(num=n), end='')
for i in range(n):
	print(slv.format(i=i), end='')

pireq = ' | '.join(['0'] + ["s%d_pi_req" % i for i in range(n)])
read = ' | '.join(['m_iob_write'] + ["s%d_iob_read" % i for i in range(n)])
split = ' | '.join(['0'] + ["s%d_dr_split" % i for i in range(n)])
rdidata = ' | '.join(['0'] + ["s%d_rdi_data" % i for i in range(n)])

print(mas.format(pireq=pireq, read=read, split=split, rdidata=rdidata))

for i in range(n):
	print(sas.format(i=i))

print('endmodule')




tclhead='''package require -exact qsys 16.1


# 
# module iobus_{n}_connect
# 
set_module_property DESCRIPTION ""
set_module_property NAME iobus_{n}_connect
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property AUTHOR ""
set_module_property DISPLAY_NAME iobus_{n}_connect
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property REPORT_TO_TALKBACK false
set_module_property ALLOW_GREYBOX_GENERATION false
set_module_property REPORT_HIERARCHY false

# 
# file sets
# 
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL iobus_{n}_connect
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
set_fileset_property QUARTUS_SYNTH ENABLE_FILE_OVERWRITE_MODE false
add_fileset_file iobus_{n}_connect.v VERILOG PATH rtl/iobus_{n}_connect.v TOP_LEVEL_FILE

# 
# connection point clock
# 
add_interface clock clock end
set_interface_property clock clockRate 0
set_interface_property clock ENABLED true
set_interface_property clock EXPORT_OF ""
set_interface_property clock PORT_NAME_MAP ""
set_interface_property clock CMSIS_SVD_VARIABLES ""
set_interface_property clock SVD_ADDRESS_GROUP ""

add_interface_port clock clk clk Input 1


# 
# connection point reset
# 
add_interface reset reset end
set_interface_property reset associatedClock clock
set_interface_property reset synchronousEdges DEASSERT
set_interface_property reset ENABLED true
set_interface_property reset EXPORT_OF ""
set_interface_property reset PORT_NAME_MAP ""
set_interface_property reset CMSIS_SVD_VARIABLES ""
set_interface_property reset SVD_ADDRESS_GROUP ""

add_interface_port reset reset reset Input 1

'''

tclslv='''# 
# connection point iobus_slave{i}
# 
add_interface iobus_slave{i} conduit end
set_interface_property iobus_slave{i} associatedClock clock
set_interface_property iobus_slave{i} associatedReset reset
set_interface_property iobus_slave{i} ENABLED true
set_interface_property iobus_slave{i} EXPORT_OF ""
set_interface_property iobus_slave{i} PORT_NAME_MAP ""
set_interface_property iobus_slave{i} CMSIS_SVD_VARIABLES ""
set_interface_property iobus_slave{i} SVD_ADDRESS_GROUP ""

add_interface_port iobus_slave{i} s{i}_iob_poweron iob_poweron Output 1
add_interface_port iobus_slave{i} s{i}_iob_reset iob_reset Output 1
add_interface_port iobus_slave{i} s{i}_datao_clear datao_clear Output 1
add_interface_port iobus_slave{i} s{i}_datao_set datao_set Output 1
add_interface_port iobus_slave{i} s{i}_cono_clear cono_clear Output 1
add_interface_port iobus_slave{i} s{i}_cono_set cono_set Output 1
add_interface_port iobus_slave{i} s{i}_iob_fm_datai iob_fm_datai Output 1
add_interface_port iobus_slave{i} s{i}_iob_fm_status iob_fm_status Output 1
add_interface_port iobus_slave{i} s{i}_rdi_pulse rdi_pulse Output 1
add_interface_port iobus_slave{i} s{i}_ios ios Output 7
add_interface_port iobus_slave{i} s{i}_iob_write iob_write Output 36
add_interface_port iobus_slave{i} s{i}_pi_req pi_req Input 7
add_interface_port iobus_slave{i} s{i}_iob_read iob_read Input 36
add_interface_port iobus_slave{i} s{i}_dr_split dr_split Input 1
add_interface_port iobus_slave{i} s{i}_rdi_data rdi_data Input 1

'''

tclmas='''# 
# connection point iobus_master
# 
add_interface iobus_master conduit end
set_interface_property iobus_master associatedClock clock
set_interface_property iobus_master associatedReset reset
set_interface_property iobus_master ENABLED true
set_interface_property iobus_master EXPORT_OF ""
set_interface_property iobus_master PORT_NAME_MAP ""
set_interface_property iobus_master CMSIS_SVD_VARIABLES ""
set_interface_property iobus_master SVD_ADDRESS_GROUP ""

add_interface_port iobus_master m_iob_poweron iob_poweron Input 1
add_interface_port iobus_master m_iob_reset iob_reset Input 1
add_interface_port iobus_master m_datao_clear datao_clear Input 1
add_interface_port iobus_master m_datao_set datao_set Input 1
add_interface_port iobus_master m_cono_clear cono_clear Input 1
add_interface_port iobus_master m_cono_set cono_set Input 1
add_interface_port iobus_master m_iob_fm_datai iob_fm_datai Input 1
add_interface_port iobus_master m_iob_fm_status iob_fm_status Input 1
add_interface_port iobus_master m_rdi_pulse rdi_pulse Input 1
add_interface_port iobus_master m_ios ios Input 7
add_interface_port iobus_master m_iob_write iob_write Input 36
add_interface_port iobus_master m_pi_req pi_req Output 7
add_interface_port iobus_master m_iob_read iob_read Output 36
add_interface_port iobus_master m_dr_split dr_split Output 1
add_interface_port iobus_master m_rdi_data rdi_data Output 1

'''

sys.stdout = tf
print(tclhead.format(n=n))
for i in range(n):
	print(tclslv.format(i=i))
print(tclmas)
