import pickle

import redis


class IOService:
    pass


class Channel:
    """channel
    """

    def __init__(self, session):
        self.channel_name = session.socket.name
        self.socket = session.socket
        self.pubsub = None

    def pub(self, data):
        # self.socket.publish(self.channel_name, pickle.dumps(data))
        self.socket.publish(self.channel_name, data)

    def sub(self):
        if self.pubsub is None:
            self.pubsub = self.socket.pubsub()
            self.pubsub.subscribe(self.channel_name)

        return self.listen()

    def listen(self):
        for item in self.pubsub.listen():
            # if item["type"] == "message":
            #     msg = {str(item["channel"], encoding="utf-8"): str(item["data"], encoding="utf-8")}
            # elif item["type"] == "subscribe":
            #     msg = {str(item["channel"], encoding="utf-8"): 'Subscribe successful'}
            yield item


class Session:
    def __init__(self, io_service, ip='192.168.99.31', port='15550', session_mode=None):
        self.io_server = io_service
        self.ip = ip
        self.port = port
        self.session_mode = session_mode

        self.socket = redis.StrictRedis(host=ip, port=int(port), db=3, password='primihub')

    def addChannel(self, name, *args) -> Channel:
        self.socket.name = name

        return Channel(self)


if __name__ == "__main__":
    ios = IOService()
    server = Session(ios, ip='192.168.99.31', port='15550', session_mode=None)
    channel = server.addChannel("int_channel")

    # channel.pub("your message.")  # pub

    msg = channel.sub()  # sub

    while True:
        data = next(msg)
        print(type(data), data)
