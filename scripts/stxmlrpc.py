#!/usr/bin/env python
from SimpleXMLRPCServer import SimpleXMLRPCServer
import socket
import time
import sftoolkit

# Create server
server = SimpleXMLRPCServer((socket.getfqdn(), 8000))

datadir = "/tmp/sflow"
logfile = open(datadir+"/xmlrpcserver.log", "w");

class MyFuncs:
	def __init__(self):
		global i 
		i = 0
	def get_cntr_index(self, agent, start, end, fields, index): 
		global i 
		logfile.write(time.asctime()+" REQUEST [%u] get_cntr_index(%s, %s, %s, %s, %s) \n"%(i, agent, start, end, fields, index))
		res = sftoolkit.get_counterdata(agent, start, end, fields, datadir, index)
		logfile.write(time.asctime()+" REPLY [%u]\n"%i);
		logfile.flush()
		i += 1
		return res

	def get_flow_switch(self, agent, start, end, fields):
		global i
		logfile.write(time.asctime()+" REQUEST [%u] get_flow_switch(%s, %s, %s, %s)\n"%(i, agent, start, end, fields));
		res = sftoolkit.get_flowdata(agent, start, end, fields, datadir, -1)
		logfile.write(time.asctime()+" REPLY [%u]\n"%i);
		logfile.flush()
		i += 1
		return res

	def get_flow_index(self, agent, start, end, fields, index):
		global i
		logfile.write(time.asctime()+" REQUEST [%u] get_flow_index(%s, %s, %s, %s, %s)\n"%(i, agent, start, end, fields, index));
		res = sftoolkit.get_flowdata(agent, start, end, fields, datadir, index)
		logfile.write(time.asctime()+" REPLY [%u]\n"%i);
		logfile.flush()
		i += 1
		return res

	def get_conv_switch(self, agent, start, end):
		global i
		logfile.write(time.asctime()+" REQUEST [%u] get_conv_switch(%s, %s, %s)\n"%(i, agent, start, end));
		res = sftoolkit.get_conversations(agent, start, end, datadir, -1)
		logfile.write(time.asctime()+" REPLY [%u]\n"%i);
		logfile.flush()
		i += 1
		return res

	def get_conv_index(self, agent, start, end, index):
		global i
		logfile.write(time.asctime()+" REQUEST [%u] get_conv_index(%s, %s, %s, %s)\n"%(i, agent, start, end, index));
		res = sftoolkit.get_conversations(agent, start, end, datadir, index)
		logfile.write(time.asctime()+" REPLY [%u]\n"%i);
		logfile.flush()
		i += 1
		return res

server.register_instance(MyFuncs())
server.serve_forever()

#f = MyFuncs()
#MyFuncs.get_conv_index(f,"10.144.46.50","2008.09.21 08:00", "2008.09.21 08:01", 235733051)
#MyFuncs.get_conv_index(f,"10.144.46.50","2008.09.21 08:00", "2008.09.21 08:01", 235733051)
#MyFuncs.get_conv_index(f,"10.144.46.50","2008.09.21 08:00", "2008.09.21 08:01", 235733051)
