#!/usr/bin/env python2.5
import os
files = [f for f in os.listdir("/dev/shm") if f.startswith("SFlow")]
while len(files) > 72:
	filename = files.pop()
	try:
		os.remove(("/dev/shm/%s" % filename))
	except:
		pass
	try:
		os.remove(("/home/netadmin/sflow/csv/%s" % filename))
	except:
		pass
	print "Deleting",filename
