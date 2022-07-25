import pickle
import time
import zmq
import zmq.asyncio


class IOService:
    pass


class Channel:
    """channel
    """

    def __init__(self, session):
        self.socket = session.socket

    def send(self, data):
        self.socket.send(pickle.dumps(data))

    def recv(self, block=True):
        if block is True:
            message = self.socket.recv()
            return pickle.loads(message)
        else:
            # Don't remove unblock recv, it's used for 
            # ClientChannelProxy and ServerChannelProxy.
            try:
                message = self.socket.recv(flags=zmq.NOBLOCK)
                return pickle.loads(message)
            except zmq.Again as e:
                time.sleep(0.1)
            except Exception as e:
                raise e

    def send_json(self, data):
        self.socket.send_json(data)

    def recv_json(self):
        return self.socket.recv_json()


class Session:
    def __init__(self, io_service, ip, port, session_mode):
        self.io_server = io_service
        self.ip = ip
        self.port = port
        self.session_mode = session_mode

    def addChannel(self, *args) -> Channel:
        context = zmq.Context()
        ctx_async = zmq.asyncio.Context()
        # rep
        if self.session_mode == "server":
            socket = context.socket(zmq.REP)
            socket.bind("tcp://{}:{}".format(self.ip, self.port))
        # req
        elif self.session_mode == "client":
            socket = context.socket(zmq.REQ)
            socket.connect("tcp://{}:{}".format(self.ip, self.port))

        # push
        elif self.session_mode == "producer":
            socket = context.socket(zmq.PUSH)
            socket.connect("tcp://{}:{}".format(self.ip, self.port))
        # pull
        elif self.session_mode == "consumer":
            socket = ctx_async.socket(zmq.PULL)
            socket.bind("tcp://{}:{}".format(self.ip, self.port))

        self.socket = socket
        print(self.socket, self.ip, self.port)
        return Channel(self)
