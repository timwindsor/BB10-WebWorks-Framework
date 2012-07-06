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

var controllerWebView,
    utils;

controllerWebView = {
    init: function (config) {
        var controller = window.qnx.webplatform.getController(),
            invocation = window.qnx.webplatform.getApplication().invocation,
            utils = require('./utils');
        controller.enableWebInspector = config.debugEnabled;
        controller.enableCrossSiteXHR = true;
        controller.visible = false;
        controller.active = false;
        controller.setGeometry(0, 0, screen.width, screen.height);
        controller.setFileSystemSandbox = false;

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

        controller.publishRemoteFunction('webview.downloadSharedFile', function (args, callback) {

            var directory   = window.qnx.webplatform.getApplication().getEnv("HOME"),
                target      = directory + "/../shared/" + args[1] + "/",
                source      = args[0],
                fileName    = args[0].replace(/^.*[\\\/]/, '');

            window.requestFileSystem  = window.requestFileSystem || window.webkitRequestFileSystem;

            if (utils.startsWith(source, "local:///")) {
                source = directory + "/../app/native/" + fileName;
            }


            function errorHandler(e) {
                alert(e);
            }

            function copyRemoteFileToShared(fileSystem) {
                var xhr = new XMLHttpRequest();
                xhr.open('GET', source, true);
                xhr.responseType = 'arraybuffer';

                xhr.onload = function(e) {
                  window.requestFileSystem(TEMPORARY, 1024 * 1024, function(fs) {
                    fs.root.getFile(fileName, {create: true}, function(fileEntry) {
                      fileEntry.createWriter(function(writer) {

                        writer.onwrite = function(e) { ... };
                        writer.onerror = function(e) { ... };

                        var bb = new BlobBuilder();
                        bb.append(xhr.response);

                        writer.write(bb.getBlob('image/png'));

                      }, onError);
                    }, onError);
                  }, onError);
                };

                xhr.send();
            }

            function copyLocalFileToShared(fileSystem) {
                fileSystem.root.getFile(source, { create: false, exclusive: false }, function (fileEntry) {
                    fileSystem.root.getDirectory(target, { create: false }, function (dirEntry) {
                        fileEntry.copyTo(dirEntry, fileEntry.name, function () {
                            callback(target + fileEntry.name);
                        }, errorHandler);
                    }, errorHandler);
                }, errorHandler);
            }

            window.requestFileSystem(window.TEMPORARY, 1024 * 1024, copyLocalFileToShared, errorHandler);

        });

        controller.publishRemoteFunction('invocation.queryTargets', function (args, callback) {
            var request = args[0];
            invocation.queryTargets(request, function (error, results) {
                callback(results);
            });
        });

        controller.publishRemoteFunction('invocation.invoke', function (value, callback) {
            console.log("ControllerWebView - publish invocation.invoke: " + value);
            var request = value[0];
            invocation.invoke(request, callback);
        });
    }
};

module.exports = controllerWebView;
