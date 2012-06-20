/*
 * Copyright (C) Research In Motion Limited 2012. All rights reserved.
 */

var listBuilder,
    listItems,
    invocation = window.qnx.webplatform.getApplication().invocation;

function init() {
    listItems = [];
}

function invokeApp(key) {
    // invoke some app based on ID
    console.log("invoking");
    var invokeRequest = {
        target: listItems[key].target,
        action: listItems[key].action
    };

    invocation.invoke(invokeRequest);
    listBuilder.hide();
}

function handleMouseDown(evt) {
    evt.preventDefault();
}

listBuilder = {
    init: init,
    setHeader: function (headerText) {
        var listHeader = document.getElementById('listHeader');
        listHeader.innerHTML = "";
        listHeader.appendChild(document.createTextNode(headerText));
    },
    populateList: function (itemArray) {
        var listContent = document.getElementById('listContent'),
            listItem,
            item; 

        // create a bunch of subdivs
        for (item in itemArray) {
            listItem = document.createElement('div');
            listItem.appendChild(document.createTextNode(itemArray[item].name));
            listItem.setAttribute('class', 'listItem');
            listItem.addEventListener('mousedown', handleMouseDown, false);
            listItem.ontouchend = invokeApp.bind(this, itemArray[item].key);
            listItems[itemArray[item].key] = {
                target: itemArray[item].key,
                action: 'bb.action.SHARE'
            };
            listContent.appendChild(listItem);
        }
        // subdivs will have a click handler to run stuff
        // hide menu after handling a click
    },
    show: function () {
        // set css class to visible
        var listContainer = document.getElementById('listContainer');
        listContainer.setAttribute('class', 'showList');
    },
    hide: function () {
        // set css class to hidden
        var listContainer = document.getElementById('listContainer');
        listContainer.setAttribute('class', 'hideList');
    }
};

module.exports = listBuilder;
