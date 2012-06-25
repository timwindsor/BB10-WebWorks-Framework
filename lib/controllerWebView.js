var controllerWebView;

controllerWebView = {
    init: function (config) {
        var controller = window.qnx.webplatform.getController();
        controller.enableWebInspector = config.debugEnabled;
        controller.enableCrossSiteXHR = true;
        controller.visible = false;
        controller.active = false;
        controller.setGeometry(0, 0, screen.width, screen.height);
        controller.publishRemoteFunction('webview.setSensitivity', function (sensitivityType) {
            qnx.callExtensionMethod('webview.setSensitivity', 3, sensitivityType);
        });
        controller.publishRemoteFunction('webview.handleContextMenuResponse', function (menuAction) {
            qnx.callExtensionMethod('webview.handleContextMenuResponse', 2, menuAction);
        });

    }
};

module.exports = controllerWebView;
