var controllerWebView;

controllerWebView = {
    init: function (config) {
        var controller = window.qnx.webplatform.getController(),
            invocation = window.qnx.webplatform.getApplication().invocation;
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

        controller.publishRemoteFunction('webview.loadURL', function (webviewId, url) {
            debugger;
            qnx.callExtensionMethod('webview.loadURL', webviewId, url);
        });

        controller.publishRemoteFunction('webview.downloadURL', function (webviewId, url, title) {
            debugger;
            qnx.callExtensionMethod("webview.downloadUrl", webviewId, url, title, 0);
        });

        controller.publishRemoteFunction('invocation.queryTargets', function (value, callback) {
            console.log("ControllerWebView: " + value);
            var request = {
                    action: 'bb.action.SHARE',
                    type: value[0],
                    target_type: invocation.TARGET_TYPE_ALL,
                    action_type: invocation.ACTION_TYPE_MENU
                };
            invocation.queryTargets(request, function (error, results) {
                console.log("error: " + error);
                callback(results);
            });
        });

        controller.publishRemoteFunction('invocation.invoke', function (value, callback) {
            console.log("ControllerWebView - publish invocation.invoke: " + value);
            var request = value;
            invocation.invoke(request);
        });
    }
};

module.exports = controllerWebView;
