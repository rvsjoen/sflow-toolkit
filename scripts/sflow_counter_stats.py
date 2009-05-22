#!/usr/bin/env python

import os

filenames = [f for f in os.listdir(".") if f.startswith("SFlow")]
filenames.sort()
#filenames = ["SFlow20090421_09_25.log","SFlow20090421_09_30.log","SFlow20090421_09_35.log","SFlow20090421_09_40.log"]
#filenames = ["SFlow20090421_09_35.log","SFlow20090421_09_40.log"]

result = {}
for f in filenames:
	print "Reading", f
	rows = [x.split(',') for x in open(f).read().split('\n') if x]
	for r in rows:
		(agent,port,timestamp) = r[:3]
		timestamp = int(timestamp)

		try:
			d = result[(agent,port)]
			if timestamp > d['end']:
				d['end'] = timestamp
			if timestamp < d['start']:
				d['start'] = timestamp
			d['cnt'] += 1

		except KeyError:
			d = {'cnt': 1, 'start': timestamp, 'end': timestamp}
			result[(agent,port)] = d


tot_rel = 0
tot_rec = 0
tot_exp = 0

for r in result:
	d = result[r]
	interval = d['end'] - d['start']
	expected = interval/30
	received = d['cnt']
	if expected == 0:
		reliability = 0.0
	else:
		reliability = float(received)/float(expected)
	tot_rel += reliability
	tot_rec += received
	tot_exp += expected
	print "Agent: %s, Interface: %s, Reliability: %.4f, Received: %u, Expected: %u, Interval: %u" % (r[0], r[1], reliability, received, expected, interval)

print "Total reliability: %.2f" % (tot_rel/len(result))
print "Expected: %u" % tot_exp
print "Received: %u" % tot_rec
