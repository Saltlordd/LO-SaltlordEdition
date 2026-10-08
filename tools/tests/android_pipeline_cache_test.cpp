#include <plume_android_pipeline_cache.h>
#include <cassert>
#include <filesystem>
#include <array>
#include <cstdio>
using namespace plume::android_pipeline_cache;
int main() {
 std::array<uint8_t,16> uuid{};uuid[0]=7;
 std::vector<uint8_t> bytes(64,0x42);uint32_t h[]={32,1,0x5143,0x1234};
 memcpy(bytes.data(),h,16);memcpy(bytes.data()+16,uuid.data(),16);
 assert(Compatible(bytes,0x5143,0x1234,uuid.data()));
 assert(!Compatible(bytes,0x1111,0x1234,uuid.data()));
 assert(!Compatible(bytes,0x5143,0x2222,uuid.data()));
 auto wrong=uuid;wrong[1]=1;assert(!Compatible(bytes,0x5143,0x1234,wrong.data()));
 assert(!Compatible(std::span(bytes).first(20),0x5143,0x1234,uuid.data()));
 auto bad=bytes;bad[0]=255;assert(!Compatible(bad,0x5143,0x1234,uuid.data()));
 bad=bytes;bad[4]=2;assert(!Compatible(bad,0x5143,0x1234,uuid.data()));
 char directory[]="/tmp/lo-pipeline-cache-XXXXXX";assert(mkdtemp(directory));
 auto path=std::string(directory)+"/cache.bin";
 assert(Write(path,bytes));assert(Read(path)==bytes);
 auto newer=bytes;newer[63]=0x10;assert(Write(path,newer));assert(Read(path)==newer);
 assert(!Write(path,std::span(bytes).first(12)));assert(Read(path)==newer);
 std::filesystem::create_symlink(path,path+".link");assert(Read(path+".link").empty());
 std::filesystem::create_symlink(path,path+".tmp");assert(!Write(path,bytes));assert(Read(path)==newer);
 std::filesystem::remove_all(directory);
 puts("PASS: cache identity, truncation, atomic replacement, prior-cache preservation, symlink refusal");
}
