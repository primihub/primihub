import time
from redis.client import Redis

r = Redis(host='192.168.99.31', port=15550, db=0, password='primihub')


def publish():
    for i in range(10):
        r.publish('int_channel', i)
        print(i)
        time.sleep(10)


import time
from redis.client import Redis

r = Redis(host='192.168.99.31', port=15550, db=0, password='primihub')


def sub():
    pub = r.pubsub()  # 返回发布订阅对象，通过这个对象你能1）订阅频道 2）监听频道中的消息
    pub.subscribe('int_channel')  # 订阅一个channel
    msg_stream = pub.listen()  # 监听消息
    for msg in msg_stream:
        print(msg)
        if msg["type"] == "message":
            print(str(msg["channel"], encoding="utf-8") + ":" + str(msg["data"], encoding="utf-8"))
        elif msg["type"] == "subscribe":
            print(str(msg["channel"], encoding="utf-8"), '订阅成功')


if __name__ == '__main__':
    publish()
    # sub()
