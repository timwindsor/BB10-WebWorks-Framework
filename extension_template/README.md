# WebWorks Extension Example (Memory Extension)

This document describes how to generate a WebWorks Extension.
The implementation is split into two parts: [Native](#native) and [JavaScript](#JavaScript)

The extension template folder includes the implementation of an example
extension that retreives the current amount of free memory on the device.

The resources in the extension template folder include:

* TestWidget/ - Resources for a WebWorks application that uses the Memory
Extension to get the amount of free memory on the device.
* example.memory/ - JavaScript source files for the example Memory Extension.
* jnext\_src\_example/common/ - JNext source files.
* jnext\_src\_example/src/ - Native source files for the Memory Extension.

The Memory Extension implementation should be used in conjunction with this 
read me file to understand how to create a WebWorks example.

## How to add an WebWorks Extension to your WebWorks Installation

1. Navigate to the  Framework/ext folder of your WebWorks Installation and
create a folder to for your extension.
2. Copy you JavaScript files to the root folder of your extension folder.
3. Create a device and a simulator folder in the root of your extension folder.
4. Build the native portion of your native extension. One shared library for
the device and another for the simulator.
5. Copy the shared library for the device into the device folder.
6. Copy the shared library for the simulator into the simulator folder.

After completing the above steps you should be able to build WebWorks
applications that can use your memory extension.

IMPORTANT: Make sure that your WebWorks applications whitelist the extension
otherwise the application will not be able to use the extension.

### <a name="native">Native Part - Overview</a>

This part of the document will describe:
1. How to setup a native extension project in eclipse.
2. The steps needed to implement a JNEXT extension on the native side.

### How to create an Extension Project with the Native SDK

1. Open the Momentics IDE. Navigate to the workbench and from the program menu
select File -> New -> BlackBerry C/C++ Project.
2. Enter a name for your project in the window that appears and click next.
3. In the following window, choose C++ for the language, Managed Build for the
Build Style and an empty shared library project for the project type.
When you're done click next.
4. Select the active configuration you want to use for this project then click
next. 
5. If you wish to use a Native SDK that is different from the workspace SDK then
uncheck the option to use the workspace SDK selelction and select a different
SDK. When you are done, click Finish. You should see your new project appear in
the Project Explorer window.
6. In order to build an extension you'll need to import
[extension resources](#import) into your new project. 

### <a name="import">How to import extension resources</a>
1. Right click on your project and select the import menu item. In the window
that appears select the file system as an import source and click next.
2. The next window will prompt you to provide a path to a folder. Select the
common and src folders located in the extension\_template/jnext\_ext\_src folder
of your WebWorks Installation folder. Import both the folders and their
contents. Then click finish. Your project should now have a common folder and a
source folder which contains source and header files. After following the
instructions below to implement an extension you should
[build your extension](#buildExtension) for both the device and the simultaor.

### How to implement a JNEXT extension on the native side

The native and javascript portion of a Webworks extension communicate with each
other through the use of an extension framework provided by JNEXT. The native
interface for the JNEXT extension can be viewed in the plugin header file
located in the common folder of your project. It also contains constants and
utility functions that can be used in your native code. Your native extension
must be detrived from JSExt which is defined in plugin.h. Therefore your
extension should include this header file.

The MemoryExtension sample code included in the extension template folder
implements the native extension interface to JNEXT.

Each native extension must implement the following callback functions:

```cpp
extern char* onGetObjList( void );
extern JSExt* onCreateObject( const string& strClassName, const string& strObjId );
```

The onGetObjList function returns a comma separated list of classes supported by
this JNEXT extension. It is used by JNEXT to determine the set of classes that
can be instantiated by this JNEXT extension.

The onCreateObject function is the other callback that must be implemented by
the native JNEXT extension. It takes two parameters. The first parameter is the
name of the class requested to be created from the javascript side. Valid names
are those that are returned in onGetObjList. The second paramter is the unique
object id for the class. This method returns a pointer to the created extension
object.

The native extension must also implement the following class:

```cpp
class JSExt
{
public:
    virtual ~JSExt() {};
    virtual string InvokeMethod( const string& strCommand ) = 0;
    virtual bool CanDelete( void ) = 0;
private:
    std::string m_id;
};
```

The m_id is an attribute contains the JNEXT id for this object. The id is
passed to the class as an argument to the constructor. It is needed to trigger
events on the JavaScript side from native.

The can delete method is used by JNEXT to determine whether your native object
can be deleted. 

The InvokeMethod function is called as a result from a request from JavaScript
to invoke a method of this particular object. The only argument to this
function is a string passed from JavaScript that this method should parsed in
order to determine which method of the native object should be executed.

If you want the native code to be able to trigger an event on the JavaScript
side then you'll need to call the SendPluginEvent function which has the
following signature:

```cpp
void SendPluginEvent( const char* szEvent, void* pContext );
```

The first parameter is a string which a space delimited string consisting of
the m_id (inherited attribute from JSExt) followed by the arguments you wish to
pass to the JavaScript on event function. The second parameter is the
m_pContext (inherited attribute from JSExt).

### <a name="buildExtension">How to build your native Extension</a>

1. Right click your project and select the Clean Project object.
2. Right click your project again and select Build Configurations -> Build Selected... .
3. A window will appear that has shows all the available build configurations
for the project. Depending on the profile you wish to use select the Device and
corresponding Simulator build profiles and click ok.
4. You should see the shared libraries generated in the folders for each Build
Configuration that you selected.

### How to change the name of the build artifact.

1. Right click on your project and selecting properties.
2. Expand the C/C++ Build option and select the Settings option.
3. In the main window select the build configuration you wish to configure and
select the build artifact tab.
4. You should see fields for the type, name, extension and prefix of the
artifact tab which you can modify to change the build artifact that is
generated. When you have completed your changes then select ok.

Please note that each modification of the build artifact corresponds to a
single configuration. If you wish to modify the build artifact of all the build
configurations you will need to modify each build configuration.

## <a name="JavaScript">JavaScript Part - Overview</a>
Under _example.memory_ folder there are following JavaScript files.

* __client.js__ - Considered to be a client side, exports APIs that are
accessible from the client's application. __client.js__ file name is mandatory.
* __index.js__ - Considered to be a server side, exports APIs that are
accessible from the _client.js_ and able to communicate with _native_ side.
__index.js_ file name is mandatory.
* __manifest.json__ - Descriptor defines Whitelisting flag, ID and Dependencies
for the extension.

__example.memory__ is the extension ID as it defined in __manifest.js__ and
serves as a prefix to all methods and fields define in the __client.js__

__example.memory__._getMemory_ - Call this method to get free memory bytes
__example.memory__._monitorMemory_ - Call this method to register and be
notified on memory events.

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
The interaction between __client.js__ and __index.js__ is made by using APIs of
__webworks__ object.

The object __webworks__ has a mapping mechanism to communicate between client
and server by calling corresponding methods and by storing the callbacks
provided in request by client side (__client.js__) and later to call those
callback and to pass parameters to them when response is ready on server side
(__index.js__).

Each method in __client.js__ when making a call to server side should provide
ID, method name and arguments to _execSync_ to get a synchronized call or to
_execAsync_ when the request is asynchronic. As a result a corresponding method
in __index.js__ is invoked.

When user is interested to get memory events through a callback, to allow
__webworks__ object to properly map this callback __client.js__ in addition to
callback itself should pass __example.memory.memoryEvent__ as an _eventID_
which will be used as a mapping key by __webworks__. On the other side when
server want to trigger the callback provided by user it will provide exactly
the same _eventID_.

```javascript

    //Client side code
    window.webworks.event.once(_ID, "example.memory.memoryEvent", cb);

    //Server side code
    _event.trigger("example.memory.memoryEvent", arg);
```

### JNEXT Interface
The interaction between __index.js__ and the _native_ side is made by using
APIs of JNEXT interface. To accomplish this a constructor function
_MemoryJNext_ is attached to __JNEXT__. The object is expected to have a
following structure:

 * _onEvent_ - Events from the _native_ side are passed as a __String__ to
    this method. _onEvent_ is a mandatory name.
 * _init_ - It performs several key operations:
     1. Requires the module exported by _native_ side -
        JNEXT.require("memoryJnext")
     2. Creates an object using acquired module and save an _id_ return by the
        call - _self.&#95;id_ =    JNEXT.createObject("memoryJnext.Memory").
     3. Register itself to be notified on events so _onEvent_ will be called -
        JNEXT.registerEvents(_self).
 * _getId_ - Returns the _id_ previously saved - return _self.&#95;id_.
 * Methods and fields used by __index.js__ to redirect, when required, calls
    that initially came from __client.js__ to the _native_ side. This example
    has two: _getMemoryJNext_ and _monitorMemoryJNext_

__index.js__ create _MemoryJNext_ object and save its instance in variable, then
later when one of the methods in __index.js__ is invoked it redirect the call to
corresponding _MemoryJNext_ method.

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
