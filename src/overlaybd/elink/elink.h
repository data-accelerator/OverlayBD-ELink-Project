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

#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <photon/common/estring.h>
#include <photon/fs/filesystem.h>
#include <string_view>
#include <photon/common/alog.h>
#include <photon/common/alog-stdstring.h>

namespace ELink{


struct ELinkObject;

class ICredentialClient; 
class IAuthPlugin;

enum class AuthPluginType {
    AliyunOSS
};

static const int ALIGNMENT_BLK = 4096;
static const int RAW_ALIGNED_SIZE = 1024;


struct ELinkObject {
    estring endpoint;
    estring bucket_name;
    estring source;
    estring mount_path;
    estring etag;
    size_t filesize = 0;

    ELinkObject(){};

    ELinkObject(std::string_view endpoint, std::string_view bucket_name, const char* raw_data, size_t size) : 
        endpoint(endpoint), bucket_name(bucket_name) {
        assert(size == RAW_ALIGNED_SIZE);
        auto p = raw_data;
        filesize = ((size_t*)raw_data)[0];
        p += sizeof(filesize);
        source = std::string_view(p, strlen(p));
        p += source.size() + 1;
        etag = std::string_view(p, strlen(p));
        p += etag.size() + 1;
        mount_path = std::string_view(p, strlen(p));
        LOG_DEBUG("parse target object. {source:` , mount: `, size: `, etag: `}", source, mount_path, filesize, etag);
    }

    estring remote_url() const {
        estring url;
        url.append("https://");
        url.append(bucket_name);
        url.append(".");
        url.append(endpoint);
        url.append(source);
        LOG_DEBUG("remote url: `", url);
        return url;
    }
};

class IReferenceList {
public:
    virtual photon::fs::IFile* get_remote_target(off_t target_index = -1) = 0;

    virtual int stat_object(off_t idx, ELinkObject *stat) = 0;


    // virtual int create_reference_object(char *out, std::string_view src, std::string_view mountpath, 
    //     ssize_t object_size, std::string_view etag) = 0;

    // return the reference index of the new object
    virtual off_t append_elink_object(std::string_view src, std::string_view mountpath, 
        ssize_t object_size, std::string_view etag) = 0;

    virtual ~IReferenceList(){};
};

IReferenceList *create_raw_reference_list(photon::fs::IFile* file, std::string_view bucket_name, std::string_view endpoint, IAuthPlugin *auth);

ICredentialClient *create_simple_cred_client(const char *fn);
IAuthPlugin *create_auth_plugin(ICredentialClient *cred, AuthPluginType type);

photon::fs::IFile *open_elink_file(IReferenceList *reflist, bool ownership = false);

int create_elink(photon::fs::IFileSystem *fs, IReferenceList *reflist, off_t ref_idx);

}