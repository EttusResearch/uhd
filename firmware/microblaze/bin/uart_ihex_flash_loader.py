#!/usr/bin/python

import serial
from optparse import OptionParser
import os, sys

#TODO: pull everything but parser out of main() and put it in a separate function we can call from another script. lets us automate loading RAM+FLASH to produce a fully-loaded image.
#TODO: make it fail gracefully -- if it gets a NOK or times out, do at least one retry.
#TODO: put hooks in (eventually) to allow verifying a firmware image so the user can more safely update the "safe" image
#TODO: how about a progress indicator? FPGA images take FOREVER. you can use wc -l to get the number of lines, or do it with file i/o.

def main():
	usage="%prog: [options] filename"
	parser = OptionParser(usage=usage)
	parser.add_option("-t", "--tty", type="string", default="/dev/ttyUSB0",
                     help="select serial port [default=%default]")
	parser.add_option("-b", "--baudrate", type=int, default=115200,
										 help="set baudrate [default=%default]")
	parser.add_option("-F", "--write-safe-firmware", action="store_const", const=1, dest="image",
										 help="write to safe firmware image")
	parser.add_option("-f", "--write-production-firmware", action="store_const", const=2, dest="image",
										 help="write to production firmware image")
	parser.add_option("-P", "--write-safe-fpga", action="store_const", const=3, dest="image",
										 help="write to safe FPGA image")
	parser.add_option("-p", "--write-production-fpga", action="store_const", const=4, dest="image",
										 help="write to production FPGA image")

	(options, args) = parser.parse_args()

	if(options.image is None):
		print("At least one of -f, -F, -p, -P must be specified.\n")
		parser.print_help()
		raise SystemExit(1)
	
	if len(args) != 1:
		parser.print_help()
		raise SystemExit(1)

	if(options.image == 3):
		print "Are you *really* sure you want to write to the failsafe FPGA image? If you mess this up your USRP2+ will become a brick. Press 'y' to continue, any other key to abort."
		if(raw_input().rstrip() is not "y"):
			print "Good choice."
			raise SystemExit(0)

	elif(options.image == 1):
		print "Are you *really* sure you want to write to the failsafe firmware image? If you mess this up your USRP2+ will only be able to be reprogrammed via the UART RAM loader.\nPress 'y' to continue, any other key to abort."
		if(raw_input().rstrip() is not "y"):
			print "Good choice."
			raise SystemExit(0)

	filename = args[0]
	f = open(filename, "r")

	#now we start doing things...
	if(os.path.exists(options.tty) is False):
		sys.stderr.write("No serial port found at %s\n" % options.tty)
		raise SystemExit(1)
	
	try:
		ser = serial.Serial(port=options.tty, timeout=1, baudrate=options.baudrate, bytesize=8, parity=serial.PARITY_NONE, stopbits=1, rtscts=0, xonxoff=0)
	except serial.SerialException:
		sys.stderr.write("Unable to open serial port\n")
		raise SystemExit(1)

	ser.open()

#test to see if a valid USRP2+ in flash load mode is connected
	ser.write("garbage\n")
	ser.readline()
	ser.write("!SECTORSIZE\n")
	reply = ser.readline().rstrip()
	if("NOK" in reply):
		sys.stderr.write("Error writing to USRP2+. Try again.\n")
		raise SystemExit(1)
	elif("OK" in reply):
		sectorsize = int(reply[3:5], 16)
		print("USRP2+ found with sector size %i. Erasing old image." % 2**sectorsize)
	else:
		sys.stderr.write("Invalid response or no USRP2+ connected.\n")
		raise SystemExit(1)

	if(options.image == 1):
		sectors = range(127, 128)
		runcmd = "!RUNSFD\n"
	elif(options.image == 2):
		sectors = range(64, 65)
		runcmd = "!RUNPFD\n"
	elif(options.image == 3):
		sectors = range(0,32)
		runcmd = "!RUNSFPGA\n"
	elif(options.image == 4):
		sectors = range(32,64)
		runcmd = "!RUNPFPGA\n"

	writeaddr = sectors[0] << sectorsize
	if(options.image < 3):
		writeaddr -= 0x8000 #i know this is awkward, but we subtract 0x8000 from the address for firmware loads. the reason we do this is that the IHX files are located at 0x8000 (RAM_BASE), and
												#doing this here allows us to use the same IHX files for RAM load as for Flash load, without relocating in objcopy or relinking with another ld script.
												#this limits us to writing above 32K for our firmware images. since the safe FPGA image located at 0x00000000 takes up 2MB of space this isn't really a worry.
												#FPGA images (.mcs) always start at 0x00000000 so they don't need this relocation.

	for sector in sectors:
		print "Erasing sector %i" % sector
		ser.write("!ERASE %i\n" % sector)
		reply = ser.readline()
		if("NOK" in reply):
			sys.stderr.write("Error erasing sector %i" % sector)
			raise SystemExit(1)
	
	print "Setting start address to %i" % writeaddr
	ser.write("!SETADDR %i\n" % writeaddr)
	if("NOK" in reply):
		sys.stderr.write("Error setting address\n")
		raise SystemExit(1)
	else:
		print reply


	for line in f:
		ser.write(line.rstrip()+'\n')
		reply = ser.readline()
		if("NOK" in reply): #TODO: simplify this logic, use (reply.rstrip() is "NOK")
			print("Received NOK reply during data write")
			raise SystemExit(1)
		elif("DONE" in reply):
			print("Finished writing program. Loading...\n")
			ser.write(runcmd)
		elif("OK" not in reply):
			print("Received invalid reply %s during data write" % reply)
			raise SystemExit(1)
		else:
			print reply.rstrip() + '\t' + line.rstrip()

	print ser.readline()


if __name__ == '__main__':
	main()
