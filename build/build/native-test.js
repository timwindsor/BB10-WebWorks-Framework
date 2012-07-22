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
var childProcess = require("child_process"),
    utils = require("./utils"),
    fs = require("fs"),
    path = require("path"),
    jWorkflow = require("jWorkflow"),
    _c = require("./conf");
// scp ~/.ssh/id_rsa.pub root@169.254.0.1:/root/.ssh/authorized_keys2

function _getCmd(ext, device, ip) {
    var cmd = "",
        deviceDir,
        simDir,
        testDir,
        SH_CMD = "bash ";

    //unit tests directories
    deviceDir = path.join(_c.EXT, ext, "device/unitTests/test");
    simDir = path.join(_c.EXT, ext, "simulator/unitTests/test");

    cmd = utils.isWindows() ? SH_CMD : "";
    testDir = device === "device" ? deviceDir : simDir;

    //If the folder exists, grab the test
    if (path.existsSync(testDir)) {
        console.log("Running Unit tests for " + ext);
        cmd += " " +
            "scp " + testDir  + " root@" + ip + ":/tmp/test && " +
            "ssh root@" + ip + " 'cd ../tmp; chmod 755 test; ./test'";
    }

    return cmd;
}

function createCmd(ext, device, ip) {
    return function (prev, baton) {
        baton.take();
        childProcess.exec(_getCmd(ext, device, ip), function (error, stdout, stderr) {
            if (stdout !== "") {
                console.log(stdout);
            }
            if (stderr !== "") {
                console.log(stderr);
            }

            if (error) {
                baton.drop(error.code);
            } else {
                baton.pass(prev);
            }
        });
    };
}

module.exports = function (prev, baton) {
    var build = jWorkflow.order(),
        i,
        thisBaton = baton,
        exts = fs.readdirSync(_c.EXT),
        args = Array.prototype.slice.call(prev),
        device = args[0][0] === "simulator" ? "simulator" : "device",
        ip = utils.isValidIPAddress(args[0][1]) ? args[0][1] : "169.254.0.1",
        omitList = args[0] || [];

    thisBaton.take();
    for (i = 0; i < exts.length; i++) {
        if (!utils.arrayContains(omitList, exts[i])) {
            build = build.andThen(createCmd(exts[i], device, ip));
        }
    }

    //catch the success case
    build = build.andThen(function () {
        thisBaton.pass();
    });

    //catch the error case
    build.start(function (error) {
        if (error) {
            thisBaton.drop(error);
        }
    });
};
