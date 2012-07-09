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
var _apiDir = __dirname + "./../../../../ext/blackberry.ui.contextmenu/",
    _libDir = __dirname + "./../../../../lib/",
    contextmenu,
    webview = require(_libDir + 'webview'),
    success;

describe("blackberry.ui.contextmenu index", function () {

    beforeEach(function () {
        contextmenu = require(_apiDir + "/index");
        success = jasmine.createSpy("success");
        spyOn(webview, 'setContextMenuEnabled');
    });


    it("enabled is called with false on the webview.setContextMenuEnabled method", function () {
        contextmenu.enabled(success, null, {
            "enabled": encodeURIComponent(JSON.stringify(false))
        }, null);

        expect(webview.setContextMenuEnabled).toHaveBeenCalledWith(false);
        expect(success).toHaveBeenCalled();
    });

    it("enabled is called with true on the webview.setContextMenuEnabled", function () {
        contextmenu.enabled(success, null, {
            "enabled": encodeURIComponent(JSON.stringify(true))
        }, null);

        expect(webview.setContextMenuEnabled).toHaveBeenCalledWith(true);
        expect(success).toHaveBeenCalled();
    });
    
    it("can add a custom menu item", function () {
        contextmenu.addMenuItem(success, null, {
            "contexts": encodeURIComponent(JSON.stringify(['ALL'])),
            "action": encodeURIComponent(JSON.stringify({actionId: 'explosion'}))
        }, null);

        expect(success).toHaveBeenCalled();
    });
    
    it("cannot add multiple custom menu items with the same actionId", function () {
        var fail = jasmine.createSpy();
        contextmenu.addMenuItem(success, fail, {
            "contexts": encodeURIComponent(JSON.stringify(['ALL'])),
            "action": encodeURIComponent(JSON.stringify({actionId: 'Copy'}))
        }, null);

        expect(fail).toHaveBeenCalled();
    });

    it("can remove a custom menu item", function () {
        contextmenu.addMenuItem(success, null, {
            "contexts": encodeURIComponent(JSON.stringify(['ALL'])),
            "action": encodeURIComponent(JSON.stringify({actionId: 'explosion'}))
        }, null);

        expect(success).toHaveBeenCalled();
    });
});
