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

var dialog,
    _overlay = require('../overlayWebView'),
    resultCallback;

function show(description, callback) {

    /* Set the callback for sending our result back to the user */
    resultCallback = callback;

    var value = JSON.stringify(description);
    _overlay.executeJavascript("window.showDialog(" + value + ")");
}

function onDialogRequested(description) {
    var returnValue = { setPreventDefault: true };

    /* Pause the event loop in the client to block executino
     * only resume when we get an RPC back with the result!
     */
    show(description, result);
    return JSON.stringify(returnValue);
}

function result(value) {
    if (resultCallback) {
        resultCallback(value);
    }

    /* Now resume the eventLoop */

}


dialog  = {
    show : show,
    onDialogRequested : onDialogRequested,
    result : result,
};

module.exports = dialog;
