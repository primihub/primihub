import pickle
import linkcontext


class Test:

    def __init__(self, t=1, a=2):
        self.t = t
        self.a = a
        Test.add_one(self)

    @staticmethod
    def add_one(self):
        self.t = self.t + 1
        self.a = self.a + 1


def Node(ip, port, use_tls, nodename="default"):
    return linkcontext.Node(ip, port, use_tls, nodename)


class GrpcServers:

    def __init__(self,
                 local_role: str = "",
                 remote_roles=[],
                 party_info=dict(),
                 task_info=dict()) -> None:
        self.local_role = local_role
        self.remote_roles = remote_roles
        self.party_info = party_info
        self.task_info = task_info
        self.link_context = linkcontext.LinkFactory.createLinkContext(
            linkcontext.LinkMode.GRPC)
        self.link_context.setTaskInfo(task_info.task_id, task_info.job_id,
                                      task_info.request_id)
        GrpcServers.build_connector(self)

    @staticmethod
    def build_connector(self):
        local_ip = self.party_info[self.local_role].ip
        local_port = self.party_info[self.local_role].port
        local_session = Node(local_ip, int(local_port), False, self.local_role)
        self.recv_channel = self.link_context.getChannel(local_session)

        self.send_channels = {}
        for role in self.remote_roles:
            self.send_channels[role] = self.link_context.getChannel(
                Node(self.party_info[role].ip, int(self.party_info[role].port),
                     False, role))

    def sender(self, key, val, dest_role=None):
        if dest_role is None:
            for role, send_channel in self.send_channels.items():
                print("sending message: ", self.local_role + "_" + key)
                send_channel.send(self.local_role + "_" + key,
                                  pickle.dumps(val))
        else:
            for tmp_role in dest_role:
                print("sending message: ", self.local_role + "_" + key)
                tmp_channel = self.send_channels.get(tmp_role, None)
                if tmp_channel is not None:
                    tmp_channel.send(self.local_role + "_" + key,
                                     pickle.dumps(val))

    def recv(self, key, source_role=None):
        results = {}
        if source_role is None:
            for tmp_role in self.remote_roles:
                print("receiving message: ", tmp_role + "_" + key)
                results[tmp_role] = pickle.loads(
                    self.recv_channel.recv(tmp_role + "_" + key))

        else:
            for tmp_role in source_role:
                print("receiving message: ", tmp_role + "_" + key)
                results[tmp_role] = pickle.loads(
                    self.recv_channel.recv(tmp_role + "_" + key))

        return results


class GrpcServer:

    def __init__(self, local_party, remote_party, party_access_info,
                 task_info) -> None:
        remote_ip = party_access_info[remote_party].ip
        local_ip = party_access_info[local_party].ip
        remote_port = party_access_info[remote_party].port
        local_port = party_access_info[local_party].port
        send_session = Node(remote_ip, int(remote_port), False, remote_party)
        recv_session = Node(local_ip, int(local_port), False, local_party)

        self.init_link_context(party_access_info[remote_party], task_info)

        self.send_channel = self.link_context.getChannel(send_session)
        self.recv_channel = self.link_context.getChannel(recv_session)

    def sender(self, key, val):
        print(f"Start to send key: {key}")
        self.send_channel.send(key, pickle.dumps(val))
        print("End send")

    def recv(self, key):
        print(f"Start to receive key: {key}")
        recv_val = self.recv_channel.recv(key)
        print("End receive")
        return pickle.loads(recv_val)

    def init_link_context(self, local_party_access_info, task_info):
        use_tls = local_party_access_info.use_tls
        self.link_context = linkcontext.LinkFactory.createLinkContext(
            linkcontext.LinkMode.GRPC)
        print(task_info['task_id'], task_info['job_id'],
              task_info['request_id'])
        self.link_context.setTaskInfo(task_info['task_id'], task_info['job_id'],
                                      task_info['request_id'])

        #not used at this moment
        '''
        if use_tls:
            # load certificate
            cert_confg = config_node.get("certificate", {})
            if cert_confg:
                ca_file = cert_confg["root_ca"]
                key_file = cert_confg["key"]
                cert_file = cert_confg["cert"]
                logger.debug(ca_file)
                logger.debug(key_file)
                logger.debug(cert_file)
                self.link_context.initCertificate(ca_file, key_file, cert_file)
        '''