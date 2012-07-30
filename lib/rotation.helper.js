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

var rotationCallbacks = {},
    delayedCallbacks = {},
    pendingViewportChanges = 0,
    self;

var viewportChanged = function (webviewId) {
    pendingViewportChanges--;
    if (pendingViewportChanges === 0) {
        window.qnx.webplatform.getApplication().removeEventListener('application.propertyViewportEvent', viewportChanged);
        qnx.callExtensionMethod('application.notifyRotateComplete');
    }
};

var onRotate = function (width, height, angle) {
    pendingViewportChanges = 0;
    delayedCallbacks = {};
    window.qnx.webplatform.getApplication().addEventListener('application.propertyViewportEvent', viewportChanged);
    qnx.callExtensionMethod('applicationWindow.setOrientation', angle);
    Object.keys(rotationCallbacks).forEach(function (webview) {
        webview = window.parseInt(webview, 10);
        qnx.callExtensionMethod("webview.setApplicationOrientation", webview, angle);
        if (rotationCallbacks[webview].isVisible(webview)) {
            rotationCallbacks[webview].callback(webview, width, height, angle);
            pendingViewportChanges++;
        } else {
            // Toss the needed rotation calls into a function, the paramaters will
            // not be available when they are later called on the non-visible webviews.
            delayedCallbacks[webview] = function () {
                qnx.callExtensionMethod("webview.setApplicationOrientation", webview, angle);
                rotationCallbacks[webview].callback(webview, width, height, angle);
            };
        }
    });
};

var onRotateDone = function () {
    // Set the orientation and geometry of non-visible webviews
    Object.keys(delayedCallbacks).forEach(function (webview) {
        delayedCallbacks[webview]();
    });
    
    // Finally, notify all webviews that rotation has completed
    Object.keys(rotationCallbacks).forEach(function (webview) {
        qnx.callExtensionMethod("webview.notifyApplicationOrientationDone", webview);
    });
};

self = {

    addWebview: function (webview, isVisible, setGeometryCallback) {
        rotationCallbacks[webview] = {callback: setGeometryCallback, isVisible: isVisible};
    },
    
    removeWebview: function (webview) {
        delete rotationCallbacks[webview];
    }
};

window.qnx.webplatform.getApplication().addEventListener('application.rotate', onRotate);
window.qnx.webplatform.getApplication().addEventListener('application.rotateDone', onRotateDone);

module.exports = self;