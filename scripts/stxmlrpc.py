#!/usr/bin/env python
from SimpleXMLRPCServer import SimpleXMLRPCServer
import socket
import sftoolkit

# Create server
server = SimpleXMLRPCServer((socket.getfqdn(), 8000))

datadir = "/tmp/sflow"

class MyFuncs:
	def get_cntr_index(self, agent, start, end, fields, index): 
		return sftoolkit.get_counterdata(agent, start, end, fields, datadir, index)

	def get_flow_switch(self, agent, start, end, fields):
		return sftoolkit.get_flowdata(agent, start, end, fields, datadir, -1)

	def get_flow_index(self, agent, start, end, fields, index):
		return sftoolkit.get_flowdata(agent, start, end, fields, datadir, index)

	def get_conv_switch(self, agent, start, end):
		return sftoolkit.get_conversations(agent, start, end, datadir, -1)

	def get_conv_index(self, agent, start, end, index):
		return sftoolkit.get_conversations(agent, start, end, datadir, index)

server.register_instance(MyFuncs())
server.serve_forever()
