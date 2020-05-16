# -*- coding: utf-8 -*-

# Source file created: 2019-03-25
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

import math
import os


class FilesystemCleanup:
    def __init__(self, rootdir):
        self.rootdir = rootdir
        self.files = None
        self.dirs = None

    def __enter__(self):
        self.files = list()
        self.dirs = list()
        return self

    def __exit__(self, type_, value, traceback):
        for f in self.files:
            os.unlink(os.path.join(self.rootdir, f))

        self.files = None

        self.dirs.reverse()
        for d in self.dirs:
            os.rmdir(os.path.join(self.rootdir, d))

        self.dirs = None

    def create_file(self, subpath, contents):
        with open(os.path.join(self.rootdir, subpath), 'w') as fileobj:
            fileobj.write(contents)

        if self.files is not None:
            self.files.append(os.path.join(subpath))

    def write_file_trunc(self, subpath, contents):
        if subpath not in self.files:
            raise Exception("Expected {} to be an existing file.".format(subpath))

        with open(os.path.join(self.rootdir, subpath), 'w') as fileobj:
            fileobj.write(contents)

    def create_dir(self, subpath):
        os.mkdir(os.path.join(self.rootdir, subpath))
        if self.dirs is not None:
            self.dirs.append(os.path.join(subpath))

    def rm_file(self, subpath):
        os.unlink(os.path.join(self.rootdir, subpath))
        self.files.remove(os.path.join(subpath))

    def rm_dir(self, subpath):
        os.rmdir(os.path.join(self.rootdir, subpath))
        self.dirs.remove(os.path.join(subpath))

    def mtime(self, subpath):
        stat = os.stat(os.path.join(self.rootdir, subpath))
        return math.floor(stat.st_mtime * 1000)
