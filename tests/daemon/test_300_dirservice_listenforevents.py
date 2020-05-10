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


class Connectivity:
    def __init__(self, channel, try_to_connect=False):
        self._channel = channel
        self._connected = False
        self._try_to_connect = try_to_connect

    def __call__(self, connectivity):
        print("gRPC channel -> {}".format(connectivity))
        if connectivity == grpc.ChannelConnectivity.READY:
            self._connected = True

    def __enter__(self):
        self._channel.subscribe(self, try_to_connect=self._try_to_connect)
        return self

    def __exit__(self, type, value, traceback):
        self._channel.unsubscribe(self)

    def wait_connected(self):
        print("wait for connection...")
        while not self._connected:
            import time
            time.sleep(0)
        print("connected")


class TestCase(unittest.TestCase):
    def setUp(self):
        self.channel = grpc.insecure_channel('localhost:45678')
        self.stub = filewatch_pb2_grpc.DirectoryStub(self.channel)
        with Connectivity(self.channel, try_to_connect=True) as connectivity:
            connectivity.wait_connected()


    def test_DIRECTORY_ADDED_when_directory_is_added(self):
        dirname = filewatch_pb2.Directoryname()
        dirname.name = "/dir"
        direvt_generator = self.stub.ListenForEvents(dirname)

        fs.create_dir("dir/subdir")
        direvt = next(direvt_generator)

        self.assertEqual(filewatch_pb2.DirectoryEvent.DIRECTORY_ADDED, direvt.event)
        self.assertEqual('/dir', direvt.name)
        self.assertEqual('subdir', direvt.dirname.name)
        mtime = fs.mtime("dir/subdir")
        self.assertEqual(mtime, direvt.dirname.modification_time.epoch)
        self.assertEqual(mtime, direvt.modification_time.epoch)
        direvt_generator.cancel()


def run_test(tempdir):
    global fs
    with FilesystemCleanup(tempdir) as fs_:
        fs = fs_
        fs.create_dir("dir")

        runner = unittest.TextTestRunner()
        result = runner.run(unittest.makeSuite(TestCase))
        return result.wasSuccessful()
