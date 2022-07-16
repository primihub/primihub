from .zmq_channel import IOService, Session


class Consumer(object):

    def __init__(self, ip, port):
        """consumer init
            zmq PULL bind
        Args:
            ip (_type_): _description_
            port (_type_): _description_
        """
        ios = IOService()
        session = Session(ios, ip, port, "consumer")  # PULL bind
        self.channel = session.addChannel()

    async def recv(self):
        # return await self.channel.recv_multipart()
        return await self.channel.recv_json()
