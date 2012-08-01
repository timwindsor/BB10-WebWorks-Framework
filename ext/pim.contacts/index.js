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
            if (args.hasOwnProperty(key)) {
                findOptions[key] = JSON.parse(decodeURIComponent(args[key]));
            }
        }

        pimContacts.find(findOptions);
        success();
    },

    save: function (success, fail, args) {
        var attributes = {},
            key;

        for (key in args) {
            if (args.hasOwnProperty(key)) {
                attributes[key] = JSON.parse(decodeURIComponent(args[key]));
            }
        }

        pimContacts.save(attributes);
        success();
    },

    remove: function (success, fail, args) {
        var attributes = { "contactId" : JSON.parse(decodeURIComponent(args.contactId)),
                           "_eventId" : JSON.parse(decodeURIComponent(args._eventId))};

        pimContacts.remove(attributes);
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
        JNEXT.invoke(self.m_id, "find " + JSON.stringify(args));
        return "";
    };

    self.save = function (args) {
        if (args.displayName) {
            args.name.displayName = args.displayName;
        } 

        if (args.nickname) {
            args.name.nickname = args.nickname;
        }

        JNEXT.invoke(self.m_id, "save " + JSON.stringify(args));
        return "";
    };

    self.remove = function (args) {
        JNEXT.invoke(self.m_id, "remove " + JSON.stringify(args));
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
