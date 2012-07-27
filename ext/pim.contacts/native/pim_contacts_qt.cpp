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
    // TODO: Should the mappings be done in native or in JavaScript?
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

std::string PimContactsQt::Find(const std::string& optionsJson)
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

void PimContactsQt::Save(const std::string& attributeJson) {
    Json::Reader reader;
    Json::Value attribute_obj;
    bool parse = reader.parse(attributeJson, attribute_obj);

    if (!parse) {
        fprintf(stderr, "%s", "error parsing\n");
        throw "Cannot parse JSON object";
    }

    if (attribute_obj.isMember("id")) {
        ContactService service;
        Contact contact = service.contactDetails(attribute_obj["id"].asInt());

        if (contact.isValid()) {
            EditContact(contact, attribute_obj);
            return;
        }
    }

    CreateContact(attribute_obj);
}

void PimContactsQt::CreateContact(const Json::Value& attributeObj) {
    const Json::Value::Members attribute_keys = attributeObj.getMemberNames();

    Contact new_contact;
    ContactBuilder contact_builder(new_contact.edit());

    for (int i = 0; i < attribute_keys.size(); i++) {
        const std::string key = attribute_keys[i];

        std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = attributeKindMap.find(key);

        if (kind_iter != attributeKindMap.end()) {
            switch (kind_iter->second) {
                case AttributeKind::Name: {
                    if (key == "displayName") {
                        std::string display_name = attributeObj[key].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::NameDisplayName, display_name);
                    } else if (key == "nickname") {
                        std::string display_name = attributeObj[key].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::NameNickname, display_name);
                    } else {
                        Json::Value name_obj = attributeObj[key];
                        contact_builder = buildGroupedAttributes(contact_builder, name_obj, kind_iter->second);
                    }
                    break;
                }
                case AttributeKind::OrganizationAffiliation: {
                    Json::Value attribute_array = attributeObj[key];

                    for (int j = 0; j < attribute_array.size(); j++) {
                        Json::Value field_obj = attribute_array[j];
                        contact_builder = buildGroupedAttributes(contact_builder, field_obj, kind_iter->second);
                    }

                    break;
                }
                case AttributeKind::Date: {
                    std::string date = attributeObj[key].asString();

                    if (key == "birthday") {
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::DateBirthday, date);
                    } else if (key == "anniversary") {
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::DateAnniversary, date);
                    }

                    break;
                }
                case AttributeKind::VideoChat: {
                    Json::Value field_array = attributeObj[key];

                    for (int j = 0; j < field_array.size(); j++) {
                        std::string videochat = field_array[j].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::VideoChatBbPlaybook, videochat);
                    }

                    break;
                }
                case AttributeKind::Note:
                case AttributeKind::Sound: {
                    std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(key);

                    if (subkind_iter != attributeSubKindMap.end()) {
                        std::string value = attributeObj[key].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, subkind_iter->second, value);
                    }

                    break;
                }
                case AttributeKind::Phone:
                case AttributeKind::Email:
                case AttributeKind::Fax:
                case AttributeKind::Pager:
                case AttributeKind::InstantMessaging:
                case AttributeKind::Website:
                case AttributeKind::Group:
                case AttributeKind::Profile: {
                    Json::Value field_array = attributeObj[key];

                    for (int j = 0; j < field_array.size(); j++) {
                        Json::Value field_obj = field_array[j];
                        contact_builder = buildFieldAttribute(contact_builder, field_obj, kind_iter->second);
                    }

                    break;
                }
                case AttributeKind::Invalid: {
                    if (key == "addresses") {
                        Json::Value address_array = attributeObj[key];

                        for (int j = 0; j < address_array.size(); j++) {
                            Json::Value address_obj = address_array[j];
                            contact_builder = buildPostalAddress(contact_builder, address_obj);
                        }
                    } else if (key == "photos") {
                        Json::Value photo_array = attributeObj[key];

                        for (int j = 0; j < photo_array.size(); j++) {
                            Json::Value photo_obj = photo_array[j];
                            contact_builder = buildPhoto(contact_builder, photo_obj);
                        }
                    } else if (key == "favorite") {
                        bool is_favorite = attributeObj[key].asBool();
                        contact_builder = contact_builder.setFavorite(is_favorite);
                    }

                    break;
                }
            }
        }
    }

    ContactService contact_service;
    new_contact = contact_service.createContact(new_contact, false);

    // TODO: return ContactId
}

void PimContactsQt::DeleteContact(const std::string& contactJson) {
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

void PimContactsQt::EditContact(Contact& contact, const Json::Value& attributeObj) {
}

ContactBuilder& PimContactsQt::addAttribute(ContactBuilder& contactBuilder, const AttributeKind::Type kind, const AttributeSubKind::Type subkind, const std::string& value) {
    ContactAttribute attribute;
    ContactAttributeBuilder attribute_builder(attribute.edit());

    attribute_builder = attribute_builder.setKind(kind);
    attribute_builder = attribute_builder.setSubKind(subkind);
    attribute_builder = attribute_builder.setValue(QString(value.c_str()));

    return contactBuilder.addAttribute(attribute);
}

ContactBuilder& PimContactsQt::addAttributeToGroup(ContactBuilder& contactBuilder, const AttributeKind::Type kind, const AttributeSubKind::Type subkind, const std::string& value, const std::string& groupKey) {
    ContactAttribute attribute;
    ContactAttributeBuilder attribute_builder(attribute.edit());

    attribute_builder = attribute_builder.setKind(kind);
    attribute_builder = attribute_builder.setSubKind(subkind);
    attribute_builder = attribute_builder.setValue(QString(value.c_str()));
    attribute_builder = attribute_builder.setGroupKey(QString(groupKey.c_str()));

    return contactBuilder.addAttribute(attribute);
}

ContactBuilder& PimContactsQt::buildGroupedAttributes(ContactBuilder& contactBuilder, const Json::Value& fieldsObj, AttributeKind::Type kind) {
    const Json::Value::Members fields = fieldsObj.getMemberNames();

    // TODO: need a better way to create group keys (perhaps based on user data)
    std::stringstream ss;
    ss << rand();
    ss << fieldsObj.get("givenName", "").asString();
    ss << fieldsObj.get("middleName", "").asString();
    ss << fieldsObj.get("familyName", "").asString();
    ss << fieldsObj.get("name", "").asString();
    ss << fieldsObj.get("title", "").asString();
    ss << fieldsObj.get("department", "").asString();
    std::string group_key = ss.str();

    for (int i = 0; i < fields.size(); i++) {
        const std::string field_key = fields[i];
        std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(field_key);

        if (subkind_iter != attributeSubKindMap.end()) {
            contactBuilder = addAttributeToGroup(contactBuilder, kind, subkind_iter->second, fieldsObj[field_key].asString(), group_key);
        }
    }

    return contactBuilder;
}

ContactBuilder& PimContactsQt::buildFieldAttribute(ContactBuilder& contactBuilder, const Json::Value& fieldObj, AttributeKind::Type kind) {
    std::string type = fieldObj["type"].asString();
    std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(type);

    if (subkind_iter != attributeSubKindMap.end()) {
        //std::string value = fieldObj["value"].asString();
        bool pref = fieldObj["pref"].asBool();

        contactBuilder = addAttribute(contactBuilder, kind, subkind_iter->second, fieldObj["value"].asString());
    }

    return contactBuilder;
}

ContactBuilder& PimContactsQt::buildPostalAddress(ContactBuilder& contactBuilder, const Json::Value& addressObj) {
    ContactPostalAddress address;
    ContactPostalAddressBuilder address_builder(address.edit());

    if (addressObj.isMember("type")) {
        std::string value = addressObj["type"].asString();
        std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(value);

        if (subkind_iter != attributeSubKindMap.end()) {
            address_builder = address_builder.setSubKind(subkind_iter->second);
        }
    }

    if (addressObj.isMember("address1")) {
        std::string value = addressObj["address1"].asString();
        address_builder = address_builder.setLine1(QString(value.c_str()));
    }

    if (addressObj.isMember("address2")) {
        std::string value = addressObj["address2"].asString();
        address_builder = address_builder.setLine2(QString(value.c_str()));
    }

    if (addressObj.isMember("locality")) {
        std::string value = addressObj["locality"].asString();
        address_builder = address_builder.setCity(QString(value.c_str()));
    }

    if (addressObj.isMember("region")) {
        std::string value = addressObj["region"].asString();
        address_builder = address_builder.setRegion(QString(value.c_str()));
    }

    if (addressObj.isMember("country")) {
        std::string value = addressObj["country"].asString();
        address_builder = address_builder.setCountry(QString(value.c_str()));
    }

    if (addressObj.isMember("postalCode")) {
        std::string value = addressObj["postalCode"].asString();
        address_builder = address_builder.setPostalCode(QString(value.c_str()));
    }

    return contactBuilder.addPostalAddress(address);
}

ContactBuilder& PimContactsQt::buildPhoto(ContactBuilder& contactBuilder, const Json::Value& photoObj) {
    ContactPhoto photo;
    ContactPhotoBuilder photo_builder(photo.edit());

    std::string filepath = photoObj["originalFilePath"].asString();
    bool pref = photoObj["pref"].asBool();

    photo_builder.setOriginalPhoto(QString(filepath.c_str()));
    photo_builder.setPrimaryPhoto(pref);

    return contactBuilder.addPhoto(photo, pref);
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
    attributeKindMap["categories"] = AttributeKind::Group;
    attributeKindMap["name"] = AttributeKind::Name;
    attributeKindMap["displayName"] = AttributeKind::Name;
    attributeKindMap["nickname"] = AttributeKind::Name;
    attributeKindMap["organizations"] = AttributeKind::OrganizationAffiliation;
    attributeKindMap["education"] = AttributeKind::Education;
    attributeKindMap["note"] = AttributeKind::Note;
    attributeKindMap["ims"] = AttributeKind::InstantMessaging;
    attributeKindMap["ringtone"] = AttributeKind::Sound;
    attributeKindMap["videoChat"] = AttributeKind::VideoChat;
    attributeKindMap["addresses"] = AttributeKind::Invalid;
    attributeKindMap["favorite"] = AttributeKind::Invalid;
    attributeKindMap["photos"] = AttributeKind::Invalid;
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
    attributeSubKindMap["categories"] = AttributeSubKind::GroupDepartment;
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
    attributeSubKindMap["ringtone"] = AttributeSubKind::SoundRingtone;
    attributeSubKindMap["note"] = AttributeSubKind::Invalid;
}

} // namespace webworks
