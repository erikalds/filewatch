# -*- coding: utf-8 -*-

# Source file created: 2019-07-27
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
import unittest

from filesys import FilesystemCleanup

import filewatch_pb2
import filewatch_pb2_grpc


fs = None


class TestCase(unittest.TestCase):
    def setUp(self):
        self.channel = grpc.insecure_channel('localhost:45678')
        self.stub = filewatch_pb2_grpc.DirectoryStub(self.channel)

    def listen_for_events(self, name_of_dir):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = name_of_dir
        direvt_generator = self.stub.ListenForEvents(dirname)
        direvt = next(direvt_generator)
        self.assertEqual(filewatch_pb2.DirectoryEvent.WATCHING_DIRECTORY, direvt.event)
        self.assertEqual(name_of_dir, direvt.name)
        self.assertEqual('.', direvt.dirname.name)
        return direvt_generator

    def test_DIRECTORY_ADDED_when_directory_is_added(self):
        direvt_generator = self.listen_for_events("/dir")

        fs.create_dir("dir/subdir")
        direvt = next(direvt_generator)

        self.assertEqual(filewatch_pb2.DirectoryEvent.DIRECTORY_ADDED, direvt.event)
        self.assertEqual('/dir', direvt.name)
        self.assertEqual('subdir', direvt.dirname.name)
        mtime = fs.mtime("dir/subdir")
        self.assertEqual(mtime, direvt.dirname.modification_time.epoch)
        self.assertEqual(mtime, direvt.modification_time.epoch)
        direvt_generator.cancel()

    def test_DIRECTORY_REMOVED_when_directory_is_removed(self):
        direvt_generator = self.listen_for_events("/otherdir")

        fs.create_dir("otherdir/somedir")
        direvt = next(direvt_generator)

        self.assertEqual(filewatch_pb2.DirectoryEvent.DIRECTORY_ADDED, direvt.event)
        fs.rm_dir("otherdir/somedir")

        direvt = next(direvt_generator)
        self.assertEqual(filewatch_pb2.DirectoryEvent.DIRECTORY_REMOVED, direvt.event)
        self.assertEqual('/otherdir', direvt.name)
        self.assertEqual('somedir', direvt.dirname.name)
        self.assertEqual(0, direvt.dirname.modification_time.epoch)
        self.assertEqual(0, direvt.modification_time.epoch)
        direvt_generator.cancel()

    def test_FILE_ADDED_when_file_is_added(self):
        direvt_generator = self.listen_for_events("/dir")

        fs.create_file("dir/file", "contents")
        direvt = next(direvt_generator)

        self.assertEqual(filewatch_pb2.DirectoryEvent.FILE_ADDED, direvt.event)
        self.assertEqual('/dir', direvt.name)
        self.assertEqual('file', direvt.dirname.name)
        mtime = fs.mtime("dir/file")
        self.assertEqual(mtime, direvt.dirname.modification_time.epoch)
        self.assertEqual(mtime, direvt.modification_time.epoch)
        direvt_generator.cancel()

    def test_FILE_REMOVED_when_file_is_removed(self):
        direvt_generator = self.listen_for_events("/otherdir")

        fs.create_file("otherdir/somefile", "contents")
        direvt = next(direvt_generator)

        self.assertEqual(filewatch_pb2.DirectoryEvent.FILE_ADDED, direvt.event)
        fs.rm_file("otherdir/somefile")

        direvt = next(direvt_generator)
        self.assertEqual(filewatch_pb2.DirectoryEvent.FILE_REMOVED, direvt.event)
        self.assertEqual('/otherdir', direvt.name)
        self.assertEqual('somefile', direvt.dirname.name)
        self.assertEqual(0, direvt.dirname.modification_time.epoch)
        self.assertEqual(0, direvt.modification_time.epoch)
        direvt_generator.cancel()


def run_test(tempdir):
    global fs
    with FilesystemCleanup(tempdir) as fs_:
        fs = fs_
        fs.create_dir("dir")
        fs.create_dir("otherdir")

        runner = unittest.TextTestRunner()
        result = runner.run(unittest.makeSuite(TestCase))
        return result.wasSuccessful()
