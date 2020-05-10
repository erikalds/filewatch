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

import grpc
import os
import unittest

import filewatch_pb2
import filewatch_pb2_grpc

from filesys import FilesystemCleanup


fs = None


class TestCase(unittest.TestCase):
    def setUp(self):
        self.channel = grpc.insecure_channel('localhost:45678')
        self.stub = filewatch_pb2_grpc.DirectoryStub(self.channel)

    def test_dirs_in_root_dir(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/"
        dirlist = self.stub.ListDirectories(dirname)
        self.assertEquals("/", dirlist.name.name)
        self.assertEquals(set(["dir1", "dir2"]),
                          set([dname.name for dname in dirlist.dirnames]))
        self.assertEquals(2, len(dirlist.dirnames))

    def test_dirs_in_sub_dir(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/dir1"
        dirlist = self.stub.ListDirectories(dirname)
        self.assertEquals("/dir1", dirlist.name.name)
        self.assertEquals(3, len(dirlist.dirnames))
        self.assertEquals(set(["dir3", "dir4", "dir5"]),
                          set([dname.name for dname in dirlist.dirnames]))

    def test_modification_times(self):
        global fs
        dirs = ["dir1", "dir2", os.path.join("dir1", "dir3"),
                 os.path.join("dir1", "dir4"),
                 os.path.join("dir1", "dir5")]
        expected = { name : fs.mtime(name) for name in dirs }
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/"
        dirlist = self.stub.ListDirectories(dirname)
        self.assertEquals(dirlist.name.modification_time.epoch,
                          fs.mtime("."))
        for dname in dirlist.dirnames:
            self.assertEquals(expected[dname.name],
                              dname.modification_time.epoch)

        dirname.name = "/dir1"
        dirlist = self.stub.ListDirectories(dirname)
        self.assertEquals(dirlist.name.modification_time.epoch,
                          fs.mtime("dir1"))
        for dname in dirlist.dirnames:
            self.assertEquals(expected[os.path.join("dir1", dname.name)],
                              dname.modification_time.epoch)

    def test_non_existing_directory(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/not_existing_dir"
        with self.assertRaisesRegex(Exception,
                                    "'/not_existing_dir' does not exist") as cm:
            dirlist = self.stub.ListDirectories(dirname)

        self.assertEquals(grpc.StatusCode.NOT_FOUND, cm.exception.code())

    def test_error_on_existing_file(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/file1.txt"
        with self.assertRaisesRegex(Exception,
                                    "'/file1.txt' is not a directory") as cm:
            dirlist = self.stub.ListDirectories(dirname)

        self.assertEquals(grpc.StatusCode.NOT_FOUND, cm.exception.code())


def run_test(tempdir):
    global fs
    with FilesystemCleanup(tempdir) as fs_:
        fs = fs_
        fs.create_file("file1.txt", "file1")
        fs.create_file("file2.txt", "file2")
        fs.create_dir("dir1")
        fs.create_dir("dir2")
        fs.create_file(os.path.join("dir1", "file3.txt"), "file3")
        fs.create_file(os.path.join("dir1", "file4.txt"), "file4")
        fs.create_file(os.path.join("dir1", "file5.txt"), "file5")
        fs.create_dir(os.path.join("dir1", "dir3"))
        fs.create_dir(os.path.join("dir1", "dir4"))
        fs.create_dir(os.path.join("dir1", "dir5"))

        runner = unittest.TextTestRunner()
        result = runner.run(unittest.makeSuite(TestCase))
        return result.wasSuccessful()
