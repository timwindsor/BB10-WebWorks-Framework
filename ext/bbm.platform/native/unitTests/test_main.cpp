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

#include <gtest/gtest.h>
#include <bps/event.h>
#include "../bbm_bps.hpp"

TEST(Connection, CanStopEvents) {
    webworks::BBMBPS *bbm = new webworks::BBMBPS();
    bbm->InitializeEvents();

    // Send an end event and see if bps_get_event receives it
    bbm->SendEndEvent();
    bps_event_t *event = NULL;
    bps_get_event(&event, 1000);
    delete bbm;

    // Use assert so that the next test does not run if this one fails
    ASSERT_TRUE(event != NULL);
}

TEST(Connection, CanGetEvents) {
    webworks::BBMBPS *bbm = new webworks::BBMBPS();
    bbm->InitializeEvents();

    bbm->SendEndEvent();
    EXPECT_EQ(0, bbm->WaitForEvents());

    delete bbm;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
