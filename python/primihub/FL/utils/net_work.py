import pickle
import linkcontext
from primihub.utils.logger_util import logger
from primihub.context import Context

def Node(ip, port, use_tls, nodename="default"):
    return linkcontext.Node(ip, port, use_tls, nodename)


class GrpcClient:

    def __init__(self, local_party, remote_party,
                 node_info, task_info) -> None:
        self.local_party = local_party
        self.remote_party = remote_party

        self.link_context = linkcontext.LinkFactory.createLinkContext(
            linkcontext.LinkMode.GRPC)
        self.link_context.setTaskInfo(task_info.task_id,
                                      task_info.job_id,
                                      task_info.request_id,
                                      task_info.sub_task_id)

        local_ip = node_info[local_party].ip
        local_port = node_info[local_party].port
        use_tls = node_info[local_party].use_tls
        logger.info(f"use_tls: {use_tls}")
        if use_tls:
            cert_conf = Context.cert_config
            root_ca_path = cert_conf.get("root_ca_path", "")
            key_path = cert_conf.get("key_path", "")
            cert_path = cert_conf.get("cert_path", "")
            logger.info(f"use_tls: {use_tls} root_ca_path: {root_ca_path}"
                        f" key_path: {key_path} cert_path: {cert_path}")
            self.link_context.initCertificate(root_ca_path, key_path, cert_path)
        recv_session = Node(local_ip, int(local_port), use_tls, local_party)
        self.recv_channel = self.link_context.getChannel(recv_session)

        remote_ip = node_info[remote_party].ip
        remote_port = node_info[remote_party].port
        use_tls = node_info[remote_party].use_tls
        send_session = Node(remote_ip, int(remote_port), use_tls, remote_party)
        self.send_channel = self.link_context.getChannel(send_session)

    def send(self, key, val):
        key = self.local_party + '_' + key
        logger.info(f"Start send {key} to {self.remote_party}")
        self.send_channel.send(key, pickle.dumps(val))
        logger.info(f"End send {key} to {self.remote_party}")

    def recv(self, key):
        key = self.remote_party + '_' + key
        logger.info(f"Start receive {key}")
        val = self.recv_channel.recv(key)
        logger.info(f"End receive {key}")
        return pickle.loads(val)


class MultiGrpcClients:

    def __init__(self, local_party, remote_parties,
                 node_info, task_info) -> None:
        self.Clients = {}
        for remote_party in remote_parties:
            client = GrpcClient(local_party, remote_party,
                                node_info, task_info)
            self.Clients[remote_party] = client

    def send_all(self, key, val):
        logger.info("Start send all")
        for client in self.Clients.values():
            client.send(key, val)
        logger.info("End send all")

    def send_selected(self, key, val, selected_remote):
        logger.info(f"Start send to {selected_remote}")
        for remote_party in selected_remote:
            client = self.Clients[remote_party]
            client.send(key, val)
        logger.info(f"End send to {selected_remote}")

    def send_seperately(self, key, valList):
        assert len(valList) == len(self.Clients)
        i = 0
        logger.info(f"Start send separately")
        for client in self.Clients.values():
            client.send(key, valList[i])
            i += 1
        logger.info(f"End send separately")

    def recv_all(self, key):
        logger.info("Start receive all")
        result = []
        for client in self.Clients.values():
            result.append(client.recv(key))
        logger.info("End receive all")
        return result

    def recv_selected(self, key, selected_remote):
        logger.info(f"Start receive {selected_remote}")
        result = []
        for remote_party in selected_remote:
            client = self.Clients[remote_party]
            result.append(client.recv(key))
        logger.info(f"End receive {selected_remote}")
        return result