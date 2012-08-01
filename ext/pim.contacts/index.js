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

var pimContacts,
    _event = require("../../lib/event");
    
module.exports = {
    find: function (success, fail, args) {
        var findOptions = {},
            key;

        for (key in args) {
            findOptions[key] = JSON.parse(decodeURIComponent(args[key]));
        }

        pimContacts.find(findOptions);
        success();
    },

    createContact: function (success, fail, args) {
        var attributes = {},
            key;

        for (key in args) {
            attributes[key] = JSON.parse(decodeURIComponent(args[key]));
        }

        pimContacts.createContact(attributes);
        success();
    },

    deleteContact: function (success, fail, args) {
        var attributes = {},
            key;

        for (key in args) {
            attributes[key] = JSON.parse(decodeURIComponent(args[key]));
        }

        pimContacts.deleteContact(attributes);
        success();
    }
};

///////////////////////////////////////////////////////////////////
// JavaScript wrapper for JNEXT plugin
///////////////////////////////////////////////////////////////////

JNEXT.PimContacts = function ()
{   
    var self = this;

    self.find = function (args) {
        var val = JNEXT.invoke(self.m_id, "find " + JSON.stringify(args));
        return "";//JSON.parse(val);
    };

    self.createContact = function (args) {
        /*var val =*/
        JNEXT.invoke(self.m_id, "createContact " + JSON.stringify(args));
        return "";
    };

    self.deleteContact = function (args) {
        /*var val =*/
        JNEXT.invoke(self.m_id, "deleteContact " + JSON.stringify(args));
        return "";
    };

    self.getId = function () {
        return self.m_id;
    };

    self.init = function () {
        if (!JNEXT.require("pimContacts")) {
            return false;
        }

        self.m_id = JNEXT.createObject("pimContacts.PimContacts");
        
        if (self.m_id === "") {
            return false;
        }

        JNEXT.registerEvents(self);
    };
   
    self.onEvent = function (strData) {
        console.log(strData);
        var arData = strData.split(" "),
            strEventDesc = arData[0];
            
        if (strEventDesc === "result") {
            var args = {};
            args.result = escape(strData.split(" ").slice(2).join(" "));
            _event.trigger(arData[1], args);
        }
    };
    
    self.m_id = "";

    self.init();
};

pimContacts = new JNEXT.PimContacts();
