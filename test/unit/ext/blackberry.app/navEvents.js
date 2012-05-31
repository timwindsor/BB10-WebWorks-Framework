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
    activeSpy = jasmine.createSpy(),
    inactiveSpy = jasmine.createSpy();

beforeEach(function () {
    GLOBAL.window = GLOBAL;
    GLOBAL.window.qnx = {
        webplatform: {
            getApplication: function () {
                return {
                    onWindowActive: activeSpy,
                    onWindowInactive: inactiveSpy
                };
            }
        }
    };
    navEvents = require(_apiDir + "navEvents");
});

describe("blackberry.app navEvents", function () {
    describe("addEventListener", function () {
        var trigger = function () {};

        it("calls application.onWindowInactive for 'pause' event", function () {
            navEvents.addEventListener("pause", trigger);
            expect(inactiveSpy).toHaveBeenCalledWith(trigger);
        });

        it("calls application.onWindowActive for 'resume' event", function () {
            navEvents.addEventListener("resume", trigger);
            expect(activeSpy).toHaveBeenCalledWith(trigger);
        });
    });

    describe("removeEventListener", function () {
        it("calls application.onWindowInactive for 'pause' event", function () {
            navEvents.removeEventListener("pause");
            expect(inactiveSpy).toHaveBeenCalledWith(null);
        });

        it("calls application.onWindowActive for 'resume' event", function () {
            navEvents.removeEventListener("resume");
            expect(activeSpy).toHaveBeenCalledWith(null);
        });
    });
});
