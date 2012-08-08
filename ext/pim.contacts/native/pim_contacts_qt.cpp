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
std::map<bbpim::ContactId, bbpim::Contact> PimContactsQt::_contactSearchMap;

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
    Json::Value contactFields;
    QSet<bbpim::ContactId> results;
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
            results = getPartialSearchResults(argsObj[key]["filter"], contactFields, favorite);
            getSortSpecs(argsObj[key]["sort"]);
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
    Json::Value returnObj;
    const Json::Value::Members attributeKeys = attributeObj.getMemberNames();

    bbpim::Contact newContact;
    bbpim::ContactBuilder contactBuilder(newContact.edit());

    for (int i = 0; i < attributeKeys.size(); i++) {
        const std::string key = attributeKeys[i];
        Json::Value returnField = addAttributeKind(contactBuilder, attributeObj[key], key);

        if (!returnField.empty()) {
            returnObj[key] = returnField;
        }
    }

    bbpim::ContactService service;
    newContact = service.createContact(newContact, false);

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
    Json::Value returnObj;
    bbpim::ContactBuilder contactBuilder(contact.edit());
    const Json::Value::Members attributeKeys = attributeObj.getMemberNames();

    for (int i = 0; i < attributeKeys.size(); i++) {
        const std::string key = attributeKeys[i];
        Json::Value returnField = syncAttributeKind(contact, attributeObj[key], key);

        //if (!returnField.empty()) {
            returnObj[key] = returnField;
        //}
    }

    bbpim::ContactService service;
    service.updateContact(contact);

    returnObj["id"] = contact.id();
    returnObj["_success"] = true;

    return returnObj;
}

Json::Value PimContactsQt::CloneContact(bbpim::Contact& contact, const Json::Value& attributeObj)
{
    bbpim::ContactService service;
    bbpim::Contact newContact;
    bbpim::ContactBuilder contactBuilder(newContact.edit());
    contactBuilder = contactBuilder.addFromContact(contact);
    contactBuilder = contactBuilder.setFavorite(contact.isFavourite());

    newContact = service.createContact(newContact, false);
    Json::Value returnObj = EditContact(newContact, attributeObj);

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

void PimContactsQt::getSortSpecs(const Json::Value& sort)
{
    _sortSpecs.clear();

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

QSet<bbpim::ContactId> PimContactsQt::getPartialSearchResults(const Json::Value& filter, const Json::Value& contactFields, const bool favorite)
{
    QSet<bbpim::ContactId> results;

    _contactSearchMap.clear();

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

    return results;
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
                // multiple fields can map to the same kind, only add kind to the list if it's not already added
                if (includeFields.count(kindIter->second) == 0) {
                    includeFields.append(kindIter->second);
                }
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

        addr["streetAddress"] = Json::Value(currentAddr.line1().toStdString());
        addr["streetOther"] = Json::Value(currentAddr.line2().toStdString());
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

                    case bbpim::AttributeKind::Date:
                    case bbpim::AttributeKind::Note:
                    case bbpim::AttributeKind::Sound: {
                        populateField(sortedResults[i], kindIter->second, contactItem, false, false);
                        break;
                    }

                    case bbpim::AttributeKind::VideoChat: {
                        contactItem[field] = Json::Value();
                        populateField(sortedResults[i], kindIter->second, contactItem[field], false, true);
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

        contactItem["id"] = Json::Value(sortedResults[i].id());

        contactArray.append(contactItem);
    }

    return contactArray;
}

/****************************************************************
 * Helper functions for Save
 ****************************************************************/

Json::Value PimContactsQt::addAttributeKind(bbpim::ContactBuilder& contactBuilder, const Json::Value& jsonObj, const std::string& field)
{
    Json::Value returnObj;
    StringToKindMap::const_iterator kindIter = _attributeKindMap.find(field);

    if (kindIter != _attributeKindMap.end()) {
        switch (kindIter->second) {
            // Attributes requiring group keys:
            case bbpim::AttributeKind::Name: {
                QList<SubkindValuePair> convertedList = convertGroupedAttributes(jsonObj, returnObj);
                addConvertedGroupedList(contactBuilder, kindIter->second, convertedList, "1");
                break;
            }
            case bbpim::AttributeKind::OrganizationAffiliation: {
                for (int i = 0; i < jsonObj.size(); i++) {
                    Json::Value orgObj;
                    std::stringstream groupKeyStream;
                    groupKeyStream << i + 1;

                    QList<SubkindValuePair> convertedList = convertGroupedAttributes(jsonObj[i], orgObj);
                    addConvertedGroupedList(contactBuilder, kindIter->second, convertedList, groupKeyStream.str());
                    returnObj.append(orgObj);
                }

                break;
            }

            // String arrays:
            case bbpim::AttributeKind::VideoChat: {
                QList<SubkindValuePair> convertedList = convertStringArray(jsonObj, returnObj, bbpim::AttributeSubKind::VideoChatBbPlaybook);
                addConvertedList(contactBuilder, kindIter->second, convertedList);
                break;
            }

            // Dates:
            case bbpim::AttributeKind::Date: {
                StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(field);

                if (subkindIter != _attributeSubKindMap.end()) {
                    std::string value = jsonObj.asString();
                    addAttributeDate(contactBuilder, kindIter->second, subkindIter->second, value);
                    returnObj = jsonObj;
                }

                break;
            }

            // Strings:
            case bbpim::AttributeKind::Note:
            case bbpim::AttributeKind::Sound: {
                StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(field);

                if (subkindIter != _attributeSubKindMap.end()) {
                    QList<SubkindValuePair> convertedList;
                    std::string value = jsonObj.asString();
                    convertedList.append(SubkindValuePair(subkindIter->second, value));
                    addConvertedList(contactBuilder, kindIter->second, convertedList);
                    returnObj = jsonObj;
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
                QList<SubkindValuePair> convertedList = convertFieldAttributes(jsonObj, returnObj);
                addConvertedList(contactBuilder, kindIter->second, convertedList);
                break;
            }

            // Special cases (treated differently in ContactBuilder):
            default: {
                if (field == "addresses") {
                    for (int i = 0; i < jsonObj.size(); i++) {
                        Json::Value addressObj = jsonObj[i];
                        returnObj.append(addPostalAddress(contactBuilder, addressObj));
                    }
                } else if (field == "photos") {
                    for (int i = 0; i < jsonObj.size(); i++) {
                        Json::Value photoObj = jsonObj[i];
                        returnObj.append(addPhoto(contactBuilder, photoObj));
                    }
                } else if (field == "favorite") {
                    bool isFavorite = jsonObj.asBool();
                    contactBuilder.setFavorite(isFavorite);
                    returnObj = jsonObj;
                }

                break;
            }
        }
    }

    return returnObj;
}

Json::Value PimContactsQt::syncAttributeKind(bbpim::Contact& contact, const Json::Value& jsonObj, const std::string& field)
{
    Json::Value returnObj;
    StringToKindMap::const_iterator kindIter = _attributeKindMap.find(field);
    bbpim::ContactBuilder contactBuilder(contact.edit());

    if (kindIter != _attributeKindMap.end()) {
        switch (kindIter->second) {
            // Attributes requiring group keys:
            case bbpim::AttributeKind::Name: {
                QList<QList<bbpim::ContactAttribute> > savedList = contact.filteredAttributesByGroupKey(kindIter->second);
                QList<SubkindValuePair> convertedList = convertGroupedAttributes(jsonObj, returnObj);

                if (!savedList.empty()) {
                    syncConvertedGroupedList(contactBuilder, kindIter->second, savedList[0], convertedList, "1");
                } else {
                    addConvertedGroupedList(contactBuilder, kindIter->second, convertedList, "1");
                }

                break;
            }
            case bbpim::AttributeKind::OrganizationAffiliation: {
                QList<QList<bbpim::ContactAttribute> > savedList = contact.filteredAttributesByGroupKey(kindIter->second);
                returnObj = syncAttributeGroup(contactBuilder, kindIter->second, savedList, jsonObj);
                break;
            }

            // String arrays:
            case bbpim::AttributeKind::VideoChat: {
                QList<bbpim::ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);
                QList<SubkindValuePair> convertedList = convertStringArray(jsonObj, returnObj, bbpim::AttributeSubKind::VideoChatBbPlaybook);
                syncConvertedList(contactBuilder, kindIter->second, savedList, convertedList);
                break;
            }

            // Dates:
            case bbpim::AttributeKind::Date: {
                StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(field);

                if (subkindIter != _attributeSubKindMap.end()) {
                    QList<bbpim::ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);
                    syncAttributeDate(contactBuilder, savedList, subkindIter->second, jsonObj.asString());
                    returnObj = jsonObj;
                }

                break;
            }

            // Strings:
            case bbpim::AttributeKind::Note:
            case bbpim::AttributeKind::Sound: {
                QList<bbpim::ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);
                QList<SubkindValuePair> convertedList;
                StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(field);

                if (subkindIter != _attributeSubKindMap.end()) {
                    std::string value = jsonObj.asString();
                    convertedList.append(SubkindValuePair(subkindIter->second, value));
                    returnObj = jsonObj;
                }

                syncConvertedList(contactBuilder, kindIter->second, savedList, convertedList);
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
                QList<bbpim::ContactAttribute> savedList = contact.filteredAttributes(kindIter->second);
                QList<SubkindValuePair> convertedList = convertFieldAttributes(jsonObj, returnObj);
                syncConvertedList(contactBuilder, kindIter->second, savedList, convertedList);
                break;
            }

            // Special cases (treated differently in ContactBuilder):
            default: {
                if (field == "addresses") {
                    QList<bbpim::ContactPostalAddress> savedList = contact.postalAddresses();
                    syncPostalAddresses(contactBuilder, savedList, jsonObj);
                    returnObj = jsonObj;
                } else if (field == "photos") {
                    QList<bbpim::ContactPhoto> savedList = contact.photos();
                    syncPhotos(contactBuilder, savedList, jsonObj);
                    returnObj = jsonObj;

                } else if (field == "favorite") {
                    bool isFavorite = jsonObj.asBool();
                    contactBuilder.setFavorite(isFavorite);
                    returnObj = jsonObj;
                }

                break;
            }
        }
    }

    return returnObj;
}


QList<SubkindValuePair> PimContactsQt::convertGroupedAttributes(const Json::Value& fieldsObj, Json::Value& returnObj)
{
    const Json::Value::Members fields = fieldsObj.getMemberNames();
    QList<SubkindValuePair> convertedList;

    for (int i = 0; i < fields.size(); i++) {
        const std::string fieldKey = fields[i];
        StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(fieldKey);

        if (subkindIter != _attributeSubKindMap.end()) {
            convertedList.append(SubkindValuePair(subkindIter->second, fieldsObj[fieldKey].asString()));
            returnObj[fieldKey] = fieldsObj[fieldKey];
        }
    }

    return convertedList;
}

QList<SubkindValuePair> PimContactsQt::convertFieldAttributes(const Json::Value& fieldArray, Json::Value& returnObj)
{
    QList<SubkindValuePair> convertedList;

    for (int i = 0; i < fieldArray.size(); i++) {
        Json::Value fieldObj = fieldArray[i];
        std::string type = fieldObj.get("type", "").asString();
        std::string value = fieldObj.get("value", "").asString();
        StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(type);

        if (subkindIter != _attributeSubKindMap.end()) {
            convertedList.append(SubkindValuePair(subkindIter->second, value));
            returnObj.append(fieldObj);
        }
    }

    return convertedList;
}

QList<SubkindValuePair> PimContactsQt::convertStringArray(const Json::Value& stringArray, Json::Value& returnObj, bbpim::AttributeSubKind::Type subkind)
{
    QList<SubkindValuePair> convertedList;

    for (int i = 0; i < stringArray.size(); i++) {
        std::string value = stringArray[i].asString();
        convertedList.append(SubkindValuePair(subkind, value));
        returnObj.append(value);
    }

    return convertedList;
}

void PimContactsQt::addConvertedList(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const QList<SubkindValuePair>& convertedList)
{
    for (int i = 0; i < convertedList.size(); i++) {
        //addAttribute(contactBuilder, kind, convertedList[i].first, convertedList[i].second);
        bbpim::ContactAttribute attribute;
        bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

        attributeBuilder = attributeBuilder.setKind(kind);
        attributeBuilder = attributeBuilder.setSubKind(convertedList[i].first);
        attributeBuilder = attributeBuilder.setValue(QString(convertedList[i].second.c_str()));

        contactBuilder.addAttribute(attribute);
    }
}

void PimContactsQt::addConvertedGroupedList(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const QList<SubkindValuePair>& convertedList, const std::string& groupKey)
{
    for (int i = 0; i < convertedList.size(); i++) {
        //addAttributeToGroup(contactBuilder, kind, convertedList[i].first, convertedList[i].second, groupKey);
        bbpim::ContactAttribute attribute;
        bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

        attributeBuilder = attributeBuilder.setKind(kind);
        attributeBuilder = attributeBuilder.setSubKind(convertedList[i].first);
        attributeBuilder = attributeBuilder.setValue(QString(convertedList[i].second.c_str()));
        attributeBuilder = attributeBuilder.setGroupKey(QString(groupKey.c_str()));

        contactBuilder.addAttribute(attribute);
    }
}

void PimContactsQt::addAttributeDate(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value)
{
    bbpim::ContactAttribute attribute;
    bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

    QDateTime date = QDateTime::fromString(QString(value.c_str()), QString("ddd MMM dd yyyy"));

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);

    if (date.isValid()) {
        attributeBuilder = attributeBuilder.setValue(date);
    } else {
        attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));
    }

    contactBuilder.addAttribute(attribute);
}

/*
void PimContactsQt::addAttribute(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value)
{
    bbpim::ContactAttribute attribute;
    bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);
    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));

    contactBuilder.addAttribute(attribute);
}
*/

/*
void PimContactsQt::addAttributeToGroup(bbpim::ContactBuilder& contactBuilder, const bbpim::AttributeKind::Type kind, const bbpim::AttributeSubKind::Type subkind, const std::string& value, const std::string& groupKey)
{
    bbpim::ContactAttribute attribute;
    bbpim::ContactAttributeBuilder attributeBuilder(attribute.edit());

    attributeBuilder = attributeBuilder.setKind(kind);
    attributeBuilder = attributeBuilder.setSubKind(subkind);
    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));
    attributeBuilder = attributeBuilder.setGroupKey(QString(groupKey.c_str()));

    contactBuilder.addAttribute(attribute);
}
*/

Json::Value PimContactsQt::addPostalAddress(bbpim::ContactBuilder& contactBuilder, const Json::Value& addressObj)
{
    Json::Value returnObj;
    bbpim::ContactPostalAddress address;
    bbpim::ContactPostalAddressBuilder addressBuilder(address.edit());

    if (addressObj.isMember("type")) {
        std::string value = addressObj["type"].asString();
        StringToSubKindMap::const_iterator subkindIter = _attributeSubKindMap.find(value);

        if (subkindIter != _attributeSubKindMap.end()) {
            addressBuilder = addressBuilder.setSubKind(subkindIter->second);
            returnObj["type"] = Json::Value(value);
        }
    }

    addressBuilder = addressBuilder.setLine1(QString(addressObj.get("streetAddress", "").asCString()));
    addressBuilder = addressBuilder.setLine2(QString(addressObj.get("streetOther", "").asCString()));
    addressBuilder = addressBuilder.setCity(QString(addressObj.get("locality", "").asCString()));
    addressBuilder = addressBuilder.setRegion(QString(addressObj.get("region", "").asCString()));
    addressBuilder = addressBuilder.setCountry(QString(addressObj.get("country", "").asCString()));
    addressBuilder = addressBuilder.setPostalCode(QString(addressObj.get("postalCode", "").asCString()));

    returnObj["streetAddress"] = addressObj["streetAddress"];
    returnObj["streetOther"] = addressObj["streetOther"];
    returnObj["locality"] = addressObj["locality"];
    returnObj["region"] = addressObj["region"];
    returnObj["country"] = addressObj["country"];
    returnObj["postalCode"] = addressObj["postalCode"];

    contactBuilder.addPostalAddress(address);
    return returnObj;
}

Json::Value PimContactsQt::addPhoto(bbpim::ContactBuilder& contactBuilder, const Json::Value& photoObj)
{
    Json::Value returnObj;
    bbpim::ContactPhoto photo;
    bbpim::ContactPhotoBuilder photoBuilder(photo.edit());

    std::string filepath = photoObj.get("originalFilePath", "").asString();
    bool pref = photoObj.get("pref", true).asBool();

    photoBuilder.setOriginalPhoto(QString(filepath.c_str()));
    photoBuilder.setPrimaryPhoto(pref);

    returnObj["originalFilePath"] = Json::Value(filepath);
    returnObj["pref"] = Json::Value(pref);
    returnObj["smallFilePath"] = Json::Value(photo.smallPhoto().toStdString());
    returnObj["largeFilePath"] = Json::Value(photo.largePhoto().toStdString());

    contactBuilder.addPhoto(photo, pref);
    return returnObj;
}


void PimContactsQt::syncConvertedList(bbpim::ContactBuilder& contactBuilder, bbpim::AttributeKind::Type kind, QList<bbpim::ContactAttribute> savedList, const QList<SubkindValuePair>& convertedList)
{
    int index;

    for (index = 0; index < savedList.size() && index < convertedList.size(); index++) {
        if (savedList[index].subKind() != convertedList[index].first || savedList[index].value().toStdString() != convertedList[index].second) {
            bbpim::ContactAttributeBuilder attributeBuilder(savedList[index].edit());
            attributeBuilder = attributeBuilder.setSubKind(convertedList[index].first);
            attributeBuilder = attributeBuilder.setValue(QString(convertedList[index].second.c_str()));
        }
    }

    if (index < savedList.size()) {
        for (; index < savedList.size(); index++) {
            contactBuilder = contactBuilder.deleteAttribute(savedList[index]);
        }
    } else if (index < convertedList.size()) {
        for (; index < convertedList.size(); index++) {
            QList<SubkindValuePair> remainingList = convertedList.mid(index);
            addConvertedList(contactBuilder, kind, remainingList);
        }
    }
}

void PimContactsQt::syncConvertedGroupedList(bbpim::ContactBuilder& contactBuilder, bbpim::AttributeKind::Type kind, QList<bbpim::ContactAttribute> savedList, QList<SubkindValuePair> convertedList, const std::string& groupKey)
{
    int index;

    for (index = 0; index < savedList.size() && index < convertedList.size(); index++) {
        bbpim::ContactAttributeBuilder attributeBuilder(savedList[index].edit());
        attributeBuilder = attributeBuilder.setSubKind(convertedList[index].first);
        attributeBuilder = attributeBuilder.setValue(QString(convertedList[index].second.c_str()));
        attributeBuilder = attributeBuilder.setGroupKey(QString(groupKey.c_str()));
    }

    if (index < savedList.size()) {
        for (; index < savedList.size(); index++) {
            contactBuilder = contactBuilder.deleteAttribute(savedList[index]);
        }
    } else if (index < convertedList.size()) {
        for (; index < convertedList.size(); index++) {
            QList<SubkindValuePair> remainingList = convertedList.mid(index);
            addConvertedList(contactBuilder, kind, remainingList);
        }
    }
}

Json::Value PimContactsQt::syncAttributeGroup(bbpim::ContactBuilder& contactBuilder, bbpim::AttributeKind::Type kind, QList<QList<bbpim::ContactAttribute> > savedList, const Json::Value& jsonObj)
{
    int i;
    Json::Value returnObj;

    for (i = 0; i < jsonObj.size() && i < savedList.size(); i++) {
        Json::Value orgObj;
        std::stringstream groupKeyStream;
        groupKeyStream << i + 1;

        QList<SubkindValuePair> convertedList = convertGroupedAttributes(jsonObj[i], orgObj);
        syncConvertedGroupedList(contactBuilder, kind, savedList[i], convertedList, groupKeyStream.str());
        returnObj.append(orgObj);
    }

    if (i < savedList.size()) {
        for (; i < savedList.size(); i++) {
            for (int j = 0; j < savedList[i].size(); j++) {
                contactBuilder = contactBuilder.deleteAttribute(savedList[i][j]);
            }
        }
    } else if (i < jsonObj.size()) {
        for (; i < jsonObj.size(); i++) {
            Json::Value orgObj;
            std::stringstream groupKeyStream;
            groupKeyStream << i + 1;

            QList<SubkindValuePair> convertedList = convertGroupedAttributes(jsonObj[i], orgObj);
            addConvertedGroupedList(contactBuilder, kind, convertedList, groupKeyStream.str());
            returnObj.append(orgObj);
        }
    }

    return returnObj;
}

void PimContactsQt::syncAttributeDate(bbpim::ContactBuilder& contactBuilder, QList<bbpim::ContactAttribute>& savedList, const bbpim::AttributeSubKind::Type subkind, const std::string& value)
{
    bool found = false;

    for (int i = 0; i < savedList.size(); i++) {
        if (savedList[i].subKind() == subkind) {
            if (found) {
                contactBuilder = contactBuilder.deleteAttribute(savedList[i]);
            } else {
                found = true;
                bbpim::ContactAttributeBuilder attributeBuilder(savedList[i].edit());
                QDateTime date = QDateTime::fromString(QString(value.c_str()), QString("ddd MMM dd yyyy"));

                if (date.isValid()) {
                    attributeBuilder = attributeBuilder.setValue(date);
                } else {
                    attributeBuilder = attributeBuilder.setValue(QString(value.c_str()));
                }
            }
        }
    }
}

//TODO(miwong) Need to optimize syncing addresses and photos
void PimContactsQt::syncPostalAddresses(bbpim::ContactBuilder& contactBuilder, QList<bbpim::ContactPostalAddress>& savedList, const Json::Value& jsonObj)
{
    for (int j = 0; j < savedList.size(); j++) {
        contactBuilder = contactBuilder.deletePostalAddress(savedList[j]);
    }

    for (int i = 0; i < jsonObj.size(); i++) {
        Json::Value addressObj = jsonObj[i];
        addPostalAddress(contactBuilder, addressObj);
    }
}

void PimContactsQt::syncPhotos(bbpim::ContactBuilder& contactBuilder, QList<bbpim::ContactPhoto>& savedList, const Json::Value& jsonObj)
{
    for (int j = 0; j < savedList.size(); j++) {
        contactBuilder = contactBuilder.deletePhoto(savedList[j]);
    }

    for (int i = 0; i < jsonObj.size(); i++) {
        Json::Value photoObj = jsonObj[i];
        addPhoto(contactBuilder, photoObj);
    }
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
    _kindAttributeMap[bbpim::AttributeKind::OrganizationAffiliation] = "organizations";
    _kindAttributeMap[bbpim::AttributeKind::Education] = "education";
    _kindAttributeMap[bbpim::AttributeKind::Note] = "note";
    _kindAttributeMap[bbpim::AttributeKind::InstantMessaging] = "ims";
    _kindAttributeMap[bbpim::AttributeKind::VideoChat] = "videoChat";
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

