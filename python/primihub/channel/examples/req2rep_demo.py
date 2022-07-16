import sys
import threading
from os import path

sys.path.append(path.abspath(path.join(path.dirname(__file__), "../")))

from channel.zmq_channel import IOService, Session

if __name__ == '__main__':
    ios = IOService()
    print(ios)

    server = Session(ios, "*", "5555", "server")
    client = Session(ios, "localhost", "5555", "client")

    s_channel = server.addChannel()
    c_channel = client.addChannel()


    def client():
        for i in range(7, 9):
            data = "clent: %d" % i
            # print(data)
            c_channel.send(data)
            message = c_channel.recv()
            print("Received reply: ", message)
        sys.exit()


    def server():
        for i in range(3):
            try:
                print("wait for client ...")
                message = s_channel.recv()
                print("message from client:", message)
                s_channel.send("server: %d" % i)

            except Exception as e:
                print('异常:', e)
        sys.exit()


    send_t = threading.Thread(target=client)
    rcv_t = threading.Thread(target=server)
    send_t.start()
    rcv_t.start()

    send_t.join()
    rcv_t.join()
