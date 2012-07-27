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

std::map<std::string, AttributeKind::Type> PimContactsQt::attributeKindMap;
std::map<std::string, AttributeSubKind::Type> PimContactsQt::attributeSubKindMap;
std::map<AttributeKind::Type, std::string> PimContactsQt::kindAttributeMap;
std::map<AttributeSubKind::Type, std::string> PimContactsQt::subKindAttributeMap;
QList<SortSpecifier> PimContactsQt::sortSpecs;

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

QSet<ContactId> PimContactsQt::singleFieldSearch(const Json::Value& search_field_json, const Json::Value& contact_fields, bool favorite)
{
    ContactService contact_service;
    ContactSearchFilters contact_filter;
    QList<AttributeKind::Type> include_fields;
    QList<SearchField::Type> search_fields;
    QList<Contact> results;
    QSet<ContactId> contact_ids_set;

    switch (search_field_json["fieldName"].asInt()) {
        case SearchField::FirstName:
            search_fields.append(SearchField::FirstName);
            break;
        case SearchField::LastName:
            search_fields.append(SearchField::LastName);
            break;
        case SearchField::CompanyName:
            search_fields.append(SearchField::CompanyName);
            break;
        case SearchField::Phone:
            search_fields.append(SearchField::Phone);
            break;
        case SearchField::Email:
            search_fields.append(SearchField::Email);
            break;
        case SearchField::BBMPin:
            search_fields.append(SearchField::BBMPin);
            break;
        case SearchField::LinkedIn:
            search_fields.append(SearchField::LinkedIn);
            break;
        case SearchField::Twitter:
            search_fields.append(SearchField::Twitter);
            break;
        case SearchField::VideoChat:
            search_fields.append(SearchField::VideoChat);
            break;
    }

    if (!search_fields.empty()) {
        contact_filter.setSearchFields(search_fields);
        contact_filter.setSearchValue(QString(search_field_json["fieldValue"].asString().c_str()));

        if (favorite) {
            contact_filter.setIsFavourite(favorite);
        }

        for (int i = 0; i < contact_fields.size(); i++) {
            std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = attributeKindMap.find(contact_fields[i].asString());

            if (kind_iter != attributeKindMap.end()) {
                include_fields.append(kind_iter->second);
            }
        }

        contact_filter.setShowAttributes(true);
        contact_filter.setIncludeAttributes(include_fields);

        results = contact_service.searchContacts(contact_filter);

        for (int i = 0; i < results.size(); i++) {
            contact_ids_set.insert(results[i].id());
            m_contactSearchMap[results[i].id()] = results[i];
        }
    }

    return contact_ids_set;
}

void PimContactsQt::populateContactField(const Contact& contact, AttributeKind::Type kind, Json::Value& contact_item)
{
    fprintf(stderr, "populateContactField: kind= %d\n", kind);
    QList<ContactAttribute> attrs = contact.filteredAttributes(kind);
    QList<ContactAttribute>::const_iterator k = attrs.constBegin();
    Json::Value array;

    while (k != attrs.constEnd()) {
        ContactAttribute curr_attr = *k;
        Json::Value val;
        std::map<AttributeSubKind::Type, std::string>::const_iterator type_iter = subKindAttributeMap.find(curr_attr.subKind());

        if (type_iter != subKindAttributeMap.end()) {
            val["type"] = Json::Value(type_iter->second);
            val["value"] = Json::Value(curr_attr.value().toStdString());
            array.append(val);
        } else {
            // TODO(rtse): not found in map
            fprintf(stderr, "populateContactField: subkind not found in map: %d\n", curr_attr.subKind());
        }
        ++k;
    }

    std::map<AttributeKind::Type, std::string>::const_iterator field_iter = kindAttributeMap.find(kind);

    if (field_iter != kindAttributeMap.end()) {
        contact_item[field_iter->second] = array;
    } else {
        // TODO(rtse): not found in map
        fprintf(stderr, "populateContactField: kind not found in map%s\n");
    }
}

void PimContactsQt::populateChildField(const Contact& contact, AttributeKind::Type kind, Json::Value& contact_field)
{
    fprintf(stderr, "populateChildField: kind= %d\n", kind);
    QList<ContactAttribute> attrs = contact.filteredAttributes(kind);
    QList<ContactAttribute>::const_iterator k = attrs.constBegin();

    while (k != attrs.constEnd()) {
        ContactAttribute curr_attr = *k;
        Json::Value val;
        std::map<AttributeSubKind::Type, std::string>::const_iterator type_iter = subKindAttributeMap.find(curr_attr.subKind());

        if (type_iter != subKindAttributeMap.end()) {
            contact_field[type_iter->second] = Json::Value(curr_attr.value().toStdString());
        } else {
            // TODO(rtse): not found in map
            fprintf(stderr, "populateContactField: subkind not found in map: %d\n", curr_attr.subKind());
        }
        ++k;
    }
}

void PimContactsQt::populateAddresses(const Contact& contact, Json::Value& contact_addrs)
{
    ContactService contact_service;
    Contact full_contact = contact_service.contactDetails(contact.id());
    QList<ContactPostalAddress> addrs = full_contact.postalAddresses();
    QList<ContactPostalAddress>::const_iterator k = addrs.constBegin();

    while (k != addrs.constEnd()) {
        ContactPostalAddress curr_addr = *k;
        Json::Value addr;

        std::map<AttributeSubKind::Type, std::string>::const_iterator type_iter = subKindAttributeMap.find(curr_addr.subKind());

        if (type_iter != subKindAttributeMap.end()) {
            addr["type"] = Json::Value(type_iter->second);
        }

        addr["address1"] = Json::Value(curr_addr.line1().toStdString());
        addr["address2"] = Json::Value(curr_addr.line2().toStdString());
        addr["country"] = Json::Value(curr_addr.country().toStdString());        
        addr["locality"] = Json::Value(curr_addr.city().toStdString());
        addr["postalCode"] = Json::Value(curr_addr.postalCode().toStdString());
        addr["region"] = Json::Value(curr_addr.region().toStdString());

        contact_addrs.append(addr);
        ++k;
    }
}

void PimContactsQt::populateOrganizations(const Contact& contact, Json::Value& contact_orgs)
{
    QList<QList<ContactAttribute> > org_attrs = contact.filteredAttributesByGroupKey(AttributeKind::OrganizationAffiliation);
    QList<QList<ContactAttribute> >::const_iterator j = org_attrs.constBegin();

    while (j != org_attrs.constEnd()) {
        QList<ContactAttribute> curr_org_attrs = *j;
        QList<ContactAttribute>::const_iterator k = curr_org_attrs.constBegin();
        Json::Value org;

        while (k != curr_org_attrs.constEnd()) {
            ContactAttribute attr = *k;
            std::map<AttributeSubKind::Type, std::string>::const_iterator type_iter = subKindAttributeMap.find(attr.subKind());

            if (type_iter != subKindAttributeMap.end()) {
                org[type_iter->second] = Json::Value(attr.value().toStdString());
            } else {
                // TODO(rtse): not found in map
                fprintf(stderr, "populateOrganizations: subkind not found in map%s\n");
            }

            ++k;
        }

        contact_orgs.append(org);
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
    QList<SortSpecifier>::const_iterator i = PimContactsQt::sortSpecs.constBegin();
    SortSpecifier sort_spec = *i;
    QString val1 = PimContactsQt::getSortFieldValue(sort_spec.first, c1);
    QString val2 = PimContactsQt::getSortFieldValue(sort_spec.first, c2);

    if (sort_spec.second == SortOrder::Ascending) {
        return val1 < val2;
    } else {
        return !(val1 < val2);
    }
}

Json::Value PimContactsQt::assembleSearchResults(const QSet<ContactId>& result_ids, const Json::Value& contact_fields, int limit)
{
    fprintf(stderr, "Beginning of assembleSearchResults, results size=%d\n", result_ids.size());
    QMap<ContactId, Contact> complete_results;

    QSet<ContactId>::const_iterator i = result_ids.constBegin();
    Contact current_contact;

    // put complete contacts in map
    while (i != result_ids.constEnd()) {
        current_contact = m_contactSearchMap[*i];
        complete_results.insertMulti(*i, current_contact);
        ++i;
    }

    // sort results based on sort specs (only using the first field for now)
    QList<Contact> sorted_results = complete_results.values();
    if (!sortSpecs.empty()) {
        qSort(sorted_results.begin(), sorted_results.end(), lessThan);
    }

    Json::Value contact_obj;
    Json::Value contact_array;

    // if limit is -1, returned all available results, otherwise return based on the number passed in find options
    if (limit == -1) {
        limit = sorted_results.size();
    } else {
        limit = min(limit, sorted_results.size());
    }

    for (int i = 0; i < limit; i++) {
        Json::Value contact_item;

        for (int j = 0; j < contact_fields.size(); j++) {
            std::string field = contact_fields[j].asString();
            std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = attributeKindMap.find(field);

            if (kind_iter != attributeKindMap.end()) {
                switch (kind_iter->second) {
                    case AttributeKind::Name: {
                        contact_item[field] = Json::Value();
                        populateChildField(sorted_results[i], kind_iter->second, contact_item[field]);
                        // TODO(rtse): other fields in name
                        break;
                    }

                    case AttributeKind::OrganizationAffiliation: {
                        contact_item[field] = Json::Value();
                        populateOrganizations(sorted_results[i], contact_item[field]);
                        break;
                    }

                    case AttributeKind::Date: {
                        populateChildField(sorted_results[i], kind_iter->second, contact_item);
                        break;
                    }

                    case AttributeKind::Email:
                    case AttributeKind::Fax:
                    case AttributeKind::Pager:
                    case AttributeKind::Phone:
                    case AttributeKind::Profile:
                    case AttributeKind::Website:
                    case AttributeKind::InstantMessaging: {
                        populateContactField(sorted_results[i], kind_iter->second, contact_item);
                        break;
                    }
                }
            } else {
                fprintf(stderr, "cannot find field=%s in map\n", field.c_str());

                if (field == "displayName") {
                    contact_item[field] = Json::Value(sorted_results[i].displayName().toStdString());
                } else if (field == "favorite") {
                    contact_item[field] = Json::Value(sorted_results[i].isFavourite());
                } else if (field == "addresses") {
                    contact_item["addresses"] = Json::Value();
                    populateAddresses(sorted_results[i], contact_item["addresses"]);
                }
            }
        }

        // TODO(rtse): always include id?
        // TODO(rtse): handle fields not under regular kinds/subkinds
        contact_item["id"] = Json::Value(sorted_results[i].id());

        contact_array.append(contact_item);
    }

    contact_obj["contacts"] = contact_array;

    return contact_obj;
}

std::string PimContactsQt::find(const std::string& args_json)
{
    fprintf(stderr, "%s", "Beginning of find\n");
    Json::Reader reader;
    Json::Value args_obj;
    bool parse = reader.parse(args_json, args_obj);

    if (!parse) {
        fprintf(stderr, "%s", "error parsing\n");
        throw "Cannot parse JSON object";
    }

    m_contactSearchMap.clear();
    sortSpecs.clear();

    Json::Value contact_fields;
    QSet<ContactId> results;

    Json::Value filter;
    Json::Value sort;
    int limit;
    bool favorite;

    const Json::Value::Members args_key = args_obj.getMemberNames();

    for (int i = 0; i < args_key.size(); i++) {
        fprintf(stderr, "find, args key size: %d\n", args_key.size());
        const std::string key = args_key[i];

        if (key == "fields") {
            contact_fields = args_obj[key];
        } else if (key == "options") {
            favorite = args_obj[key]["favorite"].asBool();
            limit = args_obj[key]["limit"].asInt();

            filter = args_obj[key]["filter"];
            if (filter.isArray()) {
                for (int j = 0; j < filter.size(); j++) {
                    QSet<ContactId> current_results = singleFieldSearch(filter[j], contact_fields, favorite);

                    fprintf(stderr, "current results[%d] size=%d\n", j, current_results.size());

                    if (current_results.empty()) {
                        // no need to continue, can return right away
                        results = current_results;
                        break;
                    } else {
                        if (j == 0) {
                            results = current_results;
                        } else {
                            results.intersect(current_results);
                        }
                    }
                }
            }

            sort = args_obj[key]["sort"];
            if (sort.isArray()) {
                for (int j = 0; j < sort.size(); j++) {
                    SortOrder::Type order;
                    SortColumn::Type sort_field;

                    if (sort[j]["desc"].asBool()) {
                        order = SortOrder::Descending;
                    } else {
                        order = SortOrder::Ascending;
                    }

                    switch (sort[j]["fieldName"].asInt()) {
                        case SortColumn::FirstName:
                            sort_field = SortColumn::FirstName;
                            break;
                        case SortColumn::LastName:
                            sort_field = SortColumn::LastName;
                            break;
                        case SortColumn::CompanyName:
                            sort_field = SortColumn::CompanyName;
                            break;
                    }

                    sortSpecs.append(SortSpecifier(sort_field, order));
                }
            }
        }
    }

    Json::Value contact_obj = assembleSearchResults(results, contact_fields, limit);
    Json::FastWriter writer;

    return writer.write(contact_obj);
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

        std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = attributeKindMap.find(key);

        if (kind_iter != attributeKindMap.end()) {
            switch (kind_iter->second) {
                case AttributeKind::Name: {
                        // TODO(miwong): Are group keys needed here?
                        Json::Value name_obj = attribute_obj[key];
                        const Json::Value::Members name_fields = name_obj.getMemberNames();

                        for (int j = 0; j < name_fields.size(); j++) {
                            const std::string name_key = name_fields[j];
                            std::map<std::string, AttributeSubKind::Type>::const_iterator name_iter = attributeSubKindMap.find(name_key);

                            if (name_iter != attributeSubKindMap.end()) {
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
                                std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(field_key);

                                if (subkind_iter != attributeSubKindMap.end()) {
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

                            std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(type);

                            if (subkind_iter != attributeSubKindMap.end()) {
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
                                    std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(value);

                                    if (subkind_iter != attributeSubKindMap.end()) {
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
    attributeKindMap["phoneNumbers"] = AttributeKind::Phone;
    attributeKindMap["faxNumbers"] = AttributeKind::Fax;
    attributeKindMap["pagerNumber"] = AttributeKind::Pager;
    attributeKindMap["emails"] = AttributeKind::Email;
    attributeKindMap["urls"] = AttributeKind::Website;
    attributeKindMap["socialNetworks"] = AttributeKind::Profile;
    attributeKindMap["anniversary"] = AttributeKind::Date;
    attributeKindMap["birthday"] = AttributeKind::Date;
    attributeKindMap["name"] = AttributeKind::Name;
    attributeKindMap["displayName"] = AttributeKind::Name;
    attributeKindMap["organizations"] = AttributeKind::OrganizationAffiliation;
    attributeKindMap["education"] = AttributeKind::Education;
    attributeKindMap["note"] = AttributeKind::Note;
    attributeKindMap["ims"] = AttributeKind::InstantMessaging;
    attributeKindMap["videoChat"] = AttributeKind::VideoChat;
    //attributeKindMap["addresses"] = AttributeKind::Invalid;
}

void PimContactsQt::createAttributeSubKindMap() {
    attributeSubKindMap["other"] = AttributeSubKind::Other;
    attributeSubKindMap["home"] = AttributeSubKind::Home;
    attributeSubKindMap["work"] = AttributeSubKind::Work;
    attributeSubKindMap["mobile"] = AttributeSubKind::PhoneMobile;
    attributeSubKindMap["direct"] = AttributeSubKind::FaxDirect;
    attributeSubKindMap["blog"] = AttributeSubKind::Blog;
    attributeSubKindMap["resume"] = AttributeSubKind::WebsiteResume;
    attributeSubKindMap["portfolio"] = AttributeSubKind::WebsitePortfolio;
    attributeSubKindMap["personal"] = AttributeSubKind::WebsitePersonal;
    attributeSubKindMap["company"] = AttributeSubKind::WebsiteCompany;
    attributeSubKindMap["facebook"] = AttributeSubKind::ProfileFacebook;
    attributeSubKindMap["twitter"] = AttributeSubKind::ProfileTwitter;
    attributeSubKindMap["linkedin"] = AttributeSubKind::ProfileLinkedIn;
    attributeSubKindMap["gist"] = AttributeSubKind::ProfileGist;
    attributeSubKindMap["tungle"] = AttributeSubKind::ProfileTungle;
    attributeSubKindMap["birthday"] = AttributeSubKind::DateBirthday;
    attributeSubKindMap["anniversary"] = AttributeSubKind::DateAnniversary;
    attributeSubKindMap["givenName"] = AttributeSubKind::NameGiven;
    attributeSubKindMap["familyName"] = AttributeSubKind::NameSurname;
    attributeSubKindMap["honorificPrefix"] = AttributeSubKind::Title;
    attributeSubKindMap["honorificSuffix"] = AttributeSubKind::NameSuffix;
    attributeSubKindMap["middleName"] = AttributeSubKind::NameMiddle;
    attributeSubKindMap["phoneticGivenName"] = AttributeSubKind::NamePhoneticGiven;
    attributeSubKindMap["phoneticFamilyName"] = AttributeSubKind::NamePhoneticSurname;
    attributeSubKindMap["name"] = AttributeSubKind::OrganizationAffiliationName;
    attributeSubKindMap["department"] = AttributeSubKind::OrganizationAffiliationDetails;
    attributeSubKindMap["title"] = AttributeSubKind::Title;
    attributeSubKindMap["BbmPin"] = AttributeSubKind::InstantMessagingBbmPin;
    attributeSubKindMap["Aim"] = AttributeSubKind::InstantMessagingAim;
    attributeSubKindMap["Aliwangwang"] = AttributeSubKind::InstantMessagingAliwangwang;
    attributeSubKindMap["GoogleTalk"] = AttributeSubKind::InstantMessagingGoogleTalk;
    attributeSubKindMap["Sametime"] = AttributeSubKind::InstantMessagingSametime;
    attributeSubKindMap["Icq"] = AttributeSubKind::InstantMessagingIcq;
    attributeSubKindMap["Jabber"] = AttributeSubKind::InstantMessagingJabber;
    attributeSubKindMap["MsLcs"] = AttributeSubKind::InstantMessagingMsLcs;
    attributeSubKindMap["Skype"] = AttributeSubKind::InstantMessagingSkype;
    attributeSubKindMap["YahooMessenger"] = AttributeSubKind::InstantMessagingYahooMessenger;
    attributeSubKindMap["YahooMessegerJapan"] = AttributeSubKind::InstantMessagingYahooMessengerJapan;
    attributeSubKindMap["BbPlaybook"] = AttributeSubKind::VideoChatBbPlaybook;
}

void PimContactsQt::createKindAttributeMap() {
    kindAttributeMap[AttributeKind::Phone] = "phoneNumbers";
    kindAttributeMap[AttributeKind::Fax] = "faxNumbers";
    kindAttributeMap[AttributeKind::Pager] = "pagerNumber";
    kindAttributeMap[AttributeKind::Email] = "emails";
    kindAttributeMap[AttributeKind::Website] = "urls";
    kindAttributeMap[AttributeKind::Profile] = "socialNetworks";
    //attributeKindMap[AttributeKind::Date] = "anniversary";
    // attributeKindMap[AttributeKind::Date] = "birthday";
    //attributeKindMap["name"] = AttributeKind::Name;
    //attributeKindMap["displayName"] = AttributeKind::Name;
    kindAttributeMap[AttributeKind::OrganizationAffiliation] = "organizations";
    kindAttributeMap[AttributeKind::Education] = "education";
    kindAttributeMap[AttributeKind::Note] = "note";
    kindAttributeMap[AttributeKind::InstantMessaging] = "ims";
    kindAttributeMap[AttributeKind::VideoChat] = "videoChat";
    //kindAttributeMap[AttributeKind::Invalid] = "addresses";
}

void PimContactsQt::createSubKindAttributeMap() {
    subKindAttributeMap[AttributeSubKind::Other] = "other";
    subKindAttributeMap[AttributeSubKind::Home] = "home";
    subKindAttributeMap[AttributeSubKind::Work] = "work";
    subKindAttributeMap[AttributeSubKind::PhoneMobile] = "mobile";
    subKindAttributeMap[AttributeSubKind::FaxDirect] = "direct";
    subKindAttributeMap[AttributeSubKind::Blog] = "blog";
    subKindAttributeMap[AttributeSubKind::WebsiteResume] = "resume";
    subKindAttributeMap[AttributeSubKind::WebsitePortfolio] = "portfolio";
    subKindAttributeMap[AttributeSubKind::WebsitePersonal] = "personal";
    subKindAttributeMap[AttributeSubKind::WebsiteCompany] = "company";
    subKindAttributeMap[AttributeSubKind::ProfileFacebook] = "facebook";
    subKindAttributeMap[AttributeSubKind::ProfileTwitter] = "twitter";
    subKindAttributeMap[AttributeSubKind::ProfileLinkedIn] = "linkedin";
    subKindAttributeMap[AttributeSubKind::ProfileGist] = "gist";
    subKindAttributeMap[AttributeSubKind::ProfileTungle] = "tungle";
    subKindAttributeMap[AttributeSubKind::DateBirthday] = "birthday";
    subKindAttributeMap[AttributeSubKind::DateAnniversary] = "anniversary";
    subKindAttributeMap[AttributeSubKind::NameGiven] = "givenName";
    subKindAttributeMap[AttributeSubKind::NameSurname] = "familyName";
    subKindAttributeMap[AttributeSubKind::Title] = "honorificPrefix";
    subKindAttributeMap[AttributeSubKind::NameSuffix] = "honorificSuffix";
    subKindAttributeMap[AttributeSubKind::NameMiddle] = "middleName";
    subKindAttributeMap[AttributeSubKind::NamePhoneticGiven] = "phoneticGivenName";
    subKindAttributeMap[AttributeSubKind::NamePhoneticSurname] = "phoneticFamilyName";
    subKindAttributeMap[AttributeSubKind::NameNickname] = "nickname"; // TODO(rtse): nickname in JS top-level, but is a subkind under name
    subKindAttributeMap[AttributeSubKind::OrganizationAffiliationName] = "name";
    subKindAttributeMap[AttributeSubKind::OrganizationAffiliationDetails] = "department";
    subKindAttributeMap[AttributeSubKind::Title] = "title";
    subKindAttributeMap[AttributeSubKind::InstantMessagingBbmPin] = "BbmPin";
    subKindAttributeMap[AttributeSubKind::InstantMessagingAim] = "Aim";
    subKindAttributeMap[AttributeSubKind::InstantMessagingAliwangwang] = "Aliwangwang";
    subKindAttributeMap[AttributeSubKind::InstantMessagingGoogleTalk] = "GoogleTalk";
    subKindAttributeMap[AttributeSubKind::InstantMessagingSametime] = "Sametime";
    subKindAttributeMap[AttributeSubKind::InstantMessagingIcq] = "Icq";
    subKindAttributeMap[AttributeSubKind::InstantMessagingJabber] = "Jabber";
    subKindAttributeMap[AttributeSubKind::InstantMessagingMsLcs] = "MsLcs";
    subKindAttributeMap[AttributeSubKind::InstantMessagingSkype] = "Skype";
    subKindAttributeMap[AttributeSubKind::InstantMessagingYahooMessenger] = "YahooMessenger";
    subKindAttributeMap[AttributeSubKind::InstantMessagingYahooMessengerJapan] = "YahooMessegerJapan";
    subKindAttributeMap[AttributeSubKind::VideoChatBbPlaybook] = "BbPlaybook";
}

} // namespace webworks
