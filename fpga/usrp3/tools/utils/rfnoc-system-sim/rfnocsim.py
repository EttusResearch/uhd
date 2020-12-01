#!/usr/bin/env python
#
# Copyright 2016 Ettus Research
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import collections
import copy
import re
import math
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from graphviz import Digraph

#------------------------------------------------------------
# Simulator Core Components
#------------------------------------------------------------
class comptype():
    """
    Simulation component type enumeration
    """
    producer = 'Producer'
    consumer = 'Consumer'
    channel  = 'Channel'
    function = 'Function'
    hardware = 'Hardware'
    other    = 'Other'

class SimulatorCore:
    """
    Core simulation engine:
    This class owns all the simulation components and
    manages time and other housekeeping operations.
    """

    def __init__(self, tick_rate):
        self.__ticks = 0
        self.__tick_rate = tick_rate
        self.__tick_aware_comps = list()
        self.__all_comps = dict()
        self.__edge_render_db = list()

    def register(self, comp, tick_aware):
        if comp.name not in self.__all_comps:
            self.__all_comps[comp.name] = comp
        else:
            raise RuntimeError('Duplicate component ' + comp.name)
        if tick_aware:
            self.__tick_aware_comps.append(comp)

    def connect(self, src, srcport, dst, dstport, render_label=None, render_color=None):
        src.connect(srcport, dst.inputs(dstport, bind=True))
        if render_label:
            self.__edge_render_db.append(
                (src.name, dst.name, 1.0, render_label, render_color))

    def connect_bidir(self, ep1, ep1port, ep2, ep2port, render_labels=None, render_colors=None):
        if render_labels:
            if not isinstance(render_labels, (list, tuple)):
                render_labels = [render_labels, render_labels]
        else:
            render_labels = [None, None]
        if render_colors:
            if not isinstance(render_colors, (list, tuple)):
                render_colors = [render_colors, render_colors]
        else:
            render_colors = [None, None]
        self.connect(ep1, ep1port, ep2, ep2port, render_labels[0], render_colors[0])
        self.connect(ep2, ep2port, ep1, ep1port, render_labels[1], render_colors[1])

    def connect_multi(self, src, srcports, dst, dstports, render_label=None, render_color=None):
        if len(srcports) != len(dstports):
            raise RuntimeError(
                'Source and destination ports should be of the same length')
        for i in range(len(srcports)):
            src.connect(srcports[i], dst.inputs(dstports[i], bind=True))
        if render_label:
            self.__edge_render_db.append((src.name, dst.name, float(len(srcports)), render_label, render_color))

    def connect_multi_bidir(self, ep1, ep1port, ep2, ep2port, render_labels=None, render_colors=None):
        if render_labels:
            if not isinstance(render_labels, (list, tuple)):
                render_labels = [render_labels, render_labels]
        else:
            render_labels = [None, None]
        if render_colors:
            if not isinstance(render_colors, (list, tuple)):
                render_colors = [render_colors, render_colors]
        else:
            render_colors = [None, None]
        self.connect_multi(ep1, ep1port, ep2, ep2port, render_labels[0], render_colors[0])
        self.connect_multi(ep2, ep2port, ep1, ep1port, render_labels[1], render_colors[1])

    def list_components(self, comptype='', name_filt=''):
        if not comptype:
            return sorted([c for c in list(self.__all_comps.keys())
                if (re.match(name_filt, self.__all_comps[c].name))])
        else:
            return sorted([c for c in list(self.__all_comps.keys())
                if (self.__all_comps[c].type == comptype and
                    re.match(name_filt, self.__all_comps[c].name))])

    def lookup(self, comp_name):
        return self.__all_comps[comp_name]

    def tick(self):
        self.__ticks += 1
        for c in self.__tick_aware_comps:
            c.tick()

    def run(self, time_s):
        for i in range(int(time_s * self.__tick_rate)):
            self.tick()

    def get_ticks(self):
        return self.__ticks

    def get_tick_rate(self):
        return self.__tick_rate

    def network_to_dot(self):
        dot = Digraph(comment='RFNoC Network Topology')
        node_ids = dict()
        next_node_id = 1
        for edgeinfo in self.__edge_render_db:
            for i in range(2):
                node = edgeinfo[i]
                if node not in node_ids:
                    node_id = next_node_id
                    node_ids[node] = node_id
                    dot.node(str(node_id), node)
                    next_node_id += 1
        for edgeinfo in self.__edge_render_db:
            dot.edge(
                tail_name=str(node_ids[edgeinfo[0]]),
                head_name=str(node_ids[edgeinfo[1]]),
                label=edgeinfo[3],
                weight=str(edgeinfo[2]), penwidth=str(edgeinfo[2]/2),
                color=str(edgeinfo[4] if edgeinfo[4] else 'black'))
        return dot

class SimComp:
    """
    Base simulation component:
    All components must inherit from SimComp.
    """

    def __init__(self, sim_core, name, ctype):
        self.__sim_core = sim_core
        self.name = name
        self.type = ctype
        self.__sim_core.register(self, (ctype == comptype.producer))

    def get_ticks(self):
        return self.__sim_core.get_ticks()

    def get_tick_rate(self):
        return self.__sim_core.get_tick_rate()

    def SimCompError(self, msg):
        raise RuntimeError(msg + ' [' + self.name + ']')

#------------------------------------------------------------
# Data stream components
#------------------------------------------------------------
class HwRsrcs():
    """
    Hardware Resources Container:
    This object holds physical hardware resource information
    that can be used to report utilization. Resource items are
    generic and can be defined by the actual simulation.
    """

    def __init__(self):
        self.__rsrcs = dict()

    def get(self, what):
        if what in self.__rsrcs:
            return self.__rsrcs[what]
        else:
            return 0.0

    def set(self, what, value):
        self.__rsrcs[what] = float(value)

    def add(self, what, value):
        if what in self.__rsrcs:
            self.__rsrcs[what] += float(value)
        else:
            self.__rsrcs[what] = float(value)

    def merge(self, other_rsrcs):
        for attr in other_rsrcs.get_attrs():
            self.add(attr, other_rsrcs.get(attr))

    def get_attrs(self):
        return list(self.__rsrcs.keys())

    def reset(self, what = None):
        if what is not None:
            if what in self.__rsrcs:
                self.__rsrcs[what] = 0.0
        else:
            self.__rsrcs = dict()

class DataStream:
    """
    Data Stream Object:
    Holds information about a date stream that passes through various block.
    The simulator simulates event on the actual stream so each stream Object
    must have a unique payload (items) to disambiguate it from the rest.
    """
    HopInfo = collections.namedtuple('HopInfo', ['location', 'latency'])

    class HopDb():
        def __init__(self, hops):
            self.__hops = hops

        def get_src(self):
            return self.__hops[0].location

        def get_dst(self):
            return self.__hops[-1].location

        def get_hops(self):
            hoparr = []
            for h in self.__hops:
                hoparr.append(h.location)
            return hoparr

        def get_latency(self, ticks, location = ''):
            latency = ticks - self.__hops[0].latency    #Hop0 always has the init timestamp
            if (self.__hops[0].location != location):
                for i in range(1,len(self.__hops)):
                    latency += self.__hops[i].latency
                    if (self.__hops[i].location == location):
                        break
            return latency

    def __init__(self, bpi, items, count, producer=None, parent=None):
        self.bpi = bpi
        self.items = []
        self.items.extend(items)
        self.count = count
        self.__hops = list()
        if producer and parent:
            raise RuntimeError('Data stream cannot have both a producer and a parent stream')
        elif producer:
            self.__hops.append(self.HopInfo(location='Gen@'+producer.name, latency=producer.get_ticks()))
        elif parent:
            self.__hops.extend(parent.get_hops())
        else:
            raise RuntimeError('Data stream must have a producer or a parent stream')

    def add_hop(self, location, latency):
        self.__hops.append(self.HopInfo(location=location, latency=latency))

    def get_hops(self):
        return self.__hops

    def get_bytes(self):
        return self.bpi * len(self.items) * self.count

    """
    Type specific methods
    """
    @staticmethod
    def submatrix_gen(matrix_id, coordinates):
        coord_arr = []
        for c in coordinates:
            if isinstance(c, collections.Iterable):
                coord_arr.append('(' + (','.join(str(x) for x in c)) + ')')
            else:
                coord_arr.append('(' + str(c) + ')')
        return matrix_id + '[' + ';'.join(coord_arr) + ']'

    @staticmethod
    def submatrix_parse(stream_id):
        m = re.match('(.+)\[(.*)\]', stream_id)
        matrix_id = m.group(1)
        coords = []
        for cstr in m.group(2).split(';'):
            coords.append([int(x) for x in re.match('\((.+)\)', cstr).group(1).split(',')])
        return (matrix_id, coords)

#------------------------------------------------------------
# Basic Network components
#------------------------------------------------------------

# Producer object.
class Producer(SimComp):
    """
    Producer Block:
    Generates data at a constant rate
    """

    def __init__(self, sim_core, name, bpi, items, max_samp_rate = float('inf'), latency = 0):
        SimComp.__init__(self, sim_core, name, comptype.producer)
        self.__bpi = bpi
        self.__items = items
        self.__bw = max_samp_rate * bpi
        self.__latency = latency
        self.__dests = list()
        self.__data_count = 0
        self.__byte_count = 0
        self.__backpressure_ticks = 0
        self.set_rate(self.get_tick_rate())

    def inputs(self, i, bind=False):
        raise self.SimCompError('This is a producer block. Cannot connect another block to it.')

    def connect(self, i, dest):
        self.__dests.append(dest)

    def set_rate(self, samp_rate):
        self.__data_count = samp_rate / self.get_tick_rate()

    def tick(self):
        if len(self.__dests) > 0:
            ready = True
            for dest in self.__dests:
                ready = ready and dest.is_ready()
            if ready:
                data = DataStream(
                    bpi=self.__bpi, items=self.__items, count=self.__data_count, producer=self)
                if self.__backpressure_ticks > 0:
                    data.add_hop('BP@'+self.name, self.__backpressure_ticks)
                data.add_hop(self.name, self.__latency)
                for dest in self.__dests:
                    dest.push(copy.deepcopy(data))
                self.__byte_count += data.get_bytes()
                self.__backpressure_ticks = 0
            else:
                self.__backpressure_ticks += 1

    def get_bytes(self):
        return self.__byte_count

    def get_util_attrs(self):
        return ['bandwidth']

    def get_utilization(self, what):
        if what in self.get_util_attrs():
            return ((self.__byte_count / (self.get_ticks() / self.get_tick_rate())) /
                    self.__bw)
        else:
            return 0.0

# Consumer object.
class Consumer(SimComp):
    """
    Consumes Block:
    Consumes data at a constant rate
    """

    def __init__(self, sim_core, name, bw = float("inf"), latency = 0):
        SimComp.__init__(self, sim_core, name, comptype.consumer)
        self.__byte_count = 0
        self.__item_db = dict()
        self.__bw = bw
        self.__latency = latency
        self.__bound = False

    def inputs(self, i, bind=False):
        if bind and self.__bound:
            raise self.SimCompError('Input ' + str(i) + ' is already driven (bound).')
        self.__bound = bind
        return self

    def connect(self, i, dest):
        raise self.SimCompError('This is a consumer block. Cannot connect to another block.')

    def is_ready(self):
        return True #TODO: Readiness can depend on bw and byte_count

    def push(self, data):
        data.add_hop(self.name, self.__latency)
        for item in data.items:
            self.__item_db[item] = DataStream.HopDb(data.get_hops())
        self.__byte_count += data.get_bytes()

    def get_items(self):
        return list(self.__item_db.keys())

    def get_bytes(self):
        return self.__byte_count

    def get_hops(self, item):
        return self.__item_db[item].get_hops()

    def get_latency(self, item, hop=None):
        if not hop:
            hop = self.get_hops(item)[-1]
        return self.__item_db[item].get_latency(self.get_ticks(), hop) / self.get_tick_rate()

    def get_util_attrs(self):
        return ['bandwidth']

    def get_utilization(self, what):
        if what in self.get_util_attrs():
            return ((self.__byte_count / (self.get_ticks() / self.get_tick_rate())) /
                    self.__bw)
        else:
            return 0.0

# Channel
class Channel(SimComp):
    """
    A resource limited IO pipe:
    From the data stream perspective, this is a passthrough
    """

    def __init__(self, sim_core, name, bw = float("inf"), latency = 0, lossy = True):
        SimComp.__init__(self, sim_core, name, comptype.channel)
        self.__bw = bw
        self.__latency = latency
        self.__lossy = lossy
        self.__dests = list()
        self.__byte_count = 0
        self.__bound = False

    def get_bytes(self):
        return self.__byte_count

    def inputs(self, i, bind=False):
        if (i != 0):
            raise self.SimCompError('An IO lane has only one input.')
        if bind and self.__bound:
            raise self.SimCompError('Input ' + str(i) + ' is already driven (bound).')
        self.__bound = bind
        return self

    def connect(self, i, dest):
        self.__dests.append(dest)

    def is_connected(self):
        return len(self.__dests) > 0

    def is_bound(self):
        return self.__bound

    def is_ready(self):
        # If nothing is hooked up to a lossy lane, it will drop data
        if self.__lossy and not self.is_connected():
            return True
        ready = self.is_connected()
        for dest in self.__dests:
            ready = ready and dest.is_ready()
        return ready

    def push(self, data):
        # If nothing is hooked up to a lossy lane, it will drop data
        if self.__lossy and not self.is_connected():
            return
        data.add_hop(self.name, self.__latency)
        for dest in self.__dests:
            dest.push(copy.deepcopy(data))
        self.__byte_count += data.get_bytes()

    def get_util_attrs(self):
        return ['bandwidth']

    def get_utilization(self, what):
        if what in self.get_util_attrs():
            return ((self.__byte_count / (self.get_ticks() / self.get_tick_rate())) /
                    self.__bw)
        else:
            return 0.0

# Function
class Function(SimComp):
    """
    A Function Component:
    A function block is something that does anything interesting with a data stream.
    A function can have multiple input and output streams.
    """

    class Arg:
        def __init__(self, num, base_func):
            self.__num = num
            self.__data = None
            self.__base_func = base_func
            self.__bound = False

        def get_num(self):
            return self.__num

        def is_ready(self):
            return self.__base_func.is_ready() and not self.__data

        def push(self, data):
            self.__data = data
            self.__base_func.notify(self.__num)

        def pop(self):
            if self.__data:
                data = self.__data
                self.__data = None
                return data
            else:
                raise RuntimeError('Nothing to pop.')

        def bind(self, bind):
            retval = self.__bound
            self.__bound = bind
            return retval

    Latencies = collections.namedtuple('Latencies', ['func','inarg','outarg'])

    def __init__(self, sim_core, name, num_in_args, num_out_args, ticks_per_exec = 1):
        SimComp.__init__(self, sim_core, name, comptype.function)
        self.__ticks_per_exec = ticks_per_exec
        self.__last_exec_ticks = 0
        self.__in_args = list()
        for i in range(num_in_args):
            self.__in_args.append(Function.Arg(i, self))
        self.__dests = list()
        for i in range(num_out_args):
            self.__dests.append(None)
        self.__in_args_pushed = dict()
        # Resources required by this function to do its job in one tick
        self.__rsrcs = HwRsrcs()
        self.__latencies = self.Latencies(func=0, inarg=[0]*num_in_args, outarg=[0]*num_out_args)

    def get_rsrcs(self):
        return self.__rsrcs

    def update_rsrcs(self, rsrcs):
        self.__rsrcs = rsrcs

    def update_latency(self, func, inarg=None, outarg=None):
        self.__latencies = self.Latencies(
            func=func,
            inarg=inarg if inarg else [0]*len(self.__in_args),
            outarg=outarg if outarg else [0]*len(self.__dests))

    def inputs(self, i, bind=False):
        if bind and self.__in_args[i].bind(True):
            raise self.SimCompError('Input argument ' + str(i) + ' is already driven (bound).')
        return self.__in_args[i]

    def connect(self, i, dest):
        self.__dests[i] = dest

    def is_ready(self):
        ready = len(self.__dests) > 0
        for dest in self.__dests:
            ready = ready and dest.is_ready()
        exec_ready = (self.get_ticks() - self.__last_exec_ticks) >= self.__ticks_per_exec
        return ready and exec_ready

    def create_outdata_stream(self, bpi, items, count):
        return DataStream(
            bpi=bpi, items=items, count=count, parent=self.__max_latency_input)

    def notify(self, arg_i):
        self.__in_args_pushed[arg_i] = True
        # Wait for all input args to come in
        if (sorted(self.__in_args_pushed.keys()) == list(range(len(self.__in_args)))):
            # Pop data out of each input arg
            max_in_latency = 0
            self.__max_latency_input = None
            arg_data_in = list()
            for arg in self.__in_args:
                d = arg.pop()
                arg_data_in.append(d)
                lat = DataStream.HopDb(d.get_hops()).get_latency(self.get_ticks())
                if lat > max_in_latency:
                    max_in_latency = lat
                    self.__max_latency_input = d
            # Call the function
            arg_data_out = self.do_func(arg_data_in)
            if not isinstance(arg_data_out, collections.Iterable):
                arg_data_out = [arg_data_out]
            # Update output args
            for i in range(len(arg_data_out)):
                arg_data_out[i].add_hop(self.name,
                    max(self.__latencies.inarg) + self.__latencies.func + self.__latencies.outarg[i])
                self.__dests[i].push(arg_data_out[i])
            # Cleanup
            self.__last_exec_ticks = self.get_ticks()
            self.__in_args_pushed = dict()

    def get_util_attrs(self):
        return []

    def get_utilization(self, what):
        return 0.0

#------------------------------------------------------------
# Plotting Functions
#------------------------------------------------------------
class Visualizer():
    def __init__(self, sim_core):
        self.__sim_core = sim_core
        self.__figure = None
        self.__fig_dims = None

    def show_network(self, engine='fdp'):
        dot = self.__sim_core.network_to_dot()
        dot.format = 'png'
        dot.engine = engine
        dot.render('/tmp/rfnoc_sim.dot', view=True, cleanup=True)

    def dump_consumed_streams(self, consumer_filt='.*'):
        comps = self.__sim_core.list_components(comptype.consumer, consumer_filt)
        print('=================================================================')
        print('Streams Received by Consumers matching (%s) at Tick = %04d'%(consumer_filt,self.__sim_core.get_ticks()))
        print('=================================================================')
        for c in sorted(comps):
            comp = self.__sim_core.lookup(c)
            for s in sorted(comp.get_items()):
                print((' - %s: (%s) Latency = %gs' % (s, c, comp.get_latency(s))))
        print('=================================================================')

    def dump_debug_audit_log(self, ctype, name_filt='.*'):
        if ctype != comptype.channel:
            raise NotImplementedError('Component type not yet supported: ' + ctype)

        comps = self.__sim_core.list_components(ctype, name_filt)
        print('=================================================================')
        print('Debug Audit for all %s Components matching (%s)'%(ctype,name_filt))
        print('=================================================================')
        for c in sorted(comps):
            comp = self.__sim_core.lookup(c)
            status = 'Unknown'
            if comp.is_bound() and comp.is_connected():
                status = 'Good'
            elif comp.is_bound() and not comp.is_connected():
                status = 'WARNING (Driven but Unused)'
            elif not comp.is_bound() and comp.is_connected():
                status = 'WARNING (Used but Undriven)'
            else:
                status = 'Unused'
            print((' - %s: Status = %s'%(c,status)))
        print('=================================================================')

    def new_figure(self, grid_dims=[1,1], fignum=1, figsize=(16, 9), dpi=72):
        self.__figure = plt.figure(num=fignum, figsize=figsize, dpi=dpi)
        self.__fig_dims = grid_dims

    def show_figure(self):
        plt.show()
        self.__figure = None

    def plot_utilization(self, ctype, name_filt='.*', grid_pos=1):
        colors = ['b','r','g','y']
        comps = self.__sim_core.list_components(ctype, name_filt)
        attrs = set()
        for c in comps:
            attrs |= set(self.__sim_core.lookup(c).get_util_attrs())
        attrs = sorted(list(attrs))

        if not self.__figure:
            self.new_figure()
            show = True
        else:
            show = False
        self.__figure.subplots_adjust(bottom=0.25)
        ax = self.__figure.add_subplot(*(self.__fig_dims + [grid_pos]))
        title = 'Resource utilization for all %s\ncomponents matching \"%s\"' % \
            (ctype, name_filt)
        ax.set_title(title)
        ax.set_ylabel('Resource Utilization (%)')
        if comps:
            ind = np.arange(len(comps))
            width = 0.95/len(attrs)
            rects = []
            ymax = 100
            for i in range(len(attrs)):
                utilz = [self.__sim_core.lookup(c).get_utilization(attrs[i]) * 100 for c in comps]
                rects.append(ax.bar(ind + width*i, utilz, width, color=colors[i%len(colors)]))
                ymax = max(ymax, int(math.ceil(max(utilz) / 100.0)) * 100)
            ax.set_ylim([0,ymax])
            ax.set_yticks(list(range(0,ymax,10)))
            ax.set_xticks(ind + 0.5)
            ax.set_xticklabels(comps, rotation=90)
            ax.legend(rects, attrs)
            ax.grid(b=True, which='both', color='0.65',linestyle='--')
        ax.plot([0, len(comps)], [100, 100], "k--", linewidth=3.0)
        if show:
            self.show_figure()

    def plot_consumption_latency(self, stream_filt='.*', consumer_filt='.*', grid_pos=1):
        streams = list()
        for c in sorted(self.__sim_core.list_components(comptype.consumer, consumer_filt)):
            for s in sorted(self.__sim_core.lookup(c).get_items()):
                if (re.match(stream_filt, s)):
                    streams.append((c, s, c + '/' + s))

        if not self.__figure:
            self.new_figure()
            show = True
        else:
            show = False
        self.__figure.subplots_adjust(bottom=0.25)
        ax = self.__figure.add_subplot(*(self.__fig_dims + [grid_pos]))
        title = 'Latency of Maximal Path Terminating in\nStream(s) matching \"%s\"\n(Consumer Filter = \"%s\")' % \
            (stream_filt, consumer_filt)
        ax.set_title(title)
        ax.set_ylabel('Maximal Source-to-Sink Latency (s)')
        if streams:
            ind = np.arange(len(streams))
            latency = [self.__sim_core.lookup(c_s_d1[0]).get_latency(c_s_d1[1]) for c_s_d1 in streams]
            rects = [ax.bar(ind, latency, 1.0, color='b')]
            ax.set_xticks(ind + 0.5)
            ax.set_xticklabels([c_s_d[2] for c_s_d in streams], rotation=90)
            attrs = ['latency']
            ax.legend(rects, attrs)
            ax.yaxis.set_major_formatter(mticker.FormatStrFormatter('%.2e'))
            ax.grid(b=True, which='both', color='0.65',linestyle='--')
        if show:
            self.show_figure()

    def plot_path_latency(self, stream_id, consumer_filt = '.*', grid_pos=1):
        path = []
        latencies = []
        for c in self.__sim_core.list_components(comptype.consumer, consumer_filt):
            for s in self.__sim_core.lookup(c).get_items():
                if (stream_id == s):
                    for h in self.__sim_core.lookup(c).get_hops(s):
                        path.append(h)
                        latencies.append(self.__sim_core.lookup(c).get_latency(s, h))
                    break
        if not self.__figure:
            self.new_figure()
            show = True
        else:
            show = False
        self.__figure.subplots_adjust(bottom=0.25)
        ax = self.__figure.add_subplot(*(self.__fig_dims + [grid_pos]))
        title = 'Accumulated Latency per Hop for Stream \"%s\"\n(Consumer Filter = \"%s\")' % \
            (stream_id, consumer_filt)
        ax.set_title(title)
        ax.set_ylabel('Maximal Source-to-Sink Latency (s)')
        if path:
            ind = np.arange(len(path))
            rects = [ax.plot(ind, latencies, '--rs')]
            ax.set_xticks(ind)
            ax.set_xticklabels(path, rotation=90)
            ax.yaxis.set_major_formatter(mticker.FormatStrFormatter('%.2e'))
            ax.grid(b=True, which='both', color='0.65',linestyle='--')
        if show:
            self.show_figure()
