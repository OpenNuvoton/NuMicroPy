import os
import sys
import re
import argparse
import subprocess

special_ip = ['EBI', 'CCAP', 'KPI', 'GPIO', 'SWDH', 'USB', 'HSUSB', 'ICE']
dmic_ip = ['DMIC0', 'DMIC1']

def refine_ip_pin(ip, pin):
	for s_ip in special_ip:
		if s_ip == ip:
			ip = ip + '0'

	for s_ip in dmic_ip:
		if s_ip == ip:
			if ip == 'DMIC0':
				pin = 'CH0_' + pin
			else:
				pin = 'CH1_' + pin			
			ip = 'DMIC0'

	if pin == None:
		pin = ip.rstrip('0123456789')
	
	return ip + '_' + pin


def alternative_pin_parser(input_file, pin_name):
	#print('do alternative_pin_parser')
	cleaned_str = pin_name.rstrip()
	cleaned_str = cleaned_str.strip("/* ").strip(" */").rstrip(" MFP")

	# 分割字串
	parts = cleaned_str.split(".")
	port_num = parts[0].lstrip('P')
	pin_num = parts[1].strip() if len(parts) > 1 else ""

	port_str = 'Port' + port_num
	port_pin_str = 'P' + port_num + pin_num

	#format to SYS_GPA_MFP0_PA0MFP_ style
	search_str = 'SYS_GP' +  port_num

	if int(pin_num) < 4:
		mfp_str = 'MFP0'
	elif int(pin_num) < 8:
		mfp_str = 'MFP1'
	elif int(pin_num) < 12:
		mfp_str = 'MFP2'
	elif int(pin_num) < 16:
		mfp_str = 'MFP3'

	search_str = search_str + '_' + mfp_str
	search_str = search_str + '_' + port_pin_str + 'MFP_'
	#print(search_str)

	alternative_pins = []

	alternative_pins.append(port_str)
	alternative_pins.append(port_pin_str)
	alternative_pins.append(mfp_str)


	#SYS.h's MFP marco would be "#define SYS_GPJ_MFP2_PJ10MFP_EBI_MCLK         (0x2UL<<SYS_GPJ_MFP2_PJ10MFP_Pos)   /*!< GPJ_MFP2 PJ10 setting for EBI_MCLK          */" style

	while input_file:
		new_line = input_file.readline()
		# Splitting the string by whitespace
		segments = new_line.split()

		if len(segments) < 2:
			break

		#Filter out "SYS_GPx_MFPx_PxxMFP_xxx_xxx"
		mfp_str= segments[1]

		# Check if the string starts with the prefix and extract the remaining part
		if mfp_str.startswith(search_str):
			ip_pin = mfp_str[len(search_str):]
		else:
			continue

		ip_pin_split = ip_pin.split("_" ,1)

		ip = ip_pin_split[0]
		if len(ip_pin_split) >= 2:
			pin = ip_pin_split[1]
		else:
			pin = None

		ip_pin = refine_ip_pin(ip, pin)
		#print(ip_pin)
		alternative_pins.append(ip_pin)


	if len(alternative_pins) > 3:
		return alternative_pins
	return None

parser=argparse.ArgumentParser(description='Pin alternative generation. Input file: BSP/Library/StdDriver/inc/sys.h')
parser.add_argument("--input_file", help='BSP\'s sys.h', required=True)
parser.add_argument("--output_file", help='CSV file')
args=parser.parse_args()

input_file_name = args.input_file
output_file_name = args.output_file

if output_file_name == None:
	output_file_name = 'template.csv'
	
try:
	input_file = open(input_file_name, "r")
except OSError:
	print("Could not open input file")
	exit -1

try:
	output_file = open(output_file_name, "w")
except OSError:
	input_file.close()
	print("Could not create output file")
	exit -2

output_file.write('#Port,Pin,MFPL/MFPH,SPIM,,,,,,,,,,,,,,,,,,,,,,,EVENTOUT,\n')

while input_file:
	line = input_file.readline()
	if not line:
		break

	alternative_pins = None
	if re.search(" MFP /*", line):
		alternative_pins = alternative_pin_parser(input_file, line)

	if alternative_pins != None:
		for pins_str in alternative_pins:
			output_file.write(pins_str)
			output_file.write(',')
		output_file.write('EVENTOUT, \n')

input_file.close()
output_file.close()
print('Done')
