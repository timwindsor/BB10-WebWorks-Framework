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

StringToKindMap PimContactsQt::_attributeKindMap;
StringToSubKindMap PimContactsQt::_attributeSubKindMap;

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

Json::Value PimContactsQt::Find(const Json::Value& optionsObj)
{
    bbpim::ContactService service;
    QList<bbpim::Contact> contactList;

    if (optionsObj.empty()) {
        bbpim::ContactListFilters contact_filter;
        contactList = service.contacts(contact_filter);
    } else {
        bbpim::ContactSearchFilters contactFilter;

        const Json::Value::Members option_keys = optionsObj.getMemberNames();
        QList<bbpim::SearchField::Type> searchFields;
        std::string searchValue;

        for (int i = 0; i < option_keys.size(); i++) {
            const std::string key = option_keys[i];
            const std::string value = optionsObj[key].asString();

            if (key == "firstName") {
                searchFields.append(bbpim::SearchField::FirstName);
                searchValue = value;
            } else if (key == "lastName") {
                searchFields.append(bbpim::SearchField::LastName);
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

Json::Value PimContactsQt::Save(const Json::Value& attributeObj)
{
    if (attributeObj.isMember("id") && attributeObj["id"].isInt()) {
        int contactId = attributeObj["id"].asInt();
        bbpim::ContactService service;

        if (contactId > 0) {
            bbpim::Contact contact = service.contactDetails(contactId);

            if (contact.isValid()) {
                return EditContact(contact, attributeObj);
            }
        } else {
            bbpim::Contact contact = service.contactDetails(contactId * -1);

            if (contact.isValid()) {
                return CloneContact(contact, attributeObj);
            }
        }
    }

    return CreateContact(attributeObj);
}

Json::Value PimContactsQt::CreateContact(const Json::Value& attributeObj)
{
    const Json::Value::Members attributeKeys = attributeObj.getMemberNames();

    bbpim::Contact newContact;
    bbpim::ContactBuilder contactBuilder(newContact.edit());

    for (int i = 0; i < attributeKeys.size(); i++) {
        const std::string key = attributeKeys[i];
        contactBuilder = buildAttributeKind(contactBuilder, attributeObj[key], key);
    }

    bbpim::ContactService service;
    newContact = service.createContact(newContact, false);

    Json::Value returnObj = attributeObj;
    returnObj["id"] = Json::Value(newContact.id());
    returnObj["_success"] = true;
    return returnObj;
}

Json::Value PimContactsQt::DeleteContact(const Json::Value& contactObj)
{
    if (contactObj.isMember("contactId") && contactObj["contactId"].isInt()) {
        bbpim::ContactId contactId = contactObj["contactId"].asInt();

        bbpim::ContactService service;
        service.deleteContact(contactId);
    }

    Json::Value returnObj;
    returnObj["_success"] = true;
    return returnObj;
}

Json::Value PimContactsQt::EditContact(bbpim::Contact& contact, const Json::Value& attributeObj)
{
    bbpim::ContactBuilder contactBuilder(contact.edit());
    const Json::Value::Members attributeKeys = attributeObj.getMemberNames();

    for (int i = 0; i < attributeKeys.size(); i++) {
        const std::string key = attributeKeys[i];
        if (key == "addresses") {
            QList<bbpim::ContactPostalAddress> savedList = contact.postalAddresses();

            for (int j = 0; j < savedList.size(); j++) {
                contactBuilder = contactBuilder.deletePostalAddress(savedList[j]);
            }
        } else if (key == "photos") {
            QList<bbpim::ContactPhoto> savedList = contact.photos();

            for (int j = 0; j < savedList.size(); j++) {
                contactBuilder = contactBuilder.deletePhoto(savedList[j]);
            }
        } else if (key == "birthday" || key == "anniversary") {
            StringToKindMap::const_iterator kindIter = _attributeKindMap.find(key);
            StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(key);

            if (kindIter != _attributeKindMap.end() && subkindIter != _attributeSubKindMap.end()) {
                QList<bbpim::ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);

                for (int j = 0; j < savedList.size(); j++) {
                    if (savedList[j].subKind() == subkindIter->second) {
                        contactBuilder = contactBuilder.deleteAttribute(savedList[j]);
                    }
                }
            }
        } else {
            StringToKindMap::const_iterator kindIter = _attributeKindMap.find(key);

            if (kindIter != _attributeKindMap.end()) {
                QList<bbpim::ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);

                for (int j = 0; j < savedList.size(); j++) {
                    contactBuilder = contactBuilder.deleteAttribute(savedList[j]);
                }
            }
        }

        contactBuilder = buildAttributeKind(contactBuilder, attributeObj[key], key);
    }

    bbpim::ContactService service;
    service.updateContact(contact);

    Json::Value returnObj = attributeObj;
    returnObj["_success"] = true;

    return returnObj;
}

Json::Value PimContactsQt::CloneContact(bbpim::Contact& contact, const Json::Value& attributeObj)
{
    bbpim::ContactService service;
    bbpim::Contact newContact;
    bbpim::ContactBuilder contactBuilder(newContact.edit());
    contactBuilder = contactBuilder.addFromContact(contact);

    // Need to add photos and favorite seperately (addFromContact does not seem to copy these...)
    QList<bbpim::ContactPhoto> copy_photos = contact.photos();
    for (int i = 0; i < copy_photos.size(); i++) {
        bbpim::ContactPhoto photo;
        bbpim::ContactPhotoBuilder photoBuilder(photo.edit());
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

bbpim::ContactBuilder& PimContactsQt::buildAttributeKind(bbpim::ContactBuilder& contactBuilder, const Json::Value& jsonObj, const std::string& field)
{
    StringToKindMap::const_iterator kindIter = _attributeKindMap.find(field);

    if (kindIter != _attributeKindMap.end()) {
        switch (kindIter->second) {
            // Attributes requiring group keys:
            case bbpim::AttributeKind::Name: {
                contactBuilder = buildGroupedAttributes(contactBuilder, jsonObj, kindIter->second, "1");
                break;
            }
            case bbpim::AttributeKind::OrganizationAffiliation: {
                for (int i = 0; i < jsonObj.size(); i++) {
                    Json::Value fieldObj = jsonObj[i];
                    std::stringstream groupKeyStream;
                    groupKeyStream << i + 1;
                    contactBuilder = buildGroupedAttributes(contactBuilder, fieldObj, kindIter->second, groupKeyStream.str());
                }

                break;
            }

            // String arrays:
            case bbpim::AttributeKind::VideoChat: {
                for (int i = 0; i < jsonObj.size(); i++) {
                    std::string value = jsonObj[i].asString();
                    contactBuilder = addAttribute(contactBuilder, kindIter->second, bbpim::AttributeSubKind::VideoChatBbPlaybook, value);
                }

                break;
            }

            // Strings:
            case bbpim::AttributeKind::Date:
            case bbpim::AttributeKind::Note:
            case bbpim::AttributeKind::Sound: {
                StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(field);

                if (subkindIter != _attributeSubKindMap.end()) {
                    std::string value = jsonObj.asString();
                    contactBuilder = addAttribute(contactBuilder, kindIter->second, subkindIter->second, value);
                }

                break;
            }

            // ContactField attributes:
            case bbpim::AttributeKind::Phone:
            case bbpim::AttributeKind::Email:
            case bbpim::AttributeKind::Fax:
            case bbpim::AttributeKind::Pager:
            case bbpim::AttributeKind::InstantMessaging:
            case bbpim::AttributeKind::Website:
            case bbpim::AttributeKind::Group:
            case bbpim::AttributeKind::Profile: {
                for (int i = 0; i < jsonObj.size(); i++) {
                    Json::Value fieldObj = jsonObj[i];
                    contactBuilder = buildFieldAttribute(contactBuilder, fieldObj, kindIter->second);
                }

                break;
            }

            // Special cases (treated differently in ContactBuilder):
            case bbpim::AttributeKind::Invalid: {
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

            // User defined an unsupported field:
            default: {
                break;
            }
        }
    }

    return contactBuilder;
}

bbpim::ContactBuilder& PimContactsQt::addAttribute(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value)
{
    bbpim::ContactAttribute attribute;
    bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);
    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));

    return contactBuilder.addAttribute(attribute);
}

bbpim::ContactBuilder& PimContactsQt::addAttributeToGroup(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value, const std::string& groupKey)
{
    bbpim::ContactAttribute attribute;
    bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);
    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));
    attributeBuilder = attributeBuilder.setGroupKey(QString(groupKey.c_str()));

    return contactBuilder.addAttribute(attribute);
}

bbpim::ContactBuilder& PimContactsQt::buildGroupedAttributes(bbpim::ContactBuilder& contactBuilder, const Json::Value& fieldsObj, bbpim::AttributeKind::Type kind, const std::string& groupKey)
{
    const Json::Value::Members fields = fieldsObj.getMemberNames();

    for (int i = 0; i < fields.size(); i++) {
        const std::string fieldKey = fields[i];
        StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(fieldKey);

        if (subkindIter != _attributeSubKindMap.end()) {
            contactBuilder = addAttributeToGroup(contactBuilder, kind, subkindIter->second, fieldsObj[fieldKey].asString(), groupKey);
        }
    }

    return contactBuilder;
}

bbpim::ContactBuilder& PimContactsQt::buildFieldAttribute(bbpim::ContactBuilder& contactBuilder, const Json::Value& fieldObj, bbpim::AttributeKind::Type kind)
{
    std::string type = fieldObj["type"].asString();
    StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(type);

    if (subkindIter != _attributeSubKindMap.end()) {
        bool pref = fieldObj["pref"].asBool();
        contactBuilder = addAttribute(contactBuilder, kind, subkindIter->second, fieldObj["value"].asString());
    }

    return contactBuilder;
}

bbpim::ContactBuilder& PimContactsQt::buildPostalAddress(bbpim::ContactBuilder& contactBuilder, const Json::Value& addressObj)
{
    bbpim::ContactPostalAddress address;
    bbpim::ContactPostalAddressBuilder addressBuilder(address.edit());

    if (addressObj.isMember("type")) {
        std::string value = addressObj["type"].asString();
        StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(value);

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

bbpim::ContactBuilder& PimContactsQt::buildPhoto(bbpim::ContactBuilder& contactBuilder, const Json::Value& photoObj)
{
    bbpim::ContactPhoto photo;
    bbpim::ContactPhotoBuilder photoBuilder(photo.edit());

    std::string filepath = photoObj["originalFilePath"].asString();
    bool pref = photoObj["pref"].asBool();

    photoBuilder.setOriginalPhoto(QString(filepath.c_str()));
    photoBuilder.setPrimaryPhoto(pref);

    return contactBuilder.addPhoto(photo, pref);
}

void PimContactsQt::createAttributeKindMap()
{
    _attributeKindMap["phoneNumbers"] = bbpim::AttributeKind::Phone;
    _attributeKindMap["faxNumbers"] = bbpim::AttributeKind::Fax;
    _attributeKindMap["pagerNumbers"] = bbpim::AttributeKind::Pager;
    _attributeKindMap["emails"] = bbpim::AttributeKind::Email;
    _attributeKindMap["urls"] = bbpim::AttributeKind::Website;
    _attributeKindMap["socialNetworks"] = bbpim::AttributeKind::Profile;
    _attributeKindMap["anniversary"] = bbpim::AttributeKind::Date;
    _attributeKindMap["birthday"] = bbpim::AttributeKind::Date;
    _attributeKindMap["categories"] = bbpim::AttributeKind::Group;
    _attributeKindMap["name"] = bbpim::AttributeKind::Name;
    _attributeKindMap["organizations"] = bbpim::AttributeKind::OrganizationAffiliation;
    _attributeKindMap["education"] = bbpim::AttributeKind::Education;
    _attributeKindMap["note"] = bbpim::AttributeKind::Note;
    _attributeKindMap["ims"] = bbpim::AttributeKind::InstantMessaging;
    _attributeKindMap["ringtone"] = bbpim::AttributeKind::Sound;
    _attributeKindMap["videoChat"] = bbpim::AttributeKind::VideoChat;
    _attributeKindMap["addresses"] = bbpim::AttributeKind::Invalid;
    _attributeKindMap["favorite"] = bbpim::AttributeKind::Invalid;
    _attributeKindMap["photos"] = bbpim::AttributeKind::Invalid;
}

void PimContactsQt::createAttributeSubKindMap()
{
    _attributeSubKindMap["other"] = bbpim::AttributeSubKind::Other;
    _attributeSubKindMap["home"] = bbpim::AttributeSubKind::Home;
    _attributeSubKindMap["work"] = bbpim::AttributeSubKind::Work;
    _attributeSubKindMap["mobile"] = bbpim::AttributeSubKind::PhoneMobile;
    _attributeSubKindMap["direct"] = bbpim::AttributeSubKind::FaxDirect;
    _attributeSubKindMap["blog"] = bbpim::AttributeSubKind::Blog;
    _attributeSubKindMap["resume"] = bbpim::AttributeSubKind::WebsiteResume;
    _attributeSubKindMap["portfolio"] = bbpim::AttributeSubKind::WebsitePortfolio;
    _attributeSubKindMap["personal"] = bbpim::AttributeSubKind::WebsitePersonal;
    _attributeSubKindMap["company"] = bbpim::AttributeSubKind::WebsiteCompany;
    _attributeSubKindMap["facebook"] = bbpim::AttributeSubKind::ProfileFacebook;
    _attributeSubKindMap["twitter"] = bbpim::AttributeSubKind::ProfileTwitter;
    _attributeSubKindMap["linkedin"] = bbpim::AttributeSubKind::ProfileLinkedIn;
    _attributeSubKindMap["gist"] = bbpim::AttributeSubKind::ProfileGist;
    _attributeSubKindMap["tungle"] = bbpim::AttributeSubKind::ProfileTungle;
    _attributeSubKindMap["birthday"] = bbpim::AttributeSubKind::DateBirthday;
    _attributeSubKindMap["anniversary"] = bbpim::AttributeSubKind::DateAnniversary;
    _attributeSubKindMap["categories"] = bbpim::AttributeSubKind::GroupDepartment;
    _attributeSubKindMap["givenName"] = bbpim::AttributeSubKind::NameGiven;
    _attributeSubKindMap["familyName"] = bbpim::AttributeSubKind::NameSurname;
    _attributeSubKindMap["honorificPrefix"] = bbpim::AttributeSubKind::Title;
    _attributeSubKindMap["honorificSuffix"] = bbpim::AttributeSubKind::NameSuffix;
    _attributeSubKindMap["middleName"] = bbpim::AttributeSubKind::NameMiddle;
    _attributeSubKindMap["nickname"] = bbpim::AttributeSubKind::NameNickname;
    _attributeSubKindMap["displayName"] = bbpim::AttributeSubKind::NameDisplayName;
    _attributeSubKindMap["phoneticGivenName"] = bbpim::AttributeSubKind::NamePhoneticGiven;
    _attributeSubKindMap["phoneticFamilyName"] = bbpim::AttributeSubKind::NamePhoneticSurname;
    _attributeSubKindMap["name"] = bbpim::AttributeSubKind::OrganizationAffiliationName;
    _attributeSubKindMap["department"] = bbpim::AttributeSubKind::OrganizationAffiliationDetails;
    _attributeSubKindMap["title"] = bbpim::AttributeSubKind::Title;
    _attributeSubKindMap["BbmPin"] = bbpim::AttributeSubKind::InstantMessagingBbmPin;
    _attributeSubKindMap["Aim"] = bbpim::AttributeSubKind::InstantMessagingAim;
    _attributeSubKindMap["Aliwangwang"] = bbpim::AttributeSubKind::InstantMessagingAliwangwang;
    _attributeSubKindMap["GoogleTalk"] = bbpim::AttributeSubKind::InstantMessagingGoogleTalk;
    _attributeSubKindMap["Sametime"] = bbpim::AttributeSubKind::InstantMessagingSametime;
    _attributeSubKindMap["Icq"] = bbpim::AttributeSubKind::InstantMessagingIcq;
    _attributeSubKindMap["Jabber"] = bbpim::AttributeSubKind::InstantMessagingJabber;
    _attributeSubKindMap["MsLcs"] = bbpim::AttributeSubKind::InstantMessagingMsLcs;
    _attributeSubKindMap["Skype"] = bbpim::AttributeSubKind::InstantMessagingSkype;
    _attributeSubKindMap["YahooMessenger"] = bbpim::AttributeSubKind::InstantMessagingYahooMessenger;
    _attributeSubKindMap["YahooMessegerJapan"] = bbpim::AttributeSubKind::InstantMessagingYahooMessengerJapan;
    _attributeSubKindMap["BbPlaybook"] = bbpim::AttributeSubKind::VideoChatBbPlaybook;
    _attributeSubKindMap["ringtone"] = bbpim::AttributeSubKind::SoundRingtone;
    _attributeSubKindMap["note"] = bbpim::AttributeSubKind::Invalid;
}

} // namespace webworks
