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

#include "pim_contacts_js.hpp"
#include "pim_contacts_qt.hpp"
//#include <json/reader.h>
//#include <sstream>
#include <string>

PimContacts::PimContacts(const std::string& id) : m_id(id)
{
}

char* onGetObjList()
{
    // Return list of classes in the object
    static char name[] = "PimContacts";
    return name;
}

JSExt* onCreateObject(const std::string& className, const std::string& id)
{
    // Make sure we are creating the right class
    if (className != "PimContacts") {
        return 0;
    }

    return new PimContacts(id);
}

std::string PimContacts::InvokeMethod(const std::string& command)
{
    try {
    int index = command.find_first_of(" ");

    string strCommand = command.substr(0, index);
    string jsonObject = command.substr(index + 1, command.length());

    if (strCommand == "find") {
        webworks::PimContactsQt pim_qt;
        return pim_qt.Find(jsonObject);

    } else if (strCommand == "save") {
        webworks::PimContactsQt pim_qt;
        return pim_qt.Save(jsonObject);

        /*
        // parse the JSON
        Json::Reader reader;
        Json::Value obj;
        bool parse = reader.parse(jsonObject, obj);

        if (!parse) {
            fprintf(stderr, "%s", "error parsing\n");
            return "Cannot parse JSON object";
        }
        */
    } else if (strCommand == "remove") {
        webworks::PimContactsQt pim_qt;
        pim_qt.DeleteContact(jsonObject);
        return "";
    }

    return "";
    } catch (std::string e) {
        return "";
    }
}

bool PimContacts::CanDelete()
{
    return true;
}

// Notifies JavaScript of an event
void PimContacts::NotifyEvent(const std::string& event)
{
    /*
    std::string eventString = m_id + " result ";
    eventString.append(event);
    SendPluginEvent(eventString.c_str(), m_pContext);
    */
}