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

#include <bb/pim/contacts/ContactService.hpp>
#include <bb/pim/contacts/ContactConsts.hpp>
#include <bb/pim/contacts/Contact.hpp>
#include <bb/pim/contacts/ContactBuilder.hpp>
#include <bb/pim/contacts/ContactAttribute.hpp>
#include <bb/pim/contacts/ContactAttributeBuilder.hpp>
#include <bb/pim/contacts/ContactPostalAddress.hpp>
#include <bb/pim/contacts/ContactPostalAddressBuilder.hpp>
#include <json/value.h>
#include <string>
#include <map>
#include "../common/plugin.h"

class PimContacts;

namespace webworks {

typedef bb::pim::contacts::ContactId ContactId;
typedef bb::pim::contacts::Contact Contact;
typedef bb::pim::contacts::ContactBuilder ContactBuilder;
typedef bb::pim::contacts::ContactPostalAddress ContactPostalAddress;
typedef bb::pim::contacts::ContactPostalAddressBuilder ContactPostalAddressBuilder;
typedef bb::pim::contacts::ContactAttributeBuilder ContactAttributeBuilder;
typedef bb::pim::contacts::ContactAttribute ContactAttribute;
typedef bb::pim::contacts::ContactService ContactService;
typedef bb::pim::contacts::ContactSearchFilters ContactSearchFilters;
typedef bb::pim::contacts::ContactListFilters ContactListFilters;
typedef bb::pim::contacts::AttributeKind AttributeKind;
typedef bb::pim::contacts::AttributeSubKind AttributeSubKind;
typedef bb::pim::contacts::SearchField SearchField;
typedef bb::pim::contacts::SortSpecifier SortSpecifier;
typedef bb::pim::contacts::SortColumn SortColumn;
typedef bb::pim::contacts::SortOrder SortOrder;

struct PimContactsThreadInfo {
    PimContacts *parent;
    Json::Value *jsonObj;
    std::string eventId;
};

class PimContactsQt {
public:
    PimContactsQt();
    ~PimContactsQt();
    Json::Value Find(const Json::Value& argsJson);
    void createContact(const std::string& attributeJson);
    void deleteContact(const std::string& contactJson);

    static void createAttributeKindMap();
    static void createAttributeSubKindMap();
    static void createKindAttributeMap();
    static void createSubKindAttributeMap();

    friend bool lessThan(const Contact& c1, const Contact& c2);

private:
    QSet<ContactId> singleFieldSearch(const Json::Value& searchFieldsJson, const Json::Value& contactFields, bool favorite);
    Json::Value assembleSearchResults(const QSet<ContactId>& results, const Json::Value& contactFields, int limit);
    void populateField(const Contact& contact, AttributeKind::Type kind, Json::Value& contactItem, bool isContactField);
    void populateOrganizations(const Contact& contact, Json::Value& contactOrgs);
    void populateAddresses(const Contact& contact, Json::Value& contactAddrs);
    
    static QString getSortFieldValue(const SortColumn::Type sortField, const Contact& contact);
    static QList<SearchField::Type> getSearchFields(const Json::Value& searchFieldsJson);
    static std::map<std::string, AttributeKind::Type> _attributeKindMap;
    static std::map<std::string, AttributeSubKind::Type> _attributeSubKindMap;
    static std::map<AttributeKind::Type, std::string> _kindAttributeMap;
    static std::map<AttributeSubKind::Type, std::string> _subKindAttributeMap;
    static QList<SortSpecifier> _sortSpecs;

    std::map<ContactId, Contact> _contactSearchMap;
};

} // namespace webworks

#endif // PIM_CONTACTS_QT_H_
