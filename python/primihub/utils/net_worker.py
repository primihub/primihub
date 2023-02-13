import pickle


class GrpcServer:

    def __init__(self, remote_ip, local_ip, remote_port, local_port,
                 context) -> None:
        # self.remote_ip = remote_ip
        # self.local_ip = local_ip
        # self.remote_port = int(remote_port)
        # self.local_port = int(local_port)
        # self.connector = context.get_link_conext()
        send_session = context.Node(remote_ip, int(remote_port), False)
        recv_session = context.Node(local_ip, int(local_port), False)

        self.send_channel = context.get_link_conext().getChannel(send_session)
        self.recv_channel = context.get_link_conext().getChannel(recv_session)

    def sender(self, key, val):
        # connector = self.connector.get_link_conext()
        # node = self.connector.Node(self.remote_ip, self.remote_port, False)
        # channel = connector.getChannel(node)
        self.send_channel.send(key, pickle.dumps(val))

    def recv(self, key):
        # connector = self.connector.get_link_conext()
        # node = self.connector.Node(self.local_ip, self.local_port, False)
        # channle = connector.getChannel(node)
        recv_val = self.recv_channel.recv(key)
        return pickle.loads(recv_val)