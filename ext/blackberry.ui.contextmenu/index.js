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

var contextmenu,
    _webview = require('./../../lib/webview'),
    _overlayWebView = require('./../../lib/overlayWebView'),
    _utils = require('./../../lib/utils'),
    _config = require('./../../lib/config.js'),
    _event = require('./../../lib/event'),
    _menuItems,
    _currentContext,
    _controller = window.qnx.webplatform.getController(),
    _application = window.qnx.webplatform.getApplication(),
    _customContextItems = {},
    CONTEXT_ALL = 'ALL',
    CONTEXT_LINK = 'LINK',
    CONTEXT_IMAGE_LINK = 'IMAGE_LINK',
    CONTEXT_IMAGE = 'IMAGE',
    CONTEXT_TEXT = 'TEXT',
    CONTEXT_INPUT = 'INPUT',
    _menuActions;


function enabled(success, fail, args, env) {
    if (args) {
        var enable = JSON.parse(decodeURIComponent(args["enabled"]));
        _webview.setContextMenuEnabled(enable);

        success('return value goes here for success');
    } else {
        fail('ContextMenuEnabled property can only be set with true false.');
    }
}

function setMenuOptions(options) {
    _menuItems = options;
}

function peekContextMenu() {
    //rpc to set head text
    //rpc to set subhead text
    //rpc to set the items
    //rpc to peek menu
}

function isMenuVisible() {
    // rpc to overlay to determine visibility
}

function setCurrentContext(context) {
    _currentContext = context;
}

function generateMenuItems(menuItems) {
    var items = [],
    i;

    for (i = 0; i < menuItems.length; i++) {
        switch (menuItems[i]) {
        case 'ClearField':
            items.push({'label': 'Clear Field', 'actionId': 'ClearField', 'imageUrl': 'assets/Browser_Cancel_Selection.png'});
            break;
        case 'SendLink':
            break;
        case 'SendImageLink':
            break;
        case 'FullMenu':
            break;
        case 'Delete':
            break;
        case 'Cancel':
            items.push({'label': 'Cancel', 'actionId': 'Cancel', 'imageUrl': 'assets/Browser_Cancel_Selection.png'});
            break;
        case 'Cut':
            items.push({'label': 'Cut', 'actionId': 'Cut', 'imageUrl': 'assets/Browser_Cut.png'});
            break;
        case 'Copy':
            items.push({'label': 'Copy', 'actionId': 'Copy', 'imageUrl': 'assets/Browser_Copy.png'});
            break;
        case 'Paste':
            items.push({'label': 'Paste', 'actionId': 'Paste', 'imageUrl': 'assets/crosscutmenu_paste.png'});
            break;
        case 'Select':
            items.push({'label': 'Select', 'actionId': 'Select', 'imageUrl': 'assets/crosscutmenu_paste.png'});
            break;
        case 'AddLinkToBookmarks':
            break;
        case 'CopyLink':
            items.push({'label': 'Copy Link', 'actionId': 'CopyLink', 'imageUrl': 'assets/Browser_CopyLink.png'});
            break;
        case 'OpenLinkInNewTab':
            break;
        case 'OpenLink':
            items.push({'label': 'Open', 'actionId': 'OpenLink', 'imageUrl': 'assets/Browser_OpenLink.png'});
            break;
        case 'SaveLinkAs':
            items.push({'label': 'Save Link as', 'actionId': 'SaveLinkAs', 'imageUrl': 'assets/Browser_SaveLink.png'});
            break;
        case 'SaveImage':
            items.push({'label': 'Save Image', 'actionId': 'SaveImage', 'imageUrl': 'assets/Browser_SaveImage.png'});
            break;
        case 'CopyImageLink':
            items.push({'label': 'Copy Image Link', 'actionId': 'CopyImageLink', 'imageUrl': 'assets/Browser_CopyImageLink.png'});
            break;
        case 'ViewImage':
            break;
        case 'Search':
            break;
        case 'ShareLink':
            // local and file protocol won't have sharelink menuitem
            if (!/^local|^file/.test(_currentContext.url)) {
                items.push({'label': 'Share Link', 'actionId': 'ShareLink', 'imageUrl': 'assets/Browser_ShareLink.png'});
            }
            break;
        case 'ShareImage':
            break;
        case 'InspectElement':
            items.push({'label': 'Inspect Element', 'actionId': 'InspectElement', 'imageUrl': 'assets/generic_81_81_placeholder.png'});
            break;
        }
    }

    if (_currentContext && _currentContext.url && _currentContext.text) {
        items.push({'headText': _currentContext.text, 'subheadText': _currentContext.url});
    }

    return items;
}

function addCustomItemsForContext(items, context) {
    var customItem; 
       
    if (_customContextItems[context]) {
        for (customItem in _customContextItems[context]) {
            items.push(_customContextItems[context][customItem]); 
        }
    }
}

function addCustomItems(menuItems, currentContext) {
      
    var context;

    // Add ALL
    addCustomItemsForContext(menuItems, CONTEXT_ALL);
   
    // Determine context
    if (currentContext.url && !currentContext.isImage) {
        context = CONTEXT_LINK;
    }
    else if (currentContext.url && currentContext.isImage) {
        context = CONTEXT_IMAGE_LINK;
    }
    else if (currentContext.isImage) {
        context = CONTEXT_IMAGE;
    }
    else if (currentContext.isInput) {
        context = CONTEXT_INPUT;
    }
    else if (currentContext.text) {
        context = CONTEXT_TEXT;
    }
    
    addCustomItemsForContext(menuItems, context);
}


function generateInvocationList(request, errorMessage) {
    var args = [request, errorMessage];
    qnx.webplatform.getController().remoteExec(1, "invocation.queryTargets", args, function (results) {
        if (results.length > 0) {
            var list = require('listBuilder');
            list.init();
            list.setHeader(results[0].label);
            list.populateList(results[0].targets, request);
            list.show();
        } else {
            alert(errorMessage);
        }
    });
}

// Default context menu response handler
function handleContextMenuResponse(args) {
    var menuAction = args[0];
    qnx.callExtensionMethod('webview.handleContextMenuResponse', 2, menuAction);
}

function loadClientURL(args) {
    console.log(args);
    var url = args[0];
    qnx.callExtensionMethod('webview.loadURL', 2, url);
}

function downloadSharedFile(args, callback) {

    var directory   = window.qnx.webplatform.getApplication().getEnv("HOME"),
        target      = directory + "/../shared/" + args[1] + "/",
        source      = args[0],
        fileName    = args[0].replace(/^.*[\\\/]/, ''),
        xhr;

    window.requestFileSystem  = window.requestFileSystem || window.webkitRequestFileSystem;

    // Check for a local file, if so, let's change it an absolute file path
    if (_utils.startsWith(source, "local:///")) {
        source = "file:/" + directory + "/../app/native/" + source.replace(/local:\/\/\//, '');
    }

    xhr = new XMLHttpRequest();
    xhr.open('GET', source, true);
    xhr.responseType = 'arraybuffer';

    function onError(error) {
        console.log(error);
    }

    xhr.onload = function (e) {
        window.requestFileSystem(window.TEMPORARY, 1024 * 1024, function (fileSystem) {
            fileSystem.root.getFile(target + fileName, {create: true}, function (fileEntry) {
                fileEntry.createWriter(function (writer) {
                    writer.onerror = function (e) {
                        console.log("Could not properly write " + fileName);
                        //pass
                    };

                    var bb = new window.WebKitBlobBuilder();
                    bb.append(xhr.response);
                    writer.write(bb.getBlob(_utils.fileNameToImageMIME(fileName)));

                    // Call the callback sending back the filepath to the image so the Viewer can be invoked
                    callback(target + fileEntry.name);
                }, onError);
            }, onError);
        }, onError);
    };

    xhr.send();
}

function saveLink() {
    if (!_currentContext || !_currentContext.url) {
        return;
    }
    var title = '';
    //TODO FIXME
    //_controller.downloadURL([_currentContext.url, title]);
}

function openLink() {
    if (!_currentContext || !_currentContext.url) {
        return;
    }
    //Update the content web view with the new URL
    loadClientURL([_currentContext.url]);
}

function shareLink() {

    if (!_currentContext || !_currentContext.url) {
        return;
    }

    var request = {
        action: 'bb.action.SHARE',
        type : 'text/plain',
        data : _currentContext.url,
        action_type: _application.invocation.ACTION_TYPE_ALL,
        target_type: _application.invocation.TARGET_TYPE_APPLICATION
    };

    generateInvocationList(request, 'No link sharing applications installed');
}


function saveImage() {

    // Ensure we have a proper context of the image to save
    if (!_currentContext || !_currentContext.isImage || !_currentContext.src) {
        return;
    }

    // Check that the proper access permissions have been enabled
    if (!_config.permissions || _config.permissions.indexOf("access_shared") === -1) {
        alert("Access shared permissions are not enabled");
        return;
    }

    var source     = _currentContext.src,
        target     = "photos";

    function onSaved(target) {

        if (target) {
            var request = {
                action: 'bb.action.VIEW',
                type: _utils.fileNameToImageMIME(target),
                uri : "file:/" + target, //target comes back with double slash, change to triple
                action_type: _application.invocation.ACTION_TYPE_ALL,
                target_type: _application.invocation.TARGET_TYPE_ALL
            };

            generateInvocationList(request, 'No image viewing applications installed');
        }
    }

    // Download the file over an RPC call to the controller, it will call our onSaved method to see if we succeeded
    downloadSharedFile([source, target], onSaved);
}

function responseHandler(menuAction) {
    if (!menuAction) {
        console.log("Menu Action was null");
        return;
    }
    console.log("Calling native with the action: " + menuAction + " on the client webview");
    handleContextMenuResponse([menuAction]);
}

function restoreDefaultMenu() {
    _menuActions = {
        'SaveLink'       : saveLink,
        'Cancel'         : responseHandler,
        'Cut'            : responseHandler,
        'Copy'           : responseHandler,
        'Paste'          : responseHandler,
        'Select'         : responseHandler,
        'CopyLink'       : responseHandler,
        'OpenLink'       : openLink,
        'SaveLinkAs'     : responseHandler,
        'CopyImageLink'  : responseHandler,
        'SaveImage'      : saveImage,
        'ShareLink'      : shareLink,
        'InspectElement' : responseHandler,
    };
    _customContextItems = {};
}

function customItemHandler(actionId) {
    _event.trigger('contextmenu.executeMenuAction', actionId);    
}

function addItem(success, fail, args, env) {
    var contexts = JSON.parse(decodeURIComponent(args["contexts"])),
        action = JSON.parse(decodeURIComponent(args["action"])),
        context;

    // Check if item already has been added
    if (_menuActions[action.actionId]) {
        return fail('Cannot add item.  A menu item with the actionId "' + action.actionId + '" already exists.');
    }

    for (context in contexts) {
        if (!_customContextItems[contexts[context]]) {
            _customContextItems[contexts[context]] = {};
        }
        _customContextItems[contexts[context]][action.actionId] = action; 
    }
    _menuActions[action.actionId] = customItemHandler.bind(this, action.actionId);
    success();
}

function safeEval(jsonString) {
    return JSON.parse('{"obj":' + jsonString + '}').obj;
}

function removeItemFromAllContexts(actionId) {
    var everyContext = [CONTEXT_ALL,
                        CONTEXT_LINK, 
                        CONTEXT_IMAGE_LINK,
                        CONTEXT_IMAGE,
                        CONTEXT_INPUT,
                        CONTEXT_TEXT],
        context;

    for (context in everyContext) {
        delete _customContextItems[everyContext[context]][actionId]; 
    }
}

function removeItem(success, fail, args, env) {
    var contexts = JSON.parse(decodeURIComponent(args["contexts"])),
        actionId = safeEval(decodeURIComponent(args["actionId"])),
        context;

    for (context in contexts) {
        if (contexts[context] === CONTEXT_ALL) {
            removeItemFromAllContexts(actionId);
        } else {
            delete _customContextItems[contexts[context]][actionId]; 
        }
    }
    delete _menuActions[actionId];
    success();
}

function init() {
    _overlayWebView.onPropertyCurrentContextEvent = function (value) {
        _currentContext = JSON.parse(value);
    };
    _overlayWebView.onContextMenuRequestEvent = function (value) {
        var menu = JSON.parse(value),
            menuItems = generateMenuItems(menu.menuItems),
            args; 

        addCustomItems(menuItems, _currentContext);

        args = JSON.stringify({'menuItems': menuItems,
                              '_currentContext': _currentContext});

        _overlayWebView.executeJavaScript("window.showMenu(" + args + ")");
        return '{"setPreventDefault":true}';
    };
    _controller.publishRemoteFunction('executeMenuAction', function (args, callback) {
        var action = args[0];
        if (action) {
            console.log("Executing action: " + args[0]);
            //Call the items[action] function //
            _menuActions[action](action);
        } else {
            console.log("No action item was set");
        }
    });
    _webview.addEventListener('DocumentLoaded', function () {
        restoreDefaultMenu();
    });
}

contextmenu = {
    init: init,
    setMenuOptions: setMenuOptions,
    peek: peekContextMenu,
    addItem: addItem,
    removeItem: removeItem
};

// Listen for the init event
qnx.webplatform.getController().addEventListener('ui.init', function () {
    init();
    restoreDefaultMenu();
});


module.exports = contextmenu;
