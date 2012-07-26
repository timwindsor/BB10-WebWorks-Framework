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

#ifndef PIM_CONTACTS_QT_H_
#define PIM_CONTACTS_QT_H_

#include <string>
#include <map>
#include <bb/pim/contacts/ContactService.hpp>
#include <bb/pim/contacts/ContactConsts.hpp>
#include <bb/pim/contacts/Contact.hpp>
#include <bb/pim/contacts/ContactBuilder.hpp>
#include <bb/pim/contacts/ContactAttribute.hpp>
#include <bb/pim/contacts/ContactAttributeBuilder.hpp>
#include <bb/pim/contacts/ContactPostalAddress.hpp>
#include <bb/pim/contacts/ContactPostalAddressBuilder.hpp>
#include "../common/plugin.h"

namespace webworks {

using namespace bb::pim::contacts;

class PimContactsQt {
public:
    PimContactsQt();
    ~PimContactsQt();
    std::string find(const std::string& optionsJson);
    void createContact(const std::string& attributeJson);
    void deleteContact(const std::string& contactJson);

    static void createAttributeKindMap();
    static void createAttributeSubKindMap();

private:
    static std::map<std::string, AttributeKind::Type> attributeKindMap;
    static std::map<std::string, AttributeSubKind::Type> attributeSubKindMap;
};

} // namespace webworks

#endif // PIM_CONTACTS_QT_H_
