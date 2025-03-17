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

#include "elink.h"
#include "photon/common/alog-stdstring.h"
#include "photon/common/alog.h"
#include "photon/common/estring.h"
#include <cstddef>
#include <string_view>
#include <unordered_map>
#include "def.h"

using namespace std;
using namespace photon::fs;

namespace ELink {

class RawReferenceList : public IReferenceList {
public:
    /* FileFormat */
    // | filesize | sourcePath + '\0' | eTag + '\0'  | mountPath + '\0' |
    // | 8 bytes  | N + 1 bytes       | N + 1 bytes  | N +1 bytes       |
    // need auto extend for object > 2TiB since ELinkMapping only supports object length no more than 2T

    photon::fs::IFile *m_file = nullptr;

    string m_bucket_name;
    string m_endpoint;
    IAuthPlugin *m_auth = nullptr;

    // todo expire time?
    unordered_map<off_t, IFile*> filepool;

    RawReferenceList(photon::fs::IFile *file, string_view bucket_name, string_view endpoint, IAuthPlugin *auth): 
        m_file(file), m_bucket_name(bucket_name), m_endpoint(endpoint), m_auth(auth)
    {}

    virtual photon::fs::IFile* get_remote_target(off_t target_index = -1) override {
        
        assert(target_index >= 0);
        if (filepool.find(target_index) != filepool.end()) {
            LOG_DEBUG("return opened file.");
            return filepool[target_index];
        }
        off_t offset = target_index * RAW_ALIGNED_SIZE;
        char buf[RAW_ALIGNED_SIZE];
        if (m_file->pread(buf, RAW_ALIGNED_SIZE, offset) != RAW_ALIGNED_SIZE) {
            LOG_ERRNO_RETURN(0, nullptr, "read reference list failed, idx: ` offset: `", target_index, offset);
        }
        auto targetObject = ELinkObject(m_endpoint, m_bucket_name, buf, RAW_ALIGNED_SIZE);
        auto r = m_auth->get_signed_object(targetObject);      
        if (r == nullptr) {
            LOG_ERRNO_RETURN(0, nullptr, "get remote object failed");
        }
        filepool[target_index] = r;
        return r;
    }

    virtual int stat_object(off_t idx, ELinkObject *stat) override {
        off_t offset = idx * RAW_ALIGNED_SIZE;
        char buf[RAW_ALIGNED_SIZE];
        if (m_file->pread(buf, RAW_ALIGNED_SIZE, offset) != RAW_ALIGNED_SIZE) {
            LOG_ERRNO_RETURN(0, -1, "read reference list failed, idx: ` offset: `", idx, offset);
        }
        *stat = ELinkObject(m_endpoint, m_bucket_name, buf, RAW_ALIGNED_SIZE);
        return 0;
    }

    virtual off_t append_elink_object(std::string_view src, std::string_view mountpath, 
        ssize_t object_size, std::string_view etag) override 
    {
        char *data = new char[RAW_ALIGNED_SIZE]{};
        DEFER(delete []data);
        auto p = data;
        ((ssize_t*)p)[0] = object_size;
        p+= sizeof(ssize_t);
        if (src[0]!='/') {
            *p='/';
            p++;
        }
        memcpy(p, src.data(), src.size());
        p+= src.size() + 1;
        if (!etag.empty()) {
            memcpy(p, etag.data(), etag.size());
            p+= etag.size() + 1;
        }
        memcpy(p, mountpath.data(), mountpath.size());
        auto offset = m_file->lseek(0, SEEK_END);
        if (m_file->pwrite(data, RAW_ALIGNED_SIZE, offset) != RAW_ALIGNED_SIZE) {
            LOG_ERRNO_RETURN(0, -1, "append reference object failed");
        }
        auto id = offset / RAW_ALIGNED_SIZE;
        LOG_DEBUG("append elink object success. idx: `, src: `, etag: `", id, src, etag);
        return id;
    }
};

IReferenceList *create_raw_reference_list(photon::fs::IFile* file, std::string_view bucket_name, std::string_view endpoint, IAuthPlugin *auth) {

    LOG_INFO("create raw reference list. using bucket name: `, endpoint: `", bucket_name, endpoint);
    return new RawReferenceList(file, bucket_name, endpoint, auth);
}

}