# -*- coding: utf-8 -*-

# Source file created: 2019-03-24
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
import math
import os
import unittest

from filesys import FilesystemCleanup

import filewatch_pb2
import filewatch_pb2_grpc


fs = None


class TestCase(unittest.TestCase):
    def setUp(self):
        self.channel = grpc.insecure_channel('localhost:45678')
        self.stub = filewatch_pb2_grpc.DirectoryStub(self.channel)

    def test_lists_files_in_root_dir(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/"
        filelist = self.stub.ListFiles(dirname)
        self.assertEquals("/", filelist.name.name)
        self.assertEquals(2, len(filelist.filenames))
        for fname in filelist.filenames:
            self.assertEquals("/", fname.dirname.name)
        self.assertEquals(set(["file1.txt", "file2.txt"]),
                          set([fname.name for fname in filelist.filenames]))

    def test_lists_files_in_sub_dir(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/dir1"
        filelist = self.stub.ListFiles(dirname)
        self.assertEquals("/dir1", filelist.name.name)
        self.assertEquals(3, len(filelist.filenames))
        for fname in filelist.filenames:
            self.assertEquals("/dir1", fname.dirname.name)

        self.assertEquals(set(["file3.txt", "file4.txt", "file5.txt"]),
                          set([fname.name for fname in filelist.filenames]))

    def test_lists_files_modification_time(self):
        global fs
        files = ["file1.txt", "file2.txt", os.path.join("dir1", "file3.txt"),
                 os.path.join("dir1", "file4.txt"),
                 os.path.join("dir1", "file5.txt")]
        expected = { name : fs.mtime(name) for name in files }
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/"
        filelist = self.stub.ListFiles(dirname)
        self.assertEquals(filelist.name.modification_time.epoch,
                          fs.mtime("."))
        for fname in filelist.filenames:
            self.assertEquals(expected[fname.name],
                              fname.modification_time.epoch)

        dirname.name = "/dir1"
        filelist = self.stub.ListFiles(dirname)
        self.assertEquals(filelist.name.modification_time.epoch,
                          fs.mtime("dir1"))
        for fname in filelist.filenames:
            self.assertEquals(expected[os.path.join("dir1", fname.name)],
                             fname.modification_time.epoch)

    def test_fails_when_directory_does_not_exist(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/not_existing_dir"
        with self.assertRaisesRegex(Exception,
                                    "'/not_existing_dir' does not exist") as cm:
            dirlist = self.stub.ListFiles(dirname)

        self.assertEquals(grpc.StatusCode.NOT_FOUND, cm.exception.code())

    def test_error_on_existing_file(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/file1.txt"
        with self.assertRaisesRegex(Exception,
                                    "'/file1.txt' is not a directory") as cm:
            dirlist = self.stub.ListFiles(dirname)

        self.assertEquals(grpc.StatusCode.NOT_FOUND, cm.exception.code())


def run_test(tempdir):
    global fs
    with FilesystemCleanup(tempdir) as fs_:
        fs = fs_
        fs.create_file("file1.txt", "file1")
        fs.create_file("file2.txt", "file2")
        fs.create_dir("dir1")
        fs.create_file(os.path.join("dir1", "file3.txt"), "file3")
        fs.create_file(os.path.join("dir1", "file4.txt"), "file4")
        fs.create_file(os.path.join("dir1", "file5.txt"), "file5")

        runner = unittest.TextTestRunner()
        result = runner.run(unittest.makeSuite(TestCase))
        return result.wasSuccessful()
