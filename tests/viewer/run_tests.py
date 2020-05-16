# -*- coding: utf-8 -*-

# Source file created: 2020-05-15
#
# filewatch - File watcher utility
# Copyright (C) 2020 Erik Åldstedt Sund
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
import sys
import time

import systest_import
from systest.common_tests import simple_start_stop_test
from systest.runningprocess import run_process
from systest.teeoutput import TeeOutput
from systest.temporarydir import create_tempdir
from systest.runtests import run_tests_grouped


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
            filewatcher_args = []
            if 'debug' in objs:
                filewatcher_args.append('--log-level=debug')
            failures = simple_start_stop_test(argv[2], filewatcher_args)
            with run_process([argv[1]] + fwdaemon_args) as p:
                with run_process([argv[2]] + filewatcher_args) as pv:
                    failures = run_tests_grouped(argv[3:], tempdir, failures)

    except Exception as e:
        logging.error(str(e))
        failures += 1

    logging.info("Tests ran for %f s" % (time.time() - starttime))

    return failures


if __name__ == '__main__':
    assert(sys.version_info.major >= 3)
    sys.exit(main(sys.argv))
