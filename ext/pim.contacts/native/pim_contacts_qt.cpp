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

Json::Value PimContactsQt::Find(const Json::Value& optionsObj)
{
    ContactService contact_service;
    QList<Contact> contact_list;

    if (optionsObj.empty()) {
        ContactListFilters contact_filter;
        contact_list = contact_service.contacts(contact_filter);
    } else {
        ContactSearchFilters contact_filter;

        const Json::Value::Members option_keys = optionsObj.getMemberNames();
        QList<SearchField::Type> search_fields;
        std::string search_value;

        for (int i = 0; i < option_keys.size(); i++) {
            const std::string key = option_keys[i];
            const std::string value = optionsObj[key].asString();

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

    return contact_obj;
}

Json::Value PimContactsQt::Save(const Json::Value& attributeObj) {
    if (attributeObj.isMember("id") && attributeObj["id"].isInt()) {
        int contact_id = attributeObj["id"].asInt();
        ContactService service;

        if (contact_id > 0) {
            Contact contact = service.contactDetails(contact_id);

            if (contact.isValid()) {
                return EditContact(contact, attributeObj);
            }
        } else {
            Contact contact = service.contactDetails(contact_id * -1);

            if (contact.isValid()) {
                return CloneContact(contact, attributeObj);
            }
        }
    }

    return CreateContact(attributeObj);
}

Json::Value PimContactsQt::CreateContact(const Json::Value& attributeObj) {
    const Json::Value::Members attribute_keys = attributeObj.getMemberNames();

    Contact new_contact;
    ContactBuilder contact_builder(new_contact.edit());

    for (int i = 0; i < attribute_keys.size(); i++) {
        const std::string key = attribute_keys[i];

        std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = attributeKindMap.find(key);

        if (kind_iter != attributeKindMap.end()) {
            switch (kind_iter->second) {
                case AttributeKind::Name: {
                    /*
                    if (key == "displayName") {
                        std::string display_name = attributeObj[key].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::NameDisplayName, display_name);
                    } else if (key == "nickname") {
                        std::string display_name = attributeObj[key].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::NameNickname, display_name);
                    }
                    */

                    Json::Value name_obj = attributeObj[key];
                    contact_builder = buildGroupedAttributes(contact_builder, name_obj, kind_iter->second);

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
                case AttributeKind::VideoChat: {
                    Json::Value field_array = attributeObj[key];

                    for (int j = 0; j < field_array.size(); j++) {
                        std::string videochat = field_array[j].asString();
                        contact_builder = addAttribute(contact_builder, kind_iter->second, AttributeSubKind::VideoChatBbPlaybook, videochat);
                    }

                    break;
                }
                case AttributeKind::Date:
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

                    // TODO: May need to add attributes in the reverse order to maintain priority/preference
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

    Json::Value return_obj = attributeObj;
    return_obj["id"] = Json::Value(new_contact.id());

    return_obj["_success"] = true;
    return return_obj;
}

Json::Value PimContactsQt::DeleteContact(const Json::Value& contactObj) {
    if (contactObj.isMember("contactId") && contactObj["contactId"].isInt()) {
        ContactId contact_id = contactObj["contactId"].asInt();

        ContactService service;
        service.deleteContact(contact_id);
    }

    Json::Value return_obj;
    return_obj["_success"] = true;
    return return_obj;
}

Json::Value PimContactsQt::EditContact(Contact& contact, const Json::Value& attributeObj) {
    ContactBuilder contact_builder(contact.edit());
    const Json::Value::Members attribute_keys = attributeObj.getMemberNames();

    for (int i = 0; i < attribute_keys.size(); i++) {
        const std::string key = attribute_keys[i];
        std::map<std::string, AttributeKind::Type>::const_iterator kind_iter = attributeKindMap.find(key);

        if (kind_iter != attributeKindMap.end()) {
            switch (kind_iter->second) {
                case AttributeKind::Name: {
                    Json::Value name_obj = attributeObj[key];
                    QList<ContactAttribute> saved_list = contact.filteredAttributes(kind_iter->second);

                    for (int j = 0; j < saved_list.size(); j++) {
                        contact_builder.deleteAttribute(saved_list[j]);
                    }

                    contact_builder = buildGroupedAttributes(contact_builder, name_obj, kind_iter->second);

                    break;
                }
                case AttributeKind::OrganizationAffiliation: {
                    Json::Value attribute_array = attributeObj[key];
                    QList<ContactAttribute> saved_list = contact.filteredAttributes(kind_iter->second);

                    for (int j = 0; j < saved_list.size(); j++) {
                        contact_builder.deleteAttribute(saved_list[j]);
                    }

                    for (int j = 0; j < attribute_array.size(); j++) {
                        Json::Value field_obj = attribute_array[j];
                        contact_builder = buildGroupedAttributes(contact_builder, field_obj, kind_iter->second);
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
                    QList<ContactAttribute> saved_list = contact.filteredAttributes(kind_iter->second);
                    Json::Value json_array = attributeObj[key];
                    int index;

                    for (index = 0; index < json_array.size(); index++) {
                        Json::Value json_field = json_array[index];
                        std::map<std::string, AttributeSubKind::Type>::const_iterator subkind_iter = attributeSubKindMap.find(json_field["type"].asString());

                        if (subkind_iter != attributeSubKindMap.end()) {
                            if (index < saved_list.size()) {
                                ContactAttributeBuilder attribute_builder(saved_list[index].edit());
                                attribute_builder = attribute_builder.setSubKind(subkind_iter->second);
                                attribute_builder = attribute_builder.setValue(QString(json_field["value"].asString().c_str()));
                            } else {
                                ContactAttribute attribute;
                                ContactAttributeBuilder attribute_builder(attribute.edit());
                                attribute_builder = attribute_builder.setKind(kind_iter->second);
                                attribute_builder = attribute_builder.setSubKind(subkind_iter->second);
                                attribute_builder = attribute_builder.setValue(QString(json_field["value"].asString().c_str()));

                                contact_builder = contact_builder.addAttribute(attribute);
                            }
                        }
                    }

                    for (; index < saved_list.size(); index++) {
                        contact_builder = contact_builder.deleteAttribute(saved_list[index]);
                    }

                    break;
                }
            }
        }
    }

    ContactService contact_service;
    contact_service.updateContact(contact);

    Json::Value return_obj = attributeObj;
    return_obj["_success"] = true;

    return return_obj;
}

Json::Value PimContactsQt::CloneContact(Contact& contact, const Json::Value& attributeObj) {
    ContactService service;
    Contact new_contact;
    ContactBuilder contact_builder(new_contact.edit());
    contact_builder = contact_builder.addFromContact(contact);

    // Need to add photos and favorite seperately (addFromContact does not seem to copy these...)
    QList<ContactPhoto> copy_photos = contact.photos();
    for (int i = 0; i < copy_photos.size(); i++) {
        ContactPhoto photo;
        ContactPhotoBuilder photo_builder(photo.edit());
        photo_builder.setOriginalPhoto(copy_photos[i].originalPhoto());
        photo_builder.setPrimaryPhoto(true);
        contact_builder = contact_builder.addPhoto(photo);
    }

    contact_builder = contact_builder.setFavorite(contact.isFavourite());

    new_contact = service.createContact(new_contact, false);
    EditContact(new_contact, attributeObj);

    Json::Value return_obj = attributeObj;
    return_obj["id"] = Json::Value(new_contact.id());

    return_obj["_success"] = true;
    return return_obj;
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
//    attributeKindMap["displayName"] = AttributeKind::Name;
//    attributeKindMap["nickname"] = AttributeKind::Name;
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
    attributeSubKindMap["nickname"] = AttributeSubKind::NameNickname;
    attributeSubKindMap["displayName"] = AttributeSubKind::NameDisplayName;
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
