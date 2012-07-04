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

var _ID = "blackberry.invoke",
    _extDir = __dirname + "./../../../../ext",
    _apiDir = _extDir + "/" + _ID,
    client,
    mockedWebworks = {
        execAsync: jasmine.createSpy("webworks.execAsync"),
        execSync: jasmine.createSpy(),
        defineReadOnlyField: jasmine.createSpy(),
        event: {
            isOn: jasmine.createSpy("webworks.event.isOn"),
            add: jasmine.createSpy("webworks.event.add")
        }
    };

describe("blackberry.invoke client", function () {
    beforeEach(function () {
        GLOBAL.window = GLOBAL;
        GLOBAL.window.btoa = jasmine.createSpy("window.btoa").andReturn("base64 string");
        mockedWebworks.event.once = jasmine.createSpy("webworks.event.once");
        GLOBAL.window.webworks = mockedWebworks;
        client = require(_apiDir + "/client");
    });

    afterEach(function () {
        delete GLOBAL.window;
        client = null;
    });

    describe("invoke", function () {

        it("should call error callback if request is not valid", function () {
            var onError = jasmine.createSpy("client onError");

            client.invoke(null, null, onError);
            expect(onError).toHaveBeenCalled();
        });

        it("should call once and execAsync", function () {
            var request = {
                    target: "abc.xyz"
                },
                callback = jasmine.createSpy("client callback");

            client.invoke(request, callback);

            expect(mockedWebworks.event.once).toHaveBeenCalledWith(_ID, "blackberry.invoke.invokeEventId", jasmine.any(Function));
            expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "invoke", {"request": request});
        });

        it("should encode data to base64 string", function () {
            var request = {
                    target: "abc.xyz",
                    data: "my string"
                },
                callback = jasmine.createSpy("client callback");

            client.invoke(request, callback);

            expect(window.btoa).toHaveBeenCalledWith("my string");
            expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "invoke", {
                "request": {
                    target: request.target,
                    data: "base64 string"
                }
            });
        });

        it("should call onError if failed to encode data to base64", function () {
            var request = {
                    target: "abc.xyz",
                    data: "my string"
                },
                onError = jasmine.createSpy("client onError");

            GLOBAL.window.btoa = jasmine.createSpy("window.btoa").andThrow("bad string");
            client.invoke(request, null, onError);
            expect(onError).toHaveBeenCalledWith("bad string");
        });

        it("should call onSuccess if invocation is successful", function () {
            var request = {
                    target: "abc.xyz"
                },
                onSuccess = jasmine.createSpy("client onSuccess"),
                onError = jasmine.createSpy("client onError");

            client.invoke(request, onSuccess);
            mockedWebworks.event.once.argsForCall[0][2]("");
            expect(onSuccess).toHaveBeenCalled();
            expect(onError).not.toHaveBeenCalled();
        });

        it("should call onError if invocation failed", function () {
            var request = {
                    target: "abc.xyz"
                },
                onSuccess = jasmine.createSpy("client onSuccess"),
                onError = jasmine.createSpy("client onError");

            client.invoke(request, null, onError);
            mockedWebworks.event.once.argsForCall[0][2]("There is an error");
            expect(onSuccess).not.toHaveBeenCalled();
            expect(onError).toHaveBeenCalled();
        });
    });

    describe("query", function () {

        beforeEach(function () {
            mockedWebworks.event.once.andCallFake(function (id, eventId, func) {
                mockedWebworks.event.handler = [];
                mockedWebworks.event.handler[eventId] = func;
            });

            mockedWebworks.execAsync.andCallFake(function (id, action, args) {

                if (id && id === "blackberry.invoke" && action && action === "query") {
                    var _queryEventId = "blackberry.invoke.queryEventId";

                    //Valid the args
                    if (args && args.request && (args.request["type"] || args.request["uri"]) &&
                            args.request["target_type"]) {
                        mockedWebworks.event.handler[_queryEventId]({"error": "", "response": {}});
                    } else {
                        mockedWebworks.event.handler[_queryEventId]({"error": "invalid_argument", "response": null});
                    }

                    delete mockedWebworks.event.handler[_queryEventId];
                }
            });
        });

        afterEach(function () {
            if (mockedWebworks.event.handler) {
                delete mockedWebworks.event.handler;
            }
        });

        it("should register an event callback to be triggered by the server side", function () {
            var request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": "ALL"
                },
                onSuccess = jasmine.createSpy("client onSuccess"),
                onError = jasmine.createSpy("client onError");

            client.query(request, onSuccess, onError);
            expect(mockedWebworks.event.once).toHaveBeenCalledWith("blackberry.invoke", "blackberry.invoke.queryEventId", jasmine.any(Function));
            expect(mockedWebworks.execAsync).toHaveBeenCalledWith("blackberry.invoke", "query", {"request": request });
        });

        it("should call success callback if the invocation is successfull", function () {
            var request = {
                    "action": "bb.action.OPEN",
                    "type": "image/*",
                    "target_type": "ALL"
                },
                onSuccess = jasmine.createSpy("client onSuccess"),
                onError = jasmine.createSpy("client onError");

            client.query(request, onSuccess, onError);
            expect(window.webworks.execAsync).toHaveBeenCalledWith("blackberry.invoke", "query", {"request": request});
            expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
            expect(onError).not.toHaveBeenCalled();
        });

        it("should trigger error callback if the invocation is unsuccessfull", function () {
            var request = {
                    "action": "bb.action.OPEN",
                    "target_type": "ALL"
                },
                onSuccess = jasmine.createSpy("client onSuccess"),
                onError = jasmine.createSpy("client onError");

            client.query(request, onSuccess, onError);
            expect(onSuccess).not.toHaveBeenCalled();
            expect(onError).toHaveBeenCalledWith(jasmine.any(String));
        });
    });

    describe("invokeViewer", function () {
        beforeEach(function () {
            mockedWebworks.event.isOn.andCallFake(function () {
                return false;
            });

            mockedWebworks.event.once.andCallFake(function (id, eventId, func) {
                mockedWebworks.event.handler = {};
                mockedWebworks.event.handler[eventId] = func;
            });

            mockedWebworks.execAsync.andCallFake(function (id, action, args) {
                var _relayViewerEventId = "blackberry.invoke.relayViewerEventId",
                    _invokeViewerEventId = "blackberry.invoke.invokeViewerEventId";

                if (id && id === "blackberry.invoke" && action) {
                    if (action === "invokeViewer") {

                        mockedWebworks.event.handler[_invokeViewerEventId]({"error": "", "viewerId": "viewer001"});

                        delete mockedWebworks.event.handler[_invokeViewerEventId];
                    } else if (action === "viewerMethod" && args.request.method === "send" &&
                                mockedWebworks.event.handler[_relayViewerEventId]) {

                        mockedWebworks.event.handler[_relayViewerEventId]({name: "name", message: "message"});

                        delete mockedWebworks.event.handler[_relayViewerEventId];
                    }
                }
            });
        });

        afterEach(function () {
            if (mockedWebworks.event.handler) {
                delete mockedWebworks.event.handler;
            }
        });


        it("should base 64 encode data", function () {
            var request = {
                data: "abcdefghijklmnopqrstuvwxyz"
            };

            client.invokeViewer(request, null, null);
            expect(window.btoa).toHaveBeenCalledWith("abcdefghijklmnopqrstuvwxyz");
        });

        it("should call onError if failed to encode data to base64", function () {
            var request = {
                data: "abcdefghijklmnopqrstuvwxyz"
            },
            onSuccess = jasmine.createSpy("client onSuccess"),
            onError = jasmine.createSpy("client onError");

            GLOBAL.window.btoa.andCallFake(function () {
                throw "btoa error";
            });

            client.invokeViewer(request, onSuccess, onError);
            expect(window.btoa).toHaveBeenCalledWith("abcdefghijklmnopqrstuvwxyz");
            expect(onSuccess).not.toHaveBeenCalled();
            expect(onError).toHaveBeenCalledWith("btoa error");

            GLOBAL.window.btoa = jasmine.createSpy("window.btoa").andReturn("base64 string");
        });

        it("should call once and execAsync", function () {
            var request = {
                    target: "abc.xyz"
                };

            client.invokeViewer(request, null, null);

            expect(mockedWebworks.event.once).toHaveBeenCalledWith(_ID, "blackberry.invoke.invokeViewerEventId", jasmine.any(Function));
            expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "invokeViewer", {"request": request});
        });

        it("should trigger success callback if viewer is invoked successfully", function () {
            var request = {
                    target: "abc.xyz"
                },
                viewer,
                onError = jasmine.createSpy("client onError"),
                onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                    viewer = response;
                });

            client.invokeViewer(request, onSuccess, onError);
            expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
            expect(onError).not.toHaveBeenCalled();
            expect(viewer).toBeDefined();
            expect(viewer.close).toBeDefined();
            expect(viewer.setSize).toBeDefined();
            expect(viewer.setPosition).toBeDefined();
            expect(viewer.setVisibility).toBeDefined();
            expect(viewer.setZOrder).toBeDefined();
            expect(viewer.send).toBeDefined();
            expect(viewer.update).toBeDefined();
        });

        it("should trigger error callback if viewer is not invoked successfully", function () {
            var request = {
                    target: "abc.xyz"
                },
                onSuccess = jasmine.createSpy("client onSuccess"),
                onError = jasmine.createSpy("client onError");

            mockedWebworks.execAsync.andCallFake(function (id, action, args) {
                if (id && id === "blackberry.invoke" && action && action === "invokeViewer") {
                    var _invokeViewerEventId = "blackberry.invoke.invokeViewerEventId";

                    mockedWebworks.event.handler[_invokeViewerEventId]({"error": "My Error", "viewer": {}});

                    delete mockedWebworks.event.handler[_invokeViewerEventId];
                }
            });

            client.invokeViewer(request, onSuccess, onError);
            expect(onSuccess).not.toHaveBeenCalled();
            expect(onError).toHaveBeenCalledWith("My Error");
        });

        describe("viewer response object", function () {

            it("close method should call execAsync", function () {
                var request = {
                        target: "abc.xyz"
                    },
                    viewerRequest = {
                        request: {
                            id: "viewer001",
                            method: "close"
                        }
                    },
                    viewer,
                    onError = jasmine.createSpy("client onError"),
                    onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                        viewer = response;
                        viewer.close();
                    });

                client.invokeViewer(request, onSuccess, onError);
                expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onError).not.toHaveBeenCalled();
                expect(viewer).toBeDefined();
                expect(viewer.close).toBeDefined();
                expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
            });

            it("setSize method should call execAync", function () {
                var request = {
                        target: "abc.xyz"
                    },
                    viewerRequest = {
                        request: {
                            id: "viewer001",
                            method: "setSize",
                            params: [400, 400]
                        }
                    },
                    viewer,
                    onError = jasmine.createSpy("client onError"),
                    onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                        viewer = response;
                        viewer.setSize(400, 400);
                    });

                client.invokeViewer(request, onSuccess, onError);
                expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onError).not.toHaveBeenCalled();
                expect(viewer).toBeDefined();
                expect(viewer.setSize).toBeDefined();
                expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
            });

            it("setPosition method should call execAync", function () {
                var request = {
                        target: "abc.xyz"
                    },
                    viewerRequest = {
                        request: {
                            id: "viewer001",
                            method: "setPosition",
                            params: [400, 400]
                        }
                    },
                    viewer,
                    onError = jasmine.createSpy("client onError"),
                    onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                        viewer = response;
                        viewer.setPosition(400, 400);
                    });

                client.invokeViewer(request, onSuccess, onError);
                expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onError).not.toHaveBeenCalled();
                expect(viewer).toBeDefined();
                expect(viewer.setPosition).toBeDefined();
                expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
            });

            it("setVisibility method should call execAync", function () {
                var request = {
                        target: "abc.xyz"
                    },
                    viewerRequest = {
                        request: {
                            id: "viewer001",
                            method: "setVisibility",
                            params: [true]
                        }
                    },
                    viewer,
                    onError = jasmine.createSpy("client onError"),
                    onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                        viewer = response;
                        viewer.setVisibility(true);
                    });

                client.invokeViewer(request, onSuccess, onError);
                expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onError).not.toHaveBeenCalled();
                expect(viewer).toBeDefined();
                expect(viewer.setVisibility).toBeDefined();
                expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
            });

            it("setZOrder method should call execAync", function () {
                var request = {
                        target: "abc.xyz"
                    },
                    viewerRequest = {
                        request: {
                            id: "viewer001",
                            method: "setZOrder",
                            params: [2]
                        }
                    },
                    viewer,
                    onError = jasmine.createSpy("client onError"),
                    onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                        viewer = response;
                        viewer.setZOrder(2);
                    });

                client.invokeViewer(request, onSuccess, onError);
                expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onError).not.toHaveBeenCalled();
                expect(viewer).toBeDefined();
                expect(viewer.setZOrder).toBeDefined();
                expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
            });

            describe("send method", function () {
                it("should call execAync", function () {
                    var request = {
                            target: "abc.xyz"
                        },
                        message = {
                            msg: "My message",
                            data: "My Data"
                        },
                        viewerRequest = {
                            request: {
                                id: "viewer001",
                                method: "send",
                                params: [message]
                            }
                        },
                        viewer,
                        onError = jasmine.createSpy("client onError"),
                        onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                            viewer = response;
                            viewer.send(message);
                        });

                    client.invokeViewer(request, onSuccess, onError);
                    expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                    expect(onError).not.toHaveBeenCalled();
                    expect(viewer).toBeDefined();
                    expect(viewer.send).toBeDefined();
                    expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
                });

                it("should not call once if no callback is supplied", function () {
                    var request = {
                            target: "abc.xyz"
                        },
                        message = {
                            msg: "My message",
                            data: "My Data"
                        },
                        viewer,
                        relayViewerEventId = "blackberry.invoke.relayViewerEventId",
                        onError = jasmine.createSpy("client onError"),
                        onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                            viewer = response;
                            viewer.send(message);
                        });

                    client.invokeViewer(request, onSuccess, onError);
                    expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                    expect(onError).not.toHaveBeenCalled();
                    expect(viewer).toBeDefined();
                    expect(viewer.send).toBeDefined();
                    expect(mockedWebworks.event.isOn).not.toHaveBeenCalledWith(relayViewerEventId);
                    expect(mockedWebworks.event.once).not.toHaveBeenCalledWith(_ID, relayViewerEventId, jasmine.any(Function));
                });

                it("should trigger the callback", function () {
                    var request = {
                            target: "abc.xyz"
                        },
                        message = {
                            msg: "My message",
                            data: "My Data"
                        },
                        viewer,
                        sendCallback = jasmine.createSpy("send callback"),
                        relayViewerEventId = "blackberry.invoke.relayViewerEventId",
                        onError = jasmine.createSpy("client onError"),
                        onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (response) {
                            viewer = response;
                            viewer.send(message, sendCallback);
                        });

                    client.invokeViewer(request, onSuccess, onError);
                    expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                    expect(onError).not.toHaveBeenCalled();
                    expect(viewer).toBeDefined();
                    expect(viewer.send).toBeDefined();
                    expect(mockedWebworks.event.isOn).toHaveBeenCalledWith(relayViewerEventId);
                    expect(mockedWebworks.event.once).toHaveBeenCalledWith(_ID, relayViewerEventId, jasmine.any(Function));
                    expect(sendCallback).toHaveBeenCalledWith("name", "message");
                });

            });

            it("update method should call execAync", function () {
                var request = {
                        target: "abc.xyz"
                    },
                    viewerRequest = {
                        request: {
                            id: "viewer001",
                            method: "update",
                        }
                    },
                    onError = jasmine.createSpy("client onError"),
                    onSuccess = jasmine.createSpy("client onSuccess").andCallFake(function (viewer) {
                        expect(viewer.update).toBeDefined();

                        viewer.update();

                        expect(mockedWebworks.execAsync).toHaveBeenCalledWith(_ID, "viewerMethod", viewerRequest);
                    });

                client.invokeViewer(request, onSuccess, onError);
                expect(onSuccess).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onError).not.toHaveBeenCalled();
            });
        });
    });
});
