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

var contextmenu,
    menuVisible,
    menuPeeked,
    currentContext,
    config,
    utils,
    includePath;


function requireLocal(id) {
    return require(!!require.resolve ? "../../" + id.replace(/\/chrome/, "") : id);
}

function init() {
    var menu = document.getElementById('contextMenu');
    menu.addEventListener('webkitTransitionEnd', contextmenu.transitionEnd.bind(contextmenu));
    config = requireLocal("../chrome/lib/config.js");
    utils = requireLocal("../chrome/lib/utils");
}

function actionEnd(actionId, menuItem) {
    if (menuItem) {
        menuItem.className = 'menuItem peekItem';
    }
    window.qnx.webplatform.getController().remoteExec(1, 'executeMenuAction', [actionId]);
    return true;
}

function actionStart(menuItem) {
    if (!menuItem || !menuPeeked) {
        return;
    }
    menuItem.className = 'menuItem showItem';
    return true;
}

function handleTouchStart(menuItem) {
    menuItem.onmousedown = function (evt) {
        evt.preventDefault();
    };
    actionStart(menuItem);
}

function handleTouchEnd(actionId, menuItem) {
    menuItem.onmouseup = null;
    actionEnd(actionId, menuItem);
}


contextmenu = {
    init: init,
    setMenuOptions: function (options) {
        var menu = document.getElementById("contextMenuContent"),
            i,
            header,
            menuItem,
            menuImage;

        while (menu.childNodes.length >= 1) {
            menu.removeChild(menu.firstChild);
        }
        contextmenu.setHeadText('');
        contextmenu.setSubheadText('');

        for (i = 0; i < options.length; i++) {
            if (options[i].headText || options[i].subheadText) {
                header = document.getElementById('contextMenuHeader');
                header.className = 'contextMenuHeader';
                if (options[i].headText) {
                    contextmenu.setHeadText(options[i].headText);
                }
                if (options[i].subheadText) {
                    contextmenu.setSubheadText(options[i].subheadText);
                }
                continue;
            }
            menuItem = document.createElement('div');
            menuImage = document.createElement('img');
            menuImage.src = options[i].imageUrl ? options[i].imageUrl : 'assets/generic_81_81_placeholder.png';
            menuItem.appendChild(menuImage);
            menuItem.appendChild(document.createTextNode(options[i].name));
            menuItem.setAttribute("class", "menuItem");

            menuItem.addEventListener('mousedown', contextmenu.handleMouseDown, false);
            menuItem.addEventListener('mousedown', actionStart.bind(this, options[i].actionId, menuItem), false);
            menuItem.addEventListener('mouseup', actionEnd.bind(this, menuItem), false);

            menuItem.ontouchstart = handleTouchStart.bind(this, menuItem);
            menuItem.ontouchend = handleTouchEnd.bind(this, options[i].actionId, menuItem);

            menu.appendChild(menuItem);
        }
    },

    handleMouseDown: function (evt) {
        evt.preventDefault();
    },

    setHeadText: function (text) {
        var headText = document.getElementById('contextMenuHeadText');
        headText.innerText = text;
    },

    setSubheadText: function (text) {
        var subheadText = document.getElementById('contextMenuSubheadText');
        subheadText.innerText = text;
    },

    showContextMenu: function (evt) {
        console.log("Showing Context Menu");
        if (menuVisible) {
            return;
        }
        var menu = document.getElementById('contextMenu');
        menu.className = 'showMenu';
        menuVisible = true;
        if (menuPeeked) {
            evt.cancelBubble = true;
            menuPeeked = false;
        }

    },

    isMenuVisible: function () {
        return menuVisible || menuPeeked;
    },

    hideContextMenu: function () {
        if (!menuVisible && !menuPeeked) {
            return;
        }
        var menu = document.getElementById('contextMenu'),
            handle = document.getElementById('contextMenuHandle');

        handle.ontouchend = null;
        handle.onmouseup = null;

        menuVisible = false;
        menuPeeked = false;
        menu.className = 'hideMenu';
        // Reset sensitivity
        window.qnx.webplatform.getController().remoteExec(1, 'webview.setSensitivity', ['SensitivityTest']);
    },

    peekContextMenu: function (show, zIndex) {
        if (menuPeeked) {
            return;
        }
        window.qnx.webplatform.getController().remoteExec(1, 'webview.setSensitivity', ['SensitivityNoFocus']);
        var menu = document.getElementById('contextMenu'),
            handle = document.getElementById('contextMenuHandle');
        handle.className = 'showContextMenuHandle';
        menuVisible = false;
        menuPeeked = true;
        menu.className = 'peekContextMenu';
    },

    transitionEnd: function () {
        var menu = document.getElementById('contextMenu'),
            handle = document.getElementById('contextMenuHandle'),
            header;
        if (menuVisible) {
            // Add the mouse event listeners first, then if we get a touch
            // remove them and handle the touch properly
            menu.onmouseup = contextmenu.peekContextMenu;
            menu.ontouchend = function (show, zIndex) {
                menu.onmouseup = null;
                contextmenu.hideContextMenu();
            };

            handle.onmouseup = contextmenu.peekContextMenu;
            handle.ontouchend = function (show, zIndex) {
                handle.onmouseup = null;
                contextmenu.peekContextMenu(show, zIndex);
            };

        } else if (menuPeeked) {
            handle.onmouseup = contextmenu.showContextMenu;
            handle.ontouchend = function (evt) {
                handle.onmouseup = null;
                contextmenu.showContextMenu(evt);
            };

            menu.onmouseup = contextmenu.showContextMenu;
            menu.ontouchend = function (evt) {
                menu.onmouseup = null;
                contextmenu.hideContextMenu();
            };

        } else {
            header = document.getElementById('contextMenuHeader');
            header.className = '';
            contextmenu.setHeadText('');
            contextmenu.setSubheadText('');
        }
    },

    setCurrentContext: function (context) {
        currentContext = context;
    }
};

module.exports = contextmenu;
