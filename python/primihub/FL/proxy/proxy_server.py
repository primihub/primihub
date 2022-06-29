from primihub.channel.proxy import ServerChannelProxy
from primihub.channel.proxy import ClientChannelProxy
from concurrent.futures import ThreadPoolExecutor
import time

if __name__ == '__main__':
    proxy = ServerChannelProxy("10090")
    proxy.StartRecvLoop();
    
    print(proxy.Get("test_1", 100))
    print(proxy.Get("test_2", 100))
    print(proxy.Get("test_3", 100))
    
    proxy.StopRecvLoop()
