/*
 * Copyright (C) Research In Motion Limited 2012. All rights reserved.
 */

var listBuilder,
    listItems;

function init() {
    listItems = [];
}

listBuilder = {
    init: init,
    populateList: function (itemArray) {
        // create a bunch of subdivs
        // subdivs will have a click handler to run stuff
        // hide menu after handling a click
    },
    show: function () {
        // set css class to visible
    },
    hide: function () {
        // set css class to hidden
    }
};

module.exports = listBuilder;
