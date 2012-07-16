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

var _apiDir = __dirname + "./../../../../ext/blackberry.app/",
    _libDir = __dirname + "./../../../../lib/",
    navEvents,
    mockedApplication;

describe("blackberry.app navEvents", function () {
    beforeEach(function () {
        mockedApplication = {
            addEventListener: jasmine.createSpy("application addEventListener"),
            removeEventListener: jasmine.createSpy("application removeEventListener")
        };
        GLOBAL.window.qnx = {
            webplatform: {
                getApplication: function () {
                    return mockedApplication;
                }
            }
        };

        var name = require.resolve(_apiDir + "navEvents");
        delete require.cache[name];
        navEvents = require(_apiDir + "navEvents");
    });

    afterEach(function () {
        mockedApplication = null;
        GLOBAL.window.qnx = null;
    });

    describe("addEventListener", function () {
        var trigger = jasmine.createSpy("trigger");

        it("should be called in webplatform.getApplication for 'pause' events", function () {
            navEvents.addEventListener("pause", trigger);
            expect(mockedApplication.addEventListener).toHaveBeenCalledWith('application.pause', trigger);
        });

        it("should be called in webplatform.getApplication for 'resume' events", function () {
            navEvents.addEventListener("resume", trigger);
            expect(mockedApplication.addEventListener).toHaveBeenCalledWith('application.resume', trigger);
        });

        it("should be called in webplatform.getApplication for 'swipedown' events", function () {
            navEvents.addEventListener("swipedown", trigger);
            expect(mockedApplication.addEventListener).toHaveBeenCalledWith('application.swipedown', trigger);
        });

        it("should be called in webplatform.getApplication for 'lowMemory' events", function () {
            navEvents.addEventListener("lowMemory", trigger);
            expect(mockedApplication.addEventListener).toHaveBeenCalledWith('application.lowMemory', trigger);
        });
    });

    describe("removeEventListener", function () {
        var trigger = jasmine.createSpy("trigger");
        
        it("should be called in webplatform.getApplication for 'pause' events", function () {
            navEvents.removeEventListener("pause", trigger);
            expect(mockedApplication.removeEventListener).toHaveBeenCalledWith('application.pause', trigger);
        });

        it("should be called in webplatform.getApplication for 'resume' events", function () {
            navEvents.removeEventListener("resume", trigger);
            expect(mockedApplication.removeEventListener).toHaveBeenCalledWith('application.resume', trigger);
        });

        it("should be called in webplatform.getApplication for 'swipedown' events", function () {
            navEvents.removeEventListener("swipedown", trigger);
            expect(mockedApplication.removeEventListener).toHaveBeenCalledWith('application.swipedown', trigger);
        });

        it("should be called in webplatform.getApplication for 'lowMemory' events", function () {
            navEvents.removeEventListener("lowMemory", trigger);
            expect(mockedApplication.removeEventListener).toHaveBeenCalledWith('application.lowMemory', trigger);
        });
    });
});
