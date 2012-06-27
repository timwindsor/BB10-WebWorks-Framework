describe("Overlay Webview", function () {
    var webview,
        libPath = "./../../../", 
        mockedController,
        mockedWebview,
        mockedApplication;

    beforeEach(function () {
        webview = require(libPath + "lib/overlayWebView");
        mockedWebview = {
            id: 42,
            enableCrossSiteXHR: undefined,
            visible: undefined,
            active: undefined,
            zOrder: undefined,
            url: undefined,
            setGeometry: jasmine.createSpy(),
            onNetworkResourceRequested: undefined,
            destroy: jasmine.createSpy(),
            executeJavaScript: jasmine.createSpy(),
            windowGroup: undefined,
            addEventListener: jasmine.createSpy(),
            enableWebEventRedirect: jasmine.createSpy()
        };
        mockedApplication = {
            windowVisible: undefined
        };
        GLOBAL.qnx = {
            callExtensionMethod: jasmine.createSpy(),
            webplatform: {
                getController: function () {
                    return mockedController;
                },
                createWebView: function (createFunction) {
                    //process.nextTick(createFunction);
                    //setTimeout(createFunction,0);
                    runs(createFunction);
                    return mockedWebview;
                },
                getApplication: function () {
                    return mockedApplication;
                }
            }
        };
        GLOBAL.window = {
            qnx: qnx
        };
        GLOBAL.screen = {
            width : 1024,
            height: 768
        };
    });

    describe("create", function () {
        it("sets up the visible webview", function () {
            webview.create();
            waits(1);
            runs(function () {
                expect(mockedWebview.enableCrossSiteXHR).toEqual(true);
                expect(mockedWebview.visible).toEqual(true);
                expect(mockedWebview.active).toEqual(true);
                expect(mockedWebview.zOrder).toEqual(1);
                expect(mockedWebview.setGeometry).toHaveBeenCalledWith(0, 0, screen.width, screen.height);

                expect(mockedApplication.windowVisible).toEqual(true);

                expect(mockedWebview.addEventListener).toHaveBeenCalledWith("ContextMenuRequestEvent", jasmine.any(Function));
                expect(mockedWebview.backgroundColor).toEqual("0x00FFFFFF");
                expect(mockedWebview.sensitivity).toEqual("SensitivityTest");
            });
        });

        it("calls the ready function", function () {
            var chuck = jasmine.createSpy();
            webview.create(chuck);
            waits(1);
            runs(function () {
                expect(chuck).toHaveBeenCalled();
            });
        });

    });

    describe("methods other than create", function () {

        it("calls the underlying destroy", function () {
            webview.create(mockedWebview);
            webview.destroy();
            expect(mockedWebview.destroy).toHaveBeenCalled();
        });
        
        it("sets the url property", function () {
            var url = "http://AWESOMESAUCE.com";
            webview.create(mockedWebview);
            webview.setURL(url);
            expect(mockedWebview.url).toEqual(url);
        });
        
        it("calls the underlying executeJavaScript", function () {
            var js = "var awesome='Jasmine BDD'";
            webview.create(mockedWebview);
            webview.executeJavascript(js);
            expect(mockedWebview.executeJavaScript).toHaveBeenCalledWith(js);
        });
        it("calls the underlying windowGroup property", function () {
            webview.create(mockedWebview);
            expect(webview.windowGroup()).toEqual(mockedWebview.windowGroup);
        });
    });

});
