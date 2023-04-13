import pickle
import linkcontext

def Node(ip, port, use_tls, nodename = "default"):
    return linkcontext.Node(ip, port, use_tls, nodename)

def init_link_context(local_party_access_info, task_info):

    use_tls = local_party_access_info.use_tls
    link_context = linkcontext.LinkFactory.createLinkContext(linkcontext.LinkMode.GRPC)
    #link_context.setTaskInfo(task_info['task_id'].encode(), task_info['job_id'].encode(), task_info['request_id'].encode())
    link_context.setTaskInfo("abc","abc","abc")
    return link_context
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
class GrpcServer:

    def __init__(self, local_party, remote_party, party_access_info, task_info) -> None:
        remote_ip = party_access_info[remote_party].ip
        local_ip = party_access_info[local_party].ip
        remote_port = party_access_info[remote_party].port
        local_port = party_access_info[local_party].port

        send_session = Node(remote_ip, int(remote_port), False, remote_party)
        recv_session = Node(local_ip, int(local_port), False, local_party)

        self.send_channel = init_link_context(party_access_info[remote_party], task_info).getChannel(send_session)
        self.recv_channel = init_link_context(party_access_info[local_party],task_info).getChannel(recv_session)

    def sender(self, key, val):
        self.send_channel.send(key, pickle.dumps(val))

    def recv(self, key):
        recv_val = self.recv_channel.recv(key)
        return pickle.loads(recv_val)