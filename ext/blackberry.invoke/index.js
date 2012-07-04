/*
 * Copyright 2011-2012 Research In Motion Limited.
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
var _event = require("../../lib/event"),
    viewers = {};

module.exports = {
    invoke: function (success, fail, args) {
        // if request contains invalid args, the invocation framework will provide error in callback
        // no validation done here
        var argReq = JSON.parse(decodeURIComponent(args["request"])),
            expectedParams = [
                "target",
                "action",
                "uri",
                "type",
                "data"
            ],
            request = {},
            callback;

        callback = function (error) {
            _event.trigger("blackberry.invoke.invokeEventId", error);
        };

        expectedParams.forEach(function (key) {
            var val = argReq[key];

            if (val) {
                request[key] = val;
            }
        });

        window.qnx.webplatform.getApplication().invocation.invoke(request, callback);
        success();
    },

    invokeViewer: function (success, fail, args) {
        var argReq = JSON.parse(decodeURIComponent(args["request"])),
            request = {},
            expectedParams = [
                "target",
                "action",
                "uri",
                "type",
                "data",
                "width",
                "height"
            ],
            callback = function (error, viewer) {
                var response = { "error": error },
                    _viewerCallbacksEventId = "blackberry.invoke.viewerCallbacksEventId";
                if (viewer && viewer.viewerId && !viewers[viewer.viewerId]) {

                    viewer.onClose = function () {
                        _event.trigger(_viewerCallbacksEventId, {
                            id: viewer.viewerId,
                            method: "onClose"
                        });
                        delete viewers[viewer.viewerId];
                    };

                    viewer.onCancel = function () {
                        _event.trigger(_viewerCallbacksEventId, {
                            id: viewer.viewerId,
                            method: "onCancel"
                        });
                        delete viewers[viewer.viewerId];
                    };

                    viewer.onReceive = function (name, message) {
                        _event.trigger(_viewerCallbacksEventId, {
                            id: viewer.viewerId,
                            method: "onReceive",
                            params: message
                        });
                    };

                    viewers[viewer.viewerId] = viewer;
                    response.viewerId = viewer.viewerId;
                }

                _event.trigger("blackberry.invoke.invokeViewerEventId", response);
            };

        expectedParams.forEach(function (key) {
            var val = argReq[key];

            if (val) {
                request[key] = val;
            }
        });

        window.qnx.webplatform.getApplication().invocation.invokeViewer(request, callback);
        success();
    },

    query: function (success, fail, args) {
        var argReq = JSON.parse(decodeURIComponent(args["request"])),
            expectedParams = [
                "action",
                "uri",
                "type",
                "action_type",
                "receiver_capabilities",
                "perimeter",
                "brokering_mod"
            ],
            expectedTypes = ["APPLICATION", "VIEWER"],
            request = {},
            callback = function (error, response) {
                _event.trigger("blackberry.invoke.queryEventId", {"error": error, "response": response});
            };

        expectedParams.forEach(function (key) {
            var val = argReq[key];

            if (val) {
                request[key] = val;
            }
        });

        // Validate target_type property in request if it exists
        if (Array.isArray(argReq["target_type"])) {

            request["target_type"] = argReq["target_type"].reduce(function (prev, current) {
                var containsType = function (type) {
                        return current === type;
                    };

                if (prev !== "ALL" && typeof current === "string" &&
                        !prev.some(containsType) && expectedTypes.some(containsType)) {

                    prev.push(current);
                }

                return prev;
            }, []);

            switch (request["target_type"].length)
            {
            case expectedTypes.length:
                request["target_type"] = "ALL";
                break;
            case 1:
                request["target_type"] = request["target_type"].pop();
                break;
            case 0:
                delete request["target_type"];
            }
        }

        window.qnx.webplatform.getApplication().invocation.queryTargets(request, callback);
        success();
    },

    viewerMethod: function (success, fail, args) {
        var argReq = JSON.parse(decodeURIComponent(args["request"])),
            viewer = viewers[argReq.id],
            callback,
            expectedMethods = [
                "close",
                "setSize",
                "setPosition",
                "setVisibility",
                "setZOrder",
                "send",
                "update"
            ],
            isWhitelisted = expectedMethods.some(function (method) {
                return argReq.method === method;
            });

        if (viewer && isWhitelisted) {
            if (argReq.method === "send" && argReq.callback && Array.isArray(argReq.params)) {
                callback = function (name, message) {
                    _event.trigger("blackberry.invoke.relayViewerEventId", {name: message.msg, data: message.dat});
                };

                argReq.params.push(callback);
            }

            viewer[argReq.method].apply(viewer, argReq.params);
            if (argReq.method === "close") {
                delete viewers[argReq.id];
            }

            success();
        } else {
            fail();
        }
    }
};

