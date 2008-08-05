#!/usr/bin/env python2.5

import os
import datetime
import time
import getopt
import sys


def usage():
	print """
	Usage: cleanup.py [-h|--help] [-d<storagedir>|--dir=<storagedir>] [-n<hours>|--num=<hours>]
	"""

def main():

	try:
		opts,args = getopt.getopt(sys.argv[1:], "hd:n:", ["help", "dir=", "num="])
	except getopt.GetoptError, err:
		print str(err)
		usage()
		sys.exit(2)
	
	num_hours = 24
	storagedir = "."

	for o,a in opts:
		if o in ("-h", "--help"):
			usage()
			sys.exit()
		if o in ("-d", "--dir"):
			storagedir = a
		if o in ("-n", "--num"):
			num_hours = int(a)

	timestamp = time.mktime(datetime.datetime.today().timetuple())
	t = datetime.datetime.fromtimestamp(timestamp - (3600*num_hours));
	agents = os.listdir("/storage/sflow");

	agents = [agent for agent in agents if agent != "0.0.0.0"]

	for agent in agents:
		try:
			os.system("rm -rf %s" % storagedir+"/"+agent+t.strftime("/%Y%m%d/%H"))
			parentdir = os.listdir(storagedir+"/"+agent+t.strftime("/%Y%m%d/"))
			if not parentdir:
				os.system("rm -rf %s" % storagedir+"/"+agent+t.strftime("/%Y%m%d/"))
		except OSError:
			pass

if __name__ == "__main__":
	main()
