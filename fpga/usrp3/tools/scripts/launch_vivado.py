#!/usr/bin/env python
#
# Notice: Some parts of this file were copied from PyBOMBS, which has a
# different copyright, and is highlighted appropriately. The following
# copyright notice pertains to all the parts written specifically for this
# script.
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
"""
Run Vivado builds
"""

from __future__ import print_function
import os
import sys
import re
import json
from datetime import datetime
import time
import argparse
import subprocess
import threading
try:
    from Queue import Queue, Empty
except ImportError:
    from queue import Queue, Empty  # Py3k

READ_TIMEOUT = 0.1 # s

#############################################################################
# The following functions were copied with minor modifications from PyBOMBS:
def get_console_width():
    '''
    Returns width of console.

    http://stackoverflow.com/questions/566746/how-to-get-console-window-width-in-python
    '''
    env = os.environ
    def ioctl_GWINSZ(fd):
        try:
            import fcntl, termios, struct
            cr = struct.unpack('hh', fcntl.ioctl(fd, termios.TIOCGWINSZ, '1234'))
        except:
            return
        return cr
    cr = ioctl_GWINSZ(0) or ioctl_GWINSZ(1) or ioctl_GWINSZ(2)
    if not cr:
        try:
            fd = os.open(os.ctermid(), os.O_RDONLY)
            cr = ioctl_GWINSZ(fd)
            os.close(fd)
        except:
            pass
    if not cr:
        cr = (env.get('LINES', 25), env.get('COLUMNS', 80))
    return cr[1]

def which(program):
    """
    Equivalent to Unix' `which` command.
    Returns None if the executable `program` can't be found.

    If a full path is given (e.g. /usr/bin/foo), it will return
    the path if the executable can be found, or None otherwise.

    If no path is given, it will search PATH.
    """
    def is_exe(fpath):
        " Check fpath is an executable "
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)
    if os.path.split(program)[0] and is_exe(program):
        return program
    else:
        for path in os.environ.get("PATH", "").split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
    return None
#
# End of functions copied from PyBOMBS.
#############################################################################

def print_timer(time_delta):
    """docstring for print_timer"""
    hours, secs = divmod(time_delta.seconds, 3600)
    mins, secs = divmod(secs, 60)
    return "[{h:02}:{m:02}:{s:02}]".format(
        h=hours, m=mins, s=secs,
    )

def list_search(patterns, string):
    " Returns True if string matches any element of pattern "
    for pattern in patterns:
        if re.search(pattern, string) is not None:
            return True
    return False

def parse_args():
    " Parses args for this script, and for Vivado. "
    parser = argparse.ArgumentParser(
        description="Run Vivado and parse output.",
    )
    parser.add_argument(
        '--no-color', action="store_true",
        help="Don't colorize output.",
    )
    parser.add_argument(
        '--vivado-command', default=None,
        help="Vivado command.",
    )
    parser.add_argument(
        '--parse-config', default=None,
        help="Additional parser configurations",
    )
    parser.add_argument(
        '-v', '--verbose', default=False,
        action='store_true',
        help="Print Vivado output")
    parser.add_argument(
        '--warnings', default=False,
        action='store_true',
        help="Print Vivado warnings")
    our_args, viv_args = parser.parse_known_args()
    return our_args, " ".join(viv_args)

class VivadoRunner(object):
    " Vivado Runner "
    colors = {
        'warning': '\033[0;35m',
        'critical warning': '\033[33m',
        'error': '\033[1;31m',
        'fatal': '\033[1;31m',
        'task': '\033[32m',
        'cmd': '\033[1;34m',
        'normal': '\033[0m',
    }
    # Black       0;30     Dark Gray     1;30
    # Blue        0;34     Light Blue    1;34
    # Green       0;32     Light Green   1;32
    # Cyan        0;36     Light Cyan    1;36
    # Red         0;31     Light Red     1;31
    # Purple      0;35     Light Purple  1;35
    # Brown       0;33     Yellow        1;33
    # Light Gray  0;37     White         1;37

    viv_tcl_cmds = {
        'synth_design' : 'Synthesis',
        'opt_design': 'Logic Optimization',
        'place_design': 'Placer',
        'route_design': 'Routing',
        'phys_opt_design': 'Physical Synthesis',
        'report_timing' : 'Timing Reporting',
        'report_power': 'Power Reporting',
        'report_drc': 'DRC',
        'write_bitstream': 'Write Bitstream',
    }

    def __init__(self, args, viv_args):
        self.status = ''
        self.args = args
        self.current_task = "Initialization"
        self.current_phase = "Starting"
        self.command = args.vivado_command + " " + viv_args
        self.notif_queue = Queue()
        self.msg_counters = {}
        self.fatal_error_found = False
        self.line_types = {
            'cmd': {
                'regexes': [
                    '^Command: .+',
                ],
                'action': self.show_cmd,
                'id': "Command",
            },
            'task': {
                'regexes': [
                    '^Starting .* Task',
                    '^.*Translating synthesized netlist.*',
                    '^\[TEST CASE .*',
                ],
                'action': self.update_task,
                'id': "Task",
            },
            'phase': {
                'regexes': [
                    '^Phase (?P<id>[a-zA-Z0-9/. ]*)$',
                    '^Start (?P<id>[a-zA-Z0-9/. ]*)$',
                    '^(?P<id>TESTBENCH STARTED: [\w_]*)$',
                ],
                'action': self.update_phase,
                'id': "Phase",
            },
            'warning': {
                'regexes': [
                    '^WARNING'
                ],
                'action': lambda x: self.act_on_build_msg('warning', x),
                'id': "Warning",
                'fatal': [
                ]
            },
            'critical warning': {
                'regexes': [
                    '^CRITICAL WARNING'
                ],
                'action': lambda x: self.act_on_build_msg('critical warning', x),
                'id': "Critical Warning",
                'fatal': [
                ]
            },
            'error': {
                'regexes': [
                    '^ERROR',
                    'no such file or directory',
                    '^Result: FAILED'
                ],
                'action': lambda x: self.act_on_build_msg('error', x),
                'id': "Error",
                'fatal': [
                    '.', # All errors are fatal by default
                ]
            },
            'test': {
                'regexes': [
                    '^ - T'
                    '^Result: '
                ],
                'action': self.update_testbench,
                'id': "Test"
            }
        }
        self.parse_config = None
        if args.parse_config is not None:
            try:
                args.parse_config = os.path.normpath(args.parse_config)
                parse_config = json.load(open(args.parse_config))
                self.add_notification(
                    "Using parser configuration from: {pc}".format(pc=args.parse_config),
                    color=self.colors.get('normal')
                )
                loadables = ('regexes', 'ignore', 'fatal')
                for line_type in self.line_types:
                    for loadable in loadables:
                        self.line_types[line_type][loadable] = \
                                self.line_types[line_type].get(loadable, []) + \
                                parse_config.get(line_type, {}).get(loadable, [])
            except (IOError, ValueError):
                self.add_notification(
                    "Could not read parser configuration from: {pc}".format(pc=args.parse_config),
                    color=self.colors.get('warning')
                )
        self.tty = sys.stdout.isatty()
        self.timer = datetime.now() # Make sure this is the last line in ctor

    def run(self):
        """
        Kick off Vivado build.

        Returns True if it all passed.
        """
        def enqueue_output(stdout_data, stdout_queue):
            " Puts the output from the process into the queue "
            for line in iter(stdout_data.readline, b''):
                stdout_queue.put(line)
            stdout_data.close()
        def poll_queue(q):
            " Safe polling from queue "
            try:
                return q.get(timeout=READ_TIMEOUT).decode('utf-8')
            except UnicodeDecodeError:
                pass
            except Empty:
                pass
            return ""
        # Start process
        self.add_notification(
            "Executing command: {cmd}".format(cmd=self.command), add_time=True, color=self.colors.get('cmd')
        )
        proc = subprocess.Popen(
            self.command,
            shell=True, # Yes we run this in a shell. Unsafe but helps with Vivado.
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT # Pipe it all out via stdout
        )
        # Init thread and queue
        q_stdout = Queue()
        t = threading.Thread(target=enqueue_output, args=(proc.stdout, q_stdout))
        # End the thread when the program terminates
        t.daemon = True
        t.start()
        status_line_t = threading.Thread(target=VivadoRunner.run_loop, args=(self.print_status_line, 0.5 if self.tty else 60*10))
        status_line_t.daemon = True
        status_line_t.start()
        # Run loop
        while proc.poll() is None or not q_stdout.empty(): # Run while process is alive
            line_stdout = poll_queue(q_stdout)
            self.update_output(line_stdout)
        success = (proc.returncode == 0) and not self.fatal_error_found
        self.cleanup_output(success)
        return success

    def update_output(self, lines):
        " Receives a line from Vivado output and acts upon it. "
        self.process_line(lines)

    @staticmethod
    def run_loop(func, delay, *args, **kwargs):
        while True:
            func(*args, **kwargs)
            time.sleep(delay)

    def print_status_line(self):
        " Prints status on stdout"
        old_status_line_len = len(self.status)
        self.update_status_line()
        sys.stdout.write("\x1b[2K\r") # Scroll cursor back to beginning and clear last line
        self.flush_notification_queue(old_status_line_len)
        sys.stdout.write(self.status)
        sys.stdout.flush()
        # Make sure we print enough spaces to clear out all of the previous message
        # if not msgs_printed:
        #     sys.stdout.write(" " * max(0, old_status_line_len - len(self.status)))

    def cleanup_output(self, success):
        " Run final printery after all is said and done. "
        # Check message counts are within limits
        self.update_phase("Finished")
        self.add_notification(
            "Process terminated. Status: {status}".format(status='Success' if success else 'Failure'),
            add_time=True,
            color=self.colors.get("task" if success else "error")
        )
        sys.stdout.write("\n")
        self.flush_notification_queue(len(self.status))
        print("")
        print("========================================================")
        print("Warnings:          ", self.msg_counters.get('warning', 0))
        print("Critical Warnings: ", self.msg_counters.get('critical warning', 0))
        print("Errors:            ", self.msg_counters.get('error', 0))
        print("")
        sys.stdout.flush()

    def process_line(self, lines):
        " process line "
        for line in [l.rstrip() for l in lines.split("\n") if len(l.strip())]:
            line_info, line_data = self.classify_line(line)
            if line_info is not None:
                self.line_types[line_info]['action'](line_data)
            elif self.args.verbose:
                print(line)

    def classify_line(self, line):
        """
        Identify the current line. Return None if the line can't be identified.
        """
        for line_type in self.line_types:
            for regex in self.line_types[line_type]['regexes']:
                re_obj = re.search(regex, line)
                if re_obj is not None:
                    return line_type, re_obj.groupdict().get('id', line)
        return None, None

    def update_status_line(self):
        " Update self.status. Does not print anything! "
        status_line = "{timer} Current task: {task} +++ Current Phase: {phase}"
        self.status = status_line.format(
            timer=print_timer(datetime.now() - self.timer),
            task=self.current_task.strip(),
            phase=self.current_phase.strip(),
        )

    def add_notification(self, msg, add_time=False, color=None):
        """
        Format msg and add it as a notification to the queue.
        """
        if add_time:
            msg = print_timer(datetime.now() - self.timer) + " " + msg
        if color is not None and not self.args.no_color:
            msg = color + msg + self.colors.get('normal')
        self.notif_queue.put(msg)

    def flush_notification_queue(self, min_len):
        " Print all strings in the notification queue. "
        msg_printed = False
        while not self.notif_queue.empty():
            msg = self.notif_queue.get().strip()
            print(msg)
            msg_printed = True
        return msg_printed

    def act_on_build_msg(self, msg_type, msg):
        """
        Act on a warning, error, critical warning, etc.
        """
        if list_search(self.line_types[msg_type].get('fatal', []), msg):
            self.add_notification(msg, color=self.colors.get('fatal'))
            self.fatal_error_found = True
        elif not list_search(self.line_types[msg_type].get('ignore', []), msg):
            self.add_notification(msg, color=self.colors.get(msg_type))
        self.msg_counters[msg_type] = self.msg_counters.get(msg_type, 0) + 1

    def show_cmd(self, tcl_cmd):
        " Show the current command "
        self.update_phase("Finished")
        tcl_cmd = tcl_cmd.replace("Command:", "").strip()
        #sys.stdout.write("\n")
        self.add_notification("Executing Tcl: " + tcl_cmd,
                              add_time=True, color=self.colors.get("cmd"))
        cmd = tcl_cmd.strip().split()[0];
        if cmd in self.viv_tcl_cmds:
            cmd = self.viv_tcl_cmds[cmd]
        self.update_task("Starting " + cmd + " Command", is_new=False)
        #self.flush_notification_queue(len(self.status))

    def update_task(self, task, is_new=True):
        " Update current task "
        # Special case: Treat "translation" as a phase as well
        if "Translating synthesized netlist" in task:
            task = "Translating Synthesized Netlist"
        filtered_task = task.replace("Starting", "").replace("Task", "").replace("Command", "")
        if is_new and (filtered_task != self.current_task):
            self.update_phase("Finished")
        self.current_task = filtered_task
        self.current_phase = "Starting"
        self.add_notification(task, add_time=True, color=self.colors.get("task"))
        sys.stdout.write("\n")
        self.print_status_line()

    def update_phase(self, phase):
        " Update current phase "
        self.current_phase = phase.strip()
        self.current_task = self.current_task.replace("Phase", "")
        sys.stdout.write("\n")
        self.print_status_line()

    def update_testbench(self, testbench):
        pass  # Do nothing


def main():
    " Go, go, go! "
    args, viv_args = parse_args()
    if args.vivado_command is None:
        if which("vivado"):
            args.vivado_command = "vivado"
        elif which("vivado_lab"):
            args.vivado_command = "vivado_lab"
        else:
            print("Cannot find Vivado executable!")
            return False
    try:
        return VivadoRunner(args, viv_args).run()
    except KeyboardInterrupt:
        print("")
        print("")
        print("Caught Ctrl-C. Exiting.")
        print("")
        return False

if __name__ == "__main__":
    exit(not main())

