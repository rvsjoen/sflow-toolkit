#!/usr/bin/env python

configfile = "/etc/stcollectd.conf"

import sys
import time
import datetime
import struct
import base64
import commands
import tempfile
import socket
import yaml

validfields_cntr = {
	"timestamp":0, 
	"agent":1,
	"subagent":2,
	"sequence":3,
	"id_type":4,
	"id_index":5,
	"if_index":6,
	"if_type":7,
	"if_speed":8,
	"if_direction":9,
	"if_status":10,
	"if_in_octets":11,
	"if_in_ucast":12,
	"if_in_mcast":13,
	"if_in_bcast":14,
	"if_in_discards":15,
	"if_in_errors":16,
	"if_in_unknown_proto":17,
	"if_out_octets":18,
	"if_out_ucast":19,
	"if_out_mcast":20,
	"if_out_bcast":21,
	"if_out_discards":22,
	"if_out_errors":23,
	"if_out_promisc":24
	} 
#TODO Add the rest of these fields in the counter samples

validfields_flow = {
	"timestamp":0, 
	"agent":1,
	"subagent":2,
	"sequence":3,
	"id_type":4,
	"id_index":5,
	"sample_rate":6,
	"sample_pool":7,
	"sample_drops":8,
	"sample_input_if_format":9,
	"sample_input_if_value":10,
	"sample_output_if_format":11,
	"sample_output_if_value":12,
	"raw_header_protocol":13,
	"raw_header_frame_length":14,
	"raw_header_stripped":15,
	"raw_header_length":16,
	"raw_header":17
	}

sflowconfig = {}
"""
Read the configuration data from the specified configuration file and store it in the config dictionary
"""
def get_configuration(configfile):
	global sflowconfig
	fp = open(configfile, "r")
	sflowconfig = yaml.load(fp.read())
	fp.close()

def date_range(start, end):
	r = ( end + datetime.timedelta( days = 1 ) - start ).days
	l = [ (start + datetime.timedelta(days=i)).timetuple()[:3] for i in range(r) ]
	return [ ("%d%02d%02d"%item,item)  for item in l ]

def reverse_lookup(d,v):
	for k in d:
		if d[k] == v:
			return k
	raise ValueError

def get_filenames(agent, start, end, datadir, type):
	start = time.strptime(start, "%Y.%m.%d %H:%M")
	end = time.strptime(end, "%Y.%m.%d %H:%M")
	start_ts = time.mktime(start)
	end_ts = time.mktime(end)
	s=datetime.date(start[0],start[1],start[2])
	e=datetime.date(end[0],end[1],end[2])
	list = date_range(s,e)
	files = []
	for day in list:
		d = day[1]
		for i in range(24):
			h = (d[0],d[1],d[2],i)
			for j in range(60):
				m = (d[0],d[1],d[2],i,j,0,0,0,-1)
				timestamp = time.mktime(m)
				if timestamp >= start_ts and timestamp < end_ts:
					files.append(datadir+"/"+agent+"/%d%02d%02d/%02d/%02d/samples_"%m[:5]+type+".dat")
	return files

def get_headers(fields, type):
	headers = []
	v = {}
	if type is "cntr":
		v = validfields_cntr
	elif type is "flow":
		v = validfields_flow
	for field in fields:
		headers.append(reverse_lookup(v,v[field]))
	return headers

def get_counterdata(agent, start, end, fields, datadir, index):
	files = get_filenames(agent, start, end, datadir, "cntr")
	result = []
	result.append(get_headers(fields.split(","), "cntr"))
	for f in files:
		r = process_file(f, index, fields.split(","), "cntr")
		if r and len(r) > 0:
			result += r
	return result

def get_flowdata(agent, start, end, fields, datadir, index):
	files = get_filenames(agent, start, end, datadir, "flow")
	result = []
	result.append(get_headers(fields.split(","), "flow"))
	for f in files:
		r = process_file(f, index, fields.split(","), "flow")
		if r and len(r) > 0:
			result += r
	return result

def process_file_binary(f, index, type):
	size = 0
	format = ""

	if type is "cntr":
		v = validfields_cntr
		size = 164
		format = "8LQ2LQ6LQ6L13L"
	elif type is "flow":
		v = validfields_flow
		size = 196
		format = "17L128s"
	try: 
		fp = open(f, "r")
		tmp = fp.read(size)
		while tmp:
			buf = struct.unpack(format, tmp)
			tmp = fp.read(size)
			if int(index) == int(buf[v["id_index"]]) or index == -1:
				sys.stdout.write(tmp)
		fp.close()
	except IOError:
		pass

def write_pcap_header():
	PCAP_MAGIC = 0xa1b2c3d4
	PCAP_MAJOR = 2
	PCAP_MINOR = 4
	hdr = struct.pack("Ihhi3I", PCAP_MAGIC, PCAP_MAJOR, PCAP_MINOR, 0, 0, 65535, 1)
	sys.stdout.write(hdr)

def process_file_pcap(f, index):
	format = "17L128s"
	size = 196
	v = validfields_flow
	try: 
		fp = open(f, "r")
		tmp = fp.read(size)
		while tmp:
			buf = struct.unpack(format, tmp)
			tmp = fp.read(size)

			if int(index) == int(buf[v["id_index"]]) or index == -1:
				hdr = struct.pack("4I", buf[v["timestamp"]], 0, buf[v["raw_header_length"]], buf[v["raw_header_frame_length"]])				
				sys.stdout.write(hdr)
				sys.stdout.write(buf[v["raw_header"]][:buf[v["raw_header_length"]]])
		fp.close()
	except IOError:
		pass

def sort_key(item):
	return -long(item[-1]);

def get_conversations(agent, start, end, datadir, index):
	f = tempfile.NamedTemporaryFile()
	stdout_bak = sys.stdout
	sys.stdout = f
	get_flowdata_pcap(agent, start, end, datadir, index)
	sys.stdout = stdout_bak
	f.flush()
	result = commands.getoutput("tshark -r "+f.name+" -q -z conv,ip")
	header = ["Host 1","Host 2","Frames <-","Bytes <-","Frames ->","Bytes ->","Frames <->","Bytes <->"]
	lines = result.split("\n")
	data = lines[6:-1]
	result = []
	for d in data:
	        v = [item for item in d.split(" ") if item]
	        v.remove("<->")
	
	        try:
	                v[0] = socket.gethostbyaddr(v[0])[0]+" ("+v[0]+")"
	        except socket.herror:
	                pass
	        try:
	                v[1] = socket.gethostbyaddr(v[1])[0]+" ("+v[1]+")"
	        except socket.herror:
	                pass
	        result.append(v)
	f.close()
	result = sorted(result[1:], key=sort_key)
	result.insert(0, header)
	return result

def get_flowdata_binary(agent, start, end, datadir, index):
	files = get_filenames(agent, start, end, datadir, "flow")
	for f in files:
		process_file_binary(f, index, "flow")

def get_counterdata_binary(agent, start, end, datadir, index):
	files = get_filenames(agent, start, end, datadir, "cntr")
	for f in files:
		process_file_binary(f, index, "cntr")

def get_flowdata_pcap(agent, start, end, datadir, index):
	files = get_filenames(agent, start, end, datadir, "flow")
	write_pcap_header()
	for f in files:
		process_file_pcap(f, index)

def process_file(f, index, fields, type):
	size = 0
	format = ""

	if type is "cntr":
		v = validfields_cntr
		size = 164
		format = "8LQ2LQ6LQ6L13L"
	elif type is "flow":
		v = validfields_flow
		size = 196
		format = "17L128s"

	try: 
		fp = open(f, "r")
		tmp = fp.read(size)
		result = []
		while tmp:
			record = []

			buf = struct.unpack(format, tmp)
			tmp = fp.read(size)
	
			if int(index) == int(buf[v["id_index"]]) or index == -1:
				for field in fields:
					if type is "flow" and field == "raw_header":
						record.append(base64.encodestring(buf[v[field]]))
					else:
						record.append(buf[v[field]])
				result.append(record)
		fp.close()
		return result
	except IOError:
		pass

if __name__ == "__main__":
	sys.exit(0)
else:
	get_configuration(configfile)
