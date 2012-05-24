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

#ifndef BPS_CONNECTION_H_
#define BPS_CONNECTION_H_

#include <bps/bps.h>
#include <bps/netstatus.h>

class BPSNetstatus
{
    virtual void Initialize() { 
        bps_initialize(); 
    }

    virtual void Shutdown() { 
        bps_shutdown(); 
    }
    
    virtual void GetAvailability(bool *available) { 
        netstatus_get_availability(available) 
    }

    virtual void GetDefaultInterface(char **interface) { 
        netstatus_get_default_interface(interface) 
    }

    virtual int GetInterfaceDetails(char *interface, netstatus_interface_details_t *details) { 
        return netstatus_get_interface_details(interface, details); 
    }

    virtual netstatus_interface_type_t GetInterfaceType(netstatus_interface_details_t details);
    virtual void FreeInterfaceDetails(netstatus_interface_details_t *details);
    virtual void Free(char *object);
    virtual int RequestEvents(int flags);
    virtual void GetEvent(bps_event_t **event, int flags);
    virtual void GetEventDomain(bps_event_t *event);
    virtual void GetDomain();
    virtual void GetEventCode(bps_event_t *event);
};

/*
class BPSNetstatusInterface
{
public:
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void GetAvailability(bool *available) = 0;
    virtual void GetDefaultInterface(char **interface) = 0;
    virtual int GetInterfaceDetails(char *interface, netstatus_interface_details_t *details) = 0;
    virtual netstatus_interface_type_t GetInterfaceType(netstatus_interface_details_t details) = 0;
    virtual void FreeInterfaceDetails(netstatus_interface_details_t *details) = 0;
    virtual void Free(char *object) = 0;
    virtual int RequestEvents(int flags) = 0;
    virtual void GetEvent(bps_event_t **event, int flags) = 0;
    virtual void GetEventDomain(bps_event_t *event) = 0;
    virtual void GetDomain() = 0;
    virtual void GetEventCode(bps_event_t *event) = 0;
};
*/

#endif /* BPS_CONNECTION_H_ */
