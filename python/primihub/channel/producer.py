from .zmq_channel import IOService, Session


class Producer(object):

    def __init__(self, ip, port):
        """producer init
            zmq PUSH connect
        Args:
            ip (_type_): _description_
            port (_type_): _description_
        """
        ios = IOService()
        session = Session(ios, ip, port, "producer")  # PUSH connect
        self.channel = session.addChannel()

    def send(self, msg):
        # self.channel.send_json(pickle.dumps(msg))
        self.channel.send_json(msg)
