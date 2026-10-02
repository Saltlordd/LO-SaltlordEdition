#include <gpu/shader/cpx_decode.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace inventory {

constexpr size_t MaxPackage = xenos::resources::cpx::kMaxDecodedSize;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Reader {
    std::span<const uint8_t> bytes;
    size_t pos = 0;

    std::span<const uint8_t> Take(size_t count) {
        Require(pos <= bytes.size() && count <= bytes.size() - pos, "truncated package table or object");
        auto result = bytes.subspan(pos, count);
        pos += count;
        return result;
    }
    uint8_t Byte() { return Take(1)[0]; }
    uint32_t U32() {
        const auto b = Take(4);
        return uint32_t(b[0]) << 24 | uint32_t(b[1]) << 16 | uint32_t(b[2]) << 8 | b[3];
    }
    int32_t I32() { return static_cast<int32_t>(U32()); }
    std::string String() {
        const auto size = I32();
        Require(size != 0 && size >= -32768 && size <= 32768, "invalid UE string length");
        if (size > 0) {
            const auto b = Take(size_t(size));
            Require(b.back() == 0, "unterminated UE string");
            return {reinterpret_cast<const char*>(b.data()), b.size() - 1};
        }
        const auto b = Take(size_t(-int64_t(size)) * 2);
        Require(b[b.size() - 1] == 0 && b[b.size() - 2] == 0, "unterminated UTF-16 UE string");
        std::string result;
        for (size_t i = 0; i + 2 < b.size(); i += 2) {
            // UE3's outer package fields are big-endian on Xbox 360, but
            // FString's UTF-16 payload is stored in little-endian order.
            uint32_t point = uint32_t(b[i]) | uint32_t(b[i + 1]) << 8;
            if (point >= 0xd800 && point <= 0xdbff) {
                Require(i + 4 < b.size(), "invalid UTF-16 surrogate");
                const uint32_t low = uint32_t(b[i + 2]) | uint32_t(b[i + 3]) << 8;
                Require(low >= 0xdc00 && low <= 0xdfff, "invalid UTF-16 surrogate");
                point = 0x10000 + ((point - 0xd800) << 10) + low - 0xdc00;
                i += 2;
            } else Require(point < 0xdc00 || point > 0xdfff, "invalid UTF-16 surrogate");
            if (point < 0x80) result += char(point);
            else if (point < 0x800) {
                result += char(0xc0 | point >> 6);
                result += char(0x80 | (point & 63));
            } else if (point < 0x10000) {
                result += char(0xe0 | point >> 12);
                result += char(0x80 | (point >> 6 & 63));
                result += char(0x80 | (point & 63));
            } else {
                result += char(0xf0 | point >> 18);
                result += char(0x80 | (point >> 12 & 63));
                result += char(0x80 | (point >> 6 & 63));
                result += char(0x80 | (point & 63));
            }
        }
        return result;
    }
};

struct Import {
    std::string objectName, className, classPackage;
    int32_t outer = 0;
};
struct Export {
    std::string name;
    int32_t classRef = 0, outer = 0;
    uint32_t serialOffset = 0, serialSize = 0;
};
struct TextureProperties {
    int32_t width = -1, height = -1, format = -1;
    std::string error;
};

struct Package {
    std::span<const uint8_t> bytes;
    uint32_t version = 0;
    std::vector<std::string> names;
    std::vector<Import> imports;
    std::vector<Export> exports;

    explicit Package(std::span<const uint8_t> data) : bytes(data) {
        Require(data.size() >= 44 && data.size() <= MaxPackage, "invalid UE package size");
        Reader r{data};
        Require(r.U32() == 0x9e2a83c1, "invalid UE package magic");
        version = r.U32();
        Require(version == 0x002a01a3 || version == 0x002901a3, "unsupported UE package version");
        const uint32_t headerSize = r.U32();
        r.String(); // Folder name, whose length varies independently of table positions.
        r.U32(); // Package flags.
        const uint32_t nameCount = r.U32(), nameOffset = r.U32();
        const uint32_t exportCount = r.U32(), exportOffset = r.U32();
        const uint32_t importCount = r.U32(), importOffset = r.U32();
        const uint32_t dependsOffset = r.U32();
        Require(nameCount <= 1000000 && exportCount <= 1000000 && importCount <= 1000000,
                "UE package table count exceeds limit");
        Require(r.pos <= nameOffset && nameOffset <= importOffset && importOffset <= exportOffset &&
                exportOffset <= dependsOffset && dependsOffset <= data.size(), "invalid UE package table offsets");
        // Some empty localized slots use a virtual header size. Only serialized
        // object ranges must lie in the decoded package, even when headerSize does not.
        (void)headerSize;
        names.reserve(nameCount);
        r = {data.first(importOffset), nameOffset};
        for (uint32_t i = 0; i < nameCount; ++i) {
            try {
                names.push_back(r.String());
                r.Take(8); // Name flags.
            } catch (const std::exception& ex) {
                throw std::runtime_error("name entry " + std::to_string(i) + " at " +
                                         std::to_string(r.pos) + ": " + ex.what());
            }
        }
        Require(r.pos <= importOffset, "name table overlaps import table");

        r = {data.first(exportOffset), importOffset};
        imports.reserve(importCount);
        for (uint32_t i = 0; i < importCount; ++i) {
            try {
                Import item;
                item.classPackage = Name(r);
                item.className = Name(r);
                item.outer = r.I32();
                item.objectName = Name(r);
                imports.push_back(std::move(item));
            } catch (const std::exception& ex) {
                throw std::runtime_error("import entry " + std::to_string(i) + " at " +
                                         std::to_string(r.pos) + ": " + ex.what());
            }
        }
        Require(r.pos <= exportOffset, "import table overlaps export table");

        r = {data.first(dependsOffset), exportOffset};
        exports.reserve(exportCount);
        for (uint32_t i = 0; i < exportCount; ++i) {
            try {
                Export item;
                item.classRef = r.I32();
                r.I32(); // Super object.
                item.outer = r.I32();
                item.name = Name(r);
                r.I32(); // Archetype object.
                r.Take(8); // Object flags.
                item.serialSize = r.U32();
                item.serialOffset = r.U32();
                // UE3 version 419: ComponentMap is a TMap<FName, int32>,
                // followed by ExportFlags, NetObjectCount (TArray<int32>)
                // and the 16-byte package GUID.
                const uint32_t componentCount = r.U32();
                Require(componentCount <= (r.bytes.size() - r.pos) / 12,
                        "invalid export component-map count");
                for (uint32_t component = 0; component < componentCount; ++component) {
                    Name(r);
                    r.I32();
                }
                r.U32(); // Export flags.
                const uint32_t netObjectCount = r.U32();
                Require(netObjectCount <= (r.bytes.size() - r.pos) / 4,
                        "invalid export net-object count");
                r.Take(size_t(netObjectCount) * 4);
                r.Take(16); // Package GUID.
                if (item.serialSize)
                    Require(item.serialOffset <= data.size() &&
                            item.serialSize <= data.size() - item.serialOffset, "export data outside package");
                exports.push_back(std::move(item));
            } catch (const std::exception& ex) {
                throw std::runtime_error("export entry " + std::to_string(i) + " at " +
                                         std::to_string(r.pos) + ": " + ex.what());
            }
        }
        Require(r.pos == dependsOffset, "export table length does not match export count");
        for (const auto& item : imports) ValidateRef(item.outer);
        for (const auto& item : exports) {
            ValidateRef(item.classRef);
            ValidateRef(item.outer);
        }
    }

    std::string Name(Reader& r) const {
        const uint32_t index = r.U32(), number = r.U32();
        Require(index < names.size() && number <= 1000000, "invalid UE name reference");
        return names[index] + (number ? "_" + std::to_string(number - 1) : "");
    }
    void ValidateRef(int32_t ref) const {
        Require(ref >= 0 ? size_t(ref) <= exports.size() :
                uint64_t(-int64_t(ref)) <= imports.size(), "object reference outside package tables");
    }
    std::string RefName(int32_t ref) const {
        if (ref > 0) return exports[size_t(ref) - 1].name;
        if (ref < 0) return imports[size_t(-int64_t(ref)) - 1].objectName;
        return {};
    }
    int32_t RefOuter(int32_t ref) const {
        if (ref > 0) return exports[size_t(ref) - 1].outer;
        if (ref < 0) return imports[size_t(-int64_t(ref)) - 1].outer;
        return 0;
    }
    std::string ObjectPath(int32_t ref) const {
        std::vector<std::string> parts;
        size_t steps = 0;
        while (ref) {
            Require(++steps <= imports.size() + exports.size(), "cyclic object outer reference");
            parts.push_back(RefName(ref));
            ref = RefOuter(ref);
        }
        std::string result;
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            if (!result.empty()) result += '.';
            result += *it;
        }
        return result;
    }
    TextureProperties Properties(const Export& item) const {
        TextureProperties result;
        try {
            Require(item.serialSize >= 12, "texture serialization too short");
            Reader r{bytes.subspan(item.serialOffset, item.serialSize)};
            r.I32(); // Net index.
            for (size_t count = 0; count < 65536; ++count) {
                const std::string property = Name(r);
                if (property == "None") return result;
                const std::string type = Name(r);
                const uint32_t size = r.U32();
                r.U32(); // Array index.
                if (type == "StructProperty") Name(r);
                if (type == "BoolProperty") r.U32();
                Reader value{r.Take(size)};
                if (property != "SizeX" && property != "SizeY" && property != "Format") continue;
                int32_t number;
                if (type == "IntProperty") number = value.I32();
                else if (type == "ByteProperty") number = value.Byte();
                else continue;
                if (property == "SizeX") result.width = number;
                else if (property == "SizeY") result.height = number;
                else result.format = number;
            }
            throw std::runtime_error("texture property count exceeds limit");
        } catch (const std::exception& ex) {
            result.error = ex.what();
        }
        return result;
    }
};

void JsonString(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (size_t i = 0; i < value.size();) {
        const auto c = static_cast<unsigned char>(value[i]);
        if (c == '"' || c == '\\') out << '\\' << char(c);
        else if (c >= 32 && c < 127) out << char(c);
        else if (c >= 0x80) {
            const size_t width = c >= 0xc2 && c <= 0xdf ? 2 :
                                 c >= 0xe0 && c <= 0xef ? 3 :
                                 c >= 0xf0 && c <= 0xf4 ? 4 : 0;
            bool valid = width && width <= value.size() - i;
            for (size_t j = 1; valid && j < width; ++j) {
                const auto next = static_cast<unsigned char>(value[i + j]);
                valid = next >= 0x80 && next <= 0xbf;
            }
            if (valid) {
                const auto second = static_cast<unsigned char>(value[i + 1]);
                valid = !(c == 0xe0 && second < 0xa0) && !(c == 0xed && second > 0x9f) &&
                        !(c == 0xf0 && second < 0x90) && !(c == 0xf4 && second > 0x8f);
            }
            if (valid) {
                out.write(value.data() + i, std::streamsize(width));
                i += width;
                continue;
            }
            out << "\\u00" << hex[c >> 4] << hex[c & 15];
        } else out << "\\u00" << hex[c >> 4] << hex[c & 15];
        ++i;
    }
    out << '"';
}
void JsonNumberOrNull(std::ostream& out, int32_t value) {
    if (value < 0) out << "null";
    else out << value;
}
uint64_t Decimal(std::string_view text) {
    uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    Require(error == std::errc{} && end == text.data() + text.size() && !text.empty(), "invalid decimal extent");
    return value;
}

std::string Process(std::string_view request) {
    std::string encoding = "unknown", status = "error", error;
    size_t decodedSize = 0;
    try {
        const auto first = request.find('\t');
        const auto second = first == request.npos ? request.npos : request.find('\t', first + 1);
        Require(first != request.npos && second != request.npos && request.find('\t', second + 1) == request.npos,
                "expected archive path, offset and length separated by tabs");
        const auto pathText = request.substr(0, first);
        Require(!pathText.empty(), "empty archive path");
        const uint64_t offset = Decimal(request.substr(first + 1, second - first - 1));
        const uint64_t length = Decimal(request.substr(second + 1));
        const auto path = std::filesystem::path(std::u8string(
            reinterpret_cast<const char8_t*>(pathText.data()), pathText.size()));
        std::ifstream source(path, std::ios::binary | std::ios::ate);
        Require(bool(source), "archive unreadable");
        const auto fileSize = source.tellg();
        Require(fileSize >= 0 && offset <= uint64_t(fileSize) && length <= uint64_t(fileSize) - offset,
                "extent outside archive");
        Require(length <= MaxPackage, "extent exceeds 128 MiB limit");
        if (length < 4) return "{\"status\":\"not_package\",\"encoding\":\"unknown\",\"decoded_size\":0,\"version\":null,\"exports\":[],\"imports\":[],\"error\":null}";
        source.seekg(std::streamoff(offset));
        uint8_t probe[16]{};
        const auto probeSize = size_t(std::min<uint64_t>(length, sizeof(probe)));
        Require(bool(source.read(reinterpret_cast<char*>(probe), std::streamsize(probeSize))), "archive probe short read");
        const bool compressed = probeSize >= 3 && probe[0] == 'c' && probe[1] == 'p' && probe[2] == 'x';
        const bool raw = probe[0] == 0x9e && probe[1] == 0x2a && probe[2] == 0x83 && probe[3] == 0xc1;
        if (!compressed && !raw)
            return "{\"status\":\"not_package\",\"encoding\":\"unknown\",\"decoded_size\":0,\"version\":null,\"exports\":[],\"imports\":[],\"error\":null}";
        encoding = compressed ? "cpx" : "raw";
        std::vector<uint8_t> stored(static_cast<size_t>(length));
        source.seekg(std::streamoff(offset));
        Require(bool(source.read(reinterpret_cast<char*>(stored.data()), std::streamsize(length))), "archive extent short read");
        std::vector<uint8_t> decoded;
        if (compressed) {
            Require(xenos::resources::cpx::Decode(stored, decoded), "invalid CPX stream");
        } else decoded.swap(stored);
        decodedSize = decoded.size();
        if (decoded.size() < 4 || decoded[0] != 0x9e || decoded[1] != 0x2a ||
            decoded[2] != 0x83 || decoded[3] != 0xc1)
            return "{\"status\":\"not_package\",\"encoding\":\"cpx\",\"decoded_size\":" +
                   std::to_string(decodedSize) + ",\"version\":null,\"exports\":[],\"imports\":[],\"error\":null}";
        Package package(decoded);
        std::ostringstream out;
        out << "{\"status\":\"ok\",\"encoding\":";
        JsonString(out, encoding);
        out << ",\"decoded_size\":" << decodedSize << ",\"version\":\""
            << (package.version == 0x002901a3 ? "002901a3" : "002a01a3") << "\",\"exports\":[";
        for (size_t i = 0; i < package.exports.size(); ++i) {
            const auto& item = package.exports[i];
            if (i) out << ',';
            const auto className = item.classRef == 0 ? "Class" : package.RefName(item.classRef);
            const auto properties = className == "Texture2D" || className == "TextureCube" ||
                className == "LightMapTexture2D" || className == "ShadowMapTexture2D" ?
                package.Properties(item) : TextureProperties{};
            out << "{\"index\":" << i << ",\"name\":";
            JsonString(out, item.name);
            out << ",\"class_name\":";
            JsonString(out, className);
            out << ",\"class_ref\":" << item.classRef << ",\"outer_ref\":" << item.outer
                << ",\"object_path\":";
            JsonString(out, package.ObjectPath(int32_t(i + 1)));
            out << ",\"serial_offset\":" << item.serialOffset << ",\"serial_size\":" << item.serialSize
                << ",\"width\":";
            JsonNumberOrNull(out, properties.width);
            out << ",\"height\":";
            JsonNumberOrNull(out, properties.height);
            out << ",\"format\":";
            JsonNumberOrNull(out, properties.format);
            out << ",\"property_error\":";
            if (properties.error.empty()) out << "null";
            else JsonString(out, properties.error);
            out << '}';
        }
        out << "],\"imports\":[";
        for (size_t i = 0; i < package.imports.size(); ++i) {
            const auto& item = package.imports[i];
            if (i) out << ',';
            out << "{\"index\":" << i << ",\"object_name\":";
            JsonString(out, item.objectName);
            out << ",\"class_name\":";
            JsonString(out, item.className);
            out << ",\"class_package\":";
            JsonString(out, item.classPackage);
            out << ",\"outer_ref\":" << item.outer << ",\"object_path\":";
            JsonString(out, package.ObjectPath(-int32_t(i + 1)));
            out << '}';
        }
        out << "],\"error\":null}";
        return out.str();
    } catch (const std::exception& ex) {
        error = ex.what();
    }
    std::ostringstream out;
    out << "{\"status\":\"error\",\"encoding\":";
    JsonString(out, encoding);
    out << ",\"decoded_size\":" << decodedSize << ",\"version\":null,\"exports\":[],\"imports\":[],\"error\":";
    JsonString(out, error);
    out << '}';
    return out.str();
}
} // namespace inventory

int main(int argc, char** argv) {
    if (argc != 1) {
        if (argc != 2 || std::string_view(argv[1]) != "--decode-cpx") {
            std::cerr << "expected no arguments or --decode-cpx\n";
            return 2;
        }
        try {
#ifdef _WIN32
            inventory::Require(_setmode(_fileno(stdin), _O_BINARY) != -1 &&
                               _setmode(_fileno(stdout), _O_BINARY) != -1,
                               "cannot set binary pipe mode");
#endif
            std::vector<uint8_t> stored;
            std::array<char, 65536> buffer{};
            for (;;) {
                std::cin.read(buffer.data(), std::streamsize(buffer.size()));
                const auto count = size_t(std::cin.gcount());
                inventory::Require(count <= inventory::MaxPackage - stored.size(),
                                   "CPX input exceeds 128 MiB limit");
                stored.insert(stored.end(), buffer.data(), buffer.data() + count);
                if (!std::cin) {
                    inventory::Require(std::cin.eof() && !std::cin.bad(), "CPX input read failed");
                    break;
                }
            }
            std::vector<uint8_t> decoded;
            inventory::Require(xenos::resources::cpx::Decode(stored, decoded), "invalid CPX stream");
            std::cout.write(reinterpret_cast<const char*>(decoded.data()),
                            std::streamsize(decoded.size()));
            std::cout.flush();
            inventory::Require(bool(std::cout), "CPX output write failed");
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << '\n';
            return 1;
        }
    }
    std::string request;
    while (std::getline(std::cin, request)) {
        try {
            std::cout << inventory::Process(request) << '\n';
        } catch (const std::exception& ex) {
            std::cout << "{\"status\":\"error\",\"encoding\":\"unknown\",\"decoded_size\":0,"
                         "\"version\":null,\"exports\":[],\"imports\":[],\"error\":";
            inventory::JsonString(std::cout, ex.what());
            std::cout << "}\n";
        }
    }
}
