/*
 * Copyright (C) Research In Motion Limited 2012. All rights reserved.
 */

var menu,    
    menuVisible,
    touchStarted = false,
    hideMenuOnTouchEnd = false;

function hideCrosscutMenu() { 
    menu.crosscutMenuVisible(false); 
}

function touchStart() {
    touchStarted = true;
    hideMenuOnTouchEnd = true;
}

function touchEnd() {
    if (menuVisible && hideMenuOnTouchEnd) {
        hideCrosscutMenu();
    }
    touchStarted = false;
}

function init() {
    var menu = document.getElementById('crosscutMenu');
    menu.addEventListener('webkitTransitionEnd', menu.transitionEnd.bind(menu));
    document.addEventListener('touchstart', touchStart, true);
    document.addEventListener('touchend', touchEnd, true);
}

menu = {
	"init": init,
    setMenuOptions: function (options) {
        var menu = document.getElementById("crosscutMenuContent"),
            menuItem,
            callback,
            menuImage,
            i;

        while (menu.childNodes.length >= 1) {
            menu.removeChild(menu.firstChild);
        }

        for (i = 0; i < options.length; i++) {
            menuItem = document.createElement('div');
            callback = options[i].function;
            menuImage = document.createElement('img');
            menuImage.src = options[i].imageUrl ? options[i].imageUrl : 'assets/generic_81_81_placeholder.png';
            menuImage.setAttribute("class", "menuImage");
            menuItem.appendChild(menuImage);
            menuItem.appendChild(document.createTextNode(options[i].name));
            menuItem.setAttribute("class", "menuItem");
            menuItem.ontouchend = callback.bind(this, menuItem);
            // FIXME: need to make this work
            //menuItem.onmousedown = function () { menuItem.attr("class", "menuItem click"); };
            menu.appendChild(menuItem);
        }
    },

    isMenuVisible: function () {
        return menuVisible;
    },

    crosscutMenuVisible: function (show, zIndex) {
        if (show === menuVisible) {
            return;
        }

        var menuElement = document.getElementById('crosscutMenu');

        if (show) {
			//TODO: 3 for iris.chromeId
            qnx.callExtensionMethod("webview.setSensitivity", 3, "SensitivityAlways");
            menuElement.className = 'showMenu';
            menuVisible = true;
            hideMenuOnTouchEnd = !touchStarted;
            event.emit('screen.menu.showing', [], true);
        } else {
			//TODO: 3 for iris.chromeId
            qnx.callExtensionMethod("webview.setSensitivity", 3, "SensitivityTest");
            menuVisible = false;
            menuElement.className = 'hideMenu';
            event.emit('screen.menu.hiding', [], true);
        }
    },

    transitionEnd: function () {
        if (menuVisible) {
            // only make the menu actionable once it is shown
            event.emit('screen.menu.shown', [], true);
        } else {
            event.emit('screen.menu.hidden', [], true);
        }
    },

    menuType: {context: 'context', overflow: 'overflow'}
};

//event.on('browser.plugins.init', init);

module.exports = menu;
