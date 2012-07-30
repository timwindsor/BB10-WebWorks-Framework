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
    Contact,
    ContactField,
    ContactAddress,
    ContactOrganization,
    ContactPhoto,
    _ID = require("./manifest.json").namespace;

_self.find = function (findOptions) {
    return window.webworks.execSync(_ID, "find", findOptions || {});
};

_self.create = function (attributes) {
    var contact = new Contact();

    for (key in attributes) {
    }

    return contact;
};

_self.save = function (attributes) {
    return window.webworks.execSync(_ID, "save", attributes);
};

_self.deleteContact = function (attributes) {
    return window.webworks.execSync(_ID, "remove", attributes);
};

Contact = function () {
    this.id = "";
    this.addresses = [];
};

Contact.prototype.save = function (onSaveSuccess, onSaveError) {
    var old_contact = window.webworks.execSync(_ID, "save", this);
    var new_contact = new Contact();

    for (key in old_contact) {
        if (old_contact.hasOwnProperty(key)) {
            new_contact[key] = old_contact[key];
        }
    }

    return new_contact;
};

Contact.prototype.remove = function (onRemoveSuccess, onRemoveError) {
    return window.webworks.execSync(_ID, "remove", attributes);
};

Contact.prototype.clone = function () {
    var contact = new Contact();

    for (key in this) {
        if (this.hasOwnProperty(key)) {
            contact[key] = this[key];
        }
    }

    contact.id = -1 * this.id;
    return contact;
};

_self.Contact = Contact;
_self.ContactField = ContactField;
_self.ContactAddress = ContactAddress;
_self.ContactOrganization = ContactOrganization;
_self.ContactPhoto = ContactPhoto;

module.exports = _self;
