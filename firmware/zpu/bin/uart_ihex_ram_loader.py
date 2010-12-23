#!/usr/bin/python

import serial
from optparse import OptionParser
import os, sys

def main():
	usage="%prog: [options] filename"
	parser = OptionParser(usage=usage)
	parser.add_option("-t", "--tty", type="string", default="/dev/ttyUSB0",
                     help="select serial port [default=%default]")
	parser.add_option("-b", "--baudrate", type=int, default=115200,
										 help="set baudrate [default=%default]")

	(options, args) = parser.parse_args()
	if len(args) != 1:
		parser.print_help()
		raise SystemExit(1)

	filename = args[0]
	f = open(filename, "r")

	#all we have to do is load the IHX file and attempt to spit it out to the serial port.
	if(os.path.exists(options.tty) is False):
		sys.stderr.write("No serial port found at %s\n" % options.tty)
		raise SystemExit(1)
	
	try:
		ser = serial.Serial(port=options.tty, timeout=1, baudrate=options.baudrate, bytesize=8, parity=serial.PARITY_NONE, stopbits=1, rtscts=0, xonxoff=0)
	except serial.SerialException:
		sys.stderr.write("Unable to open serial port\n")
		raise SystemExit(1)

	ser.open()

#test to see if a valid USRP2+ in RAM load mode is connected

	ser.write("WOOOOO\n");
	reply = ser.readline()
	if("NOK" not in reply):
		sys.stderr.write("Valid USRP2+ not connected or no response received\n")
		raise SystemExit(1)
	else:
		print("USRP2+ found.")

	for line in f:
		ser.write(line.rstrip() + '\n')
		reply = ser.readline()
		if("NOK" in reply): #blocks to wait for response
			print("Received NOK reply from USRP2+")
			raise SystemExit(1)
		elif("OK" not in reply):
			print("Received invalid reply!")
			raise SystemExit(1)
#		else:
#			print("OK received")

	print "USRP2+ RAM programmed.\nLoading program."

	#at this point it should have sent the end line of the file, which starts the program!
	#we'll just act like a dumb terminal now
#	ser.timeout = 0
#	try:
#		while 1:
#			print ser.readline()
#	except KeyboardInterrupt:
#		raise SystemExit(0)

if __name__ == '__main__':
	main()
