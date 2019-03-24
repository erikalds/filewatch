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

import os
import re
import subprocess
import sys


def is_sorted(list_):
    if not list_ or len(list_) == 1:
        return True

    for i in range(1, len(list_)):
        if list_[i - 1] > list[i]:
            return False

    return True


def group_tests(testlist):
    groups = dict()
    for test in testlist:
        mo = re.match(".*?test_([0-9]).*", test)
        if not mo:
            print("WARNING: Ignoring test %s." % test)
            continue

        group = int(mo.group(1))
        if group not in groups:
            groups[group] = list()

        groups[group].append(os.path.split(test)[-1])

    return groups


class RunningProcess:
    def __init__(self, cmd):
        self.cmd = cmd
        self.proc = None

    def __enter__(self):
        self.proc = subprocess.Popen(self.cmd)

    def __exit__(self, type_, value, traceback):
        optional_exit_code = self.proc.poll()
        procname = self.cmd.split()[0]
        if optional_exit_code is not None:
            errtext = "%s exited prematurely with exit code: %d"
            raise Exception(errtext % (procname, optional_exit_code))

        print("Terminating %s" % procname)
        self.proc.terminate()
        try:
            exit_code = self.proc.wait(3)
        except subprocess.TimeoutExpired:
            print("%s did not die in 3 seconds, killing..." % procname)
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


def main(argv):
    with run_process(argv[1]) as p:
        all_tests = group_tests(argv[2:])
        keys = all_tests.keys()
        assert(is_sorted(keys))
        for group in keys:
            failures = 0
            for test in all_tests[group]:
                print("Running test %s..." % test)
                testmod = os.path.splitext(test)[0]
                try:
                    exec("import %s" % testmod)
                    eval("%s.run_test()" % testmod)
                    print("test %s succeeded" % test)
                except Exception as e:
                    import traceback
                    traceback.print_exc()
                    failures += 1
                    print("test %s failed" % test)

            if failures > 0:
                print("%d tests failed in group %d. Bailing out." % (failures,
                                                                     group))
                return failures

    return 0


if __name__ == '__main__':
    assert(sys.version_info.major >= 3)
    sys.exit(main(sys.argv))
