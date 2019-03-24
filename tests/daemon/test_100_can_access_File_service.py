# -*- coding: utf-8 -*-

# Source file created: 2019-03-24
#
# poglw - Pure OpenGL Widgets
# Copyright (C) 2017 Erik Åldstedt Sund
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

import filewatch_pb2
import filewatch_pb2_grpc

def run_test():
    channel = grpc.insecure_channel('localhost:45678')
    stub = filewatch_pb2_grpc.FileStub(channel)
    fname = filewatch_pb2.Filename()
    fname.dirname.name = "."
    fname.name = "test.txt"
    filecontent = stub.GetContents(fname)
