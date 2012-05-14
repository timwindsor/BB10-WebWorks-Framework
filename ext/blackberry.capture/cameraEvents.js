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
function requireLocal(id) {
    return !!require.resolve ? require("../../" + id) : window.require(id);
}

var _ppsUtils = requireLocal("lib/pps/ppsUtils");

module.exports = {
    addEventListener: function (event, trigger) {
        console.log("in cameraEvents addEventListener");
        if (event) {
            event.ppsUtils = _ppsUtils.createObject();
            event.ppsUtils.init();

            if (event.ppsUtils.open(event.path, event.mode)) {
                console.log("after ppsUtils.open");
                event.ppsUtils.onChange = trigger;
            } else {
                console.log("could not open " + event.path);
            }
        }
    },
    removeEventListener: function (event) {
        if (event) {
            if (event.ppsUtils) {
                event.ppsUtils.close();
            }
        }
    }
};