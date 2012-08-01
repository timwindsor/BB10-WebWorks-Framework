#WebWorks Extensions

This document will describe:
1. How to setup a native extension project in eclipse.
2. The steps needed to implement a JNext extension on the native side.

##How to create an Extension Project with the Native SDK

1. Open the Momentics IDE. Navigate to the workbench and from the program menu select File -> New -> BlackBerry C/C++ Project.
2. Enter a name for your project in the window that appears and click next.
3. In the following window, choose C++ for the language, Managed Build for the Build Style and an empty shared library project for the project type. When you're done click next.
4. Select the active configuration you want to use for this project. If you want to use this extension in other projects then check the option to create a folder for public headers.
5. If you wish to use a Native SDK that is different from the workspace SDK then uncheck the option to use the workspace SDK selelction and select a different SDK. When you are done, click Finish. You should see your new project appear in the Project Explorer window.
6. In order to build an extension you'll need to import [import an Extension Template Project](#import) into your new project. 

##<a name="import">How to import the Extension Template Project</a>
1. Right click on your project and select the import menu item. In the window that appears select the arhive file as an import source and click next.
2. The next window will prompt you to provide a path to the archive file. The name of the file is ExtensionTemplateProject.zip and it is located in the template folder of your WebWorks Installation Folder.
3. Make sure the option to overwrite existing resources without warning is unchecked. Click finish. A common folder should now appear in the under your project.

##How to implement a JNext extension on the native side

The native and javascript portion of a Webworks extension communicate with each other through the use of an interface provided by JNext. The native interface for the JNext extension can be viewed in the plugin header file located in the common folder of your project. It also contains constants and utility functions that can be used in your native code.

Each native extension must implement the following callback functions:

extern char* onGetObjList( void );
extern JSExt* onCreateObject( const string& strClassName, const string& strObjId );

The onGetObjList returns an array of objects supported by this shared library. When you instantiate an object on the JavaScript side, JNext uses this method to determine if your shared liberary contains this object. If it does, then JNext will call the onCreateObject method and expect it to return a native object corresponding to the JavaScript object.

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

