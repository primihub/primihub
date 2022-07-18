from primihub.channel.proxy import ServerChannelProxy
from primihub.channel.proxy import ClientChannelProxy
from concurrent.futures import ThreadPoolExecutor
import time

if __name__ == '__main__':
    proxy = ClientChannelProxy("127.0.0.1", "10090")
    
    proxy.Remote("test_1", "test_1")
    time.sleep(1)
    proxy.Remote("test_2", "test_2")
    time.sleep(1)

    fut = proxy.RemoteAsync("test_3", "test_3")
    ## do something like perform some compute.
    ## wait for send to finish.
    fut.result()
    time.sleep(1)
