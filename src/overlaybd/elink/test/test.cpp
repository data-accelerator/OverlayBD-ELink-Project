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

#include "func.cpp"
#include <fcntl.h>
#include <gtest/gtest.h>


DEFINE_int32(log_level, 1, "log level");
DEFINE_string(ak, "", "accessKeyID");
DEFINE_string(sk, "", "accessKeySecret");


TEST(SimpleCredClient, parse) {
    const string content = R"(
    {
        "accessKeyID": "accessKeyID000",
        "accessKeySecret": "accessKeySecret111",
    }
    )";
    auto lfs = photon::fs::new_localfs_adaptor();
    EXPECT_NE(lfs, nullptr);
    DEFER(delete lfs);
    auto file = lfs->open("/tmp/test.cred", O_CREAT | O_TRUNC | O_RDWR, 0644);    EXPECT_NE(file, nullptr);
    file->write(content.data(), content.size());
    file->close();
    delete file;
    auto client = ELink::create_simple_cred_client("/tmp/test.cred");
    EXPECT_NE(client, nullptr);
    DEFER(delete client);
    auto r = client->access_key("asdf");
    for (auto item : r) {
        LOG_INFO("key: ", item.first, ", value: ", item.second);
    }
}

TEST(SimpleAuth, get_signed_url0) {
    char buf[1024]{};
    sprintf(buf, "{\"accessKeyID\": \"%s\", \"accessKeySecret\": \"%s\"}", FLAGS_ak.c_str(), FLAGS_sk.c_str());
    auto lfs = photon::fs::new_localfs_adaptor();
    EXPECT_NE(lfs, nullptr);
    DEFER(delete lfs);
    auto file = lfs->open("/tmp/test.cred", O_CREAT | O_TRUNC | O_RDWR, 0644);
    EXPECT_NE(file, nullptr);
    EXPECT_EQ(file->write(buf, strlen(buf)), strlen(buf));
    delete file;
    DEFER(lfs->unlink("/tmp/test.cred"));
    auto cred = ELink::create_simple_cred_client("/tmp/test.cred");
    DEFER(delete cred);
    auto auth = ELink::create_auth_plugin(cred, ELink::AuthPluginType::AliyunOSS);
    DEFER(delete auth);
    char raw[ELink::RAW_ALIGNED_SIZE]{};
    ((size_t*)raw)[0] = 2232023984;
    auto objname = "/DADI_at_Scale_fix.mov";
    memcpy(raw + sizeof(size_t), objname, strlen(objname));
    auto etag = "BD221597AF09D219E63E7A83651A28F5-400";
    memcpy(raw + sizeof(size_t) + strlen(objname) + 1, etag, strlen(etag)); 
    ELink::ELinkObject t("oss-cn-beijing.aliyuncs.com", "dadi-shared", raw, ELink::RAW_ALIGNED_SIZE);
    EXPECT_STREQ(t.etag.data(), etag);
    auto remote_file = auth->get_signed_object(t);
    EXPECT_NE(remote_file, nullptr);
    DEFER(delete remote_file);
}


TEST(SimpleAuth, get_signed_url1) {
    if (FLAGS_ak.empty() || FLAGS_sk.empty()) {
        LOG_INFO("this testcase needs --ak and --sk specified");
        return;
    }
    char buf[1024]{};
    sprintf(buf, "{\"accessKeyID\": \"%s\", \"accessKeySecret\": \"%s\"}", FLAGS_ak.c_str(), FLAGS_sk.c_str());
    auto lfs = photon::fs::new_localfs_adaptor();
    EXPECT_NE(lfs, nullptr);
    DEFER(delete lfs);
    auto file = lfs->open("/tmp/test.cred", O_CREAT | O_TRUNC | O_RDWR, 0644);
    EXPECT_NE(file, nullptr);
    EXPECT_EQ(file->write(buf, strlen(buf)), strlen(buf));
    delete file;
    DEFER(lfs->unlink("/tmp/test.cred"));
    auto cred = ELink::create_simple_cred_client("/tmp/test.cred");
    auto auth = ELink::create_auth_plugin(cred, ELink::AuthPluginType::AliyunOSS);
    DEFER(delete auth);
    char raw[ELink::RAW_ALIGNED_SIZE]{};
    ((size_t*)raw)[0] = 754176;
    auto objname = "/k8s.gcr.io-pause-3.5.tar.gz";
    memcpy(raw + sizeof(size_t), objname, strlen(objname));
    auto etag = "C4FDFB659D81309CE7C532B264E5BC7D";
    memcpy(raw + sizeof(size_t) + strlen(objname) + 1, etag, strlen(etag)); 
    ELink::ELinkObject t("oss-cn-beijing.aliyuncs.com", "dadi-shared", raw, ELink::RAW_ALIGNED_SIZE);
    EXPECT_STREQ(t.etag.data(), etag);
    auto remote_file = auth->get_signed_object(t);
    EXPECT_NE(remote_file, nullptr);
    DEFER(delete remote_file);
    auto sha256file = new_sha256_file(remote_file, false);
    auto sha256checksum = sha256file->sha256_checksum();
    EXPECT_STREQ(sha256checksum.c_str(), "sha256:2b7c3003b2aee057b4c0bd24be0ecec3f57d14074e9078feeda4c675e165cf43");
    delete sha256file;
}


TEST(Simple, elinkMapping) {
    if (FLAGS_ak.empty() || FLAGS_sk.empty()) {
        LOG_INFO("this testcase needs --ak and --sk specified");
        return;
    }
    char buf[1024]{};
    sprintf(buf, "{\"accessKeyID\": \"%s\", \"accessKeySecret\": \"%s\"}", FLAGS_ak.c_str(), FLAGS_sk.c_str());
    auto lfs = photon::fs::new_localfs_adaptor();
    EXPECT_NE(lfs, nullptr);
    DEFER(delete lfs);
    auto file = lfs->open("/tmp/test.cred", O_CREAT | O_TRUNC | O_RDWR, 0644);
    EXPECT_NE(file, nullptr);
    EXPECT_EQ(file->write(buf, strlen(buf)), strlen(buf));
    delete file;
    DEFER(lfs->unlink("/tmp/test.cred"));
    auto cred = ELink::create_simple_cred_client("/tmp/test.cred");
    auto auth = ELink::create_auth_plugin(cred, ELink::AuthPluginType::AliyunOSS);
    DEFER(delete auth);
    char raw[ELink::RAW_ALIGNED_SIZE]{};
    ((size_t*)raw)[0] = 754176;
    auto objname = "/k8s.gcr.io-pause-3.5.tar.gz";
    memcpy(raw + sizeof(size_t), objname, strlen(objname));
    auto etag = "C4FDFB659D81309CE7C532B264E5BC7D";
    memcpy(raw + sizeof(size_t) + strlen(objname) + 1, etag, strlen(etag)); 
    ELink::ELinkObject t("oss-cn-beijing.aliyuncs.com", "dadi-shared", raw, ELink::RAW_ALIGNED_SIZE);
    EXPECT_STREQ(t.etag.data(), etag);
    auto remote_file = auth->get_signed_object(t);
    EXPECT_NE(remote_file, nullptr);
    DEFER(delete remote_file);
    auto sha256file = new_sha256_file(remote_file, false);
    auto sha256checksum = sha256file->sha256_checksum();
    EXPECT_STREQ(sha256checksum.c_str(), "sha256:2b7c3003b2aee057b4c0bd24be0ecec3f57d14074e9078feeda4c675e165cf43");
    delete sha256file;
}

TEST_F(ELinkTest, create) {
     if (FLAGS_ak.empty() || FLAGS_sk.empty()) {
        LOG_INFO("this testcase needs --ak and --sk specified");
        return;
    }
    char buf[1024]{};
    sprintf(buf, "{\"accessKeyID\": \"%s\", \"accessKeySecret\": \"%s\"}", FLAGS_ak.c_str(), FLAGS_sk.c_str());
    auto lfs = photon::fs::new_localfs_adaptor();
    EXPECT_NE(lfs, nullptr);
    DEFER(delete lfs);
    auto file = lfs->open("/tmp/test.cred", O_CREAT | O_TRUNC | O_RDWR, 0644);
    EXPECT_NE(file, nullptr);
    EXPECT_EQ(file->write(buf, strlen(buf)), strlen(buf));
    delete file;
    DEFER(lfs->unlink("/tmp/test.cred"));
    auto cred = ELink::create_simple_cred_client("/tmp/test.cred");
    // set_log_output_level(0);

    auto reflist_file = fs->open("ref.list", O_CREAT | O_RDWR | O_TRUNC, 0644);
    ASSERT_NE(reflist_file, nullptr);
    auto auth = ELink::create_auth_plugin(cred, ELink::AuthPluginType::AliyunOSS);
    DEFER(delete auth);
    auto reflist = ELink::create_raw_reference_list(reflist_file, "elink-volume", "oss-ap-northeast-1-internal.aliyuncs.com", auth);
    DEFER(delete reflist);
    auto idx = reflist->append_elink_object("go1.21.13.linux-amd64.tar.gz", "/go1.21.13.linux-amd64.tar.gz", 
        66660837, "3C624B8AC6980E79653F1AD478828F7C");
    auto elinkfile = ELink::open_elink_file(reflist);
    auto imgfile = createDevice("mock", elinkfile);
    DEFER(delete imgfile;);
    make_extfs(imgfile);
    auto extfs = new_extfs(imgfile, false);
    // auto target = new_subfs(extfs, "/", true);
    // DEFER(delete target);
    EXPECT_EQ(ELink::create_elink(extfs, reflist, idx), 0);
    delete extfs;
    extfs = new_extfs(imgfile, true);
    DEFER(delete extfs);

    auto remote_target = extfs->open("/go1.21.13.linux-amd64.tar.gz", O_RDONLY);
    EXPECT_NE(remote_target, nullptr);
    struct stat _;
    EXPECT_EQ(remote_target->fstat(&_), 0);
    EXPECT_EQ(_.st_size, 66660837);
    auto sha256file = new_sha256_file(remote_target, true);
    DEFER(delete sha256file);
    auto sha256sum = sha256file->sha256_checksum();
    EXPECT_STREQ(sha256sum.c_str(), "sha256:502fc16d5910562461e6a6631fb6377de2322aad7304bf2bcd23500ba9dab4a7");
}

int main(int argc, char **argv) {
    auto seed = 154574045;
    std::cerr << "seed = " << seed << std::endl;
    srand(seed);
    set_log_output_level(1);

    ::testing::InitGoogleTest(&argc, argv);
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    log_output_level = FLAGS_log_level;
    photon::init(photon::INIT_EVENT_DEFAULT, photon::INIT_IO_DEFAULT);
    auto ret = RUN_ALL_TESTS();
    photon::fini();
    if (ret)
        LOG_ERROR_RETURN(0, ret, VALUE(ret));
}
