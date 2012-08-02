# Extension Template - Memory Example

This document contains to parts: [Native](#native) and [JavaScript](#JavaScript)

## <a name="native">Native Part - Overview</a>

This part of the document will describe:
1. How to setup a native extension project in eclipse.
2. The steps needed to implement a JNext extension on the native side.

##How to create an Extension Project with the Native SDK

1. Open the Momentics IDE. Navigate to the workbench and from the program menu select File -> New -> BlackBerry C/C++ Project.
2. Enter a name for your project in the window that appears and click next.
3. In the following window, choose C++ for the language, Managed Build for the Build Style and an empty shared library project for the project type. When you're done click next.
4. Select the active configuration you want to use for this project. If you want to use this extension in other projects then check the option to create a folder for public headers.
5. If you wish to use a Native SDK that is different from the workspace SDK then uncheck the option to use the workspace SDK selection and select a different SDK. When you are done, click Finish. You should see your new project appear in the Project Explorer window.
6. In order to build an extension you'll need to import [import an Extension Template Project](#import) into your new project. 

##<a name="import">How to import the Extension Template Project</a>
1. Right click on your project and select the import menu item. In the window that appears select the archive file as an import source and click next.
2. The next window will prompt you to provide a path to the archive file. The name of the file is ExtensionTemplateProject.zip and it is located in the template folder of your WebWorks Installation Folder.
3. Make sure the option to overwrite existing resources without warning is unchecked. Click finish. A common folder should now appear in the under your project.

##How to implement a JNext extension on the native side

The native and JavaScript portion of a WebWorks extension communicate with each other through the use of an interface provided by JNext. The native interface for the JNext extension can be viewed in the plug-in header file located in the common folder of your project. It also contains constants and utility functions that can be used in your native code.

Each native extension must implement the following callback functions:

    extern char* onGetObjList( void );
    extern JSExt* onCreateObject( const string& strClassName, const string& strObjId );

The onGetObjList returns an array of objects supported by this shared library. When you instantiate an object on the JavaScript side, JNext uses this method to determine if your shared library contains this object. If it does, then JNext will call the onCreateObject method and expect it to return a native object corresponding to the JavaScript object.

The rest of the interface depends on whether or not you implement the C or C++ interface. Both interfaces come in two parts namely the ability of the JavaScript to trigger native code through method calls and the native code being able to trigger JavaScript code through the use of events.

###The C Interface

You'll need to implement the following interface:

    typedef void (*SendPluginEv)( const char* szEvent, void* pContext );
    char* SetEventFunc(SendPluginEv funcPtr);
    char* InvokeFunction( const char* szCommand, void* pContext );

The InvokeFunction handles the calling of native code from JavaScript. When the JavaScript side calls the native side it passes a space delimited string that the native InvokeFunction uses to determine which action to perform. The return value of the string is passed back to the JavaScript side as a return value.

The native code can trigger a method on the JavaScript side by defining a method that conforms to the SendPluginEv signature and then registers that function as an event function using the set event function e.g:


    void triggerJSEvent( const char* szEvent, void* pContext) {
        //Your code goes here
    }

    SetEventFunc(triggerJSEvent);

###The C++ Interface

You'll need to implement the following class:

    class JSExt
    {
    public:
        virtual ~JSExt() {};
        virtual string InvokeMethod( const string& strCommand ) = 0;
        virtual bool CanDelete( void ) = 0;
        virtual void TryDelete( void ) {}
    public:
        void* m_pContext;
    };

The MemoryExtension sample project implements the JSExt class which is the C++ interface to JNext.


## <a name="JavaScript">JavaScript Part - Overview</a>
Under _example.memory_ folder there are following JavaScript files.

* __client.js__ - Considered to be a client side, exports APIs that are accessible from the client's application. __client.js__ file name is mandatory.
* __index.js__ - Considered to be a server side, exports APIs that are accessible from the _client.js_ and able to communicate with _native_ side. __index.js_ file name is mandatory.
* __manifest.json__ - Descriptor defines Whitelisting flag, ID and Dependencies for the extension.

__example.memory__ is the extension ID as it defined in __manifest.js__ and serves as a prefix to all methods and fields define in the __client.js__

__example.memory__._getMemory_ - Call this method to get free memory bytes
__example.memory__._monitorMemory_ - Call this method to register and be notified on memory events.

####See sample code below:

```javascript

    function memoryUsageCallback(memoryInByte) {
        alert("Event Callback. Free memory: " + memoryInByte);
    }

    var freeMemory = example.memory.getMemory();
    alert("Free memory: " + freeMemory);

    example.memory.monitorMemory(memoryUsageCallback);
```

## JavaScript Part - In Depth
The interaction between __client.js__ and __index.js__ is made by using APIs of __webworks__ object.

The object __webworks__ has a mapping mechanism to communicate between client and server by calling corresponding methods and by storing the callbacks provided in request by client side (__client.js__) and later to call those callback and to pass parameters to them when response is ready on server side (__index.js__).

Each method in __client.js__ when making a call to server side should provide ID, method name and arguments to _execSync_ to get a synchronized call or to _execAsync_ when the request is asynchronic. As a result a corresponding method in __index.js__ is invoked.

When user is interested to get memory events through a callback, to allow __webworks__ object to properly map this callback __client.js__ in addition to callback itself should pass __example.memory.memoryEvent__ as an _eventID_ which will be used as a mapping key by __webworks__. On the other side when server want to trigger the callback provided by user it will provide exactly the same _eventID_.

```javascript

    //Client side code
    window.webworks.event.once(_ID, "example.memory.memoryEvent", cb);

    //Server side code
    _event.trigger("example.memory.memoryEvent", arg);
```

### JNEXT Interface
The interaction between __index.js__ and the _native_ side is made by using APIs of JNEXT interface. To accomplish this a constructor function _MemoryJNext_ is attached to __JNEXT__. The object is expected to have a following structure:

 * _onEvent_ - Events from the _native_ side are passed as a __String__ to this method. _onEvent_ is a mandatory name.
 * _init_ - It performs several key operations:
     1. Requires the module exported by _native_ side - JNEXT.require("memoryJnext")
     2. Creates an object using acquired module and save an _id_ return by the call - _self.&#95;id_ =    JNEXT.createObject("memoryJnext.Memory").
     3. Register itself to be notified on events so _onEvent_ will be called - JNEXT.registerEvents(_self).
 * _getId_ - Returns the _id_ previously saved - return _self.&#95;id_.
 * Methods and fields used by __index.js__ to redirect, when required, calls that initially came from __client.js__ to the _native_ side. This example has two: _getMemoryJNext_ and _monitorMemoryJNext_

__index.js__ create _MemoryJNext_ object and save its instance in variable, then later when one of the methods in __index.js__ is invoked it redirect the call to corresponding _MemoryJNext_ method.

```javascript

    //Code in user's app.
    var freeMemory = example.memory.getMemory();

    //Code in user app trigger this code in client.js
    return window.webworks.execSync(_ID, "getMemoryServer", null);

    //Code in client.js trigger this code in index.js
    getMemoryServer: function (success, fail, args, env) {
        ....
    }

    //Finally in index.js code that belongs to MemoryJNext object is triggered where
    //call to native side takes place and returned value is passed back to the user.
    _self.getMemoryJNext = function () {
        return JNEXT.invoke(_self._id, "getMemoryNative");
    };
```
