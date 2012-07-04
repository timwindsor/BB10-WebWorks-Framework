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

var _apiDir = __dirname + "./../../../../ext/blackberry.invoke/",
    mockedInvocation,
    index;

describe("blackberry.invoke index", function () {

    beforeEach(function () {
        mockedInvocation = {
            invoke: jasmine.createSpy("invocation.invoke"),
            queryTargets: jasmine.createSpy("invocation.queryTargets"),
            invokeViewer: jasmine.createSpy("invocation.invokeViewer")
        };
        GLOBAL.window = {};
        GLOBAL.window.qnx = {
            callExtensionMethod : function () {},
            webplatform: {
                getApplication: function () {
                    return {
                        invocation: mockedInvocation
                    };
                }
            }
        };

        index = require(_apiDir + "index");
    });

    afterEach(function () {
        mockedInvocation = null;
        GLOBAL.window.qnx = null;
        index = null;
    });

    describe("invoke", function () {
        it("can invoke with target", function () {
            var successCB = jasmine.createSpy(),
                mockedArgs = {
                    "request": encodeURIComponent(JSON.stringify({target: "abc.xyz"}))
                };

            index.invoke(successCB, null, mockedArgs);
            expect(mockedInvocation.invoke).toHaveBeenCalledWith({
                target: "abc.xyz"
            }, jasmine.any(Function));
            expect(successCB).toHaveBeenCalled();
        });

        it("can invoke with uri", function () {
            var successCB = jasmine.createSpy(),
                mockedArgs = {
                    "request": encodeURIComponent(JSON.stringify({uri: "http://www.rim.com"}))
                };

            index.invoke(successCB, null, mockedArgs);
            expect(mockedInvocation.invoke).toHaveBeenCalledWith({
                uri: "http://www.rim.com"
            }, jasmine.any(Function));
            expect(successCB).toHaveBeenCalled();
        });
    });

    describe("query", function () {

        it("will whitelist properties of the request object", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "myProperty": "myValue",
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": ["APPLICATION", "VIEWER"]
                },
                whitelistRequest = JSON.parse(JSON.stringify(request)),
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            delete whitelistRequest.myProperty;
            whitelistRequest["target_type"] = "ALL";

            index.query(success, fail, args);
            expect(mockedInvocation.queryTargets).toHaveBeenCalledWith(whitelistRequest, jasmine.any(Function));
        });

        it("can query the invocation framework", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": ["APPLICATION", "VIEWER"]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.query(success, fail, args);
            request["target_type"] = "ALL";
            expect(mockedInvocation.queryTargets).toHaveBeenCalledWith(request, jasmine.any(Function));
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("can perform a query for application targets", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": ["APPLICATION"]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.query(success, fail, args);
            request["target_type"] = "APPLICATION";
            expect(mockedInvocation.queryTargets).toHaveBeenCalledWith(request, jasmine.any(Function));
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("can perform a query for viewer targets", function  () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": ["VIEWER"]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.query(success, fail, args);
            request["target_type"] = "VIEWER";
            expect(mockedInvocation.queryTargets).toHaveBeenCalledWith(request, jasmine.any(Function));
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("can perform a query for all targets", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": ["APPLICATION", "VIEWER"]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.query(success, fail, args);
            request["target_type"] = "ALL";
            expect(mockedInvocation.queryTargets).toHaveBeenCalledWith(request, jasmine.any(Function));
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("will not generate a target_type property in the request if it is not an array", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": "APPLICATION"
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.query(success, fail, args);
            delete request["target_type"];
            expect(mockedInvocation.queryTargets).toHaveBeenCalledWith(request, jasmine.any(Function));
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });
    });

    describe("invokeViewer", function () {

        it("should whitelist properties of the request object", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "target": "com.domain.subdomain.appName.viewer",
                    "action": "bb.action.OPEN",
                    "uri": "file://:",
                    "type": "text/plain",
                    "data": "My Data",
                    "width": 300,
                    "height": 300,
                    "myProperty": "MyPropertyValue"
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.invokeViewer(success, fail, args);
            delete request.myProperty;
            expect(mockedInvocation.invokeViewer).toHaveBeenCalledWith(request, jasmine.any(Function));
        });

        it("should be able to invoke a viewer", function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    "target": "com.domain.subdomain.appName.viewer",
                    "action": "bb.action.OPEN",
                    "uri": "file://:",
                    "type": "text/plain",
                    "data": "My Data",
                    "width": 300,
                    "height": 300,
                    "myProperty": "MyPropertyValue"
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.invokeViewer(success, fail, args);
            delete request.myProperty;
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });
    });

    describe("viewer object", function () {
        var viewer,
            event = require("../../../../lib/event"),
            executed = {};

        beforeEach(function () {
            var invokeSuccess = jasmine.createSpy("invoke success"),
                invokeFailure = jasmine.createSpy("invoke fail"),
                invokeArgs = {
                    "request": encodeURIComponent(JSON.stringify({}))
                };

            spyOn(event, "trigger");

            mockedInvocation.invokeViewer.andCallFake(function (request, callback) {
                viewer = {
                    "viewerId": "viewer0",
                    "close": jasmine.createSpy("viewer.close"),
                    "setSize": jasmine.createSpy("viewer.setSize"),
                    "setPosition": jasmine.createSpy("viewer.setPosition"),
                    "setVisibility": jasmine.createSpy("viewer.setVisibility"),
                    "setZOrder": jasmine.createSpy("viewer.setZOrder"),
                    "send": jasmine.createSpy("viewer.send").andCallFake(function (message, messageCallback) {
                        if (messageCallback) {
                            messageCallback("name", { msg: "message", dat: "data"});
                        }
                    }),
                    "update": jasmine.createSpy("viewer.update"),
                    "receive": jasmine.createSpy("viewer.receive")
                };
                callback(null, viewer);
            });

            index.invokeViewer(invokeSuccess, invokeFailure, invokeArgs);
        });

        afterEach(function () {
            var success = jasmine.createSpy(),
                fail = jasmine.createSpy(),
                request = {
                    id: "viewer0",
                    method: "close"
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            viewer = null;
        });

        it("should be able to close the viewer", function () {
            var success = jasmine.createSpy("close success"),
                fail = jasmine.createSpy("close failure"),
                secondSuccess = jasmine.createSpy("close second success"),
                secondFail = jasmine.createSpy("close second fail"),
                request = {
                    id: "viewer0",
                    method: "close"
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.close).toHaveBeenCalled();
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();

            /* viewer should have been deleted by viewer.close so trying to execute
             * another function on the same viewer object should fail.
             */
            index.viewerMethod(secondSuccess, secondFail, args);
            expect(executed.close).toBeFalsy();
            expect(secondSuccess).not.toHaveBeenCalled();
            expect(secondFail).toHaveBeenCalled();
        });

        it("should be able to set the size of the viewer", function () {
            var success = jasmine.createSpy("setSize success"),
                fail = jasmine.createSpy("setSize failure"),
                request = {
                    id: "viewer0",
                    method: "setSize",
                    params: [300, 300]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.setSize).toHaveBeenCalledWith(300, 300);
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("should be able to set the position of the viewer", function () {
            var success = jasmine.createSpy("setPosition success"),
                fail = jasmine.createSpy("setPosition failure"),
                request = {
                    id: "viewer0",
                    method: "setPosition",
                    params: [300, 300]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.setPosition).toHaveBeenCalledWith(300, 300);
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("should be able to set the visibility of the viewer", function () {
            var success = jasmine.createSpy("setVisibility success"),
                fail = jasmine.createSpy("setVisibility failure"),
                request = {
                    id: "viewer0",
                    method: "setVisibility",
                    params: [true]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.setVisibility).toHaveBeenCalledWith(true);
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("should be able to set the ZOrder for the viewer", function () {
            var success = jasmine.createSpy("setZOrder success"),
                fail = jasmine.createSpy("setZOrder failure"),
                request = {
                    id: "viewer0",
                    method: "setZOrder",
                    params: [0]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.setZOrder).toHaveBeenCalledWith(0);
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        describe("send method", function () {
            it("should be able to send a message without a callback to the viewer", function () {
                var success = jasmine.createSpy("send success"),
                    fail = jasmine.createSpy("send failure"),
                    request = {
                        id: "viewer0",
                        method: "send",
                        params: ["message"],
                        callback: false
                    },
                    args = {
                        "request": encodeURIComponent(JSON.stringify(request))
                    };

                index.viewerMethod(success, fail, args);
                expect(event.trigger).not.toHaveBeenCalledWith("blackberry.invoke.relayViewerEventId", jasmine.any(Object));
                expect(viewer.send).toHaveBeenCalledWith("message");
                expect(success).toHaveBeenCalled();
                expect(fail).not.toHaveBeenCalled();
            });

            it("should be able to send a message with a callback to the viewer", function () {
                var success = jasmine.createSpy("send success"),
                    fail = jasmine.createSpy("send failure"),
                    request = {
                        id: "viewer0",
                        method: "send",
                        params: ["message"],
                        callback: true
                    },
                    args = {
                        "request": encodeURIComponent(JSON.stringify(request))
                    },
                    response = {
                        "name": "message",
                        "data": "data"
                    };

                index.viewerMethod(success, fail, args);
                expect(event.trigger).toHaveBeenCalledWith("blackberry.invoke.relayViewerEventId", response);
                expect(viewer.send).toHaveBeenCalledWith("message", jasmine.any(Function));
                expect(success).toHaveBeenCalled();
                expect(fail).not.toHaveBeenCalled();
            });
        });

        it("should be able to refresh the viewer", function () {
            var success = jasmine.createSpy("refresh success"),
                fail = jasmine.createSpy("refresh failure"),
                request = {
                    id: "viewer0",
                    method: "update"
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.update).toHaveBeenCalled();
            expect(success).toHaveBeenCalled();
            expect(fail).not.toHaveBeenCalled();
        });

        it("should not call non whitelisted functions", function () {
            var success = jasmine.createSpy("refresh success"),
                fail = jasmine.createSpy("refresh failure"),
                message = {
                    msg: "My message",
                    dat: "My data"
                },
                request = {
                    id: "viewer0",
                    method: "receive",
                    params: [message]
                },
                args = {
                    "request": encodeURIComponent(JSON.stringify(request))
                };

            index.viewerMethod(success, fail, args);
            expect(viewer.receive).not.toHaveBeenCalled();
            expect(success).not.toHaveBeenCalled();
            expect(fail).toHaveBeenCalled();
        });

        it("should have the callback functions defined", function () {
            expect(viewer.onClose).toBeDefined();
            expect(viewer.onCancel).toBeDefined();
            expect(viewer.onReceive).toBeDefined();
        });
    });
});

