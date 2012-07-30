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
var greetingWithNative,
    _event = require("../../lib/event");
    
module.exports = {
    getGreeting: function (success, fail, args, env) {
        var greeting = {};
        
        try {
            if (args.classicGreeting) {
                greeting.classicGreeting = JSON.parse(decodeURIComponent(args.classicGreeting));
            }
            
            greetingWithNative.getGreeting(greeting);
            success();
        } catch (e) {
            fail(-1, e);
        }
    }
};

///////////////////////////////////////////////////////////////////
// JavaScript wrapper for JNEXT plugin
///////////////////////////////////////////////////////////////////

JNEXT.GreetingWithNative = function ()
{   
    var _self = this;
        _self._id = "";


    _self.getGreeting = function (classicGreeting) {
        var returnValue = JNEXT.invoke(_self._id, "getGreeting " + JSON.stringify(settings));
        
        return returnValue;
    };

    _self.getId = function () {
        return _self._id;
    };

    _self.init = function () {
        if (!JNEXT.require("greeting")) {   
            return false;
        }

        _self._id = JNEXT.createObject("greeting.GreetingWithNative");
        
        if (!_self._id || _self._id === "") {   
            return false;
        }

        JNEXT.registerEvents(_self);
    };
    
    _self.init();
};

greetingWithNative = new JNEXT.GreetingWithNative();
