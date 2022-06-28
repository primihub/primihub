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
        self.executor_ = ThreadPoolExecutor(max_workers=10)

    def Remote(self, val, tag):
        msg = {"v": val, "tag": tag}
        self.chann_.send(msg)
        _ = self.chann_.recv()

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

    def StartRecvLoop(self):
        def recv_loop(channel, recv_cache):
            logger.info("Start recv loop.")
            while (not self.stop_signal_):
                try:
                    msg = channel.recv(block=False)
                except Exception as e:
                    logger.error(e)
                    break
                
                if msg is None:
                    continue

                key = msg["tag"]
                value = msg["v"]
                if recv_cache.get(key, None) is not None:
                    logger.warn(
                        "Hash entry for tag {} is not empty, replace old value".format(key))
                    del recv_cache[key]

                logger.debug("Recv msg with tag {}.".format(key))
                recv_cache[key] = value
                channel.send("ok")
            logger.info("Recv loop stops.")

        self.recv_loop_fut_ = self.executor_.submit(
            recv_loop, self.chann_, self.recv_cache_)

    def StopRecvLoop(self):
        self.stop_signal_ = True
        self.recv_loop_fut_.result()
        logger.info("Recv loop already exit, clear cached value.")
        keys = self.recv_cache_.keys()
        for key in keys:
            del self.recv_cache_[key]
        del self.recv_cache_

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
