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

#include <json/value.h>
#include <json/writer.h>
#include <json/reader.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <map>
#include "pim_contacts_qt.hpp"

namespace webworks {

std::map<std::string, AttributeKind::Type> PimContactsQt::attributeKindMap;
std::map<std::string, AttributeSubKind::Type> PimContactsQt::attributeSubKindMap;

PimContactsQt::PimContactsQt()
{
    static bool mapInit = false;

    if (!mapInit) {
        createAttributeKindMap();
        createAttributeSubKindMap();
        mapInit = true;
    }
}

PimContactsQt::~PimContactsQt()
{
}

std::string PimContactsQt::find(const std::string& optionsJson)
{
    Json::Reader reader;
    Json::Value options_obj;
    bool parse = reader.parse(optionsJson, options_obj);

    if (!parse) {
        fprintf(stderr, "%s", "error parsing\n");
        throw "Cannot parse JSON object";
    }

    ContactService contact_service;
    QList<Contact> contact_list;

    if (options_obj.empty()) {
        ContactListFilters contact_filter;
        contact_list = contact_service.contacts(contact_filter);
    } else {
        ContactSearchFilters contact_filter;

        const Json::Value::Members option_keys = options_obj.getMemberNames();
        QList<SearchField::Type> search_fields;
        std::string search_value;

        for (int i = 0; i < option_keys.size(); i++) {
            const std::string key = option_keys[i];
            const std::string value = options_obj[key].asString();

            if (key == "firstName") {
                search_fields.append(SearchField::FirstName);
                search_value = value;
            } else if (key == "lastName") {
                search_fields.append(SearchField::LastName);
                search_value = value;
            }
        }

        if (!search_fields.empty()) {
            contact_filter = contact_filter.setSearchFields(search_fields);
            contact_filter = contact_filter.setSearchValue(QString(search_value.c_str()));
        }

        contact_list = contact_service.searchContacts(contact_filter);
    }
    
    Json::Value contact_obj;
    Json::Value contact_array;

    for (int i = 0; i < contact_list.size(); i++) {
        Json::Value contact_item;

        contact_item["displayName"] = Json::Value(contact_list[i].displayName().toStdString());
        contact_item["contactId"] = Json::Value(contact_list[i].id());

        contact_array.append(contact_item);
    }

    contact_obj["contacts"] = contact_array;

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
                case AttributeKind::Name:
                    {
                        // TODO: Are group keys needed here?
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
                case AttributeKind::OrganizationAffiliation:
                    {
                        Json::Value attribute_array = attribute_obj[key];

                        for (int j = 0; j < attribute_array.size(); j++) {
                            Json::Value field_obj = attribute_array[j];
                            const Json::Value::Members fields = field_obj.getMemberNames();

                            // TODO: need to use group keys for organizations

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
                case AttributeKind::Profile:
                    {
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
                case AttributeKind::Invalid:
                    {
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
    attributeKindMap["addresses"] = AttributeKind::Invalid;
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

} // namespace webworks
