# NOTE This test should run after bazel build :primihub_channel
import sys
from os import path

bazel_bin = path.abspath(path.join(path.dirname(__file__), "../../../bazel-bin"))
sys.path.insert(0, bazel_bin)
print(sys.path)

import primihub_channel  # this is pybinb11 warpper c++ library
import threading

ios = primihub_channel.IOService(0)
print(ios)

server = primihub_channel.Session(
    ios, "localhost:1213", primihub_channel.SessionMode.Server, "endpoint")
client = primihub_channel.Session(
    ios, "localhost:1213", primihub_channel.SessionMode.Client, "endpoint")

s_channel = server.addChannel("TestChannel", "TestChannel")
c_channel = client.addChannel("TestChannel", "TestChannel")


def send():
    s_channel.send("Hello World!")


def rcv():
    rcv_s = c_channel.recv()
    print("rcv:", rcv_s)


send_t = threading.Thread(target=send)
rcv_t = threading.Thread(target=rcv)
send_t.start()
rcv_t.start()

send_t.join()
rcv_t.join()
