var exception = require('./exception'),
    Whitelist = require('./policy/whitelist').Whitelist,
    request,
    webview,
    ACCEPT_RESPONSE = {setAction: "ACCEPT"},
    DENY_RESPONSE = {setAction: "DENY"},
    SUBSTITUTE_RESPONSE = {setAction: "SUBSTITUTE"};
                    

function _formMessage(url, origin, sid) {
    var tokens = url.split("blackberry/")[1].split("/");

    // Url format: http://localhost:8472/blackberry/service/action/ext/method?args
    return {
        request : {
            params : {
                service : tokens[0],
                action : tokens[1],
                ext : tokens[2],
                method : tokens[3] && tokens[3].indexOf("?") >= 0 ? tokens[3].split("?")[0] : tokens[3],
                args : tokens[3] && tokens[3].indexOf("?") >= 0 ? tokens[3].split("?")[1] : null
            },
            //TODO: Something for the body
            body : undefined,
            origin : origin
        },
        response : {
            send : function (code, data) {
                var responseText;
                if (typeof(data) === 'string') {
                    responseText = data;
                } else {
                    responseText = JSON.stringify(data);
                }

                webview.notifyOpen(sid, code, "OK");
                webview.notifyHeaderReceived(sid, "Access-Control-Allow-Origin", "*");
                webview.notifyDataReceived(sid, responseText, responseText.length);
                webview.notifyDone(sid);
            }
        }
    };
}

function networkResourceRequestedHandler(value) {
    var obj = JSON.parse(value),
        url = obj.url,
        whitelist = new Whitelist(),
        server,
        message,
        response;

    if (url.match("^http://localhost:8472/blackberry/")) {
        server = require("./server");
        message = _formMessage(url, webview.originalLocation, obj.sid);
        response = SUBSTITUTE_RESPONSE;
        server.handle(message.request, message.response);
    } else {
        if (whitelist.isAccessAllowed(url)) {
            response = ACCEPT_RESPONSE;
        } else {
            response = DENY_RESPONSE;
            webview.executeJavascript("alert('Access to \"" + url + "\" not allowed')");
        }
    }
    return JSON.stringify(response);
}

module.exports = {
    init : function (args) {
        webview = args.webview;
        return {
            networkResourceRequestedHandler: networkResourceRequestedHandler
        };
    }
};
