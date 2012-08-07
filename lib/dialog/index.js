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
    nativeEventId = 'DialogRequested',
    resultCallback;

function show(description, callback) {

    /* Set the callback for sending our result back to the user */
    resultCallback = callback;

    var value = JSON.stringify(description);
    _overlay.executeJavascript("window.showDialog(" + value + ")");
}

function result(value) {
    if (resultCallback) {
        resultCallback(value);
    }

    /* Pause the eventLoop on this sychronous event, it will cause the thread to block until we return
     * from the RPC call with the parameters
     */
    console.log("Resuming event");
    if (value.ok) {
        qnx.callExtensionMethod('eventLoop.resumeEvent', nativeEventId,
        '{"setPreventDefault": true' +
        (value.hasOwnProperty('username') ? ', "setUsername": "' + encodeURIComponent(value.username) + '"' : '') +
        (value.hasOwnProperty('password') ? ', "setPassword": "' + encodeURIComponent(value.password) + '"' : '') +
        (value.hasOwnProperty('oktext') ? ', "setResult": "' + value.oktext + '"' : '') + '}'
        );
    }
    if (value.cancel) {
        qnx.callExtensionMethod('eventLoop.resumeEvent', nativeEventId, '{"setPreventDefault": true, "setResult": null}');
    }
    if (value.save) {
        qnx.callExtensionMethod('eventLoop.resumeEvent', nativeEventId, '{"setPreventDefault": true, "setResult": "save"}');
    }
    if (value.never) {
        qnx.callExtensionMethod('eventLoop.resumeEvent', nativeEventId, '{"setPreventDefault": true, "setResult": "never"}');
    }
}

function onDialogRequested(eventArguments) {

    console.log(arguments);
    /* Pause the event loop in the client to block executino
     * only resume when we get an RPC back with the result!
     * dangerous
     */
    console.log(eventId);
    nativeEventId = eventId;
    qnx.callExtensionMethod('eventLoop.delayEvent', eventId);
    console.log("Delaying Event");
    show(description);
    //return '{ "setPreventDefault" : true }';
}

dialog  = {
    show : show,
    onDialogRequested : onDialogRequested,
    result : result,
};

module.exports = dialog;
