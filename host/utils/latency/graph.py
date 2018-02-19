#!/usr/bin/env python
#
# Copyright 2012 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import sys, re
from optparse import OptionParser

import matplotlib.pyplot as plt
import matplotlib.font_manager
import numpy as np

try:
    from gnuradio.eng_option import eng_option
except:
    eng_option = None

_units = [
    (3, "k"),
    (6, "M"),
    (9, "G")
]

def _format_rate(rate):
    for (u1, s1), (u2, s2) in zip(_units, _units[1:]):
        n = pow(10, u1)
        if rate >= n and rate < pow(10, u2):
            r = rate % n
            if r > 0:
                return str(1.0 * rate / n) + " " + s1
            else:
                return str(rate / n) + " " + s1
    return str(rate) + " "


def _sort(series, keys):
    if len(keys) == 0:
        return []
    key = keys[0]
    rev = False
    if key[0] == '-':
        key = key[1:]
        rev = True
    l = []
    for s in series:
        if s[key] not in l:
            l += [s[key]]
    l.sort()
    if rev:
        l.reverse()
    return [(key, l)] + _sort(series, keys[1:])


def _order(series, sort_list):
    if len(sort_list) == 0:
        return series
    (sort_key, sort_key_list) = sort_list[0]
    if len(sort_key_list) == 0:
        return []
    #print sort_key, sort_key_list
    l = []
    for s in series:
        if s[sort_key] == sort_key_list[0]:
            l += [s]
    #print l
    return _order(l, sort_list[1:]) + _order(series, [(sort_list[0][0], sort_list[0][1][1:])] + sort_list[1:])


def get_option_parser():
    usage = "%prog: [options]"
    opt_kwds = {}
    if eng_option: opt_kwds['option_class'] = eng_option
    parser = OptionParser(usage=usage, **opt_kwds)

    parser.add_option("", "--id", type="string", help="device ID [default: %default]", default=None)
    parser.add_option("", "--sort", type="string", help="sort order [default: %default]", default="rate -spb -spp")
    parser.add_option("", "--output", type="string", help="output file [default: %default]", default=None)
    parser.add_option("", "--output-type", type="string", help="output file type [default: %default]", default="pdf")
    parser.add_option("", "--output-size", type="string", help="output file size [default: %default pixels]",
                      default="1600,900")
    parser.add_option("", "--xrange", type="float", help="X range [default: %default]", default=None)
    parser.add_option("", "--title", type="string", help="additional title [default: %default]", default=None)
    parser.add_option("", "--legend", type="string", help="legend position [default: %default]", default="lower right")
    parser.add_option("", "--diff", action="store_true", help="compare results instead of just plotting them", default=None)
    return parser


def get_sorted_series(args, options):
    series = []

    if len(args) > 0:
        with open(args[0]) as f:
            lines = f.readlines()
    else:
        lines = sys.stdin.readlines()
    if lines is None or len(lines) == 0:
        return

    for line in lines:
        line = line.strip()
        if len(line) == 0:
            continue
        x = {'file': line}
        idx2 = 0
        idx = line.find("latency-stats")
        if idx > 0:
            x['prefix'] = line[0:idx]
        idx = line.find(".id_")
        if idx > -1:
            idx += 4
            idx2 = line.find("-", idx)
            x['id'] = line[idx:idx2]
            if options.id is None:
                options.id = x['id']
            elif options.id != x['id']:
                print "Different IDs:", options.id, x['id']
        idx = line.find("-rate_")
        if idx > -1:
            idx += 6
            idx2 = line.find("-", idx)
            x['rate'] = int(line[idx:idx2])
        idx = line.find("-spb_")
        if idx > -1:
            idx += 5
            idx2 = line.find("-", idx)
            x['spb'] = int(line[idx:idx2])
        idx = line.find("-spp_")
        if idx > -1:
            idx += 5
            #idx2 = line.find(".", idx)
            idx2 = re.search("\D", line[idx:])
            if idx2:
                idx2 = idx + idx2.start()
            else:
                idx2 = -1
            x['spp'] = int(line[idx:idx2])
        idx = line.rfind(".")
        if idx > -1 and idx >= idx2:
            idx2 = re.search("\d", line[::-1][len(line) - idx:])
            if idx2 and (idx2.start() > 0):
                idx2 = idx2.start()
                x['suffix'] = line[::-1][len(line) - idx:][0:idx2][::-1]
        print x
        series += [x]

    sort_keys = options.sort.split()
    print sort_keys
    sorted_key_list = _sort(series, sort_keys)
    print sorted_key_list
    series = _order(series, sorted_key_list)

    return series


def main():
    # Create object with all valid options
    parser = get_option_parser()

    # Read in given command line options and arguments
    (options, args) = parser.parse_args()


    # series contains path and attributes for all data sets given by args.
    series = get_sorted_series(args, options)

    # Read in actual data sets from file
    data = read_series_data(series)

    if options.diff:
        data = calculate_data_diff(data)


    # Get all the wanted properties for this plot
    plt_props = get_plt_props(options)
    print plt_props

    mpl_plot(data, plt_props)

    return 0


def read_series_data(series):
    if series is None: return []
    result = []
    for s in series:
        data = {}
        [data_x, data_y] = np.loadtxt(s['file'], delimiter=" ", unpack=True)
        data['x'] = data_x
        data['y'] = data_y
        data['metadata'] = s
        result.append(data)
    return result


def find_values(data, key):
    result = []
    for d in data:
        val = d['metadata'][key]
        if not val in result:
            result.append(val)
    return result


def find_match(data, key, val):
    result = []
    for d in data:
        meta = d['metadata']
        if meta[key] == val:
            result.append(d)
    return result

def get_data_diff(data):
    if not data:
        return data # just return. User didn't input any data.
    if len(data) < 2:
        return data[0] # Single data set. Can't calculate a diff.

    print "diff %d: rate %s, spb %s, spp %s" % (len(data), data[0]['metadata']['rate'], data[0]['metadata']['spb'], data[0]['metadata']['spp'])

    data = align_data(data)

    min_len = len(data[0]['x'])
    for d in data:
        min_len = min(min_len, len(d['x']))

    metadiff = ""
    for d in data:
        m = d['metadata']['prefix']
        for r in "/._":
            m = m.replace(r, "")
        metadiff += m + "-"

    xd = data[0]['x'][0:min_len]
    yd = data[0]['y'][0:min_len]
    meta = data[0]['metadata']
    meta['diff'] = metadiff
    other = data[1:]
    for d in other:
        y = d['y']
        for i in range(len(yd)):
            yd[i] -= y[i]

    result = {}
    result['x'] = xd
    result['y'] = yd
    result['metadata'] = meta
    return result

def align_data(data):
    x_start = 0
    for d in data:
        x_start = max(x_start, d['x'][0])

    for i in range(len(data)):
        s = np.where(data[i]['x'] == x_start)[0]
        data[i]['x'] = data[i]['x'][s:]
        data[i]['y'] = data[i]['y'][s:]

    return data


def calculate_data_diff(data):
    spps = find_values(data, "spp")
    spbs = find_values(data, "spb")
    rates = find_values(data, "rate")
    print spps, "\t", spbs, "\t", rates
    result = []
    for rate in rates:
        rd = find_match(data, "rate", rate)
        for spb in spbs:
            bd = find_match(rd, "spb", spb)
            for spp in spps:
                pd = find_match(bd, "spp", spp)
                if len(pd) > 0:
                    result.append(get_data_diff(pd))

    return result


def get_plt_props(options):
    plt_props = {}
    plt_out = None
    if options.output is not None:
        try:
            idx = options.output_size.find(",")
            x = int(options.output_size[0:idx])
            y = int(options.output_size[idx + 1:])
            plt_out = {'name': options.output,
                       'type': options.output_type,
                       'size': [x, y]}
        except:
            plt_out = None

    plt_props['output'] = plt_out

    if not options.id: options.id = "no data"
    plt_title = "Latency (" + options.id + ")"
    if options.title is not None and len(options.title) > 0:
        plt_title += " - " + options.title
    plt_props['title'] = plt_title

    plt_props['xlabel'] = "Latency (us)"
    plt_props['ylabel'] = "Normalised success of on-time burst transmission"

    plt_legend_loc = None
    if options.legend is not None:
        plt_legend_loc = options.legend
    plt_props['legend'] = plt_legend_loc

    plt_xrange = None
    if options.xrange is not None:
        plt_xrange = [0, options.xrange]
    plt_props['xrange'] = plt_xrange
    return plt_props


def mpl_plot(data, props):
    plt_out = props['output']
    plt_title = props['title']
    plt_xlabel = props['xlabel']
    plt_ylabel = props['ylabel']
    plt_legend_loc = props['legend']
    plt_xrange = props['xrange']

    markers = ['.', ',', 'o', 'v', '^', '<', '>', '1', '2', '3', '4', '8',
               's', 'p', '*', 'h', 'H', '+', 'D', 'd', '|', '_']
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k', 'w']
    midx = 0

    # plot available data.
    mylegend = []
    for d in data:
        mylegend.append(get_legend_str(d['metadata']))
        plt.plot(d['x'], d['y'], marker=markers[midx], markerfacecolor=None)
        midx = (midx + 1) % len(markers)

    # Set all plot properties
    plt.title(plt_title)
    plt.xlabel(plt_xlabel)
    plt.ylabel(plt_ylabel)
    plt.grid()
    fontP = matplotlib.font_manager.FontProperties()
    fontP.set_size('x-small')
    plt.legend(mylegend, loc=plt_legend_loc, prop=fontP, ncol=2)
    if plt_xrange is not None:
        plt.xlim(plt_xrange)

    # Save plot to file, if option is given.
    if plt_out is not None:
        fig = plt.gcf() # get current figure
        dpi = 100.0 # Could be any value. It exists to convert the input in pixels to inches/dpi.
        figsize = (plt_out['size'][0] / dpi, plt_out['size'][1] / dpi) # calculate figsize in inches
        fig.set_size_inches(figsize)
        name = plt_out['name'] + "." + plt_out['type']
        plt.savefig(name, dpi=dpi, bbox_inches='tight')

    plt.show()


def get_legend_str(meta):
    lt = ""
    if meta.has_key('diff') and meta['diff']:
        lt += meta['diff'] + " "
    lt += "%ssps, SPB %d, SPP %d" % (_format_rate(meta['rate']), meta['spb'], meta['spp'])
    return lt


if __name__ == '__main__':
    main()
