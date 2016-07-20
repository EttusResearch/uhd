#!/usr/bin/python

import sys

if len(sys.argv) != 3:
    print 'Usage: ' + sys.argv[0] + ' <table filename> <table name>'
    sys.exit(1)

with open(sys.argv[1], 'rb') as f:
    print ('static const std::vector<twinrx_gain_config_t> ' +
           sys.argv[2] + ' = boost::assign::list_of')
    i = -1
    for line in f.readlines():
        if (i != -1):
            row = line.strip().split(',')
            print ('    ( twinrx_gain_config_t( %s, %6.1f, %s, %s, %s, %s ) )' % (
                str(i).rjust(5),
                float(row[0].rjust(6)),
                row[1].rjust(6), row[2].rjust(6),
                ('true' if int(row[3]) == 1 else 'false').rjust(5),
                ('true' if int(row[4]) == 1 else 'false').rjust(5)))
        else:
            print ('    //                      %s, %s, %s, %s, %s, %s' % (
                'Index', '  Gain', 'Atten1', 'Atten2', ' Amp1', ' Amp2'))
        i += 1
    print ';'
