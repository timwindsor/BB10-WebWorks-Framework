define('htmlcontextmenu', function (require, exports, module) {

/*
 * Copyright (C) Research In Motion Limited 2012. All rights reserved.
 */

var self,
    //tabs = require('chrome/tabs'),
    menuVisible,
    menuPeeked;

function init() {
    var menu = document.getElementById('contextMenu');
    menu.addEventListener('webkitTransitionEnd', self.transitionEnd.bind(self));
}

self = {
    init: init,
    setMenuOptions: function (options) {
        var menu = document.getElementById("contextMenuContent");

        while (menu.childNodes.length >= 1) {
            menu.removeChild(menu.firstChild);
        }
        self.setHeadText('');
        self.setSubheadText('');

        for (var i = 0; i < options.length; i++) {
            if (options[i].headText || options[i].subheadText) {
                var header = document.getElementById('contextMenuHeader');
                header.className = 'contextMenuHeader';
                if (options[i].headText) {
                    self.setHeadText(options[i].headText);
                }
                if (options[i].subheadText) {
                    self.setSubheadText(options[i].subheadText);
                }
                continue;
            }
            var menuItem = document.createElement('div');
            var callback = options[i].function;
            var menuImage = document.createElement('img');
            menuImage.src = options[i].imageUrl ? options[i].imageUrl : 'assets/generic_81_81_placeholder.png';
            menuItem.appendChild(menuImage);
            menuItem.appendChild(document.createTextNode(options[i].name));
            menuItem.setAttribute("class", "menuItem");
            menuItem.ontouchend = callback.bind(this, menuItem);
            menuItem.addEventListener('mousedown', self.handleMouseDown, false);
            // FIXME: need to make this work
            //menuItem.onmousedown = function () { menuItem.attr("class", "menuItem click"); };
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
        var menu = document.getElementById('contextMenu');
        menu.removeEventListener('touchend', self.hideContextMenu, false);
        var handle = document.getElementById('contextMenuHandle');
        handle.removeEventListener('touchend', self.showContextMenu, false);
        menuVisible = false;
        menuPeeked = false;
        menu.className = 'hideMenu';
        /*tabs.getSelected(0, function (tab) {
            if (tab) {
                qnx.callExtensionMethod("webview.notifyContextMenuCancelled", tab.id);
            }
        });*/
        // Reset sensitivity
        qnx.callExtensionMethod("webview.setSensitivity", iris.chromeId, "SensitivityTest");
    },

    peekContextMenu: function (show, zIndex) {
        if (menuVisible || menuPeeked) {
            return;
        }
        qnx.callExtensionMethod("webview.setSensitivity", iris.chromeId, "SensitivityNoFocus");
        var menu = document.getElementById('contextMenu');
        var handle = document.getElementById('contextMenuHandle');
        handle.className = 'showContextMenuHandle';
        menuVisible = false;
        menuPeeked = true;
        menu.className = 'peekContextMenu';
    },

    transitionEnd: function () {
        var menu = document.getElementById('contextMenu');
        var handle = document.getElementById('contextMenuHandle');
        if (menuVisible) {
            menu.addEventListener('touchend', self.hideContextMenu, false);
            handle.removeEventListener('touchend', self.showContextMenu, false);
        } else if (menuPeeked) {
            handle.addEventListener('touchend', self.showContextMenu, false);
            menu.addEventListener('touchend', self.hideContextMenu, false);
        } else {
            var header = document.getElementById('contextMenuHeader');
            header.className = '';
            self.setHeadText('');
            self.setSubheadText('');
        }
    }

};

//event.on('browser.plugins.init', init);

module.exports = self;
});
