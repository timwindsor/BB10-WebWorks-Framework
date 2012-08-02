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
#include <QSet>
#include <QMap>
#include <QtAlgorithms>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include "pim_contacts_qt.hpp"

namespace webworks {

StringToKindMap PimContactsQt::_attributeKindMap;
StringToSubKindMap PimContactsQt::_attributeSubKindMap;
KindToStringMap PimContactsQt::_kindAttributeMap;
SubKindToStringMap PimContactsQt::_subKindAttributeMap;
QList<bbpim::SortSpecifier> PimContactsQt::_sortSpecs;

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

/****************************************************************
 * Public Functions
 ****************************************************************/

Json::Value PimContactsQt::Find(const Json::Value& argsObj)
{
    fprintf(stderr, "%s", "Beginning of find\n");

    _contactSearchMap.clear();
    _sortSpecs.clear();

    Json::Value contactFields;
    QSet<bbpim::ContactId> results;

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
                    QSet<bbpim::ContactId> currentResults = singleFieldSearch(filter[j], contactFields, favorite);

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
                    bbpim::SortOrder::Type order;
                    bbpim::SortColumn::Type sortField;

                    if (sort[j]["desc"].asBool()) {
                        order = bbpim::SortOrder::Descending;
                    } else {
                        order = bbpim::SortOrder::Ascending;
                    }

                    switch (sort[j]["fieldName"].asInt()) {
                        case bbpim::SortColumn::FirstName:
                            sortField = bbpim::SortColumn::FirstName;
                            break;
                        case bbpim::SortColumn::LastName:
                            sortField = bbpim::SortColumn::LastName;
                            break;
                        case bbpim::SortColumn::CompanyName:
                            sortField = bbpim::SortColumn::CompanyName;
                            break;
                    }

                    _sortSpecs.append(bbpim::SortSpecifier(sortField, order));
                }
            }
        }
    }

    Json::Value returnObj;
    returnObj["_success"] = true;
    returnObj["contacts"] = assembleSearchResults(results, contactFields, limit);

    return returnObj;
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

/****************************************************************
 * Helper functions for Find
 ****************************************************************/

QList<bbpim::SearchField::Type> PimContactsQt::getSearchFields(const Json::Value& searchFieldsJson)
{
    QList<bbpim::SearchField::Type> searchFields;

    switch (searchFieldsJson["fieldName"].asInt()) {
        case bbpim::SearchField::FirstName:
            searchFields.append(bbpim::SearchField::FirstName);
            break;
        case bbpim::SearchField::LastName:
            searchFields.append(bbpim::SearchField::LastName);
            break;
        case bbpim::SearchField::CompanyName:
            searchFields.append(bbpim::SearchField::CompanyName);
            break;
        case bbpim::SearchField::Phone:
            searchFields.append(bbpim::SearchField::Phone);
            break;
        case bbpim::SearchField::Email:
            searchFields.append(bbpim::SearchField::Email);
            break;
        case bbpim::SearchField::BBMPin:
            searchFields.append(bbpim::SearchField::BBMPin);
            break;
        case bbpim::SearchField::LinkedIn:
            searchFields.append(bbpim::SearchField::LinkedIn);
            break;
        case bbpim::SearchField::Twitter:
            searchFields.append(bbpim::SearchField::Twitter);
            break;
        case bbpim::SearchField::VideoChat:
            searchFields.append(bbpim::SearchField::VideoChat);
            break;
    }

    return searchFields;
}

QSet<bbpim::ContactId> PimContactsQt::singleFieldSearch(const Json::Value& searchFieldsJson, const Json::Value& contact_fields, bool favorite)
{
    QList<bbpim::SearchField::Type> searchFields = PimContactsQt::getSearchFields(searchFieldsJson);
    QSet<bbpim::ContactId> contactIds;

    if (!searchFields.empty()) {
        bbpim::ContactService contactService;
        bbpim::ContactSearchFilters contactFilter;
        QList<bbpim::AttributeKind::Type> includeFields;
        QList<bbpim::Contact> results;

        contactFilter.setSearchFields(searchFields);
        contactFilter.setSearchValue(QString(searchFieldsJson["fieldValue"].asString().c_str()));

        if (favorite) {
            contactFilter.setIsFavourite(favorite);
        }

        for (int i = 0; i < contact_fields.size(); i++) {
            std::map<std::string, bbpim::AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(contact_fields[i].asString());

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

void PimContactsQt::populateField(const bbpim::Contact& contact, bbpim::AttributeKind::Type kind, Json::Value& contactItem, bool isContactField, bool isArray)
{
    fprintf(stderr, "populateField kind= %d\n", kind);
    QList<bbpim::ContactAttribute> attrs = contact.filteredAttributes(kind);
    QList<bbpim::ContactAttribute>::const_iterator k = attrs.constBegin();

    while (k != attrs.constEnd()) {
        bbpim::ContactAttribute currentAttr = *k;
        Json::Value val;
        SubKindToStringMap::const_iterator typeIter = _subKindAttributeMap.find(currentAttr.subKind());

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
                    if (kind == bbpim::AttributeKind::Date) {
                        QString format = "yyyy-MM-dd";
                        contactItem[typeIter->second] = Json::Value(currentAttr.valueAsDateTime().date().toString(format).toStdString());
                    } else {
                        if (kind == bbpim::AttributeKind::Note) {
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

void PimContactsQt::populateAddresses(const bbpim::Contact& contact, Json::Value& contactAddrs)
{
    bbpim::ContactService contactService;
    bbpim::Contact fullContact = contactService.contactDetails(contact.id());
    QList<bbpim::ContactPostalAddress> addrs = fullContact.postalAddresses();
    QList<bbpim::ContactPostalAddress>::const_iterator k = addrs.constBegin();

    while (k != addrs.constEnd()) {
        bbpim::ContactPostalAddress currentAddr = *k;
        Json::Value addr;

        SubKindToStringMap::const_iterator typeIter = _subKindAttributeMap.find(currentAddr.subKind());

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

void PimContactsQt::populateOrganizations(const bbpim::Contact& contact, Json::Value& contactOrgs)
{
    QList<QList<bbpim::ContactAttribute> > orgAttrs = contact.filteredAttributesByGroupKey(bbpim::AttributeKind::OrganizationAffiliation);
    QList<QList<bbpim::ContactAttribute> >::const_iterator j = orgAttrs.constBegin();

    while (j != orgAttrs.constEnd()) {
        QList<bbpim::ContactAttribute> currentOrgAttrs = *j;
        QList<bbpim::ContactAttribute>::const_iterator k = currentOrgAttrs.constBegin();
        Json::Value org;

        while (k != currentOrgAttrs.constEnd()) {
            bbpim::ContactAttribute attr = *k;
            SubKindToStringMap::const_iterator typeIter = _subKindAttributeMap.find(attr.subKind());

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

void PimContactsQt::populatePhotos(const bbpim::Contact& contact, Json::Value& contactPhotos)
{
    bbpim::ContactService contactService;
    bbpim::Contact fullContact = contactService.contactDetails(contact.id());
    QList<bbpim::ContactPhoto> photos = fullContact.photos();
    bbpim::ContactPhoto primaryPhoto = fullContact.primaryPhoto();
    QList<bbpim::ContactPhoto>::const_iterator k = photos.constBegin();

    while (k != photos.constEnd()) {
        Json::Value photo;

        photo["originalFilePath"] = Json::Value((*k).originalPhoto().toStdString());
        photo["largeFilePath"] = Json::Value((*k).largePhoto().toStdString());
        photo["smallFilePath"] = Json::Value((*k).smallPhoto().toStdString());
        photo["pref"] = Json::Value((primaryPhoto.id() == (*k).id()));

        contactPhotos.append(photo);
        ++k;
    }
}

QString PimContactsQt::getSortFieldValue(const bbpim::SortColumn::Type sort_field, const bbpim::Contact& contact)
{
    switch (sort_field) {
        case bbpim::SortColumn::FirstName:
            return contact.sortFirstName();
        case bbpim::SortColumn::LastName:
            return contact.sortLastName();
        case bbpim::SortColumn::CompanyName:
            return contact.sortCompanyName();
    }

    return QString();
}

bool PimContactsQt::lessThan(const bbpim::Contact& c1, const bbpim::Contact& c2)
{
    QList<bbpim::SortSpecifier>::const_iterator i = PimContactsQt::_sortSpecs.constBegin();
    bbpim::SortSpecifier sortSpec;
    QString val1, val2;

    do {
        sortSpec = *i;
        val1 = PimContactsQt::getSortFieldValue(sortSpec.first, c1);
        val2 = PimContactsQt::getSortFieldValue(sortSpec.first, c2);
        ++i;
    } while (val1 == val2 && i != PimContactsQt::_sortSpecs.constEnd());

    if (sortSpec.second == bbpim::SortOrder::Ascending) {
        return val1 < val2;
    } else {
        return !(val1 < val2);
    }
}

Json::Value PimContactsQt::assembleSearchResults(const QSet<bbpim::ContactId>& resultIds, const Json::Value& contactFields, int limit)
{
    fprintf(stderr, "Beginning of assembleSearchResults, results size=%d\n", resultIds.size());
    QMap<bbpim::ContactId, bbpim::Contact> completeResults;
    QSet<bbpim::ContactId>::const_iterator i = resultIds.constBegin();

    // put complete contacts in map
    while (i != resultIds.constEnd()) {
        completeResults.insertMulti(*i, _contactSearchMap[*i]);
        ++i;
    }

    // sort results based on sort specs
    QList<bbpim::Contact> sortedResults = completeResults.values();
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
            std::map<std::string, bbpim::AttributeKind::Type>::const_iterator kindIter = _attributeKindMap.find(field);

            if (kindIter != _attributeKindMap.end()) {
                switch (kindIter->second) {
                    case bbpim::AttributeKind::Name: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], false, false);
                        break;
                    }

                    case bbpim::AttributeKind::OrganizationAffiliation: {
                        contactItem[field] = Json::Value();
                        populateOrganizations(sortedResults[i], contactItem[field]);
                        break;
                    }

                    case bbpim::AttributeKind::Date: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case bbpim::AttributeKind::Note: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case bbpim::AttributeKind::Sound: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case bbpim::AttributeKind::VideoChat: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], false, false);
                        break;
                    }

                    case bbpim::AttributeKind::Email:
                    case bbpim::AttributeKind::Fax:
                    case bbpim::AttributeKind::Pager:
                    case bbpim::AttributeKind::Phone:
                    case bbpim::AttributeKind::Profile:
                    case bbpim::AttributeKind::Website:
                    case bbpim::AttributeKind::InstantMessaging: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], true, false);
                        break;
                    }

                    // Special cases (treated differently in ContactBuilder):
                    default: {
                        if (field == "addresses") {
                            contactItem[field] = Json::Value();
                            populateAddresses(sortedResults[i], contactItem[field]);
                        } else if (field == "photos") {
                            contactItem[field] = Json::Value();
                            populatePhotos(sortedResults[i], contactItem[field]);
                        } else if (field == "favorite") {
                            contactItem[field] = Json::Value(sortedResults[i].isFavourite());
                        }

                        break;
                    }
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

/****************************************************************
 * Helper functions for Save
 ****************************************************************/

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
            default: {
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

/****************************************************************
 * Mapping functions
 ****************************************************************/

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

void PimContactsQt::createKindAttributeMap() {
    _kindAttributeMap[bbpim::AttributeKind::Phone] = "phoneNumbers";
    _kindAttributeMap[bbpim::AttributeKind::Fax] = "faxNumbers";
    _kindAttributeMap[bbpim::AttributeKind::Pager] = "pagerNumber";
    _kindAttributeMap[bbpim::AttributeKind::Email] = "emails";
    _kindAttributeMap[bbpim::AttributeKind::Website] = "urls";
    _kindAttributeMap[bbpim::AttributeKind::Profile] = "socialNetworks";
    //attributeKindMap[bbpim::AttributeKind::Date] = "anniversary";
    // attributeKindMap[bbpim::AttributeKind::Date] = "birthday";
    //attributeKindMap["name"] = bbpim::AttributeKind::Name;
    //attributeKindMap["displayName"] = bbpim::AttributeKind::Name;
    _kindAttributeMap[bbpim::AttributeKind::OrganizationAffiliation] = "organizations";
    _kindAttributeMap[bbpim::AttributeKind::Education] = "education";
    _kindAttributeMap[bbpim::AttributeKind::Note] = "note";
    _kindAttributeMap[bbpim::AttributeKind::InstantMessaging] = "ims";
    _kindAttributeMap[bbpim::AttributeKind::VideoChat] = "videoChat";
    //kindAttributeMap[bbpim::AttributeKind::Invalid] = "addresses";
    _kindAttributeMap[bbpim::AttributeKind::Sound] = "ringtone";
    _kindAttributeMap[bbpim::AttributeKind::Website] = "urls";
}

void PimContactsQt::createSubKindAttributeMap() {
    _subKindAttributeMap[bbpim::AttributeSubKind::Other] = "other";
    _subKindAttributeMap[bbpim::AttributeSubKind::Home] = "home";
    _subKindAttributeMap[bbpim::AttributeSubKind::Work] = "work";
    _subKindAttributeMap[bbpim::AttributeSubKind::PhoneMobile] = "mobile";
    _subKindAttributeMap[bbpim::AttributeSubKind::FaxDirect] = "direct";
    _subKindAttributeMap[bbpim::AttributeSubKind::Blog] = "blog";
    _subKindAttributeMap[bbpim::AttributeSubKind::WebsiteResume] = "resume";
    _subKindAttributeMap[bbpim::AttributeSubKind::WebsitePortfolio] = "portfolio";
    _subKindAttributeMap[bbpim::AttributeSubKind::WebsitePersonal] = "personal";
    _subKindAttributeMap[bbpim::AttributeSubKind::WebsiteCompany] = "company";
    _subKindAttributeMap[bbpim::AttributeSubKind::ProfileFacebook] = "facebook";
    _subKindAttributeMap[bbpim::AttributeSubKind::ProfileTwitter] = "twitter";
    _subKindAttributeMap[bbpim::AttributeSubKind::ProfileLinkedIn] = "linkedin";
    _subKindAttributeMap[bbpim::AttributeSubKind::ProfileGist] = "gist";
    _subKindAttributeMap[bbpim::AttributeSubKind::ProfileTungle] = "tungle";
    _subKindAttributeMap[bbpim::AttributeSubKind::DateBirthday] = "birthday";
    _subKindAttributeMap[bbpim::AttributeSubKind::DateAnniversary] = "anniversary";
    _subKindAttributeMap[bbpim::AttributeSubKind::NameGiven] = "givenName";
    _subKindAttributeMap[bbpim::AttributeSubKind::NameSurname] = "familyName";
    _subKindAttributeMap[bbpim::AttributeSubKind::Title] = "honorificPrefix";
    _subKindAttributeMap[bbpim::AttributeSubKind::NameSuffix] = "honorificSuffix";
    _subKindAttributeMap[bbpim::AttributeSubKind::NameMiddle] = "middleName";
    _subKindAttributeMap[bbpim::AttributeSubKind::NamePhoneticGiven] = "phoneticGivenName";
    _subKindAttributeMap[bbpim::AttributeSubKind::NamePhoneticSurname] = "phoneticFamilyName";
    _subKindAttributeMap[bbpim::AttributeSubKind::NameNickname] = "nickname";
    _subKindAttributeMap[bbpim::AttributeSubKind::NameDisplayName] = "displayName";
    _subKindAttributeMap[bbpim::AttributeSubKind::OrganizationAffiliationName] = "name";
    _subKindAttributeMap[bbpim::AttributeSubKind::OrganizationAffiliationDetails] = "department";
    _subKindAttributeMap[bbpim::AttributeSubKind::Title] = "title";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingBbmPin] = "BbmPin";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingAim] = "Aim";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingAliwangwang] = "Aliwangwang";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingGoogleTalk] = "GoogleTalk";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingSametime] = "Sametime";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingIcq] = "Icq";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingJabber] = "Jabber";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingMsLcs] = "MsLcs";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingSkype] = "Skype";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingYahooMessenger] = "YahooMessenger";
    _subKindAttributeMap[bbpim::AttributeSubKind::InstantMessagingYahooMessengerJapan] = "YahooMessegerJapan";
    _subKindAttributeMap[bbpim::AttributeSubKind::VideoChatBbPlaybook] = "BbPlaybook";
    _subKindAttributeMap[bbpim::AttributeSubKind::SoundRingtone] = "ringtone";
}

} // namespace webworks

