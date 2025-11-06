#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

"""Batch testbench execution script

This script searches the locations that you specify for testbenches and then
executes them using the desired simulator. Work is automatically grouped into
jobs for parallel execution and the results are printed to the console.
Testbenches are identified by the presence of the viv_sim_preamble in the
Makefile."""

import argparse
import os
import sys
import subprocess
import logging
import re
import time
import datetime

from junit_xml import TestSuite, TestCase, to_xml_report_file
from queue import Queue
from threading import Thread

# -------------------------------------------------------
# Utilities
# -------------------------------------------------------

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
BASE_DIR = os.path.split(os.path.split(SCRIPT_DIR)[0])[0]

_LOG = logging.getLogger(os.path.basename(__file__))
_LOG.setLevel(logging.INFO)
_STDOUT = logging.StreamHandler()
_LOG.addHandler(_STDOUT)
_FORMATTER = logging.Formatter("[%(name)s] - %(levelname)s - %(message)s")
_STDOUT.setFormatter(_FORMATTER)

RETCODE_SUCCESS = 0
RETCODE_PARSE_ERR = -1
RETCODE_EXEC_ERR = -2
RETCODE_COMPILE_ERR = -3
RETCODE_UNKNOWN_ERR = -4


def retcode_to_str(code):
    """Convert internal status code to string"""
    code = int(code)
    if code > RETCODE_SUCCESS:
        return "AppError({code})".format(code=code)
    else:
        return {
            RETCODE_SUCCESS: "OK",
            RETCODE_PARSE_ERR: "ParseError",
            RETCODE_EXEC_ERR: "ExecError",
            RETCODE_COMPILE_ERR: "CompileError",
            RETCODE_UNKNOWN_ERR: "UnknownError",
        }[code]


def result_to_string(result):
    """Convert a result dictionary to a string summarizing the results"""
    if "module" in result:
        retval = "Expected={:02d}, Run={:02d}, Passed={:02d}, Elapsed={:s}, Returncode={:s}".format(
            result["tc_expected"],
            result["tc_run"],
            result["tc_passed"],
            result["wall_time"],
            retcode_to_str(result["retcode"]),
        )
    else:
        retval = "Returncode={:s}".format(retcode_to_str(result["retcode"]))
    return retval


def log_with_header(what, minlen=0, ch="#"):
    """Print with a header around the text"""
    padlen = max(int((minlen - len(what)) / 2), 1)
    toprint = (" " * padlen) + what + (" " * padlen)
    _LOG.info(ch * len(toprint))
    _LOG.info(toprint)
    _LOG.info(ch * len(toprint))


# -------------------------------------------------------
# Simulation Functions
# -------------------------------------------------------


def read_excludes_file(excludes_fname):
    if excludes_fname:
        return [l.strip() for l in open(excludes_fname) if (l.strip() and "#" not in l)]
    else:
        return []


def find_sims_on_fs(basedir, excludes):
    """Find all testbenches in the specific basedir
    Testbenches are defined as directories with a
    Makefile that includes viv_sim_preamble.mak
    """
    sims = {}
    for root, _, files in os.walk(basedir):
        if "/build-ip/" in root:
            # Exclude any testbenches in the IP build outputs, since these are
            # duplicates.
            continue
        name = os.path.relpath(root, basedir)
        if "Makefile" in files:
            with open(os.path.join(root, "Makefile"), "r") as mfile:
                for l in mfile.readlines():
                    if re.match(".*include.*viv_sim_preamble.mak.*", l) is not None:
                        if name not in excludes:
                            sims.update({name: root})
                            break
    return sims


def gather_target_sims(basedir, targets, excludes):
    """Parse the specified targets and gather simulations to run
    Remove duplicates and sort alphabetically
    """
    fs_sims = find_sims_on_fs(basedir, excludes)
    if not isinstance(targets, list):
        targets = [targets]
    sim_names = set()
    for target in targets:
        for name in sorted(fs_sims):
            if re.match(target, name) is not None:
                sim_names.add(name)
    target_sims = []
    for name in sorted(sim_names):
        target_sims.append((name, fs_sims[name]))
    return target_sims


def parse_output(simout):
    # Gather results (basic metrics)
    results = {"retcode": RETCODE_SUCCESS, "stdout": simout, "passed": False}
    # Look for the following in the log:
    # - A start timestamp (indicates that Vivado started)
    # - The testbench infrastructure start header (indicates that the TB started)
    # - A stop timestamp (indicates that the TB stopped)
    tb_started = False
    compile_started = False
    results["start_time"] = "<unknown>"
    results["wall_time"] = "<unknown>"
    for line in simout.split(b"\n"):
        tsm = re.match(rb"TESTBENCH STARTED: (.+)", line)
        if tsm is not None:
            tb_started = True
        csm = re.match(rb"source .*viv_sim_project.tcl", line)
        if csm is not None:
            compile_started = True
        # xsim log
        vsm = re.match(rb"# Start of session at: (.+)", line)
        if vsm is not None:
            results["start_time"] = str(vsm.group(1), "ascii")
        tfm = re.match(rb"launch_simulation:.*; elapsed = (.+) \..*", line)
        if tfm is not None:
            results["wall_time"] = str(tfm.group(1), "ascii")
        # modelsim log
        vsm = re.match(rb"# Start time: (.+)", line)
        if vsm is not None:
            results["start_time"] = str(vsm.group(1), "ascii")
        tfm = re.match(rb"# End time:.*, Elapsed time: (.+)", line)
        if tfm is not None:
            results["wall_time"] = str(tfm.group(1), "ascii")

    # Parse testbench results
    #
    # We have two possible formats to parse because we have two simulation
    # test executors.
    #
    # ModelSim and Questa print "# " at the start of each line. In some cases,
    # Questa Base will print an extra line with "# " between the actual lines.
    tb_match_fmt0 = [
        b".*TESTBENCH FINISHED: (.+)\n",
        b"(?:# \n)?(?:# )? - Time elapsed:   (\\d+) ns.*\n",
        b"(?:# \n)?(?:# )? - Tests Expected: (\\d+)\n",
        b"(?:# \n)?(?:# )? - Tests Run:      (\\d+)\n",
        b"(?:# \n)?(?:# )? - Tests Passed:   (\\d+)\n",
        b"(?:# \n)?(?:# )?Result: (PASSED|FAILED).*",
    ]
    m_fmt0 = re.match(b"".join(tb_match_fmt0), simout, re.DOTALL)
    tb_match_fmt1 = [
        b".*TESTBENCH FINISHED: (.*)\n",
        b"(?:# \n)?(?:# )? - Time elapsed:  (\\d+) ns.*\n",
        b"(?:# \n)?(?:# )? - Tests Run:     (\\d+)\n",
        b"(?:# \n)?(?:# )? - Tests Passed:  (\\d+)\n",
        b"(?:# \n)?(?:# )? - Tests Failed:  (\\d+)\n",
        b"(?:# \n)?(?:# )?Result: (PASSED|FAILED).*",
    ]
    m_fmt1 = re.match(b"".join(tb_match_fmt1), simout, re.DOTALL)

    # Remove escape characters (colors) from Vivado output
    ansi_escape = re.compile(r"(?:\x1B[\(@-_]|[\x80-\x9F])[0-?]*[ -/]*[@-~]")
    plain_simout = ansi_escape.sub("", simout.decode("utf-8"))

    # Check for $error() and $fatal() output, which may be missed by the
    # testbench or may occur in a subsequent instance, after a pass.
    tb_match_error = [
        "\n",
        "(Error|Fatal): .*\n.?",
        "Time: .+\n",
    ]
    m_error = re.search("".join(tb_match_error), plain_simout)

    # Figure out the return code
    retcode = RETCODE_UNKNOWN_ERR
    if m_fmt0 is not None or m_fmt1 is not None:
        retcode = RETCODE_SUCCESS
        if m_fmt0 is not None:
            results["passed"] = m_fmt0.group(6) == b"PASSED" and m_error is None
            results["module"] = m_fmt0.group(1)
            results["sim_time_ns"] = int(m_fmt0.group(2))
            results["tc_expected"] = int(m_fmt0.group(3))
            results["tc_run"] = int(m_fmt0.group(4))
            results["tc_passed"] = int(m_fmt0.group(5))
        else:
            results["passed"] = m_fmt1.group(6) == b"PASSED" and m_error is None
            results["module"] = m_fmt1.group(1)
            results["sim_time_ns"] = int(m_fmt1.group(2))
            results["tc_expected"] = int(m_fmt1.group(3))
            results["tc_run"] = int(m_fmt1.group(3))
            results["tc_passed"] = int(m_fmt1.group(4))
    elif tb_started:
        retcode = RETCODE_PARSE_ERR
    elif compile_started:
        retcode = RETCODE_COMPILE_ERR
    else:
        retcode = RETCODE_EXEC_ERR
    results["retcode"] = retcode
    return results


def run_sim(path, simulator, basedir, setupenv):
    """Run the simulation at the specified path
    The simulator can be specified as the target
    A environment script can be run optionally
    """
    try:
        # Optionally run an environment setup script
        if setupenv is None:
            setupenv = ""
            # Check if environment was setup
            if "VIVADO_PATH" not in os.environ:
                return {
                    "retcode": RETCODE_EXEC_ERR,
                    "passed": False,
                    "stdout": bytes("Simulation environment was not initialized\n", "utf-8"),
                }
        else:
            setupenv = ". " + os.path.realpath(setupenv) + ";"
        # Run the simulation
        return parse_output(
            subprocess.check_output(
                'cd {workingdir}; /bin/bash -c "{setupenv} make ip 2>&1; make {simulator} 2>&1"'.format(
                    workingdir=path, setupenv=setupenv, simulator=simulator
                ),
                shell=True,
            )
        )
    except subprocess.CalledProcessError as e:
        return {"retcode": int(abs(e.returncode)), "passed": False, "stdout": e.output}
    except Exception as e:
        _LOG.error("Target " + path + " failed to run:\n" + str(e))
        return {"retcode": RETCODE_EXEC_ERR, "passed": False, "stdout": bytes(str(e), "utf-8")}
    except:
        _LOG.error("Target " + path + " failed to run")
        return {
            "retcode": RETCODE_UNKNOWN_ERR,
            "passed": False,
            "stdout": bytes("Unknown Exception", "utf-8"),
        }


def run_sim_queue(run_queue, out_queue, simulator, basedir, setupenv):
    """Thread worker for a simulation runner
    Pull a job from the run queue, run the sim, then place
    output in out_queue
    """
    while not run_queue.empty():
        (name, path) = run_queue.get()
        try:
            _LOG.info("Starting: %s", name)
            result = run_sim(path, simulator, basedir, setupenv)
            out_queue.put((name, result))
            _LOG.info(
                "FINISHED: %s (%s, %s)",
                name,
                retcode_to_str(result["retcode"]),
                "PASS" if result["passed"] else "FAIL!",
            )
        except KeyboardInterrupt:
            _LOG.warning("Target " + name + " received SIGINT. Aborting...")
            out_queue.put(
                (
                    name,
                    {
                        "retcode": RETCODE_EXEC_ERR,
                        "passed": False,
                        "stdout": bytes("Aborted by user", "utf-8"),
                    },
                )
            )
        except Exception as e:
            _LOG.error("Target " + name + " failed to run:\n" + str(e))
            out_queue.put(
                (
                    name,
                    {
                        "retcode": RETCODE_UNKNOWN_ERR,
                        "passed": False,
                        "stdout": bytes(str(e), "utf-8"),
                    },
                )
            )
        finally:
            run_queue.task_done()


# -------------------------------------------------------
# Script Actions
# -------------------------------------------------------


def do_list(args):
    """List all simulations that can be run"""
    excludes = read_excludes_file(args.excludes)
    for (name, path) in gather_target_sims(args.basedir, args.target, excludes):
        print(name)
    return 0


def do_run(args):
    """Build a simulation queue based on the specified
    args and process it
    """
    run_queue = Queue(maxsize=0)
    out_queue = Queue(maxsize=0)
    _LOG.info("Queueing the following targets to simulate:")
    excludes = read_excludes_file(args.excludes)
    name_maxlen = 0
    for (name, path) in gather_target_sims(args.basedir, args.target, excludes):
        run_queue.put((name, path))
        name_maxlen = max(name_maxlen, len(name))
        _LOG.info("* " + name)
    # Spawn tasks to run builds
    num_sims = run_queue.qsize()
    num_jobs = min(num_sims, int(args.jobs))
    _LOG.info("Started " + str(num_jobs) + " job(s) to process queue...")
    results = {}
    for i in range(num_jobs):
        worker = Thread(
            target=run_sim_queue,
            args=(run_queue, out_queue, args.simulator, args.basedir, args.setupenv),
        )
        worker.setDaemon(False)
        worker.start()
    # Wait for build queue to become empty
    start = datetime.datetime.now()
    try:
        sim_count = -1
        while out_queue.qsize() < num_sims:
            tdiff = str(datetime.datetime.now() - start).split(".", 2)[0]
            if args.logged:
                # Print number of TBs completed and elapsed time whenever a
                # simulation completes.
                if sim_count != out_queue.qsize():
                    print(
                        ">>> [%s] (%d/%d simulations completed) <<<"
                        % (tdiff, out_queue.qsize(), num_sims)
                    )
                    sim_count = out_queue.qsize()
            else:
                # Print elapsed time and number of TBs completed once per
                # second, overwriting the same line each time.
                print(
                    "\r>>> [%s] (%d/%d simulations completed) <<<"
                    % (tdiff, out_queue.qsize(), num_sims),
                    end="\r",
                    flush=True,
                )
            time.sleep(1.0)
        sys.stdout.write("\n")
    except (KeyboardInterrupt):
        _LOG.warning("Received SIGINT. Aborting... (waiting for pending jobs to finish)")
        # Flush run queue
        while not run_queue.empty():
            (name, path) = run_queue.get()
        raise SystemExit(1)

    results = {}
    result_all = 0
    while not out_queue.empty():
        (name, result) = out_queue.get()
        results[name] = result
        line = "#" * 70
        sys.stdout.buffer.write(bytes("%s\n Begin TB Log: %s\n%s\n" % (line, name, line), "utf-8"))
        sys.stdout.buffer.write(result["stdout"])
        sys.stdout.buffer.write(bytes("%s\n End TB Log: %s\n%s\n" % (line, name, line), "utf-8"))
        if not result["passed"]:
            result_all += 1
    sys.stdout.write("\n\n\n")
    sys.stdout.flush()
    time.sleep(1.0)

    hdr_len = name_maxlen + 62  # 62 is the report line length
    log_with_header("RESULTS", hdr_len)
    for name in sorted(results):
        result = results[name]
        _LOG.info(
            "* %s : %s (%s)",
            name.ljust(name_maxlen),
            ("Passed" if result["passed"] else "FAILED"),
            result_to_string(result),
        )
    _LOG.info("=" * hdr_len)
    _LOG.info(
        "SUMMARY: %d out of %d tests passed. Time elapsed was %s"
        % (num_sims - result_all, num_sims, str(datetime.datetime.now() - start).split(".", 2)[0])
    )
    _LOG.info("#" * hdr_len)
    if args.report:
        do_report(args, results)
    return result_all


def do_cleanup(args):
    """Run make cleanall for all simulations"""
    setupenv = args.setupenv
    if setupenv is None:
        setupenv = ""
        # Check if environment was setup
        if "VIVADO_PATH" not in os.environ:
            raise RuntimeError("Simulation environment was not initialized")
    else:
        setupenv = ". " + os.path.realpath(setupenv) + ";"
    excludes = read_excludes_file(args.excludes)
    for (name, path) in gather_target_sims(args.basedir, args.target, excludes):
        _LOG.info("Cleaning up %s", name)
        old_path = os.getcwd()
        os.chdir(path)
        subprocess.Popen("{setupenv} make cleanall".format(setupenv=setupenv), shell=True).wait()
        os.chdir(old_path)
    return 0


def do_report_csv(args, results):
    """Generate report file (.csv)"""
    template = {
        "module": "<unknown>",
        "status": "NOT_RUN",
        "retcode": "<unknown>",
        "start_time": "<unknown>",
        "wall_time": "<unknown>",
        "sim_time_ns": 0,
        "tc_expected": 0,
        "tc_run": 0,
        "tc_passed": 0,
    }
    with open(args.report, "w") as repfile:
        repfile.write((",".join([x.upper() for x in template.keys()])) + "\n")
        for name in sorted(results):
            line = template.copy()
            result = results[name]
            if result["retcode"] != RETCODE_SUCCESS:
                line["module"] = str(name)
                line["retcode"] = retcode_to_str(result["retcode"])
                line["status"] = "ERROR"
                if "start_time" in result:
                    line["start_time"] = result["start_time"]
            else:
                line = result
                line["module"] = name
                line["status"] = "PASSED" if result["passed"] else "FAILED"
                line["retcode"] = retcode_to_str(result["retcode"])
            repfile.write(
                (",".join([str(line[x]).replace(",", " ") for x in template.keys()])) + "\n"
            )
    _LOG.info("Testbench report (CSV format) written to " + args.report)
    return 0


def do_report_junit(args, results):
    """Generate JUnit report file (.xml)"""
    test_cases = []
    for name in sorted(results):
        result = results[name]
        if "wall_time" in result:
            h, m, s = map(int, result["wall_time"].split(":"))
            elapsed_sec = h * 3600 + m * 60 + s
        else:
            elapsed_sec = None
        test_case = TestCase(name=name, elapsed_sec=elapsed_sec)
        if result["retcode"] != RETCODE_SUCCESS:
            test_case.add_error_info(
                message=retcode_to_str(result["retcode"]), output=result["stdout"].decode()
            )
        test_cases.append(test_case)
    test_suite = TestSuite(name=f"{args.simulator} testbenches", test_cases=test_cases)
    with open(args.report, "w") as repfile:
        to_xml_report_file(repfile, [test_suite], prettyprint=True)
    _LOG.info("Testbench report (JUnit format) written to " + args.report)
    return 0


def do_report(args, results=None):
    """Generate report file (either .csv or .xml)"""

    def _load_results(args):
        results = {}
        excludes = read_excludes_file(args.excludes)
        for (name, path) in gather_target_sims(args.basedir, args.target, excludes):
            logpath = os.path.join(path, args.simulator + ".log")
            if os.path.isfile(logpath):
                with open(logpath, "rb") as logfile:
                    result = parse_output(logfile.read())
                    results[name] = result
        return results

    suffix = os.path.splitext(args.report)[1]
    if results is None:
        results = _load_results(args)
    if suffix == ".csv":
        return do_report_csv(args, results)
    elif suffix == ".xml":
        return do_report_junit(args, results)
    else:
        raise ValueError(f"Unsupported report suffix: {suffix}")


# Parse command line options
def get_options():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "-d",
        "--basedir",
        default=BASE_DIR,
        help="Base directory in which to search for testbenches",
    )
    parser.add_argument(
        "-s",
        "--simulator",
        default="xsim",
        help="Simulator make target (e.g., xsim, vsim, etc.)",
    )
    parser.add_argument(
        "-e",
        "--setupenv",
        default=None,
        help="Optional environment setup script to run for each TB",
    )
    parser.add_argument(
        "-r", "--report", default="testbench_report.csv", help="Name of the output report file"
    )
    parser.add_argument(
        "-x",
        "--excludes",
        default=None,
        help="Name of the excludes file. It contains all targets to exclude.",
    )
    parser.add_argument("-j", "--jobs", default=1, help="Number of parallel simulation jobs to run")
    parser.add_argument(
        "-l",
        "--logged",
        action="store_true",
        default=False,
        help="Output is logged, so don't show per-second timer",
    )
    parser.add_argument(
        "action",
        choices=["run", "cleanup", "list", "report"],
        default="list",
        help=(
            "Action to perform, which may be one of:\n"
            "  run:     Run the testbenches\n"
            "  cleanup: Run 'make cleanall' for each testbench\n"
            "  list:    List all the testbenches found\n"
            "  report:  Generate a report from the last run"
        ),
    )
    parser.add_argument(
        "target",
        nargs="*",
        default=".*",
        help="Space-separated list of regular expressions for the testbenches to run",
    )
    return parser.parse_args()


def main():
    args = get_options()
    actions = {
        "list": do_list,
        "run": do_run,
        "cleanup": do_cleanup,
        "report": do_report,
    }
    return actions[args.action](args)


if __name__ == "__main__":
    exit(main())
