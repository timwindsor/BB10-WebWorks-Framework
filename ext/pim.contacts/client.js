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
    _ID = require("./manifest.json").namespace;

/*
_self.find = function (findOptions) {
    return window.webworks.execSync(_ID, "find", findOptions || {});
};
*/
_self.find = function (contactFields, onFindSuccess, onFindError, findOptions) {
    // TODO validation

    // TODO async, invoke callbacks
    return window.webworks.execSync(_ID, "find", {
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

_self.ContactFindOptions = require("./ContactFindOptions");

module.exports = _self;
