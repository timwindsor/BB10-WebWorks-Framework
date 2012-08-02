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
    ContactAddress = require("./ContactAddress"),
    ContactOrganization = require("./ContactOrganization"),
    ContactField = require("./ContactField");

function populateFieldArray(contactProps, field, ClassName) {
    if (contactProps[field]) {
        var list = [];
        contactProps[field].forEach(function (obj) {
            if (ClassName === ContactField) {
                list.push(new ClassName(obj.type, obj.value));
            } else {
                list.push(new ClassName(obj));
            }
        });
        contactProps[field] = list;
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
    };

    window.webworks.event.once(_ID, "tempEventId", callback);

    return window.webworks.execAsync(_ID, "find", {
        "_eventId": "tempEventId", // should use real event id
        "fields": contactFields,
        "options": findOptions
    });
};

_self.createContact = function (attributes) {
    return window.webworks.execSync(_ID, "createContact", attributes);
};

_self.deleteContact = function (attributes) {
    return window.webworks.execSync(_ID, "deleteContact", attributes);
};

_self.Contact = Contact;
_self.ContactName = ContactName;
_self.ContactAddress = ContactAddress;
_self.ContactOrganization = ContactOrganization;
_self.ContactField = ContactField;
_self.ContactFindOptions = require("./ContactFindOptions");

module.exports = _self;
