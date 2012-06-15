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

#include <bps/bps.h>
#include "BPSMaster.hpp"
#include <stdio.h>
#include <iostream>

bool BPSMaster::m_eventsRunning;
int BPSMaster::m_channel;
int BPSMaster::m_domain;
std::list<BPSEventHandler *> BPSMaster::m_listeners;

BPSMaster::BPSMaster()
{
    bps_initialize();
    m_domain = bps_register_domain();
    m_channel = bps_channel_get_active();
    StartThread();
}

BPSMaster::~BPSMaster()
{
    StopThread();
    bps_shutdown();
}

void BPSMaster::StartThread()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&m_thread, &attr, MainEventThread, NULL);
    pthread_attr_destroy(&attr);
}

void BPSMaster::StopThread()
{
    NotifyInternalEvent(ShutdownThread, NULL);
    // wait for thread to shutdown
    pthread_join(m_thread, NULL);
}

void BPSMaster::NotifyInternalEvent(InternalEvent e, void *payload)
{
    bps_event_t *event = NULL;
    bps_event_payload_t eventPayload;
    eventPayload.data1 = reinterpret_cast<uintptr_t>(payload);
    bps_event_create(&event, m_domain, e, &eventPayload, NULL);
    bps_channel_push_event(m_channel, event);
}

void* BPSMaster::MainEventThread(void *)
{
    m_channel = bps_channel_get_active();
    
    while (m_eventsRunning) {
        bps_event_t *event;
        bps_get_event(&event, -1); // blocking
        int currentDomain = bps_event_get_domain(event);

        if (m_domain == currentDomain) {
            InternalEvent code = static_cast<InternalEvent>(bps_event_get_code(event));
            BPSEventHandler *handler = reinterpret_cast<BPSEventHandler *>(bps_event_get_payload(event)->data1);
            std::list<BPSEventHandler *>::iterator it;
          
            switch (code) {
                case ListenerAdded:
                    handler->OnBPSInit();
                    break;
                case ShutdownThread:
                    for (it = m_listeners.begin(); it != m_listeners.end(); it++) {
                        (*it)->OnBPSShutdown();
                    }
                    m_eventsRunning = false;
                    break;
                default:
                    break;
            }

        } else {
            std::list<BPSEventHandler *>::iterator it;
            for (it = m_listeners.begin(); it != m_listeners.end(); it++) {
                if ((*it)->BPSDomain() == currentDomain) {
                    (*it)->OnBPSEvent(event);
                }
            }
        }
    }

    return NULL;
}

void BPSMaster::AddEventListener(BPSEventHandler *handler)
{
    while (!m_eventsRunning);
    m_listeners.push_back(handler);
    NotifyInternalEvent(ListenerAdded, reinterpret_cast<void *>(handler));
}

void BPSMaster::RemoveEventListener(BPSEventHandler *handler)
{
    m_listeners.remove(handler);
}

BPSMaster* BPSMaster::GetInstance()
{
    static BPSMaster instance;
    return &instance;
}

int BPSMaster::GetChannel()
{
    return m_channel;
}
