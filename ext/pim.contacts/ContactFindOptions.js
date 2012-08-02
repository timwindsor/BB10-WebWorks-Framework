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

/**
 * ContactFindOptions.
 * @constructor
 * @param filter search fields
 * @param sort sort fields and order
 * @param limit max number of contacts to return
 * @param favorite if set, only favorite contacts will be returned
 */

var ContactFindOptions = function (filter, sort, limit, favorite) {
    this.filter = filter || null;
    this.sort = sort || null;
    this.limit = limit || -1; // -1 for returning all results
    this.favorite = favorite || false;
};

ContactFindOptions.SEARCH_FIELD_GIVEN_NAME = 0;
ContactFindOptions.SEARCH_FIELD_FAMILY_NAME = 1;
ContactFindOptions.SEARCH_FIELD_ORGANIZATION_NAME = 2;
ContactFindOptions.SEARCH_FIELD_PHONE = 3;
ContactFindOptions.SEARCH_FIELD_EMAIL = 4;
ContactFindOptions.SEARCH_FIELD_BBMPIN = 5;
ContactFindOptions.SEARCH_FIELD_LINKEDIN = 6;
ContactFindOptions.SEARCH_FIELD_TWITTER = 7;
ContactFindOptions.SEARCH_FIELD_VIDEO_CHAT = 8;

ContactFindOptions.SORT_FIELD_GIVEN_NAME = 0;
ContactFindOptions.SORT_FIELD_FAMILY_NAME = 1;
ContactFindOptions.SORT_FIELD_ORGANIZATION_NAME = 2;

module.exports = ContactFindOptions;