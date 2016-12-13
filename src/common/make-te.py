#!/usr/bin/env python3

import sys

inf = open(sys.argv[1])
eventf = open(sys.argv[2], 'w')
enumsf = open(sys.argv[3], 'w')

enumsf.write(\
'''
/* this file is auto generated, edit textevents.in instead! */

enum
{
''')

eventf.write(\
'''
/* this file is auto generated, edit textevents.in instead! */

const struct text_event te[] = {
''')

try:
	while True:
		name = inf.readline().strip()
		event_enum = inf.readline().strip()
		event_help = inf.readline().strip()
		event_str = inf.readline().strip()
		args = inf.readline().strip()
		translate = True

		if args[0] == 'n':
			args = int(args[1]) | 128
			translate = False
		else:
			args = int(args)

		if translate:
			event_str = 'N_("%s")' %event_str
		else:
			event_str = '"%s"' %event_str

		enumsf.write('\t%s,\n' %event_enum)
		eventf.write('\n{"%s", %s, %u,\n%s},\n' %(
			name, event_help, args, event_str,
		))

		inf.readline() # whitespace
except IndexError:
    pass

enumsf.write('\tNUM_XP\n};\n')
eventf.write('};\n')

