# TODO Mock Session & Channel


session_map = dict()
endpoint_map = dict()

session_map["endpoint1"] = {"endpoint1": []}
session_map["endpoint2"] = {"endpoint2": []}
session_map["endpoint3"] = {"endpoint3": []}


class MockIOService:
    pass


class MockChannel:
    """ mock channel
    """

    def __init__(self, session):
        self.session = session.session
        self.endpoint = session.endpoint

    def send(self, data):
        print("send message: ", data)
        if data is not None:
            self.session[self.endpoint].append(data)
            print(self.session)

    def recv(self, data_size=0):
        message = self.session[self.endpoint]
        if len(message) > 0:
            return self.session[self.endpoint].pop()
        return None

    def close(self):
        print("channel close")
        self.session_map = {}


class MockSession:
    def __init__(self, io_service, address, session_mode, endpoint):
        self.io_server = io_service
        self.address = address
        self.session_mode = session_mode
        self.endpoint = endpoint
        self.session = session_map[endpoint]

        # print("session init: ", self.session)

    def addChannel(self, *args) -> MockChannel:
        # print(args)
        ch = MockChannel(self)
        return ch
