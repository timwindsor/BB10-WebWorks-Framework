/*
* Copyright 2012 Research In Motion Limited.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <string>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory_js.hpp"

using namespace std;

/**
 * Default constructor. Initializes the thread that can be used to get memory
 * usage.
 */
Memory::Memory() {
	m_thread = 0;
}

/**
 * Memory destructor. Cleans up the thread.
 */
Memory::~Memory() {
	fprintf(stderr, "native side: Memory Object deleted\n");
	Memory::StopThread();
}

/**
 * Method used by JNext to determine if the object can be deleted.
 */
bool Memory::CanDelete() {
	return true;
}

/**
 * Method that retreives the current amount of free memory in the system.
 */
long getMemory() {
	struct stat statbuf;
	paddr_t freemem;

	stat("/proc", &statbuf);
	freemem = (paddr_t)statbuf.st_size;

	return freemem;
}

/**
 * Utility function to convert a long into a string.
 */
string convertLongToString(long l) {
	stringstream ss;
	ss << l;
	return ss.str();
}

/**
 * Method used by the getMemoryUsage thread to pass the amount of free memory
 * on the JavaScript side by firing an event.
 */
void Memory::SendJNextEvent(long fm) {
	fprintf(stderr, "native side: thread sending JNext Event started\n");
    std::string eventString = "FreeMemory: " + convertLongToString(fm);
    fprintf(stderr, "native side: memory usage %s\n", eventString.c_str());
    if (SendPluginEvent == NULL) {
    	fprintf(stderr, "native side: SendPluginEvent is null\n");
    } else {
    	fprintf(stderr, "native side: SendPluginEvent is GOOD\n");
    }
	//SendPluginEvent("abcde", m_pContext);
	fprintf(stderr, "native side: thread sending JNext Event done\n");
}

/**
 * Thread that retrieves the current amount of free memory every second and
 * sends it to the JavaScript side using an event. The thread shall continue
 * to retrieve the memory usage until the native object is destroyed on the
 * JavaScript side.
 */
void* MemoryThread(void* parent)
{
	fprintf(stderr, "native side: thread init started\n");
	Memory *pParent = static_cast<Memory *>(parent);

	while (true) {
		long fm = getMemory();
		pParent->SendJNextEvent(fm);
		sleep(1);
	}

	fprintf(stderr, "native side: thread init done\n");
    return NULL;
}

/**
 * Method responsible for starting the thread to get memory usage. Only one
 * thread can be created per native JavaScript instance. This method returns
 * true if the thread was created successfully and false othrewise.
 */
bool Memory::StartThread() {
	if (!m_thread) {
		pthread_attr_t thread_attr;
		pthread_attr_init(&thread_attr);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

		pthread_create(&m_thread, &thread_attr, MemoryThread, static_cast<void *>(this));
		pthread_attr_destroy(&thread_attr);
		return true;
	} else {
		return false;
	}
}

/**
 * Method responsible for terminating a thread.
 */
void Memory::StopThread() {
	m_thread = 0;
}

/**
 * Method used to start the get memory usage thread. The method shall return a
 * string to the JavaScript side indicating whether or not the memory
 * monitoring was initialized.
 */
string Memory::monitorMemoryNative() {
	if (Memory::StartThread()) {
		return "Memory Monitoring Initialized";
	} else {
		return "Memory already being monitored";
	}
}

/**
 * It will be called from JNext JavaScript side with passed string.
 * This method implements the interface for the JavaScript to native binding
 * for invoking native code. This method is triggered when JNext.invoke is
 * called on the JavaScript side with this native objects id.
 */
string Memory::InvokeMethod(const string& command) {
	// Get the method name string
	if (SendPluginEvent == NULL) {
		fprintf(stderr, "native side: SendPluginEvent is null\n");
	} else {
		fprintf(stderr, "native side: SendPluginEvent is GOOD\n");
	}

    int index = command.find_first_of(" ");
	string strCommand = command.substr(0, index);
	fprintf(stderr, "native side: %s\n", strCommand.c_str());

	// Determine which function should be executed
	if (strCommand == "getMemoryNative") {
		return convertLongToString(getMemory());
	} else if (strCommand == "monitorMemoryNative") {
		return monitorMemoryNative();
	} else {
		return "Unsupported Method";
	}
}

/**
 * This method returns the list of objects implemented by this native
 * extension.
 */
char* onGetObjList() {
	static char name[] = "Memory";
	return name;
}

/**
 * This method is used by JNext to instantiate the Memory object when
 * an object is created on the JavaScript server side.
 */
JSExt* onCreateObject(const string& className, const string& id) {
	if (className == "Memory") {
		return new Memory();
	}

	return NULL;
}




