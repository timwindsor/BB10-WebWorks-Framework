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
var Contact,
    _ID = require("./manifest.json").namespace; // normally 2nd-level require does not work in client side, but manifest has already been required in client.js, so this is ok

function S4() {
    return (((1 + Math.random()) * 0x10000) | 0).toString(16).substring(1);
}

function guid() {
    return (S4() + S4() + "-" + S4() + "-" + S4() + "-" + S4() + "-" + S4() + S4() + S4());
}

/**
 * Contains information about a single contact.
 * @constructor
 * @param properties
 */
Contact = function (properties) {
    this.displayName = properties && properties.displayName ? properties.displayName : "";
    this.name = properties && properties.name ? properties.name : null; // ContactName
    this.nickname = properties && properties.nickname ? properties.nickname : "";
    this.phoneNumbers = properties && properties.phoneNumbers ? properties.phoneNumbers : null; // ContactField[]
    this.faxNumbers = properties && properties.faxNumbers ? properties.faxNumbers : null; // ContactField[]
    this.pagerNumbers = properties && properties.pagerNumbers ? properties.pagerNumbers : null; // ContactField[]
    this.emails = properties && properties.emails ? properties.emails : null; // ContactField[]
    this.addresses = properties && properties.addresses ? properties.addresses : null; // ContactAddress[]
    this.ims = properties && properties.ims ? properties.ims : null; // ContactField[]
    this.organizations = properties && properties.organizations ? properties.organizations : null; // ContactOrganization[]
    this.birthday = properties && properties.birthday ? properties.birthday : null;
    this.anniversary = properties && properties.anniversary ? properties.anniversary : null;
    this.note = properties && properties.note ? properties.note : "";
    this.photos = properties && properties.photos ? properties.photos : null; // ContactField[]
    this.categories = properties && properties.categories ? properties.categories : null; // ContactField[]
    this.urls = properties && properties.urls ? properties.urls : null; // ContactField[]
    this.videoChat = properties && properties.videoChat ? properties.videoChat : null; // String[]
    this.socialNetworks = properties && properties.socialNetworks ? properties.socialNetworks : null; // ContactField[]
    this.ringtone = properties && properties.ringtone ? properties.ringtone : "";
    this.favorite = properties && properties.favorite ? properties.favorite : false;

    var privateId = properties && properties.id ? properties.id : "0";
    Object.defineProperty(this, "id", { "value": privateId });
};

Contact.prototype.save = function (onSaveSuccess, onSaveError) {
    var args = {},
        key,
        successCallback = onSaveSuccess,
        errorCallback = onSaveError,
        saveCallback;

    for (key in this) {
        if (this.hasOwnProperty(key) && this[key] !== null) {
            args[key] = this[key];
        }
    }

    if (args.nickname) {
        args.name = args.name || {};
        args.name.nickname = args.nickname;
    }

    if (args.displayName) {
        args.name = args.name || {};
        args.name.displayName = args.displayName;
    }

    if (args.birthday && args.birthday.toDateString) {
        args.birthday = args.birthday.toDateString();
    }

    if (args.anniversary && args.anniversary.toDateString) {
        args.anniversary = args.anniversary.toDateString();
    }

    args.id = window.parseInt(this.id);
    args._eventId = guid();

    saveCallback = function (args) {
        var result = JSON.parse(unescape(args.result)),
            newContact;

        if (result._success) {
            if (successCallback) {
                if (result.name && result.name.nickname) {
                    result.nickname = result.name.nickname;
                }

                if (result.name && result.name.displayName) {
                    result.displayName = result.name.displayName;
                }

                result.id = result.id.toString();
                result.birthday = new Date(result.birthday);
                result.anniversary = new Date(result.anniversary);

                newContact = new Contact(result);
                successCallback(newContact);
            }
        } else {
            if (errorCallback) {
                errorCallback(result);
            }
        }
    };

    window.webworks.event.once(_ID, args._eventId, saveCallback);
    return window.webworks.execAsync(_ID, "save", args);
};

Contact.prototype.remove = function (onRemoveSuccess, onRemoveError) {
    var args = {},
        successCallback = onRemoveSuccess,
        errorCallback = onRemoveError,
        removeCallback;

    args.contactId = window.parseInt(this.id);
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

    window.webworks.event.once(_ID, args._eventId, removeCallback);
    return window.webworks.execAsync(_ID, "remove", args);
};

Contact.prototype.clone = function () {
    var contact = new Contact({"id" : -1 * this.id}),
        key;

    for (key in this) {
        if (this.hasOwnProperty(key)) {
            contact[key] = this[key];
        }
    }

    return contact;
};

module.exports = Contact;
