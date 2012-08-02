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
    ContactName,
    ContactOrganization,
    ContactPhoto,
    _ID = require("./manifest.json").namespace;

function S4() {
    return (((1 + Math.random()) * 0x10000) | 0).toString(16).substring(1);
}

function guid() {
    return (S4() + S4() + "-" + S4() + "-" + S4() + "-" + S4() + "-" + S4() + S4() + S4());
}

_self.find = function (findOptions) {
    return window.webworks.execSync(_ID, "find", findOptions || {});
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

Contact = function () {
    this.id = "";

    // Undefined?
    this.addresses = undefined;
    this.anniversary = undefined;
    this.birthday = undefined;
    this.categories = undefined;
    this.displayName = undefined;
    this.emails = undefined;
    this.favorite = undefined;
    this.faxNumbers = undefined;
    this.ims = undefined;
    this.name = undefined;
    this.nickname = undefined;
    this.note = undefined;
    this.organizations = undefined;
    this.pagerNumbers = undefined;
    this.phoneNumbers = undefined;
    this.photos = undefined;
    this.ringtone = undefined;
    this.socialNetworks = undefined;
    this.urls = undefined;
    this.videoChat = undefined;
};

Contact.prototype.save = function (onSaveSuccess, onSaveError) {
    var args = {},
        key,
        successCallback = onSaveSuccess,
        errorCallback = onSaveError,
        saveCallback;

    for (key in this) {
        if (this.hasOwnProperty(key) && this[key] !== undefined) {
            args[key] = this[key];
        }
    }

    args._eventId = guid();

    saveCallback = function (args) {
        var result = JSON.parse(unescape(args.result));

        if (result._success) {
            this.id = result.id;

            if (successCallback) {
                successCallback(this);
            }
        } else {
            if (errorCallback) {
                errorCallback(result);
            }
        }
    };

    window.webworks.event.once(_ID, args._eventId, saveCallback.bind(this));
    return window.webworks.execAsync(_ID, "save", args);
};

Contact.prototype.remove = function (onRemoveSuccess, onRemoveError) {
    var args = {},
        successCallback = onRemoveSuccess,
        errorCallback = onRemoveError,
        removeCallback;

    args.contactId = this.id;
    args._eventId = guid();

    removeCallback = function (args) {
        var result = JSON.parse(unescape(args.result));

        if (result._success) {
            if (successCallback) {
                successCallback();
            }
        } else {
            if (errorCallback) {
                errorCallback(result);
            }
        }
    };

    window.webworks.event.once(_ID, args._eventId, removeCallback.bind(this));
    return window.webworks.execAsync(_ID, "remove", args);
};

Contact.prototype.clone = function () {
    var contact = new Contact(),
        key;

    for (key in this) {
        if (this.hasOwnProperty(key)) {
            contact[key] = this[key];
        }
    }

    contact.id = -1 * this.id;
    return contact;
};

ContactField = function (type, value, pref) {
    this.type = type || "";
    this.value = value || "";
    this.pref = pref || false;
};

ContactAddress = function (type, address1, address2, locality, region, postalCode, country, pref) {
    this.type = type || "";
    this.address1 = address1 || "";
    this.address2 = address2 || "";
    this.locality = locality || "";
    this.region = region || "";
    this.postalCode = postalCode || "";
    this.country = country || "";
    this.pref = pref || false;
};

ContactName = function () {
};

ContactOrganization = function (name, department, title, pref) {
    this.name = name || "";
    this.department = department || "";
    this.title = title || "";
    this.pref = pref || false;
};

ContactPhoto = function (originalFilePath, pref) {
    this.originalFilePath = originalFilePath || "";
    this.pref = pref || false;
};

_self.Contact = Contact;
_self.ContactField = ContactField;
_self.ContactAddress = ContactAddress;
_self.ContactName = ContactName;
_self.ContactOrganization = ContactOrganization;
_self.ContactPhoto = ContactPhoto;

module.exports = _self;
