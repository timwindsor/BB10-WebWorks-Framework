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

#include <json/writer.h>
#include <json/reader.h>
#include <stdio.h>
#include <QSet>
#include <QMap>
#include <QtAlgorithms>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include "pim_contacts_qt.hpp"

namespace webworks {

std::map<std::string, AttributeKind::Type> PimContactsQt::_attributeKindMap;
std::map<std::string, AttributeSubKind::Type> PimContactsQt::_attributeSubKindMap;
std::map<AttributeKind::Type, std::string> PimContactsQt::_kindAttributeMap;
std::map<AttributeSubKind::Type, std::string> PimContactsQt::_subKindAttributeMap;
QList<SortSpecifier> PimContactsQt::_sortSpecs;

PimContactsQt::PimContactsQt()
{
    static bool mapInit = false;

    if (!mapInit) {
        createAttributeKindMap();
        createAttributeSubKindMap();
        createKindAttributeMap();
        createSubKindAttributeMap();
        mapInit = true;
    }
}

PimContactsQt::~PimContactsQt()
{
}

QList<SearchField::Type> PimContactsQt::getSearchFields(const Json::Value& searchFieldsJson)
{
    QList<SearchField::Type> searchFields;

    switch (searchFieldsJson["fieldName"].asInt()) {
        case SearchField::FirstName:
            searchFields.append(SearchField::FirstName);
            break;
        case SearchField::LastName:
            searchFields.append(SearchField::LastName);
            break;
        case SearchField::CompanyName:
            searchFields.append(SearchField::CompanyName);
            break;
        case SearchField::Phone:
            searchFields.append(SearchField::Phone);
            break;
        case SearchField::Email:
            searchFields.append(SearchField::Email);
            break;
        case SearchField::BBMPin:
            searchFields.append(SearchField::BBMPin);
            break;
        case SearchField::LinkedIn:
            searchFields.append(SearchField::LinkedIn);
            break;
        case SearchField::Twitter:
            searchFields.append(SearchField::Twitter);
            break;
        case SearchField::VideoChat:
            searchFields.append(SearchField::VideoChat);
            break;
    }

    return searchFields;
}

QSet<ContactId> PimContactsQt::singleFieldSearch(const Json::Value& searchFieldsJson, const Json::Value& contact_fields, bool favorite)
{
    QList<SearchField::Type> searchFields = PimContactsQt::getSearchFields(searchFieldsJson);
    QSet<ContactId> contactIds;

    if (!searchFields.empty()) {
        ContactService contactService;
        ContactSearchFilters contactFilter;
        QList<AttributeKind::Type> includeFields;
        QList<Contact> results;

        contactFilter.setSearchFields(searchFields);
        contactFilter.setSearchValue(QString(searchFieldsJson["fieldValue"].asString().c_str()));

        if (favorite) {
            contactFilter.setIsFavourite(favorite);
        }

        for (int i = 0; i < contact_fields.size(); i++) {
            std::map<std::string, AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(contact_fields[i].asString());

            if (kindIter != _attributeKindMap.end()) {
                includeFields.append(kindIter->second);
            } else {
                fprintf(stderr, "Could not find search field in map: %s\n", contact_fields[i].asString().c_str());
            }
        }

        contactFilter.setShowAttributes(true);
        contactFilter.setIncludeAttributes(includeFields);

        results = contactService.searchContacts(contactFilter);

        for (int i = 0; i < results.size(); i++) {
            contactIds.insert(results[i].id());
            _contactSearchMap[results[i].id()] = results[i];
        }
    }

    return contactIds;
}

void PimContactsQt::populateField(const Contact& contact, AttributeKind::Type kind, Json::Value& contactItem, bool isContactField, bool isArray)
{
    fprintf(stderr, "populateField kind= %d\n", kind);
    QList<ContactAttribute> attrs = contact.filteredAttributes(kind);
    QList<ContactAttribute>::const_iterator k = attrs.constBegin();

    while (k != attrs.constEnd()) {
        ContactAttribute currentAttr = *k;
        Json::Value val;
        std::map<AttributeSubKind::Type, std::string>::const_iterator typeIter = _subKindAttributeMap.find(currentAttr.subKind());

        if (typeIter != _subKindAttributeMap.end()) {
            if (isContactField) {
                val["type"] = Json::Value(typeIter->second);
                val["value"] = Json::Value(currentAttr.value().toStdString());
                contactItem.append(val);
            } else {
                if (isArray) {
                    val[typeIter->second] = Json::Value(currentAttr.value().toStdString());
                    contactItem.append(val);
                } else {
                    if (kind == AttributeKind::Date) {
                        //Json::Value::Int64 numMillisecs;
                        //numMillisecs = static_cast<Json::Value::Int64> (currentAttr.valueAsDateTime().toMSecsSinceEpoch());
                        //yyyy-MM-ddThh:mm:ss.zzzZ
                        QString format = "yyyy-MM-dd";
                        // if it's a date field, store number of milliseconds since UTC and let JS convert it
                        fprintf(stderr, "date=%s\n", currentAttr.valueAsDateTime().date().toString(format).toStdString().c_str());
                        contactItem[typeIter->second] = Json::Value(currentAttr.valueAsDateTime().date().toString(format).toStdString());
                    } else {
                        if (kind == AttributeKind::Note) {
                            contactItem["note"] = Json::Value(currentAttr.value().toStdString());
                        } else {
                            contactItem[typeIter->second] = Json::Value(currentAttr.value().toStdString());
                        }
                    }
                }
            }
        } else {
            // TODO(rtse): not found in map
            fprintf(stderr, "populateField: subkind not found in map: %d\n", currentAttr.subKind());
        }
        ++k;
    }
}

void PimContactsQt::populateAddresses(const Contact& contact, Json::Value& contactAddrs)
{
    ContactService contactService;
    Contact fullContact = contactService.contactDetails(contact.id());
    QList<ContactPostalAddress> addrs = fullContact.postalAddresses();
    QList<ContactPostalAddress>::const_iterator k = addrs.constBegin();

    while (k != addrs.constEnd()) {
        ContactPostalAddress currentAddr = *k;
        Json::Value addr;

        std::map<AttributeSubKind::Type, std::string>::const_iterator typeIter = _subKindAttributeMap.find(currentAddr.subKind());

        if (typeIter != _subKindAttributeMap.end()) {
            addr["type"] = Json::Value(typeIter->second);
        }

        addr["address1"] = Json::Value(currentAddr.line1().toStdString());
        addr["address2"] = Json::Value(currentAddr.line2().toStdString());
        addr["country"] = Json::Value(currentAddr.country().toStdString());
        addr["locality"] = Json::Value(currentAddr.city().toStdString());
        addr["postalCode"] = Json::Value(currentAddr.postalCode().toStdString());
        addr["region"] = Json::Value(currentAddr.region().toStdString());

        contactAddrs.append(addr);
        ++k;
    }
}

void PimContactsQt::populateOrganizations(const Contact& contact, Json::Value& contactOrgs)
{
    QList<QList<ContactAttribute> > orgAttrs = contact.filteredAttributesByGroupKey(AttributeKind::OrganizationAffiliation);
    QList<QList<ContactAttribute> >::const_iterator j = orgAttrs.constBegin();

    while (j != orgAttrs.constEnd()) {
        QList<ContactAttribute> currentOrgAttrs = *j;
        QList<ContactAttribute>::const_iterator k = currentOrgAttrs.constBegin();
        Json::Value org;

        while (k != currentOrgAttrs.constEnd()) {
            ContactAttribute attr = *k;
            std::map<AttributeSubKind::Type, std::string>::const_iterator typeIter = _subKindAttributeMap.find(attr.subKind());

            if (typeIter != _subKindAttributeMap.end()) {
                org[typeIter->second] = Json::Value(attr.value().toStdString());
            } else {
                // TODO(rtse): not found in map
                fprintf(stderr, "populateOrganizations: subkind not found in map%s\n");
            }

            ++k;
        }

        contactOrgs.append(org);
        ++j;
    }
}

QString PimContactsQt::getSortFieldValue(const SortColumn::Type sort_field, const Contact& contact)
{
    switch (sort_field) {
        case SortColumn::FirstName:
            return contact.sortFirstName();
        case SortColumn::LastName:
            return contact.sortLastName();
        case SortColumn::CompanyName:
            return contact.sortCompanyName();
    }

    return QString();
}

bool lessThan(const Contact& c1, const Contact& c2)
{
    QList<SortSpecifier>::const_iterator i = PimContactsQt::_sortSpecs.constBegin();
    SortSpecifier sortSpec;
    QString val1, val2;

    do {
        sortSpec = *i;
        val1 = PimContactsQt::getSortFieldValue(sortSpec.first, c1);
        val2 = PimContactsQt::getSortFieldValue(sortSpec.first, c2);
        ++i;
    } while (val1 == val2 && i != PimContactsQt::_sortSpecs.constEnd());

    if (sortSpec.second == SortOrder::Ascending) {
        return val1 < val2;
    } else {
        return !(val1 < val2);
    }
}

Json::Value PimContactsQt::assembleSearchResults(const QSet<ContactId>& resultIds, const Json::Value& contactFields, int limit)
{
    fprintf(stderr, "Beginning of assembleSearchResults, results size=%d\n", resultIds.size());
    QMap<ContactId, Contact> completeResults;
    QSet<ContactId>::const_iterator i = resultIds.constBegin();

    // put complete contacts in map
    while (i != resultIds.constEnd()) {
        completeResults.insertMulti(*i, _contactSearchMap[*i]);
        ++i;
    }

    // sort results based on sort specs
    QList<Contact> sortedResults = completeResults.values();
    if (!_sortSpecs.empty()) {
        qSort(sortedResults.begin(), sortedResults.end(), lessThan);
    }

    Json::Value contactArray;

    // if limit is -1, returned all available results, otherwise return based on the number passed in find options
    if (limit == -1) {
        limit = sortedResults.size();
    } else {
        limit = min(limit, sortedResults.size());
    }

    for (int i = 0; i < limit; i++) {
        Json::Value contactItem;

        for (int j = 0; j < contactFields.size(); j++) {
            std::string field = contactFields[j].asString();
            std::map<std::string, AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(field);

            if (kindIter != _attributeKindMap.end()) {
                switch (kindIter->second) {
                    case AttributeKind::Name: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], false, false);
                        break;
                    }

                    case AttributeKind::OrganizationAffiliation: {
                        contactItem[field] = Json::Value();
                        populateOrganizations(sortedResults[i], contactItem[field]);
                        break;
                    }

                    case AttributeKind::Date: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case AttributeKind::Note: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case AttributeKind::Sound: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case AttributeKind::VideoChat: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], false, false);
                        break;
                    }

                    case AttributeKind::Email:
                    case AttributeKind::Fax:
                    case AttributeKind::Pager:
                    case AttributeKind::Phone:
                    case AttributeKind::Profile:
                    case AttributeKind::Website:
                    case AttributeKind::InstantMessaging: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], true, false);
                        break;
                    }
                }
            } else {
                fprintf(stderr, "cannot find field=%s in map\n", field.c_str());

                if (field == "favorite") {
                    contactItem[field] = Json::Value(sortedResults[i].isFavourite());
                } else if (field == "addresses") {
                    contactItem["addresses"] = Json::Value();
                    populateAddresses(sortedResults[i], contactItem["addresses"]);
                }
            }
        }

        // TODO(rtse): always include id?
        // TODO(rtse): handle fields not under regular kinds/subkinds
        contactItem["id"] = Json::Value(sortedResults[i].id());

        contactArray.append(contactItem);
    }

    return contactArray;
}

Json::Value PimContactsQt::Find(const Json::Value& argsObj)
{
    fprintf(stderr, "%s", "Beginning of find\n");

    _contactSearchMap.clear();
    _sortSpecs.clear();

    Json::Value contactFields;
    QSet<ContactId> results;

    Json::Value filter;
    Json::Value sort;
    int limit;
    bool favorite;

    const Json::Value::Members argsKey = argsObj.getMemberNames();

    for (int i = 0; i < argsKey.size(); i++) {
        const std::string key = argsKey[i];

        if (key == "fields") {
            contactFields = argsObj[key];
        } else if (key == "options") {
            favorite = argsObj[key]["favorite"].asBool();
            limit = argsObj[key]["limit"].asInt();

            filter = argsObj[key]["filter"];
            if (filter.isArray()) {
                for (int j = 0; j < filter.size(); j++) {
                    QSet<ContactId> currentResults = singleFieldSearch(filter[j], contactFields, favorite);

                    if (currentResults.empty()) {
                        // no need to continue, can return right away
                        results = currentResults;
                        break;
                    } else {
                        if (j == 0) {
                            results = currentResults;
                        } else {
                            results.intersect(currentResults);
                        }
                    }
                }
            }

            sort = argsObj[key]["sort"];
            if (sort.isArray()) {
                for (int j = 0; j < sort.size(); j++) {
                    SortOrder::Type order;
                    SortColumn::Type sortField;

                    if (sort[j]["desc"].asBool()) {
                        order = SortOrder::Descending;
                    } else {
                        order = SortOrder::Ascending;
                    }

                    switch (sort[j]["fieldName"].asInt()) {
                        case SortColumn::FirstName:
                            sortField = SortColumn::FirstName;
                            break;
                        case SortColumn::LastName:
                            sortField = SortColumn::LastName;
                            break;
                        case SortColumn::CompanyName:
                            sortField = SortColumn::CompanyName;
                            break;
                    }

                    _sortSpecs.append(SortSpecifier(sortField, order));
                }
            }
        }
    }

    Json::Value returnObj;
    returnObj["_success"] = true;
    returnObj["contacts"] = assembleSearchResults(results, contactFields, limit);

    return returnObj;
}

void PimContactsQt::createContact(const std::string& attributeJson) {
    Json::Reader reader;
    Json::Value attribute_obj;
    bool parse = reader.parse(attributeJson, attribute_obj);

    if (!parse) {
        fprintf(stderr, "%s", "error parsing\n");
        throw "Cannot parse JSON object";
    }

    const Json::Value::Members attribute_keys = attribute_obj.getMemberNames();

    Contact new_contact;
    ContactBuilder contact_builder(new_contact.edit());

    for (int i = 0; i < attribute_keys.size(); i++) {
        const std::string key = attribute_keys[i];

        std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = _attributeKindMap.find(key);

        if (kind_iter != _attributeKindMap.end()) {
            switch (kind_iter->second) {
                case AttributeKind::Name: {
                        // TODO(miwong): Are group keys needed here?
                        Json::Value name_obj = attribute_obj[key];
                        const Json::Value::Members name_fields = name_obj.getMemberNames();

                        for (int j = 0; j < name_fields.size(); j++) {
                            const std::string name_key = name_fields[j];
                            std::map<std::string, AttributeSubKind::Type>::const_iterator name_iter = _attributeSubKindMap.find(name_key);

                            if (name_iter != _attributeSubKindMap.end()) {
                                ContactAttribute attribute;
                                ContactAttributeBuilder attribute_builder(attribute.edit());

                                attribute_builder = attribute_builder.setKind(AttributeKind::Name);
                                attribute_builder = attribute_builder.setSubKind(name_iter->second);
                                attribute_builder = attribute_builder.setValue(QString(name_obj[name_key].asString().c_str()));

                                contact_builder = contact_builder.addAttribute(attribute);
                            }
                        }

                        break;
                    }
                case AttributeKind::OrganizationAffiliation: {
                        Json::Value attribute_array = attribute_obj[key];

                        for (int j = 0; j < attribute_array.size(); j++) {
                            Json::Value field_obj = attribute_array[j];
                            const Json::Value::Members fields = field_obj.getMemberNames();

                            // TODO(miwong): need to use group keys for organizations

                            for (int k = 0; k < fields.size(); k++) {
                                const std::string field_key = fields[k];
                                std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = _attributeSubKindMap.find(field_key);

                                if (subkind_iter != _attributeSubKindMap.end()) {
                                    ContactAttribute attribute;
                                    ContactAttributeBuilder attribute_builder(attribute.edit());

                                    attribute_builder = attribute_builder.setKind(kind_iter->second);
                                    attribute_builder = attribute_builder.setSubKind(subkind_iter->second);
                                    attribute_builder = attribute_builder.setValue(QString(field_obj[field_key].asString().c_str()));

                                    contact_builder = contact_builder.addAttribute(attribute);
                                }
                            }
                        }

                        break;
                    }
                case AttributeKind::Phone:
                case AttributeKind::Email:
                case AttributeKind::Fax:
                case AttributeKind::Pager:
                case AttributeKind::InstantMessaging:
                case AttributeKind::Website:
                case AttributeKind::Profile: {
                        Json::Value field_array = attribute_obj[key];

                        for (int j = 0; j < field_array.size(); j++) {
                            Json::Value field_obj = field_array[j];

                            std::string type = field_obj["type"].asString();
                            std::string value = field_obj["value"].asString();
                            bool pref = field_obj["pref"].asBool();

                            std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = _attributeSubKindMap.find(type);

                            if (subkind_iter != _attributeSubKindMap.end()) {
                                ContactAttribute attribute;
                                ContactAttributeBuilder attribute_builder(attribute.edit());

                                attribute_builder = attribute_builder.setKind(kind_iter->second);
                                attribute_builder = attribute_builder.setSubKind(subkind_iter->second);
                                attribute_builder = attribute_builder.setValue(QString(value.c_str()));

                                contact_builder = contact_builder.addAttribute(attribute);
                            }
                        }

                        break;
                    }
                case AttributeKind::Invalid: {
                        if (key == "addresses") {
                            Json::Value address_array = attribute_obj[key];

                            for (int j = 0; j < address_array.size(); j++) {
                                Json::Value address_obj = address_array[j];

                                ContactPostalAddress address;
                                ContactPostalAddressBuilder address_builder(address.edit());

                                if (address_obj.isMember("type")) {
                                    std::string value = address_obj["type"].asString();
                                    std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = _attributeSubKindMap.find(value);

                                    if (subkind_iter != _attributeSubKindMap.end()) {
                                        address_builder = address_builder.setSubKind(subkind_iter->second);
                                    }
                                }

                                if (address_obj.isMember("address1")) {
                                    std::string value = address_obj["address1"].asString();
                                    address_builder = address_builder.setLine1(QString(value.c_str()));
                                }

                                if (address_obj.isMember("address2")) {
                                    std::string value = address_obj["address2"].asString();
                                    address_builder = address_builder.setLine2(QString(value.c_str()));
                                }

                                if (address_obj.isMember("locality")) {
                                    std::string value = address_obj["locality"].asString();
                                    address_builder = address_builder.setCity(QString(value.c_str()));
                                }

                                if (address_obj.isMember("region")) {
                                    std::string value = address_obj["region"].asString();
                                    address_builder = address_builder.setRegion(QString(value.c_str()));
                                }

                                if (address_obj.isMember("country")) {
                                    std::string value = address_obj["country"].asString();
                                    address_builder = address_builder.setCountry(QString(value.c_str()));
                                }

                                if (address_obj.isMember("postalCode")) {
                                    std::string value = address_obj["postalCode"].asString();
                                    address_builder = address_builder.setPostalCode(QString(value.c_str()));
                                }

                                contact_builder = contact_builder.addPostalAddress(address);
                            }
                        }

                        break;
                    }
            }
        }
    }

    ContactService contact_service;
    contact_service.createContact(new_contact, false);
}

void PimContactsQt::deleteContact(const std::string& contactJson) {
    Json::Reader reader;
    Json::Value obj;
    bool parse = reader.parse(contactJson, obj);

    if (!parse) {
        fprintf(stderr, "%s", "error parsing\n");
        throw "Cannot parse JSON object";
    }

    ContactId contact_id = obj["contactId"].asInt();

    ContactService service;
    service.deleteContact(contact_id);
}

void PimContactsQt::createAttributeKindMap() {
    _attributeKindMap["phoneNumbers"] = AttributeKind::Phone;
    _attributeKindMap["faxNumbers"] = AttributeKind::Fax;
    _attributeKindMap["pagerNumber"] = AttributeKind::Pager;
    _attributeKindMap["emails"] = AttributeKind::Email;
    _attributeKindMap["urls"] = AttributeKind::Website;
    _attributeKindMap["socialNetworks"] = AttributeKind::Profile;
    _attributeKindMap["anniversary"] = AttributeKind::Date;
    _attributeKindMap["birthday"] = AttributeKind::Date;
    _attributeKindMap["name"] = AttributeKind::Name;
    _attributeKindMap["displayName"] = AttributeKind::Name;
    _attributeKindMap["organizations"] = AttributeKind::OrganizationAffiliation;
    _attributeKindMap["education"] = AttributeKind::Education;
    _attributeKindMap["note"] = AttributeKind::Note;
    _attributeKindMap["ims"] = AttributeKind::InstantMessaging;
    _attributeKindMap["videoChat"] = AttributeKind::VideoChat;
    _attributeKindMap["ringtone"] = AttributeKind::Sound;
    //_attributeKindMap["addresses"] = AttributeKind::Invalid;
}

void PimContactsQt::createAttributeSubKindMap() {
    _attributeSubKindMap["other"] = AttributeSubKind::Other;
    _attributeSubKindMap["home"] = AttributeSubKind::Home;
    _attributeSubKindMap["work"] = AttributeSubKind::Work;
    _attributeSubKindMap["mobile"] = AttributeSubKind::PhoneMobile;
    _attributeSubKindMap["direct"] = AttributeSubKind::FaxDirect;
    _attributeSubKindMap["blog"] = AttributeSubKind::Blog;
    _attributeSubKindMap["resume"] = AttributeSubKind::WebsiteResume;
    _attributeSubKindMap["portfolio"] = AttributeSubKind::WebsitePortfolio;
    _attributeSubKindMap["personal"] = AttributeSubKind::WebsitePersonal;
    _attributeSubKindMap["company"] = AttributeSubKind::WebsiteCompany;
    _attributeSubKindMap["facebook"] = AttributeSubKind::ProfileFacebook;
    _attributeSubKindMap["twitter"] = AttributeSubKind::ProfileTwitter;
    _attributeSubKindMap["linkedin"] = AttributeSubKind::ProfileLinkedIn;
    _attributeSubKindMap["gist"] = AttributeSubKind::ProfileGist;
    _attributeSubKindMap["tungle"] = AttributeSubKind::ProfileTungle;
    _attributeSubKindMap["birthday"] = AttributeSubKind::DateBirthday;
    _attributeSubKindMap["anniversary"] = AttributeSubKind::DateAnniversary;
    _attributeSubKindMap["givenName"] = AttributeSubKind::NameGiven;
    _attributeSubKindMap["familyName"] = AttributeSubKind::NameSurname;
    _attributeSubKindMap["honorificPrefix"] = AttributeSubKind::Title;
    _attributeSubKindMap["honorificSuffix"] = AttributeSubKind::NameSuffix;
    _attributeSubKindMap["middleName"] = AttributeSubKind::NameMiddle;
    _attributeSubKindMap["phoneticGivenName"] = AttributeSubKind::NamePhoneticGiven;
    _attributeSubKindMap["phoneticFamilyName"] = AttributeSubKind::NamePhoneticSurname;
    _attributeSubKindMap["name"] = AttributeSubKind::OrganizationAffiliationName;
    _attributeSubKindMap["department"] = AttributeSubKind::OrganizationAffiliationDetails;
    _attributeSubKindMap["title"] = AttributeSubKind::Title;
    _attributeSubKindMap["BbmPin"] = AttributeSubKind::InstantMessagingBbmPin;
    _attributeSubKindMap["Aim"] = AttributeSubKind::InstantMessagingAim;
    _attributeSubKindMap["Aliwangwang"] = AttributeSubKind::InstantMessagingAliwangwang;
    _attributeSubKindMap["GoogleTalk"] = AttributeSubKind::InstantMessagingGoogleTalk;
    _attributeSubKindMap["Sametime"] = AttributeSubKind::InstantMessagingSametime;
    _attributeSubKindMap["Icq"] = AttributeSubKind::InstantMessagingIcq;
    _attributeSubKindMap["Jabber"] = AttributeSubKind::InstantMessagingJabber;
    _attributeSubKindMap["MsLcs"] = AttributeSubKind::InstantMessagingMsLcs;
    _attributeSubKindMap["Skype"] = AttributeSubKind::InstantMessagingSkype;
    _attributeSubKindMap["YahooMessenger"] = AttributeSubKind::InstantMessagingYahooMessenger;
    _attributeSubKindMap["YahooMessegerJapan"] = AttributeSubKind::InstantMessagingYahooMessengerJapan;
    _attributeSubKindMap["BbPlaybook"] = AttributeSubKind::VideoChatBbPlaybook;
}

void PimContactsQt::createKindAttributeMap() {
    _kindAttributeMap[AttributeKind::Phone] = "phoneNumbers";
    _kindAttributeMap[AttributeKind::Fax] = "faxNumbers";
    _kindAttributeMap[AttributeKind::Pager] = "pagerNumber";
    _kindAttributeMap[AttributeKind::Email] = "emails";
    _kindAttributeMap[AttributeKind::Website] = "urls";
    _kindAttributeMap[AttributeKind::Profile] = "socialNetworks";
    //attributeKindMap[AttributeKind::Date] = "anniversary";
    // attributeKindMap[AttributeKind::Date] = "birthday";
    //attributeKindMap["name"] = AttributeKind::Name;
    //attributeKindMap["displayName"] = AttributeKind::Name;
    _kindAttributeMap[AttributeKind::OrganizationAffiliation] = "organizations";
    _kindAttributeMap[AttributeKind::Education] = "education";
    _kindAttributeMap[AttributeKind::Note] = "note";
    _kindAttributeMap[AttributeKind::InstantMessaging] = "ims";
    _kindAttributeMap[AttributeKind::VideoChat] = "videoChat";
    //kindAttributeMap[AttributeKind::Invalid] = "addresses";
    _kindAttributeMap[AttributeKind::Sound] = "ringtone";
    _kindAttributeMap[AttributeKind::Website] = "urls";
}

void PimContactsQt::createSubKindAttributeMap() {
    _subKindAttributeMap[AttributeSubKind::Other] = "other";
    _subKindAttributeMap[AttributeSubKind::Home] = "home";
    _subKindAttributeMap[AttributeSubKind::Work] = "work";
    _subKindAttributeMap[AttributeSubKind::PhoneMobile] = "mobile";
    _subKindAttributeMap[AttributeSubKind::FaxDirect] = "direct";
    _subKindAttributeMap[AttributeSubKind::Blog] = "blog";
    _subKindAttributeMap[AttributeSubKind::WebsiteResume] = "resume";
    _subKindAttributeMap[AttributeSubKind::WebsitePortfolio] = "portfolio";
    _subKindAttributeMap[AttributeSubKind::WebsitePersonal] = "personal";
    _subKindAttributeMap[AttributeSubKind::WebsiteCompany] = "company";
    _subKindAttributeMap[AttributeSubKind::ProfileFacebook] = "facebook";
    _subKindAttributeMap[AttributeSubKind::ProfileTwitter] = "twitter";
    _subKindAttributeMap[AttributeSubKind::ProfileLinkedIn] = "linkedin";
    _subKindAttributeMap[AttributeSubKind::ProfileGist] = "gist";
    _subKindAttributeMap[AttributeSubKind::ProfileTungle] = "tungle";
    _subKindAttributeMap[AttributeSubKind::DateBirthday] = "birthday";
    _subKindAttributeMap[AttributeSubKind::DateAnniversary] = "anniversary";
    _subKindAttributeMap[AttributeSubKind::NameGiven] = "givenName";
    _subKindAttributeMap[AttributeSubKind::NameSurname] = "familyName";
    _subKindAttributeMap[AttributeSubKind::Title] = "honorificPrefix";
    _subKindAttributeMap[AttributeSubKind::NameSuffix] = "honorificSuffix";
    _subKindAttributeMap[AttributeSubKind::NameMiddle] = "middleName";
    _subKindAttributeMap[AttributeSubKind::NamePhoneticGiven] = "phoneticGivenName";
    _subKindAttributeMap[AttributeSubKind::NamePhoneticSurname] = "phoneticFamilyName";
    _subKindAttributeMap[AttributeSubKind::NameNickname] = "nickname";
    _subKindAttributeMap[AttributeSubKind::NameDisplayName] = "displayName";
    _subKindAttributeMap[AttributeSubKind::OrganizationAffiliationName] = "name";
    _subKindAttributeMap[AttributeSubKind::OrganizationAffiliationDetails] = "department";
    _subKindAttributeMap[AttributeSubKind::Title] = "title";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingBbmPin] = "BbmPin";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingAim] = "Aim";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingAliwangwang] = "Aliwangwang";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingGoogleTalk] = "GoogleTalk";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingSametime] = "Sametime";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingIcq] = "Icq";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingJabber] = "Jabber";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingMsLcs] = "MsLcs";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingSkype] = "Skype";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingYahooMessenger] = "YahooMessenger";
    _subKindAttributeMap[AttributeSubKind::InstantMessagingYahooMessengerJapan] = "YahooMessegerJapan";
    _subKindAttributeMap[AttributeSubKind::VideoChatBbPlaybook] = "BbPlaybook";
    _subKindAttributeMap[AttributeSubKind::SoundRingtone] = "ringtone";
}

} // namespace webworks
