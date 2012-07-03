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

// Warning: Dialog plugin will not work until it is ported from XUI
var self;

function hide(evt) {
    if (!x$('#dialog').hasClass('hidden')) {
        x$('#dialog').addClass('hidden');
    }
}

function show(desc) {
    var dialog = x$('#dialog'),
        panel = x$('#dialog-panel'),
        header = x$(document.createElement('div')),
        content = x$(document.createElement('div')),
        input,
        buttons = x$(document.createElement('div')),
        divider,
        button = x$(document.createElement('button')),
        button2,
        res = {};

    if (!dialog.hasClass('hidden')) {
        dialog.addClass('hidden');
    }
    panel.html('inner', '');

    header.addClass('dialog-header');
    content.addClass('dialog-content');
    buttons.addClass('dialog-buttons');
    button.addClass('dialog-button');

    switch (desc.dialogType) {
    case 'JavaScriptAlert':
        header.bottom(x$(document.createTextNode(desc.title ? desc.title : "JavaScript Alert"))); // TODO: i18n. Omit?
        content.bottom(desc.htmlmessage ? desc.htmlmessage : x$(document.createTextNode(desc.message)));
        button.bottom(x$(document.createTextNode(desc.oklabel ? desc.oklabel : "OK"))) // TODO: i18n
              .on('click', hide);

        panel.bottom(header)
             .bottom(content)
             .bottom(buttons
                .bottom(button));

        res.ok = button[0];
        break;
    case 'JavaScriptConfirm':
        header.bottom(x$(document.createTextNode(desc.title ? desc.title : "JavaScript Confirm"))); // TODO: i18n. Omit?
        content.bottom(desc.htmlmessage ? desc.htmlmessage : x$(document.createTextNode(desc.message)));
        button.bottom(x$(document.createTextNode(desc.oklabel ? desc.oklabel : "OK"))) // TODO: i18n
              .on('click', hide);
        divider = x$(document.createElement('div'))
            .addClass('dialog-button-divider');
        button2 = x$(document.createElement('button'))
            .addClass('dialog-button')
            .bottom(x$(document.createTextNode(desc.cancellabel ? desc.cancellabel : "Cancel"))) // TODO: i18n
            .on('click', hide);

        panel.bottom(header)
             .bottom(content)
             .bottom(buttons
                .bottom(button)
                .bottom(divider)
                .bottom(button2));

        res.ok = button[0];
        res.cancel = button2[0];
        res.oktext = 'true';
        break;
    case 'JavaScriptPrompt':
        header.bottom(x$(document.createTextNode(desc.title ? desc.title : "JavaScript Prompt"))); // TODO: i18n. Omit?
        input = x$(document.createElement('input'))
            .attr('type', 'text')
            .addClass('dialog-input')
            .on('keydown', function (keyEvent) { 
                if (parseInt(keyEvent.keyCode, 10) === 13) { 
                    button.click(); 
                } 
            });
        content.bottom(desc.htmlmessage ? desc.htmlmessage : x$(document.createTextNode(desc.message)));
        button.bottom(x$(document.createTextNode(desc.oklabel ? desc.oklabel : "OK"))) // TODO: i18n
              .on('click', hide);
        divider = x$(document.createElement('div'))
            .addClass('dialog-button-divider');
        button2 = x$(document.createElement('button'))
            .addClass('dialog-button')
            .bottom(x$(document.createTextNode(desc.cancellabel ? desc.cancellabel : "Cancel"))) // TODO: i18n
            .on('click', hide);

        panel.bottom(header)
             .bottom(content
                .bottom(input))
             .bottom(buttons
                .bottom(button)
                .bottom(divider)
                .bottom(button2));

        res.ok = button[0];
        res.cancel = button2[0];
        res.__defineGetter__('oktext', function () { 
            return input[0].value; 
        });
        break;
    case 'Generic':
    case 'GeolocationPermission':
    case 'DatabaseQuotaExceeded':
    case 'AuthenticationChallenge':
        /* falls through */
    default:
        return;
    }
    dialog.removeClass('hidden');
    return res;
}

function requestDialog(id, evt, evtId) {
    if (id === iris.chromeId) {
        console.error("chrome shouldn't spawn dialogs. Doing nothing.");
        return;
    }

    var res = show(evt);
    if (res) {
        qnx.callExtensionMethod('eventLoop.delayEvent', evtId);
        if (res.ok) {
            x$(res.ok).on('click', function () {
                qnx.callExtensionMethod(
                    'eventLoop.resumeEvent',
                    evtId,
                    '{"setPreventDefault": true' + (res.hasOwnProperty('oktext') ? ', "setResult": "' + res.oktext + '"' : '') + '}'
                );
            });
        }
        if (res.cancel) {
            x$(res.cancel).on('click', function () {
                qnx.callExtensionMethod('eventLoop.resumeEvent', evtId, '{"setPreventDefault": true}');
            });
        }
    }
}

self = {
    /**
     * description can have
     *   title - the title of the dialog
     *   message - the dialog's message. Text only
     *   htmlmessage - alternate message content, can contain HTML
     *   oklabel - the label for the primary/action/OK button
     *   cancellabel - the label for the secondary/dismiss/Cancel button
     *
     * @returns object res
     *   ok - The ok button element. Attach your click handlers here
     *   cancel - The cancel button element. Attach your click handlers here
     *   oktext - The string "true" for hitting OK on a Confirm, the input's value for hitting OK on a Prompt, or absent
     */
    showDialog: function (description) {
        return show(description);
    }
};

module.exports = self;
