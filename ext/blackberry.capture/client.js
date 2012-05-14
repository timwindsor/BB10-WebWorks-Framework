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

var _self = {}, 
    _ID = "blackberry.capture";

_self.captureImage = function (captureSuccess, captureError, options) {
	var eventId = parseInt(Math.floor((Math.random() * 1000000) + 1), 10),
		args = {};

	window.webworks.event.once(_ID, eventId, function (data) {
		if (data.code >= 0) {
			captureSuccess(data.mediaFiles);
		} else {
			captureError(data);
		}
	});

	return window.webworks.execAsync(_ID, "captureImage", args);
};

module.exports = _self;
