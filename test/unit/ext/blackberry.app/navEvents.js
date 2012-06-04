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
    addActiveSpy = jasmine.createSpy(),
    addInactiveSpy = jasmine.createSpy(),
    removeActiveSpy = jasmine.createSpy(),
    removeInactiveSpy = jasmine.createSpy();

beforeEach(function () {
    GLOBAL.window = GLOBAL;
    GLOBAL.window.qnx = {
        webplatform: {
            getApplication: function () {
                return {
                    addWindowActiveListener: addActiveSpy,
                    addWindowInactiveListener: addInactiveSpy,
                    removeWindowActiveListener: removeActiveSpy,
                    removeWindowInactiveListener: removeInactiveSpy
                };
            }
        }
    };
    navEvents = require(_apiDir + "navEvents");
});

describe("blackberry.app navEvents", function () {
    describe("addEventListener", function () {
        var trigger = function () {};

        it("calls application.addWindowInactiveListener for 'pause' event", function () {
            navEvents.addEventListener("pause", trigger);
            expect(addInactiveSpy).toHaveBeenCalledWith(trigger);
        });

        it("calls application.addWindowActiveListener for 'resume' event", function () {
            navEvents.addEventListener("resume", trigger);
            expect(addActiveSpy).toHaveBeenCalledWith(trigger);
        });
    });

    describe("removeEventListener", function () {
        var trigger = function () {};

        it("calls application.onWindowInactive for 'pause' event", function () {
            navEvents.removeEventListener("pause", trigger);
            expect(removeInactiveSpy).toHaveBeenCalledWith(trigger);
        });

        it("calls application.onWindowActive for 'resume' event", function () {
            navEvents.removeEventListener("resume", trigger);
            expect(removeActiveSpy).toHaveBeenCalledWith(trigger);
        });
    });
});
