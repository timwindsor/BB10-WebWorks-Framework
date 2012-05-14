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

var _event = requireLocal("lib/event"),
    _picturesTaken = [],
    _actionMap = {
        photoIndex: {
            context: require("./cameraEvents"),
            event: {
                name: "photoIndex",
                path: "/pps/services/multimedia/sync/changes?wait,delta",
                mode: 0
            },
            trigger: function (data) {
                // TODO store image to array
                console.log("photoIndex callback");
                console.log(data);
            }
        },
        cameraStatus: {
            context: require("./cameraEvents"),
            event: {
                name: "cameraEvents",
                path: "/pps/services/multimedia/camera/status?wait,delta",
                mode: 0
            },
            trigger: function (data) {
                // TODO fire client side callback
                console.log("cameraStatus callback");
                console.log(data);
            }
        }
    },
    ERROR_ID = -1;

module.exports = {
    captureImage: function (success, fail, args) {
        try {
            var photoIndexAction = _actionMap["photoIndex"],
                cameraStatusAction = _actionMap["cameraStatus"];

            _event.on(photoIndexAction);
            _event.on(cameraStatusAction);

            qnx.callExtensionMethod("navigator.invoke", "camera://photo");

            if (success) {
                success();
            }
        } catch (e) {
            if (fail) {
                fail(ERROR_ID, e);
            }
        }
    }
};
