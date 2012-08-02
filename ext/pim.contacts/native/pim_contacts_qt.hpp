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

#include <json/value.h>
#include <bb/pim/contacts/ContactService.hpp>
#include <bb/pim/contacts/ContactConsts.hpp>
#include <bb/pim/contacts/Contact.hpp>
#include <bb/pim/contacts/ContactBuilder.hpp>
#include <bb/pim/contacts/ContactAttribute.hpp>
#include <bb/pim/contacts/ContactAttributeBuilder.hpp>
#include <bb/pim/contacts/ContactPostalAddress.hpp>
#include <bb/pim/contacts/ContactPostalAddressBuilder.hpp>
#include <bb/pim/contacts/ContactPhoto.hpp>
#include <bb/pim/contacts/ContactPhotoBuilder.hpp>
#include <string>
#include <map>
#include "../common/plugin.h"

class PimContacts;

namespace webworks {

namespace bbpim = bb::pim::contacts;

typedef std::map<std::string, bbpim::AttributeKind::Type> StringToKindMap;
typedef std::map<std::string, bbpim::AttributeSubKind::Type> StringToSubKindMap;

struct PimContactsThreadInfo {
    PimContacts *parent;
    Json::Value *jsonObj;
    std::string eventId;
};

class PimContactsQt {
public:
    PimContactsQt();
    ~PimContactsQt();
    Json::Value Find(const Json::Value& optionsObj);
    Json::Value Save(const Json::Value& attributeObj);
    Json::Value CreateContact(const Json::Value& attributeObj);
    Json::Value DeleteContact(const Json::Value& contactObj);
    Json::Value EditContact(bbpim::Contact& contact, const Json::Value& attributeObj);
    Json::Value CloneContact(bbpim::Contact& contact, const Json::Value& attributeObj);

private:
    bbpim::ContactBuilder& buildAttributeKind(bbpim::ContactBuilder& contactBuilder, const Json::Value& jsonObj, const std::string& field);
    bbpim::ContactBuilder& buildGroupedAttributes(bbpim::ContactBuilder& contactBuilder, const Json::Value& fieldsObj, bbpim::AttributeKind::Type kind, const std::string& groupKey);
    bbpim::ContactBuilder& buildFieldAttribute(bbpim::ContactBuilder& contactBuilder, const Json::Value& fieldObj, bbpim::AttributeKind::Type kind);
    bbpim::ContactBuilder& buildPostalAddress(bbpim::ContactBuilder& contactBuilder, const Json::Value& addressObj);
    bbpim::ContactBuilder& buildPhoto(bbpim::ContactBuilder& contactBuilder, const Json::Value& photoObj);

    bbpim::ContactBuilder& addAttribute(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value);
    bbpim::ContactBuilder& addAttributeToGroup(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value, const std::string& groupKey);

    static void createAttributeKindMap();
    static void createAttributeSubKindMap();
    static StringToKindMap _attributeKindMap;
    static StringToSubKindMap _attributeSubKindMap;
};

} // namespace webworks

#endif // PIM_CONTACTS_QT_H_
