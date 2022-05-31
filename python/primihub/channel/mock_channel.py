"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """


session_map = dict()
endpoint_map = dict()


class MockIOService:
    pass


class MockChannel:
    """ mock channel
    """
    def __init__(self, session):
        self.session = session

    def send(self, data):
        print("send data...")
        print(self.session)
        self.session["server"] = data

    def recv(self, data_size=0):
        print("recv message...")
        send_data = self.session["server"]
        self.session["client"] = send_data

        # mock 消息接收后被消费
        # self.session["server"] = None
        # print("del send", self.session)

        recv_data = self.session["client"]
        # self.session["client"] = None

        # print("del recv", self.session)
        return recv_data

    def close(self):
        print("channel close")
        self.session_map = {}


class MockSession:
    def __init__(self, io_service, address, session_mode, endpoint):
        self.io_server = io_service
        self.address = address
        self.session_mode = session_mode
        self.endpoint = endpoint
        session_map[endpoint] = endpoint_map
        if session_mode == "server":
            server_map = {"server": None}
            endpoint_map.update(server_map)
        if session_mode == "client":
            client_map = {"client": None}
            endpoint_map.update(client_map)
        self.session = session_map[endpoint]
        print("session: ", self.session)

    def addChannel(self, *args) -> MockChannel:
        print(args)
        ch = MockChannel(self.session)
        return ch


