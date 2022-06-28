from primihub.channel.proxy import ServerChannelProxy
from primihub.channel.proxy import ClientChannelProxy
from concurrent.futures import ThreadPoolExecutor
import time

def server_fn():
    proxy = ServerChannelProxy("10090")
    proxy.StartRecvLoop();
    
    print(proxy.Get("test_1", 100))
    print(proxy.Get("test_2", 100))
    print(proxy.Get("test_3", 100))
    
    proxy.StopRecvLoop()

if __name__ == '__main__':
    proxy = ClientChannelProxy("127.0.0.1", "10090")
    proxy.Remote("test_1", "test_1")
    time.sleep(1)
    proxy.Remote("test_2", "test_2")
    time.sleep(1)
    proxy.Remote("test_3", "test_3")
    time.sleep(1)
