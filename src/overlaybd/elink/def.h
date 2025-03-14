/*
   Copyright The Overlaybd Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once

#include <photon/fs/filesystem.h>
#include <string.h>
#include <assert.h>

#include <photon/common/estring.h>
#include <photon/common/string_view.h>
#include <unordered_map>
#include "elink.h"

namespace ELink {




class ICredentialClient {
public:
    virtual std::unordered_map<estring, estring> access_key(std::string_view url) = 0;
    virtual ~ICredentialClient() {};
};

class IAuthPlugin {
public:
    // virtual std::unordered_map<estring, estring> get_signed_info(const TargetObject &remote_url, std::string_view etag, size_t filesize) = 0;
    virtual photon::fs::IFile* get_signed_object(const ELinkObject &t) = 0;
    virtual ~IAuthPlugin(){};
};


}