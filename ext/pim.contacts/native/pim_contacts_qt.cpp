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

std::map<std::string, AttributeKind::Type> PimContactsQt::_attributeKindMap;
std::map<std::string, AttributeSubKind::Type> PimContactsQt::_attributeSubKindMap;

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
    ContactService service;
    QList<Contact> contactList;

    if (optionsObj.empty()) {
        ContactListFilters contact_filter;
        contactList = service.contacts(contact_filter);
    } else {
        ContactSearchFilters contactFilter;

        const Json::Value::Members option_keys = optionsObj.getMemberNames();
        QList<SearchField::Type> searchFields;
        std::string searchValue;

        for (int i = 0; i < option_keys.size(); i++) {
            const std::string key = option_keys[i];
            const std::string value = optionsObj[key].asString();

            if (key == "firstName") {
                searchFields.append(SearchField::FirstName);
                searchValue = value;
            } else if (key == "lastName") {
                searchFields.append(SearchField::LastName);
                searchValue = value;
            }
        }

        if (!searchFields.empty()) {
            contactFilter = contactFilter.setSearchFields(searchFields);
            contactFilter = contactFilter.setSearchValue(QString(searchValue.c_str()));
        }

        contactList = service.searchContacts(contactFilter);
    }
    
    Json::Value contactObj;
    Json::Value contactArray;

    for (int i = 0; i < contactList.size(); i++) {
        Json::Value contactItem;

        contactItem["displayName"] = Json::Value(contactList[i].displayName().toStdString());
        contactItem["contactId"] = Json::Value(contactList[i].id());

        contactArray.append(contactItem);
    }

    contactObj["contacts"] = contactArray;

    return contactObj;
}

Json::Value PimContactsQt::Save(const Json::Value& attributeObj) {
    if (attributeObj.isMember("id") && attributeObj["id"].isInt()) {
        int contactId = attributeObj["id"].asInt();
        ContactService service;

        if (contactId > 0) {
            Contact contact = service.contactDetails(contactId);

            if (contact.isValid()) {
                return EditContact(contact, attributeObj);
            }
        } else {
            Contact contact = service.contactDetails(contactId * -1);

            if (contact.isValid()) {
                return CloneContact(contact, attributeObj);
            }
        }
    }

    return CreateContact(attributeObj);
}

Json::Value PimContactsQt::CreateContact(const Json::Value& attributeObj) {
    const Json::Value::Members attributeKeys = attributeObj.getMemberNames();

    Contact newContact;
    ContactBuilder contactBuilder(newContact.edit());

    for (int i = 0; i < attributeKeys.size(); i++) {
        const std::string key = attributeKeys[i];
        contactBuilder = buildAttributeKind(contactBuilder, attributeObj[key], key);
    }

    ContactService service;
    newContact = service.createContact(newContact, false);

    Json::Value returnObj = attributeObj;
    returnObj["id"] = Json::Value(newContact.id());
    returnObj["_success"] = true;
    return returnObj;
}

Json::Value PimContactsQt::DeleteContact(const Json::Value& contactObj) {
    if (contactObj.isMember("contactId") && contactObj["contactId"].isInt()) {
        ContactId contactId = contactObj["contactId"].asInt();

        ContactService service;
        service.deleteContact(contactId);
    }

    Json::Value returnObj;
    returnObj["_success"] = true;
    return returnObj;
}

Json::Value PimContactsQt::EditContact(Contact& contact, const Json::Value& attributeObj) {
    ContactBuilder contactBuilder(contact.edit());
    const Json::Value::Members attributeKeys = attributeObj.getMemberNames();

    for (int i = 0; i < attributeKeys.size(); i++) {
        const std::string key = attributeKeys[i];
        if (key == "addresses") {
            QList<ContactPostalAddress> savedList = contact.postalAddresses();

            for (int j = 0; j < savedList.size(); j++) {
                contactBuilder = contactBuilder.deletePostalAddress(savedList[j]);
            }
        } else if (key == "photos") {
            QList<ContactPhoto> savedList = contact.photos();

            for (int j = 0; j < savedList.size(); j++) {
                contactBuilder = contactBuilder.deletePhoto(savedList[j]);
            }
        } else if (key == "birthday" || key == "anniversary") {
            std::map<std::string, AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(key);
            std::map<std::string, AttributeSubKind::Type>::const_iterator subkindIter = _attributeSubKindMap.find(key);

            if (kindIter != _attributeKindMap.end() && subkindIter != _attributeSubKindMap.end()) {
                QList<ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);

                for (int j = 0; j < savedList.size(); j++) {
                    if (savedList[j].subKind() == subkindIter->second) {
                        contactBuilder = contactBuilder.deleteAttribute(savedList[j]);
                    }
                }
            }
        } else {
            std::map<std::string, AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(key);

            if (kindIter != _attributeKindMap.end()) {
                QList<ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);

                for (int j = 0; j < savedList.size(); j++) {
                    contactBuilder = contactBuilder.deleteAttribute(savedList[j]);
                }
            }
        }

        contactBuilder = buildAttributeKind(contactBuilder, attributeObj[key], key);
    }

    ContactService service;
    service.updateContact(contact);

    Json::Value returnObj = attributeObj;
    returnObj["_success"] = true;

    return returnObj;
}

Json::Value PimContactsQt::CloneContact(Contact& contact, const Json::Value& attributeObj) {
    ContactService service;
    Contact newContact;
    ContactBuilder contactBuilder(newContact.edit());
    contactBuilder = contactBuilder.addFromContact(contact);

    // Need to add photos and favorite seperately (addFromContact does not seem to copy these...)
    QList<ContactPhoto> copy_photos = contact.photos();
    for (int i = 0; i < copy_photos.size(); i++) {
        ContactPhoto photo;
        ContactPhotoBuilder photoBuilder(photo.edit());
        photoBuilder.setOriginalPhoto(copy_photos[i].originalPhoto());
        photoBuilder.setPrimaryPhoto(true);
        contactBuilder = contactBuilder.addPhoto(photo);
    }

    contactBuilder = contactBuilder.setFavorite(contact.isFavourite());

    newContact = service.createContact(newContact, false);
    EditContact(newContact, attributeObj);

    Json::Value returnObj = attributeObj;
    returnObj["id"] = Json::Value(newContact.id());

    returnObj["_success"] = true;
    return returnObj;
}

ContactBuilder& PimContactsQt::buildAttributeKind(ContactBuilder& contactBuilder, const Json::Value& jsonObj, const std::string& field) {
    std::map<std::string, AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(field);

    if (kindIter != _attributeKindMap.end()) {
        switch (kindIter->second) {
            case AttributeKind::Name: {
                contactBuilder = buildGroupedAttributes(contactBuilder, jsonObj, kindIter->second, "1");
                break;
            }
            case AttributeKind::OrganizationAffiliation: {
                for (int i = 0; i < jsonObj.size(); i++) {
                    Json::Value fieldObj = jsonObj[i];
                    std::stringstream groupKeyStream;
                    groupKeyStream << i + 1;
                    contactBuilder = buildGroupedAttributes(contactBuilder, fieldObj, kindIter->second, groupKeyStream.str());
                }

                break;
            }
            case AttributeKind::VideoChat: {
                for (int i = 0; i < jsonObj.size(); i++) {
                    std::string value = jsonObj[i].asString();
                    contactBuilder = addAttribute(contactBuilder, kindIter->second, AttributeSubKind::VideoChatBbPlaybook, value);
                }

                break;
            }
            case AttributeKind::Date:
            case AttributeKind::Note:
            case AttributeKind::Sound: {
                std::map<std::string, AttributeSubKind::Type>::const_iterator subkindIter = _attributeSubKindMap.find(field);

                if (subkindIter != _attributeSubKindMap.end()) {
                    std::string value = jsonObj.asString();
                    contactBuilder = addAttribute(contactBuilder, kindIter->second, subkindIter->second, value);
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
                for (int i = 0; i < jsonObj.size(); i++) {
                    Json::Value fieldObj = jsonObj[i];
                    contactBuilder = buildFieldAttribute(contactBuilder, fieldObj, kindIter->second);
                }

                break;
            }
            case AttributeKind::Invalid: {
                if (field == "addresses") {
                    for (int i = 0; i < jsonObj.size(); i++) {
                        Json::Value addressObj = jsonObj[i];
                        contactBuilder = buildPostalAddress(contactBuilder, addressObj);
                    }
                } else if (field == "photos") {
                    for (int i = 0; i < jsonObj.size(); i++) {
                        Json::Value photoObj = jsonObj[i];
                        contactBuilder = buildPhoto(contactBuilder, photoObj);
                    }
                } else if (field == "favorite") {
                    bool isFavorite = jsonObj.asBool();
                    contactBuilder = contactBuilder.setFavorite(isFavorite);
                }

                break;
            }
        }
    }

    return contactBuilder;
}

ContactBuilder& PimContactsQt::addAttribute(ContactBuilder& contactBuilder, const AttributeKind::Type kind, const AttributeSubKind::Type subkind, const std::string& value) {
    ContactAttribute attribute;
    ContactAttributeBuilder attributeBuilder(attribute.edit());

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);
    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));

    return contactBuilder.addAttribute(attribute);
}

ContactBuilder& PimContactsQt::addAttributeToGroup(ContactBuilder& contactBuilder, const AttributeKind::Type kind, const AttributeSubKind::Type subkind, const std::string& value, const std::string& groupKey) {
    ContactAttribute attribute;
    ContactAttributeBuilder attributeBuilder(attribute.edit());

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);
    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));
    attributeBuilder = attributeBuilder.setGroupKey(QString(groupKey.c_str()));

    return contactBuilder.addAttribute(attribute);
}

ContactBuilder& PimContactsQt::buildGroupedAttributes(ContactBuilder& contactBuilder, const Json::Value& fieldsObj, AttributeKind::Type kind, const std::string& groupKey) {
    const Json::Value::Members fields = fieldsObj.getMemberNames();

    for (int i = 0; i < fields.size(); i++) {
        const std::string fieldKey = fields[i];
        std::map<std::string, AttributeSubKind::Type>::const_iterator subkindIter = _attributeSubKindMap.find(fieldKey);

        if (subkindIter != _attributeSubKindMap.end()) {
            contactBuilder = addAttributeToGroup(contactBuilder, kind, subkindIter->second, fieldsObj[fieldKey].asString(), groupKey);
        }
    }

    return contactBuilder;
}

ContactBuilder& PimContactsQt::buildFieldAttribute(ContactBuilder& contactBuilder, const Json::Value& fieldObj, AttributeKind::Type kind) {
    std::string type = fieldObj["type"].asString();
    std::map<std::string, AttributeSubKind::Type>::const_iterator subkindIter = _attributeSubKindMap.find(type);

    if (subkindIter != _attributeSubKindMap.end()) {
        bool pref = fieldObj["pref"].asBool();
        contactBuilder = addAttribute(contactBuilder, kind, subkindIter->second, fieldObj["value"].asString());
    }

    return contactBuilder;
}

ContactBuilder& PimContactsQt::buildPostalAddress(ContactBuilder& contactBuilder, const Json::Value& addressObj) {
    ContactPostalAddress address;
    ContactPostalAddressBuilder addressBuilder(address.edit());

    if (addressObj.isMember("type")) {
        std::string value = addressObj["type"].asString();
        std::map<std::string, AttributeSubKind::Type>::const_iterator subkindIter = _attributeSubKindMap.find(value);

        if (subkindIter != _attributeSubKindMap.end()) {
            addressBuilder = addressBuilder.setSubKind(subkindIter->second);
        }
    }

    addressBuilder = addressBuilder.setLine1(QString(addressObj.get("address1", "").asCString()));
    addressBuilder = addressBuilder.setLine2(QString(addressObj.get("address2", "").asCString()));
    addressBuilder = addressBuilder.setCity(QString(addressObj.get("locality", "").asCString()));
    addressBuilder = addressBuilder.setRegion(QString(addressObj.get("region", "").asCString()));
    addressBuilder = addressBuilder.setCountry(QString(addressObj.get("country", "").asCString()));
    addressBuilder = addressBuilder.setPostalCode(QString(addressObj.get("postalCode", "").asCString()));

    return contactBuilder.addPostalAddress(address);
}

ContactBuilder& PimContactsQt::buildPhoto(ContactBuilder& contactBuilder, const Json::Value& photoObj) {
    ContactPhoto photo;
    ContactPhotoBuilder photoBuilder(photo.edit());

    std::string filepath = photoObj["originalFilePath"].asString();
    bool pref = photoObj["pref"].asBool();

    photoBuilder.setOriginalPhoto(QString(filepath.c_str()));
    photoBuilder.setPrimaryPhoto(pref);

    return contactBuilder.addPhoto(photo, pref);
}

void PimContactsQt::createAttributeKindMap() {
    _attributeKindMap["phoneNumbers"] = AttributeKind::Phone;
    _attributeKindMap["faxNumbers"] = AttributeKind::Fax;
    _attributeKindMap["pagerNumbers"] = AttributeKind::Pager;
    _attributeKindMap["emails"] = AttributeKind::Email;
    _attributeKindMap["urls"] = AttributeKind::Website;
    _attributeKindMap["socialNetworks"] = AttributeKind::Profile;
    _attributeKindMap["anniversary"] = AttributeKind::Date;
    _attributeKindMap["birthday"] = AttributeKind::Date;
    _attributeKindMap["categories"] = AttributeKind::Group;
    _attributeKindMap["name"] = AttributeKind::Name;
    _attributeKindMap["organizations"] = AttributeKind::OrganizationAffiliation;
    _attributeKindMap["education"] = AttributeKind::Education;
    _attributeKindMap["note"] = AttributeKind::Note;
    _attributeKindMap["ims"] = AttributeKind::InstantMessaging;
    _attributeKindMap["ringtone"] = AttributeKind::Sound;
    _attributeKindMap["videoChat"] = AttributeKind::VideoChat;
    _attributeKindMap["addresses"] = AttributeKind::Invalid;
    _attributeKindMap["favorite"] = AttributeKind::Invalid;
    _attributeKindMap["photos"] = AttributeKind::Invalid;
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
    _attributeSubKindMap["categories"] = AttributeSubKind::GroupDepartment;
    _attributeSubKindMap["givenName"] = AttributeSubKind::NameGiven;
    _attributeSubKindMap["familyName"] = AttributeSubKind::NameSurname;
    _attributeSubKindMap["honorificPrefix"] = AttributeSubKind::Title;
    _attributeSubKindMap["honorificSuffix"] = AttributeSubKind::NameSuffix;
    _attributeSubKindMap["middleName"] = AttributeSubKind::NameMiddle;
    _attributeSubKindMap["nickname"] = AttributeSubKind::NameNickname;
    _attributeSubKindMap["displayName"] = AttributeSubKind::NameDisplayName;
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
    _attributeSubKindMap["ringtone"] = AttributeSubKind::SoundRingtone;
    _attributeSubKindMap["note"] = AttributeSubKind::Invalid;
}

} // namespace webworks
