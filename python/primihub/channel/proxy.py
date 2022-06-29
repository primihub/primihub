import logging
import time
from concurrent.futures import ThreadPoolExecutor
from primihub.channel.zmq_channel import IOService, Session

LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
logging.basicConfig(level=logging.DEBUG,
                    format=LOG_FORMAT, datefmt=DATE_FORMAT)
logger = logging.getLogger("proxy")


class ClientChannelProxy:
    def __init__(self, host, port):
        self.ios_ = IOService()
        self.sess_ = Session(self.ios_, host, port, "client")
        self.chann_ = self.sess_.addChannel()
        self.executor_ = ThreadPoolExecutor()
    
    # Send val and it's tag to server side, server 
    # has cached val when the method return. 
    def Remote(self, val, tag):
        msg = {"v": val, "tag": tag}
        self.chann_.send(msg)
        _ = self.chann_.recv()
    
    # Send val and it's tag to server side, client begin the send action
    # in a thread when the the method reutrn but not ensure that server
    # has cached this val. Use 'fut.result()' to wait for server to cache it,
    # this makes send value and other action running in the same time.
    def RemoteAsync(self, val, tag):
        def send_fn(channel, msg):
            channel.send(msg)
            _ = channel.recv()

        msg = {"v": val, "tag": tag}
        fut = self.executor_.submit(send_fn, (self.chann_, msg))

        return fut


class ServerChannelProxy:
    def __init__(self, port):
        self.ios_ = IOService()
        self.sess_ = Session(self.ios_, "*", port, "server")
        self.chann_ = self.sess_.addChannel()
        self.executor_ = ThreadPoolExecutor()
        self.recv_cache_ = {}
        self.stop_signal_ = False
        self.recv_loop_fut_ = None
    
    # Start a recv thread to receive value and cache it.
    def StartRecvLoop(self):
        def recv_loop():
            logger.info("Start recv loop.")
            while (not self.stop_signal_):
                try:
                    msg = self.chann_.recv(block=False)
                except Exception as e:
                    logger.error(e)
                    break

                if msg is None:
                    continue

                key = msg["tag"]
                value = msg["v"]
                if self.recv_cache_.get(key, None) is not None:
                    logger.warn(
                        "Hash entry for tag {} is not empty, replace old value".format(key))
                    del self.recv_cache_[key]

                logger.debug("Recv msg with tag {}.".format(key))
                self.recv_cache_[key] = value
                self.chann_.send("ok")
            logger.info("Recv loop stops.")

        self.recv_loop_fut_ = self.executor_.submit(recv_loop)
    
    # Stop recv thread.
    def StopRecvLoop(self):
        self.stop_signal_ = True
        self.recv_loop_fut_.result()
        logger.info("Recv loop already exit, clean cached value.")
        keys = self.recv_cache_.keys()
        for key in keys:
            del self.recv_cache_[key]
        del self.recv_cache_
    
    # Get value from cache, and the check will repeat at most 'retries' times,
    # and sleep 0.3s after each check to avoid check all the time.
    def Get(self, tag, retries=100):
        for i in range(retries):
            val = self.recv_cache_.get(tag, None)
            if val:
                del self.recv_cache_[tag]
                return val
            else:
                time.sleep(0.3)

        logger.warn("Can't get value for tag {}, timeout.".format(tag))
        return None
