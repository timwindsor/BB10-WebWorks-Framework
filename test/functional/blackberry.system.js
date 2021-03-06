/*
 * Copyright 2011-2012 Research In Motion Limited.
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

function testSystemValue(field, value) {
    expect(blackberry.system[field]).toBeDefined();
    expect(blackberry.system[field]).toEqual(value);
}

function testSystemReadOnly(field) {
    var before = blackberry.system[field];
    blackberry.system[field] = "MODIFIED";
    expect(blackberry.system[field]).toEqual(before);
}

describe("blackberry.system", function () {
    var waitForTimeout = 15000;
    
    it("blackberry.system should exist", function () {
        expect(blackberry.system).toBeDefined();
    });

    it("blackberry.system ALLOW and DENY should be defined", function () {
        testSystemValue("ALLOW", 0);
        testSystemValue("DENY", 1);
    });

    it("blackberry.system ALLOW and DENY should be read-only", function () {
        testSystemReadOnly("ALLOW");
        testSystemReadOnly("DENY");
    });

    it("blackberry.system.hasPermission should exist", function () {
        expect(blackberry.system.hasPermission).toBeDefined();
    });

    it("blackberry.system.hasCapability should exist", function () {
        expect(blackberry.system.hasCapability).toBeDefined();
    });

    it("blackberry.system.hasPermission should return true for module in whitelist", function () {
        expect(blackberry.system.hasPermission("blackberry.system")).toBe(blackberry.system.ALLOW);
    });

    it("blackberry.system.hasPermission should return false for module not in whitelist", function () {
        expect(blackberry.system.hasPermission("blackberry.system.event")).toBe(blackberry.system.DENY);
    });

    it("blackberry.system.hasCapability should return true for wifi", function () {
        expect(blackberry.system.hasCapability("network.wlan")).toBeTruthy();
    });

    it("blackberry.system.hasCapability should return false for unknown capability", function () {
        expect(blackberry.system.hasCapability("bake.cookies")).toBeFalsy();
    });

    describe("EventListener", function () {

        afterEach(function () {
            window.confirm("Unplug unit from power source OR make sure ChargingState is 'NC'");
        });

        it("should not be called when the EventListener is removed", function () {
            var onBatteryStatusChange = jasmine.createSpy("onBatteryStatusChange");
                
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);

            window.confirm("[Device]Connect to a power source. [SIM]Change 'ChargingState' to 'DC' in charger PPS Object");

            waitsFor(function () { 
                return (onBatteryStatusChange.callCount === 0); 
            }, "Battery event fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange).not.toHaveBeenCalled();
            });
        });

        it("should call two eventListeners listening to the same event", function () {
            var onBatteryStatusChangeA = jasmine.createSpy("onBatteryStatusChangeA"),
                onBatteryStatusChangeB = jasmine.createSpy("onBatteryStatusChangeB");
                
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChangeA);
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChangeB);

            window.confirm("[Device]Connect to a power source. [SIM]Change 'ChargingState' to 'DC' in charger PPS Object");

            waitsFor(function () { 
                return onBatteryStatusChangeA.callCount && onBatteryStatusChangeB.callCount; 
            }, "Battery event fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChangeA).toHaveBeenCalled();
                expect(onBatteryStatusChangeB).toHaveBeenCalled();
                blackberry.event.removeEventListener('batterystatus', onBatteryStatusChangeA);
                blackberry.event.removeEventListener('batterystatus', onBatteryStatusChangeB);
            });
        });

        it("should fire event once even if same eventListeners called multiple times", function () {
            var onBatteryStatusChange = jasmine.createSpy("onBatteryStatusChange");
                
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);

            window.confirm("[Device]Connect to a power source. [SIM]Change 'ChargingState' to 'DC' in charger PPS Object");

            waitsFor(function () { 
                return onBatteryStatusChange.callCount; 
            }, "Battery event fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange.callCount).toEqual(1);
                blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            }); 
        });

        it("should fire event even when re-register it 3 times and more", function () {
            var onBatteryStatusChange = jasmine.createSpy("onBatteryStatusChange");
                
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);

            window.confirm("[Device]Connect to a power source. [SIM]Change 'ChargingState' to 'DC' in charger PPS Object");

            waitsFor(function () { 
                return onBatteryStatusChange.callCount; 
            }, "Battery event fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange.callCount).toEqual(1);
                blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            }); 
        });
    });


    describe("batterystatus", function () {
        var onBatteryStatusChange;

        beforeEach(function () {
            onBatteryStatusChange = jasmine.createSpy("onBatteryStatusChange");
            blackberry.event.addEventListener('batterystatus', onBatteryStatusChange);
        });

        afterEach(function () {
            blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            onBatteryStatusChange = null;
        });

        it("should be called when the battery level decreases", function () {
            window.confirm("[Device]Drain the battery. [SIM]Decrease the 'StateOfCharge' in battery PPS Object");

            waitsFor(function () { 
                return onBatteryStatusChange.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange).toHaveBeenCalled();
                expect(onBatteryStatusChange.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryStatusChange.mostRecentCall.args[0].isPlugged).toBeFalsy();
            });
        });

        it("should be called when the power source is connected", function () {
            window.confirm("[Device]Connect to a power source. [SIM]Change 'ChargingState' to 'DC' in charger PPS Object");

            waitsFor(function () { 
                return onBatteryStatusChange.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange).toHaveBeenCalled();
                expect(onBatteryStatusChange.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryStatusChange.mostRecentCall.args[0].isPlugged).toBeTruthy();
                blackberry.event.removeEventListener('batterystatus', onBatteryStatusChange);
            });
        });

        it("should be called when the battery level increases", function () {
            window.confirm("[Device]Please wait for battery to increase charge by 1%. [SIM]Increase the 'StateOfCharge' in battery PPS Object");

            waitsFor(function () { 
                return onBatteryStatusChange.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange).toHaveBeenCalled();
                expect(onBatteryStatusChange.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryStatusChange.mostRecentCall.args[0].isPlugged).toBeTruthy();
            });
        });

        it("should be called when the power source is disconnected", function () {
            window.confirm("[Device]Please unplug the power source. [SIM]Change 'ChargingState' to 'NC' in charger PPS Object");

            waitsFor(function () {
                return onBatteryStatusChange.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryStatusChange).toHaveBeenCalled();
                expect(onBatteryStatusChange.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryStatusChange.mostRecentCall.args[0].isPlugged).toBeFalsy();
            });
        });
    });
    
    describe("batterylow", function () {
        var onBatteryLow;

        beforeEach(function () {
            onBatteryLow = jasmine.createSpy("onBatteryLow");
            blackberry.event.addEventListener('batterylow', onBatteryLow);
        });

        afterEach(function () {
            blackberry.event.removeEventListener('batterylow', onBatteryLow);
            onBatteryLow = null;
        });

        it("should be called when the battery level decreases to lower than 15%", function () {
            window.confirm("[Device]Drain the battery to level lower than 15%. [SIM]Set 'StateOfCharge' in battery PPS Object to level lower than 15%");

            waitsFor(function () { 
                return onBatteryLow.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryLow).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onBatteryLow.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryLow.mostRecentCall.args[0].isPlugged).toBeDefined();
            });
        });
        
        it("should not be called when the battery level decreases but still higher or equal to 15%", function () {
            window.confirm("[Device]Drain the battery to level higher or equal to 15%. [SIM]Set 'StateOfCharge' in battery PPS Object to level higher or equal to 15%");

            waitsFor(function () { 
                return (onBatteryLow.callCount === 0);
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryLow).not.toHaveBeenCalled();
            });
        });

        it("should not be called to any change to power source state", function () {
            window.confirm("[Device]Change the power source state (Plug/Unplug). [SIM]Tweak 'ChargingState' to 'NC' or 'DC' in charger PPS Object");

            waitsFor(function () { 
                return (onBatteryLow.callCount === 0);
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryLow).not.toHaveBeenCalled();
            });
        });
        
        it("should trigger callback for low event when re-entering threshold after adding event lister after all were removed", function () {

            window.confirm("[Device]Drain the battery to level lower than 15%. [SIM]Set 'StateOfCharge' in battery PPS Object to level lower than 15%");
            blackberry.event.removeEventListener('batterylow', onBatteryLow);
            window.confirm("[Device]Charge the battery to level higher or equal to 15%. [SIM]Set 'StateOfCharge' in battery PPS Object to level higher or equal to 15");
            window.confirm("[Device]Drain the battery to level lower than 15%. [SIM]Set 'StateOfCharge' in battery PPS Object to level lower than 15%");
            blackberry.event.addEventListener('batterylow', onBatteryLow);

            waitsFor(function () { 
                return onBatteryLow.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryLow).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onBatteryLow.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryLow.mostRecentCall.args[0].isPlugged).toBeDefined();
            });
        });
        

    });   
    
    describe("batterycritical", function () {
        var onBatteryCritical;

        beforeEach(function () {
            onBatteryCritical = jasmine.createSpy("onBatteryCritical");
            blackberry.event.addEventListener('batterycritical', onBatteryCritical);
        });

        afterEach(function () {
            blackberry.event.removeEventListener('batterycritical', onBatteryCritical);
            onBatteryCritical = null;
        });

        it("should be called when the battery level decreases to lower than 5%", function () {
            window.confirm("[Device]Drain the battery to level lower than 5%. [SIM]Set 'StateOfCharge' in battery PPS Object to level lower than 5%");

            waitsFor(function () { 
                return onBatteryCritical.callCount;
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryCritical).toHaveBeenCalledWith(jasmine.any(Object));
                expect(onBatteryCritical.mostRecentCall.args[0].level).toBeDefined();
                expect(onBatteryCritical.mostRecentCall.args[0].isPlugged).toBeDefined();
            });
        });
        
        it("should not be called when the battery level decreases but still higher or equal to 5%", function () {
            window.confirm("[Device]Drain the battery to level higher or equal to 5%. [SIM]Set 'StateOfCharge' in battery PPS Object to level higher or equal to 5%");

            waitsFor(function () { 
                return (onBatteryCritical.callCount === 0);
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryCritical).not.toHaveBeenCalled();
            });
        });
        
        it("should not be called to any change to power source state", function () {
            window.confirm("[Device]Change the power source state (Plug/Unplug). [SIM]Tweak 'ChargingState' to 'NC' or 'DC' in charger PPS Object");

            waitsFor(function () { 
                return (onBatteryCritical.callCount === 0);
            }, "event never fired", waitForTimeout);

            runs(function () {
                expect(onBatteryCritical).not.toHaveBeenCalled();
            });
        });
    });

    describe("device properties", function () {
        it('blackberry.system.hardwareId should exist', function () {
            testSystemValue("hardwareId");
        });

        it('blackberry.system.hardwareId should be read-only', function () {
            testSystemReadOnly("hardwareId");
        });

        it('blackberry.system.softwareVersion should exist', function () {
            testSystemValue("softwareVersion");
        });

        it('blackberry.system.softwareVersion should be read-only', function () {
            testSystemReadOnly("softwareVersion");
        });
    });
});
