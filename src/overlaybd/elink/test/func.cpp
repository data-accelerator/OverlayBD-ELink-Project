

#include <photon/photon.h>
#include <photon/common/io-alloc.h>
#include <photon/common/alog.h>
#include <photon/common/alog-stdstring.h>
#include <photon/fs/localfs.h>
#include <photon/io/fd-events.h>
#include <photon/net/socket.h>
#include <photon/thread/thread11.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include <vector>
#include <string.h>
#include <photon/fs/subfs.h>
#include "../def.h"
#include "../../../tools/sha256file.h"
#include "../../lsmt/file.h"
#include <photon/fs/subfs.h>
#include <photon/fs/extfs/extfs.h>


using namespace std;
using namespace photon::fs;

#define FILE_SIZE (2 * 1024 * 1024)
#define IMAGE_SIZE 512UL<<20
class ELinkTest : public ::testing::Test {
protected:
    virtual void SetUp() override{
        fs = photon::fs::new_localfs_adaptor();

        ASSERT_NE(nullptr, fs);
        if (fs->access(workdir.c_str(), 0) != 0) {
            auto ret = fs->mkdir(workdir.c_str(), 0755);
            ASSERT_EQ(0, ret);
        }

        fs = photon::fs::new_subfs(fs, workdir.c_str(), true);
        ASSERT_NE(nullptr, fs);
    }
    virtual void TearDown() override{
        for (auto fn : filelist){
            fs->unlink(fn.c_str());
        }
        if (fs)
            delete fs;
    }

    int download(const std::string &url, std::string out = "") {
        if (out == "") {
            out = workdir + "/" + std::string(basename(url.c_str()));
        }
        if (fs->access(out.c_str(), 0) == 0)
            return 0;
        // download file
        std::string cmd = "curl -s -o " + out + " " + url;
        LOG_INFO(VALUE(cmd.c_str()));
        auto ret = system(cmd.c_str());
        if (ret != 0) {
            LOG_ERRNO_RETURN(0, -1, "download failed: `", url.c_str());
        }
        return 0;
    }

    int download_decomp(const std::string &url) {
        // download file
        std::string cmd = "wget -q -O - " + url +" | gzip -d -c >" +
                          workdir + "/latest.tar";
        LOG_INFO(VALUE(cmd.c_str()));
        auto ret = system(cmd.c_str());
        if (ret != 0) {
            LOG_ERRNO_RETURN(0, -1, "download failed: `", url.c_str());
        }
        return 0;
    }


    int write_file(photon::fs::IFile *file) {
        std::string bb = "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz01";
        ssize_t size = 0;
        ssize_t ret;
        struct stat st;
        LOG_INFO(VALUE(bb.size()));
        while (size < FILE_SIZE) {
            ret = file->write(bb.data(), bb.size());
            EXPECT_EQ(bb.size(), ret);
            ret = file->fstat(&st);
            EXPECT_EQ(0, ret);
            ret = file->lseek(0, SEEK_CUR);
            EXPECT_EQ(st.st_size, ret);
            size += bb.size();
        }
        LOG_INFO("write ` byte", size);
        EXPECT_EQ(FILE_SIZE, size);
        return 0;
    }

    IFile *createDevice(const char *fn, IFile *elinkfile, size_t virtual_size = IMAGE_SIZE){
        auto fn_idx = std::string(fn)+".idx";
        auto fn_meta = std::string(fn)+".meta";
        DEFER({
            filelist.push_back(fn_idx);
            filelist.push_back(fn_meta);
        });
        auto fmeta = fs->open(fn_idx.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        auto findex = fs->open(fn_meta.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        LSMT::WarpFileArgs args(findex, fmeta, elinkfile);
        args.virtual_size = virtual_size;
        return create_warpfile(args, false);
    }

    int do_verify(IFile *verify, IFile *test, off_t offset = 0, ssize_t count = -1) {

        if (count == -1) {
            count = verify->lseek(0, SEEK_END);
            auto len = test->lseek(0, SEEK_END);
            if (count != len) {
                LOG_ERROR("check logical length failed");
                return -1;
            }
        }
        LOG_INFO("start verify, virtual size: `", count);

        ssize_t LEN = 1UL<<20;
        char vbuf[1UL<<20], tbuf[1UL<<20];
        // set_log_output_level(0);
        for (off_t i = 0; i < count; i+=LEN) {
            LOG_DEBUG("`", i);
            auto ret_v = verify->pread(vbuf, LEN, i);
            auto ret_t = test->pread(tbuf, LEN, i);
            if (ret_v == -1 || ret_t == -1) {
                LOG_ERROR_RETURN(0, -1, "pread(`,`) failed. (ret_v: `, ret_t: `)",
                    i, LEN, ret_v, ret_v);
            }
            if (ret_v != ret_t) {
                LOG_ERROR_RETURN(0, -1, "compare pread(`,`) return code failed. ret:` / `(expected)",
                    i, LEN, ret_t, ret_v);
            }
            if (memcmp(vbuf, tbuf, ret_v)!= 0){
                LOG_ERROR_RETURN(0, -1, "compare pread(`,`) buffer failed.", i, LEN);
            }
        }
        return 0;
    }

    std::string workdir = "/tmp/elinktest";
    photon::fs::IFileSystem *fs;
    std::vector<std::string> filelist;
};