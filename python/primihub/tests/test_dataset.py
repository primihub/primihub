import pytest
from os import path
import sys

import ctypes

from primihub import dataset as ds
import pybind11_example

here = path.abspath(path.dirname(__file__))

def test_read_csv_dataset():
    dr = ds.driver("csv")
    cursor = ds.driver("csv")().read(path=here+'/data/matrix.csv')
    d = cursor.read()
    print(type(d.as_arrow()))

    o = ctypes.py_object(d.as_arrow())
    print(type(o))
    pybind11_example.cpp_function(d.as_arrow())


if __name__ == "__main__":
    sys.exit(test_read_csv_dataset())