/*
* Copyright 2012 Research In Motion Limited.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef BPSMASTER_HPP_
#define BPSMASTER_HPP_

#include <list>
#include <pthread.h>
#include "BPSEventHandler.hpp"

enum InternalEvent {
    ListenerAdded = 0,
    ShutdownThread
};

class BPSMaster {
public:
    BPSMaster();
    ~BPSMaster();
    void AddEventListener(BPSEventHandler *handler);
    void RemoveEventListener(BPSEventHandler *handler);
    static void* MainEventThread(void *);
    static BPSMaster* GetInstance();
    static int GetChannel();
private:
    void StartThread();
    void StopThread();
    void NotifyInternalEvent(InternalEvent e, void *payload);
    bool m_eventsRunning;
    pthread_t m_thread;
    static std::list<BPSEventHandler *> m_listeners;
    static int m_domain;
    static int m_channel;
};

#endif /* BPSMASTER_HPP_ */
