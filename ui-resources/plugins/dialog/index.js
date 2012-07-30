/*
 * Copyright (C) Research In Motion Limited 2012. All rights reserved.
 */

var dialog;

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
        button3,
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
        header.bottom(x$(document.createTextNode(desc.title ? desc.title : "JavaScript Alert")));
        content.bottom(desc.htmlmessage ? desc.htmlmessage : x$(document.createTextNode(desc.message)));
        button.bottom(x$(document.createTextNode(desc.oklabel ? desc.oklabel : "OK")))
              .on('click', hide);

        panel.bottom(header)
             .bottom(content)
             .bottom(buttons
                .bottom(button));

        res.ok = button[0];
        break;
    case 'InsecureSubresourceLoadPolicyConfirm':
        desc.title = "Insecure Contents Confirm";
        desc.oklabel = "Yes";
        desc.cancellabel = "No";
        /* falls through */
    case 'JavaScriptConfirm':
        header.bottom(x$(document.createTextNode(desc.title ? desc.title : "JavaScript Confirm")));
        content.bottom(desc.htmlmessage ? desc.htmlmessage : x$(document.createTextNode(desc.message)));
        button.bottom(x$(document.createTextNode(desc.oklabel ? desc.oklabel : "OK")))
              .on('click', hide);
        divider = x$(document.createElement('div'))
            .addClass('dialog-button-divider');
        button2 = x$(document.createElement('button'))
            .addClass('dialog-button')
            .bottom(x$(document.createTextNode(desc.cancellabel ? desc.cancellabel : "Cancel")))
            .on('click', hide);

        panel.bottom(header)
             .bottom(content)
             .bottom(buttons
                .bottom(button)
                .bottom(divider)
                .bottom(button2));

        if (desc.thirdOptionLabel) {
            button3 = x$(document.createElement('button'))
                .addClass('dialog-button')
                .bottom(x$(document.createTextNode(desc.thirdOptionLabel)))
                .on('click', hide);
            buttons
                .bottom(x$(document.createElement('div'))
                    .addClass('dialog-button-divider'))
                .bottom(button3);
            res.thirdOptionButton = button3[0];
        }

        res.ok = button[0];
        res.cancel = button2[0];
        res.oktext = 'true';
        break;
    case 'JavaScriptPrompt':
        header.bottom(x$(document.createTextNode(desc.title ? desc.title : "JavaScript Prompt")));
        input = x$(document.createElement('input'))
            .attr('type', 'text')
            .addClass('dialog-input')
            .attr('value', desc.result)
            .on('keydown', function (keyEvent) {
                if (parseInt(keyEvent.keyCode, 10) === 13) {
                    button.click();
                }
            });
        content.bottom(desc.htmlmessage ? desc.htmlmessage : x$(document.createTextNode(desc.message)));
        button.bottom(x$(document.createTextNode(desc.oklabel ? desc.oklabel : "OK")))
              .on('click', hide);
        divider = x$(document.createElement('div'))
            .addClass('dialog-button-divider');
        button2 = x$(document.createElement('button'))
            .addClass('dialog-button')
            .bottom(x$(document.createTextNode(desc.cancellabel ? desc.cancellabel : "Cancel")))
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
    case 'NotificationPermission':
    case 'DatabaseQuotaExceeded':
        /* falls through */
    default:
        return;
    }
    dialog.removeClass('hidden');
    return res;
}

/*function requestDialog(id, evt, evtId, returnValue) {
    if (id === iris.chromeId) {
        console.error("chrome shouldn't spawn dialogs. Doing nothing.");
        return;
    }
    // have to bring it to the front, as with the current process model one dialog suspends all tabs
    tabs.update(id, {selected: true});

    var res = show(evt);
    if (res) {
        qnx.callExtensionMethod('eventLoop.delayEvent', evtId);
        if (res.ok) {
            x$(res.ok).on('click', function () {
                qnx.callExtensionMethod(
                    'eventLoop.resumeEvent',
                    evtId,
                    '{"setPreventDefault": true'
                        + (res.hasOwnProperty('username') ? ', "setUsername": "' + encodeURIComponent(res.username) + '"' : '')
                        + (res.hasOwnProperty('password') ? ', "setPassword": "' + encodeURIComponent(res.password) + '"' : '')
                        + (res.hasOwnProperty('oktext') ? ', "setResult": "' + res.oktext + '"' : '') + '}'
                );
            });
        }
        if (res.cancel) {
            x$(res.cancel).on('click', function () {
                qnx.callExtensionMethod('eventLoop.resumeEvent', evtId, '{"setPreventDefault": true, "setResult": null}');
            });
        }
        if (res.save) {
            x$(res.save).on('click', function () {
                qnx.callExtensionMethod('eventLoop.resumeEvent', evtId, '{"setPreventDefault": true, "setResult": "save"}');
            });
        }
        if (res.never) {
            x$(res.never).on('click', function () {
                qnx.callExtensionMethod('eventLoop.resumeEvent', evtId, '{"setPreventDefault": true, "setResult": "never"}');
            });
        }
    }
}
*/
dialog = {
    /**
     * description can have
     *   title - the title of the dialog
     *   message - the dialog's message. Text only
     *   htmlmessage - alternate message content, can contain HTML
     *   oklabel - the label for the primary/action/OK button
     *   cancellabel - the label for the secondary/dismiss/Cancel button
     *   neverlabel - the label for "never remember this site" action of save credential dialog
     *
     * @returns object res
     *   ok - The ok button element. Attach your click handlers here
     *   cancel - The cancel button element. Attach your click handlers here
     *   oktext - The string "true" for hitting OK on a Confirm, the input's value for hitting OK on a Prompt, or absent
     *   username - User name for authentication challenge dialog
     *   password - Password for authentication challenge dialog
     */
    showDialog: function (description) {
        return show(description);
    }
};

module.exports = dialog;
