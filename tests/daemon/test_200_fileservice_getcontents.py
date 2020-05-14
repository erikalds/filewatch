# -*- coding: utf-8 -*-

# Source file created: 2020-05-12
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
        self.stub = filewatch_pb2_grpc.FileStub(self.channel)

    def test_can_get_contents_of_file(self):
        filename = filewatch_pb2.Filename();
        filename.dirname.name = "."
        filename.name = "file1.txt"
        content = self.stub.GetContents(filename)
        self.assertEqual(content.dirname.name, ".")
        self.assertEqual(content.dirname.modification_time.epoch, fs.mtime("."))
        self.assertEqual(content.filename.name, "file1.txt")
        self.assertEqual(content.filename.modification_time.epoch,
                         fs.mtime("file1.txt"))
        self.assertEqual(['file1'], content.lines)

    def test_can_get_contents_of_modified_file(self):
        contents = """Lorem ipsum dolor sit amet,
consectetur adipiscing elit,
sed do eiusmod tempor incididunt
ut labore et dolore magna aliqua.
"""
        fs.write_file_trunc("file1.txt", contents)
        filename = filewatch_pb2.Filename();
        filename.dirname.name = "."
        filename.name = "file1.txt"
        content = self.stub.GetContents(filename)
        self.assertEqual(content.dirname.name, ".")
        self.assertEqual(content.dirname.modification_time.epoch, fs.mtime("."))
        self.assertEqual(content.filename.name, "file1.txt")
        self.assertEqual(content.filename.modification_time.epoch,
                         fs.mtime("file1.txt"))
        self.assertEqual(contents.splitlines() + [''], content.lines)

    def test_can_get_contents_of_file_in_other_directory(self):
        contents = "this is some\ncontents"
        fs.create_dir('a_dir')
        p = os.path.join('a_dir', 'filename.txt')
        fs.create_file(p, contents)
        filename = filewatch_pb2.Filename();
        filename.dirname.name = "a_dir"
        filename.name = "filename.txt"
        content = self.stub.GetContents(filename)
        self.assertEqual(content.dirname.name, "a_dir")
        self.assertEqual(content.dirname.modification_time.epoch,
                         fs.mtime("a_dir"))
        self.assertEqual(content.filename.name, "filename.txt")
        self.assertEqual(content.filename.modification_time.epoch, fs.mtime(p))
        self.assertEqual(contents.splitlines(), content.lines)

    def test_file_not_found(self):
        filename = filewatch_pb2.Filename()
        filename.dirname.name = "/dir"
        filename.name = "notexisting.txt"
        with self.assertRaisesRegex(Exception,
                                    "'/dir/notexisting.txt' does not exist."):
            content = self.stub.GetContents(filename)

    def test_not_a_file(self):
        fs.create_dir('dir')
        filename = filewatch_pb2.Filename()
        filename.dirname.name = "/"
        filename.name = "dir"
        with self.assertRaisesRegex(Exception,
                                    "'/dir' is not a file."):
            content = self.stub.GetContents(filename)


def run_test(tempdir):
    global fs
    with FilesystemCleanup(tempdir) as fs_:
        fs = fs_
        fs.create_file("file1.txt", "file1")

        runner = unittest.TextTestRunner(verbosity=2)
        result = runner.run(unittest.makeSuite(TestCase))
        return result.wasSuccessful()
