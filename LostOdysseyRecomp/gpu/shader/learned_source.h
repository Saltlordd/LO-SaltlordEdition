#pragma once
#include "source_store.h"
namespace xenos::resources {
// Read only a recipe dependency; never scan or trust cache filenames alone.
inline bool ReadLearnedSource(const std::filesystem::path& path, uint64_t hash, std::vector<uint32_t>& words) {
    words.clear();std::error_code ec;
    if (std::filesystem::is_symlink(path,ec)||ec||!std::filesystem::is_regular_file(path,ec)||ec)return false;
    std::ifstream in(path,std::ios::binary|std::ios::ate);const auto size=in.tellg();
    if(size<12||size>SourceStore::MaxSourceBytes||size%4)return false;
    words.resize(size_t(size)/4);in.seekg(0);
    if(!in.read(reinterpret_cast<char*>(words.data()),size)||SourceStore::CodeHash({reinterpret_cast<const uint8_t*>(words.data()),size_t(size)})!=hash){words.clear();return false;}
    return true;
}
}
