import pickle

import zmq


class IOService:
    pass


class Channel:
    """channel
    """

    def __init__(self, session):
        self.socket = session.socket

    def send(self, data):
        print("send message: ", data)
        if data is not None:
            self.socket.send(pickle.dumps(data))

    def recv(self):
        print("wait for recv ...")
        message = self.socket.recv()
        return pickle.loads(message)


class Session:
    def __init__(self, io_service, ip, port, session_mode):
        self.io_server = io_service
        self.ip = ip
        self.port = port
        self.session_mode = session_mode

    def addChannel(self, *args) -> Channel:
        context = zmq.Context()
        if self.session_mode == "server":
            socket = context.socket(zmq.REP)
            socket.bind("tcp://{}:{}".format(self.ip, self.port))
        elif self.session_mode == "client":
            socket = context.socket(zmq.REQ)
            socket.connect("tcp://{}:{}".format(self.ip, self.port))
        self.socket = socket
        return Channel(self)
