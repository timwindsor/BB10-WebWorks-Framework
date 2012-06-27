/*
 *  Copyright 2012 Research In Motion Limited.
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

var controllerWebView;

controllerWebView = {
    init: function (config) {
        var controller = window.qnx.webplatform.getController(),
            invocation = window.qnx.webplatform.getApplication().invocation;
        controller.enableWebInspector = config.debugEnabled;
        controller.enableCrossSiteXHR = true;
        controller.visible = false;
        controller.active = false;
        controller.setGeometry(0, 0, screen.width, screen.height);

        controller.publishRemoteFunction('webview.setSensitivity', function (args) {
            var sensitivityType = args[0];
            qnx.callExtensionMethod('webview.setSensitivity', 3, sensitivityType);
        });

        controller.publishRemoteFunction('webview.handleContextMenuResponse', function (args) {
            var menuAction = args[0];
            qnx.callExtensionMethod('webview.handleContextMenuResponse', 2, menuAction);
        });

        controller.publishRemoteFunction('webview.loadURL', function (args) {
            console.log(args);
            var url = args[0];
            qnx.callExtensionMethod('webview.loadURL', 2, url);
        });

        controller.publishRemoteFunction('webview.downloadURL', function (args) {
            console.log(args);
            var url = args[0],
                title = args[1];
            qnx.callExtensionMethod("webview.downloadUrl", 2, url, title, 0);
        });

        controller.publishRemoteFunction('invocation.queryTargets', function (value, callback) {
            console.log("ControllerWebView: " + value);
            var request = {
                    action: 'bb.action.SHARE',
                    type: value[0],
                    target_type: invocation.TARGET_TYPE_ALL,
                    action_type: invocation.ACTION_TYPE_MENU
                };
            invocation.queryTargets(request, function (error, results) {
                console.log("error: " + error);
                callback(results);
            });
        });

        controller.publishRemoteFunction('invocation.invoke', function (value, callback) {
            console.log("ControllerWebView - publish invocation.invoke: " + value);
            var request = value;
            invocation.invoke(request);
        });
    }
};

module.exports = controllerWebView;
