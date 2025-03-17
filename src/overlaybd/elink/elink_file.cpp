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

#include "def.h"
#include "elink.h"
#include "photon/common/alog.h"
#include "photon/fs/filesystem.h"
#include <sys/types.h>
#include <sys/fcntl.h>
#include "../lsmt/file.h"
#include "../lsmt/index.h"
#include "photon/fs/virtual-file.h"
#include <photon/fs/fiemap.h>
#include <errno.h>

using namespace photon::fs;
using namespace std;

namespace ELink{

class ELinkFile : public photon::fs::VirtualReadOnlyFile
{
public:
    IReferenceList *m_reflist = nullptr;
    bool m_ownership = false;
    // unordered_map<off_t, IFile*> m_files;

    ELinkFile(IReferenceList *reflist, bool ownership) : m_reflist(reflist), m_ownership(ownership) {};

    virtual ~ELinkFile() {
        if (m_ownership) {
            delete m_reflist;
        }
    }

    virtual ssize_t pread(void *buf, size_t count, off_t offset) override {

        LSMT::SegmentMapping m{
            0, 0,
            (uint64_t)offset / LSMT::ALIGNMENT, 
            0
        };
        off_t ref_idx = m.reference_index();
        off_t inner_offset = m.inner_offest();
        assert(ref_idx>=0);
        LOG_DEBUG("read from elink object {ref_idx: `, offset: `}", ref_idx, inner_offset);
        auto file = m_reflist->get_remote_target(ref_idx);
        if (file == nullptr) {
            LOG_ERRNO_RETURN(EACCES, -1, "failed to get remote file");
        }
        ssize_t readn = file->pread(buf, count, inner_offset * LSMT::ALIGNMENT);
        if (readn < (ssize_t)count) {
            // check end of file
            struct stat st;
            file->fstat(&st);
            if (inner_offset * LSMT::ALIGNMENT + count < (size_t)st.st_size) {
                LOG_ERRNO_RETURN(0, -1, "failed to read remote file (offset: `, count: `,  actual read: `)", offset, count, readn);
            }
            // fill zero
            memset((char*)buf + readn, 0, count - readn);
        }
        return count;
    };

    UNIMPLEMENTED_POINTER(IFileSystem* filesystem() override);
    UNIMPLEMENTED(int fstat(struct stat *st) override);

};

photon::fs::IFile *open_elink_file(IReferenceList *reflist, bool ownership) {

    return new ELinkFile(reflist, ownership);
}

int create_elink(photon::fs::IFileSystem *fs, IReferenceList *reflist, off_t ref_idx) {

    ELinkObject target;
    if (reflist->stat_object(ref_idx, &target) != 0) {
        LOG_ERRNO_RETURN(EINVAL, -1, "failed to stat object");
    }
    auto fn = target.mount_path[0] != '/' ? estring("/") + target.mount_path : target.mount_path;
    //TODO mkdir -p dirname(fn)
    auto elink_obj = fs->open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0666);
    if (elink_obj == nullptr) {
        LOG_ERRNO_RETURN(EINVAL, -1, "failed to create elink object in FS");
    }

    struct photon::fs::fiemap_t<8192> fie(0, target.filesize);

    if (elink_obj->fallocate(0, 0, target.filesize) != 0 || elink_obj->fiemap(&fie)!=0 ){
        return -1;
    }
    auto imgfile = (IFile*)(fs->get_underlay_object());
    assert(imgfile != nullptr);
    off_t inner_offset = 0;
    auto count = ((target.filesize + ALIGNMENT_BLK - 1) / ALIGNMENT_BLK) * ALIGNMENT_BLK;
    for (uint32_t i = 0; i < fie.fm_mapped_extents; i++) {
        LSMT::ELinkMapping lba;
        lba.offset = fie.fm_extents[i].fe_physical;
        lba.count = (fie.fm_extents[i].fe_length < count ? fie.fm_extents[i].fe_length : count);
        lba.ref_idx = ref_idx;
        lba.inner_offset = inner_offset;
        int nwrite = imgfile->ioctl(LSMT::IFileRW::ELinkData, lba);
        if (nwrite < 0) {
            LOG_ERRNO_RETURN(0, -1, "failed to write lba");
        }
        inner_offset += nwrite;
        count-=lba.count;
    }
    return 0;
}

}