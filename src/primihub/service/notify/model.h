/*
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
 */




/* Primihub notification toplogy 

   ---------    notifyStatus/notifyResult   ----------------   publish/subscribe   -----------------------------    Any channel  -----------------
   |  Node | -----------------------------> |NotifyDelegate | --------------------> |NotifyServer Implemention 1| ------------> |Register Client 1| 
   ---------                                ----------------  |                     -----------------------------                -----------------
                                                              | publish/subscribe    ----------------------------                ------------------
                                                              |--------------------> |NotifyServer Implemention 2| ------------> |Register Client 2|
                                                                                     ----------------------------                ------------------
*/

class NotifyDelegate {
public:
    virtual void notifyStatus(const std::string &status) = 0;
    virtual void notifyResult(const std::string &result) = 0;
};



class NotifyServer {
    public:
    // TODO methods:
    // 1. task context register
    // 2. subscribe task notrify use task context id
    // 3. client context register
    // 4. client notification implement

    private:
    // TODO members:
    // Running task context list
    // client context list
    // channel id
};

class NotifyService {
    public:
    // TODO methods:
    // 1. create notify channel by config file.
    private:
    std::shared_ptr<NotifyDelegate> notify_delegate_;
    std::list<NotifyChannel> notify_channel_list_;
}