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
        
        controller.publishRemoteFunction('invocation.queryTargets', function (value, callback) {
            console.log("ControllerWebView: " + value);
            var request = {
                    action: 'bb.action.SHARE',
                    type: JSON.parse(value)[0],
                    target_type: invocation.TARGET_TYPE_ALL,
                    action_type: invocation.ACTION_TYPE_MENU
                };
            invocation.queryTargets(request, function (error, results) {
                console.log("error: " + error);
                callback(results);
            });
        });
    }
};

module.exports = controllerWebView;
