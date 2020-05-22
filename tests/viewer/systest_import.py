import os
import sys


thisdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.split(thisdir)[0]
sys.path.append(parentdir)
