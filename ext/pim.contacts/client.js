/*
 * Copyright 2010-2011 Research In Motion Limited.
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
 
var _self = {},
    _ID = require("./manifest.json").namespace,
    Contact = require("./Contact"),
    ContactName = require("./ContactName"),
    ContactOrganization = require("./ContactOrganization"),
    ContactAddress = require("./ContactAddress"),
    ContactField = require("./ContactField"),
    ContactPhoto = require("./ContactPhoto");

function S4() {
    return (((1 + Math.random()) * 0x10000) | 0).toString(16).substring(1);
}

function guid() {
    return (S4() + S4() + "-" + S4() + "-" + S4() + "-" + S4() + "-" + S4() + S4() + S4());
}

function populateFieldArray(contactProps, field, ClassName) {
    if (contactProps[field]) {
        var list = [],
            photo;
        contactProps[field].forEach(function (obj) {
            if (ClassName === ContactField) {
                list.push(new ClassName(obj.type, obj.value));
            } else if (ClassName === ContactPhoto) {
                photo = new ContactPhoto(obj.originalFilePath, obj.pref);
                photo.largeFilePath = obj.largeFilePath;
                photo.smallFilePath = obj.smallFilePath;
                list.push(photo);
            } else {
                list.push(new ClassName(obj));
            }
        });
        contactProps[field] = list;
    }
}

function populateDate(contactProps, field) {
    if (contactProps[field]) {
        contactProps[field] = new Date(contactProps[field]);
    }
}

_self.find = function (contactFields, onFindSuccess, onFindError, findOptions) {
    // TODO validation
    var callback = function (args) {
            var result = JSON.parse(unescape(args.result)),
                contacts = result.contacts,
                realContacts = [];

            if (result._success) {
                if (onFindSuccess) {
                    contacts.forEach(function (contact) {
                        var name = new ContactName(contact.name);

                        populateFieldArray(contact, "addresses", ContactAddress);
                        populateFieldArray(contact, "organizations", ContactOrganization);
                        populateFieldArray(contact, "emails", ContactField);
                        populateFieldArray(contact, "phoneNumbers", ContactField);
                        populateFieldArray(contact, "faxNumbers", ContactField);
                        populateFieldArray(contact, "pagerNumbers", ContactField);
                        populateFieldArray(contact, "ims", ContactField);
                        populateFieldArray(contact, "socialNetworks", ContactField);
                        populateFieldArray(contact, "urls", ContactField);
                        populateFieldArray(contact, "photos", ContactPhoto);

                        populateDate(contact, "birthday");
                        populateDate(contact, "anniversary");

                        contact.displayName = contact.name.displayName;
                        contact.nickname = contact.name.nickname;
                        contact.name = name;

                        realContacts.push(new Contact(contact));
                    });

                    onFindSuccess(realContacts);
                }
            } else {
                if (onFindError) {
                    onFindError(result);
                }
            }
        },
        eventId = guid();

    window.webworks.event.once(_ID, eventId, callback);

    return window.webworks.execAsync(_ID, "find", {
        "_eventId": eventId,
        "fields": contactFields,
        "options": findOptions
    });
};

_self.create = function (properties) {
    var contact = new Contact(),
        key;

    for (key in properties) {
        if (properties.hasOwnProperty(key)) {
            contact[key] = properties[key];
        }
    }

    return contact;
};

_self.Contact = Contact;
_self.ContactField = ContactField;
_self.ContactAddress = ContactAddress;
_self.ContactName = ContactName;
_self.ContactOrganization = ContactOrganization;
_self.ContactPhoto = ContactPhoto;
_self.ContactFindOptions = require("./ContactFindOptions");

module.exports = _self;
