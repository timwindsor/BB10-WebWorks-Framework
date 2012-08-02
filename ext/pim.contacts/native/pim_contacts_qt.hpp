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
typedef std::map<bbpim::AttributeKind::Type, std::string> KindToStringMap;
typedef std::map<bbpim::AttributeSubKind::Type, std::string> SubKindToStringMap;

struct PimContactsThreadInfo {
    PimContacts *parent;
    Json::Value *jsonObj;
    std::string eventId;
};

class PimContactsQt {
public:
    PimContactsQt();
    ~PimContactsQt();
    Json::Value Find(const Json::Value& argsObj);
    Json::Value Save(const Json::Value& attributeObj);
    Json::Value CreateContact(const Json::Value& attributeObj);
    Json::Value DeleteContact(const Json::Value& contactObj);
    Json::Value EditContact(bbpim::Contact& contact, const Json::Value& attributeObj);
    Json::Value CloneContact(bbpim::Contact& contact, const Json::Value& attributeObj);

private:
    // Helper functions for Find
    QSet<bbpim::ContactId> singleFieldSearch(const Json::Value& searchFieldsJson, const Json::Value& contactFields, bool favorite);
    Json::Value assembleSearchResults(const QSet<bbpim::ContactId>& results, const Json::Value& contactFields, int limit);
    void populateField(const bbpim::Contact& contact, bbpim::AttributeKind::Type kind, Json::Value& contactItem, bool isContactField, bool isArray);
    void populateOrganizations(const bbpim::Contact& contact, Json::Value& contactOrgs);
    void populateAddresses(const bbpim::Contact& contact, Json::Value& contactAddrs);
    void populatePhotos(const bbpim::Contact& contact, Json::Value& contactPhotos);

    static QString getSortFieldValue(const bbpim::SortColumn::Type sortField, const bbpim::Contact& contact);
    static QList<bbpim::SearchField::Type> getSearchFields(const Json::Value& searchFieldsJson);

    static bool lessThan(const bbpim::Contact& c1, const bbpim::Contact& c2);

    // Helper functions for Save
    bbpim::ContactBuilder& buildAttributeKind(bbpim::ContactBuilder& contactBuilder, const Json::Value& jsonObj, const std::string& field);
    bbpim::ContactBuilder& buildGroupedAttributes(bbpim::ContactBuilder& contactBuilder, const Json::Value& fieldsObj, bbpim::AttributeKind::Type kind, const std::string& groupKey);
    bbpim::ContactBuilder& buildFieldAttribute(bbpim::ContactBuilder& contactBuilder, const Json::Value& fieldObj, bbpim::AttributeKind::Type kind);
    bbpim::ContactBuilder& buildPostalAddress(bbpim::ContactBuilder& contactBuilder, const Json::Value& addressObj);
    bbpim::ContactBuilder& buildPhoto(bbpim::ContactBuilder& contactBuilder, const Json::Value& photoObj);

    bbpim::ContactBuilder& addAttribute(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value);
    bbpim::ContactBuilder& addAttributeDate(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value);
    bbpim::ContactBuilder& addAttributeToGroup(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value, const std::string& groupKey);

    // Mappings between JSON strings and attribute kinds/subkinds
    static void createAttributeKindMap();
    static void createAttributeSubKindMap();
    static void createKindAttributeMap();
    static void createSubKindAttributeMap();

    static StringToKindMap _attributeKindMap;
    static StringToSubKindMap _attributeSubKindMap;
    static KindToStringMap _kindAttributeMap;
    static SubKindToStringMap _subKindAttributeMap;
    static QList<bbpim::SortSpecifier> _sortSpecs;

    std::map<bbpim::ContactId, bbpim::Contact> _contactSearchMap;
};

} // namespace webworks

#endif // PIM_CONTACTS_QT_H_
