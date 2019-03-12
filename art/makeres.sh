#!/bin/sh

files='op_panel ind_panel1 ind_panel2 io_panel extra_panel\
key_n key_d key_u lamp_on lamp_off switch_d switch_u'

for i in $files; do
#	echo static u8 $i[] = {
	xxd -i ${i}.png
#	echo '};'
#	echo
done > panelart.inc
