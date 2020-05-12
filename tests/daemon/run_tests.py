# -*- coding: utf-8 -*-

# Source file created: 2019-03-23
#
# filewatch - File watcher utility
# Copyright (C) 2019 Erik Åldstedt Sund
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
# To contact the author, e-mail at erikalds@gmail.com or through
# regular mail:
#   Erik Åldstedt Sund
#   Darres veg 14
#   NO-7540 KLÆBU
#   NORWAY

import logging
import os
import re
import subprocess
import sys
import tempfile
import time


def is_sorted(list_):
    if not list_ or len(list_) == 1:
        return True

    for i in range(1, len(list_)):
        if list_[i - 1] > list_[i]:
            return False

    return True


def group_tests(testlist):
    groups = dict()
    for test in testlist:
        mo = re.match(".*?test_([0-9]).*", test)
        if not mo:
            logging.warning("Ignoring test %s." % test)
            continue

        group = int(mo.group(1))
        if group not in groups:
            groups[group] = list()

        groups[group].append(os.path.split(test)[-1])

    return groups


class TemporaryDir:
    def __init__(self, containing_dir):
        self.containing_dir = containing_dir
        self.path = None

    def __enter__(self):
        self.path = tempfile.mkdtemp(dir=self.containing_dir)
        logging.info("Created tempdir: %s" % self.path)
        return self

    def __exit__(self, type_, value, traceback):
        os.rmdir(self.path)
        logging.info("Deleted tempdir: %s" % self.path)


def create_tempdir(containing_dir):
    return TemporaryDir(containing_dir)


class RunningProcess:
    def __init__(self, cmd):
        self.cmd = cmd
        self.proc = None

    def __enter__(self):
        self.proc = subprocess.Popen(self.cmd)
        return self.proc

    def __exit__(self, type_, value, traceback):
        optional_exit_code = self.proc.poll()
        procname = self.cmd[0].split()[0]
        if optional_exit_code is not None:
            errtext = "%s exited prematurely with exit code: %d"
            raise Exception(errtext % (procname, optional_exit_code))

        logging.info("Terminating %s" % procname)
        self.proc.terminate()
        try:
            exit_code = self.proc.wait(3)
        except subprocess.TimeoutExpired:
            logging.error("%s did not die in 3 seconds, killing..." % procname)
            self.proc.kill()
            try:
                exit_code = self.proc.wait(5)
            except subprocess.TimeoutExpired:
                errtext = "WARNING: %s did not die in 5 seconds.\n" % procname
                errtext += "You should clean up manually."
                raise Exception(errtext)

        if exit_code != 0:
            raise Exception("%s returned exit code: %d" % (procname, exit_code))


def run_process(cmd):
    return RunningProcess(cmd)


class CapturingStreamer:
    def __init__(self, file_, stream):
        self._file = file_
        self._stream = stream

    def flush(self):
        self._file.flush()
        self._stream.flush()

    def write(self, text):
        self._file.write(text)
        self._stream.write(text)


class TeeOutput:
    def __init__(self, filename):
        self._file = open(filename, 'w')
        self._ostreams = [CapturingStreamer(self._file, sys.stdout),
                          CapturingStreamer(self._file, sys.stderr)]
        sys.stdout = self._ostreams[0]
        sys.stderr = self._ostreams[1]


def simple_start_stop_test(fwdaemon_path, fwdaemon_args):
    logging.info("Running simple start/stop test...")
    try:
        with run_process([fwdaemon_path] + fwdaemon_args) as p:
            try:
                p.wait(timeout=1) # should be able to run for 1 second
                logging.error("%s failed with return code %d within 1 second"
                              % (fwdaemon_path, p.poll()))
            except subprocess.TimeoutExpired:
                pass # expected to time out

        logging.info("Simple start/stop test PASSED")
        return 0
    except Exception as e:
        logging.error("Simple start/stop test FAILED")
        logging.error(str(e))
        return 1


def handle_args(argv):
    objs = list()
    if '--tee-output' in argv:
        idx = argv.index('--tee-output')
        argv.pop(idx)
        objs.append(TeeOutput(argv.pop(idx)))

    if '--log-debug' in argv:
        objs.append('debug')

    return objs


def main(argv):
    logging.basicConfig(level=logging.INFO,
                        format="[%(asctime)s] [%(levelname)s] [systest] %(message)s")
    objs = handle_args(argv)
    starttime = time.time()
    failures = 0
    try:
        with create_tempdir(".") as tempdir:
            fwdaemon_args = [tempdir.path]
            if 'debug' in objs:
                fwdaemon_args.append('--log-level=debug')
            failures = simple_start_stop_test(argv[1], fwdaemon_args)
            with run_process([argv[1]] + fwdaemon_args) as p:
                all_tests = group_tests(argv[2:])
                keys = list(all_tests.keys())
                keys.sort()
                assert(is_sorted(keys))
                for group in keys:
                    for test in all_tests[group]:
                        logging.info("Running test %s..." % test)
                        testmod = os.path.splitext(test)[0]
                        try:
                            exec("import %s" % testmod)
                            retval = eval("%s.run_test(tempdir.path)" % testmod)
                            if retval is not None and retval is False:
                                logging.error("test %s failed" % test)
                                failures += 1
                            else:
                                logging.info("test %s succeeded" % test)
                        except:
                            import traceback
                            traceback.print_exc()
                            failures += 1
                            logging.error("test %s failed" % test)

                    if failures > 0:
                        logging.error("%d tests failed in group %d. Bailing out."
                                      % (failures, group))
                        break
    except Exception as e:
        logging.error(str(e))
        failures += 1

    logging.info("Tests ran for %f s" % (time.time() - starttime))

    return failures


if __name__ == '__main__':
    assert(sys.version_info.major >= 3)
    sys.exit(main(sys.argv))
