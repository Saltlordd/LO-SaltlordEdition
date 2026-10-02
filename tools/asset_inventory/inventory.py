#!/usr/bin/env python3
"""Read-only four-disc asset inventory; metadata only, no extracted game payloads."""
from __future__ import annotations

import argparse
from collections import Counter
from contextlib import closing
import csv
from datetime import datetime, timezone
import hashlib
import gzip
import json
from pathlib import Path
import re
import sqlite3
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "modding"))
from lo_mod import make_key

ARCHIVES = ("LO.fpd", "xenon_chr.fpd", "xenon_event.fpd", "xenon_field.fpd",
            "xenon_obj.fpd", "xenon_scr.fpd", "xenon_sys.fpd", "xenon_vfx.fpd",
            "xenon_world.fpd", "xenon_battle.fpd", "xenon_loc.fpd", "xenon_mov.fpd", "xenon_snd.fpd")
ALPHABET = "\0" + "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_.\\"
SUFFIXES = ("", "_sndw", "_scrw", "_mapw", "_lvdw", "_navw", "_colw", "_camw",
            "_map", "_cam", "_bx", "_mw", "_a", "_d", "_f", "_m", "_p", "_u", "_w", "_0", "_1", "_2", "_00", "_01",
            "_0mw", "_nav", "_elgt", "_000a0", "_010a0", "_020a0", "_030a0", "_040a0")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read_fpi(disc: Path):
    """Decode the loader's packed filename dictionary and reconcile every record."""
    data = (disc / "LO.fpi").read_bytes()
    require(64 <= len(data) <= 2 * 1024 * 1024, "invalid FPI length")
    def read(fmt, at):
        require(0 <= at <= len(data) - struct.calcsize(fmt), "FPI read out of bounds")
        return struct.unpack_from(fmt, data, at)[0]
    def u16(at): return read("<H", at)
    def u32(at): return read("<I", at)
    used, count = u16(12) * 2048, u16(26)
    begin, entries, dictionary, extensions = (u32(p) for p in (32, 36, 40, 44))
    require(u32(8) == 0x10000 and u16(24) == 1 and count == 13, "unsupported FPI version/archive count")
    require(1 <= data[20] <= 4 and data[21] == 4, "unsupported disc set")
    require(64 <= begin and begin + count * 48 <= entries <= dictionary <= used <= len(data), "invalid FPI tables")
    require(entries + u32(28) * 24 == dictionary, "FPI entry count mismatch")
    def unpack(at):
        require(dictionary <= at < used, "FPI dictionary pointer out of bounds")
        word = u16(at)
        require(word // 1600 < 40, "invalid packed filename")
        value = ALPHABET[word // 40 % 40] + ALPHABET[word // 1600]
        require(at + 2 + word % 40 * 2 <= used, "truncated packed filename")
        for i in range(word % 40):
            word = u16(at + 2 + i * 2)
            require(word // 1600 < 40, "invalid packed filename")
            value += ALPHABET[word % 40] + ALPHABET[word // 40 % 40] + ALPHABET[word // 1600]
        return value.split("\0", 1)[0].lower()
    def name(bits):
        if not bits & 0x3ffff:
            return ""
        result = unpack(dictionary + (bits & 0x3ffff) * 2) + SUFFIXES[bits >> 18 & 31]
        ext = bits >> 23 & 31
        return result + ("." + unpack(dictionary + u16(extensions + (ext - 1) * 2) * 2) if ext else "")
    files, total_records, directory_count = [], 0, 0
    for i, expected in enumerate(ARCHIVES):
        ar = begin + i * 48
        archive = name(u32(ar + 24))
        require(archive == expected.lower(), f"unexpected archive slot {i}: {archive}")
        source = disc / expected
        size = source.stat().st_size
        base = ar + u32(ar + 4)
        end = dictionary if i == count - 1 else ar + 48 + u32(ar + 52)
        require(entries <= base <= end <= dictionary and (end - base) % 24 == 0, "invalid archive record range")
        record_count = (end - base) // 24
        prefix = name(u32(ar + 20))
        pending, visited = [(0, u16(ar + 2), 0, prefix + "\\" if prefix else "")], set()
        while pending:
            first, length, depth, parent = pending.pop()
            require(depth < 64 and first + length <= record_count, "FPI tree out of bounds")
            for j in range(first, first + length):
                require(j not in visited, "FPI tree cycle/overlap")
                visited.add(j)
                p = base + j * 24
                bits = u32(p)
                path = (parent + name(bits)).replace("\\", "/")
                require(path and not path.startswith("/") and ".." not in path.split("/"), "invalid virtual path")
                if bits & 0x10000000:
                    directory_count += 1
                    pending.append((u32(p + 20), u16(p + 14), depth + 1, path + "/"))
                else:
                    offset, length = (u32(p + 8) & 0xffffff) * 2048, u32(p + 16)
                    require(offset <= size and length <= size - offset, f"extent outside {source}: {path}")
                    files.append(dict(disc=disc.name, archive=expected, path=path, offset=offset, length=length))
        require(len(visited) == record_count, f"unreachable FPI records in {archive}: {record_count-len(visited)}")
        total_records += record_count
    require(total_records == u32(28), "unreconciled FPI records")
    return files, dict(disc=disc.name, disc_number=data[20], fpi_sha256=hashlib.sha256(data).hexdigest(),
                       records=total_records, directories=directory_count, files=len(files))


def classify(cls, name=""):
    if name.startswith("Default__"):
        return "default_templates"
    if cls in {"StaticMesh", "StaticMeshRPG", "SkeletalMesh", "Mesh", "VertexMesh"}:
        return "models"
    if cls in {"SkeletalMesh_VC", "StaticMeshActor", "SkeletalMeshActor", "SkeletalMeshActorMAT", "fpPawn"}:
        return "scene_actors"
    if cls in {"Model", "Polys", "Terrain"}:
        return "world_geometry"
    if (cls.endswith("Component") or cls == "CharacterLightComponentRPG") and not cls.startswith("MaterialExpression"):
        return "scene_components"
    if cls.startswith("Texture") or cls in {"LightMapTexture2D", "ShadowMapTexture2D", "TerrainWeightMapTexture"}:
        return "textures"
    if cls in {"Font", "MultiFont"}:
        return "fonts"
    if cls.startswith("MaterialExpression"):
        return "material_nodes"
    if cls.startswith("Material"):
        return "materials"
    if cls == "AnimTree" or cls.startswith(("AnimNode", "rpAnimNode", "wmAnimNode", "faAnimNode")):
        return "animation_graphs"
    if cls.startswith(("Anim", "Morph")) or cls == "FaceFXAnimSet":
        return "animations"
    if cls in {"ImageRectAnimation", "LOIR_AnimVectorBase", "LOIR_AnimFloatBase"}:
        return "cinematic_animation"
    if cls in {"PhysicsAsset", "PhysicsAssetInstance", "RB_BodySetup", "RB_BodyInstance", "RB_ConstraintSetup", "RB_ConstraintInstance"}:
        return "physics"
    if cls in {"ShadowMap1D", "ShadowMap2D"}:
        return "lighting_data"
    if cls.startswith(("Particle", "LensFlare")):
        return "effects"
    if cls.startswith(("Sound", "Audio")):
        return "audio"
    if cls.startswith("UI"):
        return "ui_objects"
    if cls.startswith(("Interp", "Sequence", "SeqAct", "SeqCond", "SeqEvent", "SeqVar")):
        return "sequences"
    if cls in {"Class", "Function", "State", "Enum", "Const", "ScriptStruct"} or cls.endswith("Property"):
        return "script_objects"
    if "ShaderCache" in cls:
        return "shader_caches"
    if cls in {"World", "Level", "WorldInfo", "LevelStreamingKismet", "LevelStreamingAlwaysLoaded"}:
        return "worlds_levels"
    if cls == "Package":
        return "groups"
    if cls == "ObjectRedirector":
        return "redirectors"
    if cls.startswith("Distribution"):
        return "parameter_curves"
    return "other_objects"


def ui_reason(path, cls=""):
    if cls.startswith("UI") or cls in {"Font", "MultiFont"}:
        return "export_class"
    parts = path.lower().replace("\\", "/").split("/")
    stem = Path(parts[-1]).stem
    if any(p in {"chr", "obj"} for p in parts):
        return ""
    if any(p in {"menu", "ui", "fonts", "hud", "tutorial", "staffroll"} for p in parts[:-1]):
        return "package_directory_heuristic"
    if (re.search(r"menu|font|hud|tutorial|controller|button|icon|dialog|guide|interface|(?:^|[_-])ui(?:[_-]|$)", stem)
            or stem in {"defaultuiskin", "engineresources", "enginefonts", "rpbattlecommon", "rpfieldcommon",
                        "uiarcpackage", "rpworldmapcommon", "rpworldmap", "rpgameover", "rpnavi",
                        "rpstaffroll", "title", "titleparts", "vwimages"}
            or "uitargetring" in stem):
        return "package_name_heuristic"
    return ""


def family(path):
    parts = path.split("/")
    return parts[2] if parts[:2] == ["bin", "xenon"] and len(parts) > 2 else parts[0]


def language(path):
    parts = path.split("/")
    return parts[3] if len(parts) > 4 and parts[:3] == ["bin", "xenon", "loc"] else ""


def file_kind(path, magic):
    """File-level use hints; keep these separate from parsed UE3 export classes."""
    ext = Path(path).suffix
    if ext == ".dds":
        return "standalone_textures", "extension"
    if ext == ".wmv":
        return "movies", "extension"
    if ext == ".xwv":
        return "audio_streams", "extension"
    if ext == ".xse" and "/snd/" in path:
        return "audio_banks", "directory_and_extension"
    if ext == ".jmd" and "/mes/" in path:
        return "dialogue_tables", "directory_and_extension"
    if ext == ".sub" and "/subtitle/" in path:
        return "subtitles", "directory_and_extension"
    if "/scr/scr/" in path and (ext == ".bin" or re.fullmatch(r"\.r\d+", ext)):
        return "event_scripts", "directory_and_extension"
    if ext == ".sed" and "/seq/" in path:
        return "sequence_data", "directory_and_extension_heuristic"
    if ext == ".col":
        return "collision_data", "extension_heuristic"
    if ext == ".cam" or (ext == ".xml" and "_cam" in path):
        return "camera_data", "name_heuristic"
    if path.startswith("rpgame/localization/") or (ext == ".dat" and "/loc/" in path):
        return "localized_data", "directory_heuristic"
    if ext == ".ini":
        return "configuration", "extension"
    if ext in {".xml", ".xmb"}:
        return "structured_data", "extension_heuristic"
    return "other_files", "unclassified"


def digest_extent(stream, offset, length):
    stream.seek(offset)
    digest, prefix, remaining = hashlib.sha256(), b"", length
    while remaining:
        block = stream.read(min(1024 * 1024, remaining))
        require(block, "short resource read")
        if not prefix:
            prefix = block[:4]
        digest.update(block)
        remaining -= len(block)
    return digest.hexdigest(), prefix.hex()


SCHEMA = """
PRAGMA foreign_keys=ON;
CREATE TABLE metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL);
CREATE TABLE sources(path TEXT PRIMARY KEY, size INTEGER, mtime_ns INTEGER, sha256 TEXT);
CREATE TABLE payloads(sha256 TEXT PRIMARY KEY, stored_size INTEGER, magic TEXT, encoding TEXT,
 decoded_size INTEGER, version TEXT, status TEXT, error TEXT);
CREATE TABLE files(id INTEGER PRIMARY KEY, disc TEXT, archive TEXT, path TEXT, offset INTEGER,
 length INTEGER, sha256 TEXT REFERENCES payloads, extension TEXT, family TEXT, language TEXT, ui_reason TEXT);
CREATE TABLE exports(sha256 TEXT REFERENCES payloads, export_index INTEGER, name TEXT, class_name TEXT,
 class_ref INTEGER, outer_ref INTEGER, object_path TEXT, serial_offset INTEGER, serial_size INTEGER,
 category TEXT, width INTEGER, height INTEGER, format INTEGER, property_error TEXT,
 PRIMARY KEY(sha256, export_index));
CREATE TABLE imports(sha256 TEXT REFERENCES payloads, import_index INTEGER, object_name TEXT,
 class_name TEXT, class_package TEXT, outer_ref INTEGER, object_path TEXT, PRIMARY KEY(sha256, import_index));
CREATE TABLE assets(id INTEGER PRIMARY KEY, path TEXT, sha256 TEXT, export_index INTEGER, category TEXT,
 ui_reason TEXT, mod_key TEXT, key_error TEXT, UNIQUE(path, sha256, export_index),
 FOREIGN KEY(sha256,export_index) REFERENCES exports(sha256,export_index));
CREATE INDEX files_sha ON files(sha256);
CREATE INDEX files_path ON files(path);
CREATE INDEX exports_category ON exports(category);
CREATE INDEX exports_class ON exports(class_name);
CREATE INDEX assets_category ON assets(category);
CREATE INDEX assets_sha ON assets(sha256);
CREATE INDEX assets_key ON assets(mod_key);
CREATE VIEW catalog AS SELECT a.id,a.path AS package,a.sha256,a.export_index,e.name AS object,
 e.object_path,e.class_name AS cls,a.category,a.ui_reason,a.mod_key,a.key_error,
 e.serial_offset,e.serial_size,e.width,e.height,e.format,e.property_error,
 p.encoding,p.decoded_size FROM assets a JOIN exports e USING(sha256,export_index) JOIN payloads p USING(sha256);
"""


def open_readonly(path):
    connection = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
    connection.row_factory = sqlite3.Row
    return connection


def table_csv(db, path, query, params=()):
    result = db.execute(query, params)
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "wt", encoding="utf-8-sig", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([c[0] for c in result.description])
        writer.writerows(result)


def summarize(db):
    def count(sql): return db.execute(sql).fetchone()[0]
    def rows(sql): return [dict(r) for r in db.execute(sql)]
    return dict(
        file_occurrences=count("SELECT count(*) FROM files"),
        distinct_virtual_paths=count("SELECT count(DISTINCT path) FROM files"),
        unique_payloads=count("SELECT count(*) FROM payloads"),
        package_occurrences=count("SELECT count(*) FROM files JOIN payloads USING(sha256) WHERE status='ok'"),
        unique_packages=count("SELECT count(*) FROM payloads WHERE status='ok'"),
        failed_payloads=count("SELECT count(*) FROM payloads WHERE status='error'"),
        failed_file_occurrences=count("SELECT count(*) FROM files JOIN payloads USING(sha256) WHERE status='error'"),
        unique_content_exports=count("SELECT count(*) FROM exports"),
        unique_content_imports=count("SELECT count(*) FROM imports"),
        source_file_bytes=count("SELECT sum(size) FROM sources"),
        asset_variants=count("SELECT count(*) FROM assets"),
        canonical_mod_keys=count("SELECT count(DISTINCT mod_key) FROM assets WHERE mod_key!=''"),
        keys_with_content_variants=count("SELECT count(*) FROM (SELECT mod_key FROM assets WHERE mod_key!='' GROUP BY mod_key HAVING count(DISTINCT sha256)>1)"),
        invalid_mod_keys=count("SELECT count(*) FROM assets WHERE key_error!=''"),
        export_occurrences=count("SELECT count(*) FROM files JOIN exports USING(sha256)"),
        property_errors=count("SELECT count(*) FROM exports WHERE property_error!=''"),
        ui_asset_variants=count("SELECT count(*) FROM assets WHERE ui_reason!=''"),
        archives=rows("SELECT archive,count(*) occurrences,sum(length) logical_bytes FROM files GROUP BY archive ORDER BY archive"),
        extensions=rows("SELECT extension,count(*) occurrences,count(DISTINCT sha256) unique_payloads,sum(length) logical_bytes FROM files GROUP BY extension ORDER BY occurrences DESC"),
        families=rows("SELECT family,count(*) occurrences,count(DISTINCT path) paths FROM files GROUP BY family ORDER BY occurrences DESC"),
        categories=rows("SELECT e.category,count(*) unique_content_exports,(SELECT count(*) FROM assets a WHERE a.category=e.category) asset_variants FROM exports e GROUP BY e.category ORDER BY unique_content_exports DESC"),
        classes=rows("SELECT class_name,category,count(*) unique_content_exports,sum(serial_size) serialized_bytes FROM exports GROUP BY class_name,category ORDER BY unique_content_exports DESC"),
        languages=rows("SELECT language,count(*) occurrences,count(DISTINCT path) paths FROM files GROUP BY language ORDER BY language"),
        textures=rows("SELECT class_name,width,height,format,count(*) unique_content_exports FROM exports WHERE category='textures' GROUP BY class_name,width,height,format ORDER BY count(*) DESC"),
        statuses=rows("SELECT status,encoding,count(*) unique_payloads FROM payloads GROUP BY status,encoding"),
    )


def report(db, output):
    meta = {r[0]: json.loads(r[1]) for r in db.execute("SELECT * FROM metadata")}
    require(meta.get('complete') is True, "catalog scan/reparse is incomplete; do not publish a partial report")
    require(not output.resolve().is_relative_to(Path(meta['game_root']).resolve()), "report output must be outside game data")
    output.mkdir(parents=True, exist_ok=True)
    summary = summarize(db)
    (output / "summary.json").write_text(json.dumps({"metadata": meta, **summary}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    table_csv(db, output / "files.csv", "SELECT files.*,p.encoding,p.status,p.error FROM files JOIN payloads p USING(sha256) ORDER BY id")
    table_csv(db, output / "failures.csv", "SELECT f.*,p.error FROM files f JOIN payloads p USING(sha256) WHERE p.status='error' ORDER BY f.id")
    table_csv(db, output / "classes.csv", "SELECT class_name,category,count(*) unique_content_exports,sum(serial_size) serialized_bytes FROM exports GROUP BY class_name,category ORDER BY count(*) DESC")
    table_csv(db, output / "texture_dimensions.csv", "SELECT class_name,width,height,format,count(*) unique_content_exports FROM exports WHERE category='textures' GROUP BY class_name,width,height,format ORDER BY count(*) DESC")
    table_csv(db, output / "key_variants.csv", "SELECT mod_key,count(DISTINCT sha256) package_content_variants FROM assets WHERE mod_key!='' GROUP BY mod_key HAVING count(DISTINCT sha256)>1 ORDER BY mod_key")
    nonpackage_counts, nonpackage_unique = Counter(), {}
    with (output / "nonpackage_files.csv").open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["disc", "archive", "path", "offset", "length", "sha256", "magic", "encoding", "kind", "basis"])
        for row in db.execute("SELECT f.disc,f.archive,f.path,f.offset,f.length,f.sha256,p.magic,p.encoding FROM files f JOIN payloads p USING(sha256) WHERE p.status='not_package' ORDER BY f.id"):
            kind, basis = file_kind(row['path'], row['magic'])
            writer.writerow([*row, kind, basis])
            nonpackage_counts[kind] += 1
            nonpackage_unique.setdefault(kind, set()).add(row['sha256'])
    category_files = {}
    for category in summary["categories"]:
        key = category["category"]
        name = key + (".csv.gz" if category['asset_variants'] > 100000 else ".csv")
        category_files[key] = name
        table_csv(db, output / name, "SELECT * FROM catalog WHERE category=? ORDER BY package,export_index,sha256", (key,))
    table_csv(db, output / "ui.csv", "SELECT * FROM catalog WHERE ui_reason!='' ORDER BY package,export_index,sha256")
    def md_table(headers, rows):
        return ["| " + " | ".join(headers) + " |", "| " + " | ".join("---" for _ in headers) + " |"] + [
            "| " + " | ".join(str(c).replace("|", "\\|").replace("\n", " ") for c in row) + " |" for row in rows]
    lines = ["# Lost Odyssey 资源统计 / Asset inventory", "", f"生成时间（UTC）：{datetime.now(timezone.utc).isoformat()}", "",
             "只读扫描全部指定光盘的 FPI 文件树，并按 SHA-256 去重每个完整资源条目。所有 UE3/CPX 候选均尝试解析；未解析包保留在 failures.csv。输出仅有元数据，不包含原始纹理、模型、音频、脚本正文。", "",
             "## 覆盖范围和计数口径", "",
             f"- 光盘：{', '.join(d['disc'] for d in meta.get('discs', []))}；源代码：`{meta.get('source_commit', '')}`。",
             f"- FPI 文件出现次数：**{summary['file_occurrences']:,}**；不同虚拟路径：**{summary['distinct_virtual_paths']:,}**；不同完整条目内容：**{summary['unique_payloads']:,}**。",
             f"- 已解析包：**{summary['package_occurrences']:,}** 个光盘条目 / **{summary['unique_packages']:,}** 个不同内容；失败：**{summary['failed_file_occurrences']:,}** 个条目 / **{summary['failed_payloads']:,}** 个不同内容。",
             f"- 包内 exports：**{summary['unique_content_exports']:,}**（按包内容+索引去重）；按光盘展开：**{summary['export_occurrences']:,}**。Imports：**{summary['unique_content_imports']:,}**，仅作为依赖引用，不加入资源数量。",
             f"- 路径+包内容+export 索引：**{summary['asset_variants']:,}** 个资源变体；有效 Mod keys：**{summary['canonical_mod_keys']:,}**；其中 **{summary['keys_with_content_variants']:,}** 个 key 对应多份包内容。",
             f"- Texture 属性解析错误：**{summary['property_errors']:,}**；不合法的 Mod key：**{summary['invalid_mod_keys']:,}**。异常均保留，不补猜测值。",
             "- 去重单位是完整包内容，不是跨不同包的单个模型/像素内容；无法据此认定有多少个独立角色或视觉上不同的贴图。",
             "- 模型仅包含 mesh 资源类，场景中的 Actor/Component 实例和 BSP/Terrain 单独计数；Default__ 类默认模板也单独保留。",
             "- UI 是用途标签，与 textures/fonts/models 等类型重叠；class 标签来自导出类，其余 UI 标签来自包路径启发式，不代表逐项画面验收。",
             "- 仅覆盖 manifest 指定的本地光盘资料；未扫描未提供的其他地区版、补丁或 DLC，未统计 XEX 内嵌对象、未引用的 FPD 空隙及运行时生成资源。",
             "- 本报告不证明模型/任意游戏纹理已有替换消费者。现有通用 Mod API 的原生菜单 atlas/font-page consumer 范围见仓库 Modding 文档。", "",
             "## 类型", ""]
    lines += md_table(["类别", "去重包内对象", "路径/内容变体", "查询表"], [
        [c['category'], c['unique_content_exports'], c['asset_variants'], f"[{category_files[c['category']]}]({category_files[c['category']]})"] for c in summary['categories']])
    lines += ["", f"UI 用途标签共 {summary['ui_asset_variants']:,} 个变体，见 [ui.csv](ui.csv)，不可再与上表相加。", "", "## 文件类型", ""]
    lines += md_table(["后缀", "光盘条目", "去重内容", "逻辑字节数"], [[r['extension'] or '(none)', r['occurrences'], r['unique_payloads'], r['logical_bytes']] for r in summary['extensions']])
    lines += ["", "## 非 UE3 文件用途", "", "以下按路径/后缀分类，依据见 nonpackage_files.csv；未宣称已解析内部事件、音轨、消息或几何。独立 DDS 与上面的 UE3 Texture exports 使用不同计数单位。", ""]
    lines += md_table(["用途", "光盘条目", "去重内容"], [[kind, count, len(nonpackage_unique[kind])] for kind, count in nonpackage_counts.most_common()])
    summary['nonpackage_kinds'] = [dict(kind=kind, occurrences=count, unique_payloads=len(nonpackage_unique[kind])) for kind, count in nonpackage_counts.most_common()]
    summary['category_files'] = category_files
    (output / "summary.json").write_text(json.dumps({"metadata": meta, **summary}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    lines += ["", "## FPD 容器", ""] + md_table(["容器", "条目数", "逻辑字节数"], [[r['archive'], r['occurrences'], r['logical_bytes']] for r in summary['archives']])
    lines += ["", "## 语言目录", ""] + md_table(["语言标记", "光盘条目", "不同路径"], [[r['language'] or '(non-localized)', r['occurrences'], r['paths']] for r in summary['languages']])
    lines += ["", "## 导出类（全部）", ""] + md_table(["UE3 class", "类别", "去重包内对象", "序列化字节数"], [[r['class_name'], r['category'], r['unique_content_exports'], r['serialized_bytes']] for r in summary['classes']])
    lines += ["", "## 查询与偏移", "",
              "- `catalog.sqlite` 保存 files、payloads、exports、imports、assets、sources 及 catalog 视图。",
              "- 超过 100,000 行的分类 CSV 使用 gzip 压缩（.csv.gz），解压即可作为普通 UTF-8 CSV 打开。",
              "- `files.csv` 包含全部文件（含音频、视频、脚本、配置、未知格式），可用 sha256 关联 catalog/exports/imports。",
              "- `files.offset/length` 是 FPD 内的原始条目位置；`serial_offset/serial_size` 是解包后的 UE3 包内对象位置。CPX 包的两种偏移不能直接相加。",
              "- `class_ref/outer_ref` 是原始 UE3 有符号引用：正数 export+1、负数 import+1、0 空引用。CSV/Mod key 的 export_index 是 0 起始。",
              "- `sources` 的 FPI 和松散文件有完整 SHA-256；FPD 记录大小/mtime，各索引条目单独完整哈希。扫描前后大小/mtime一致并不等于对源盘做了完整镜像校验。",
              "- `format` 是游戏序列化的像素格式整数；保留原值，未解码像素。不推断未知格式。TextureCube 等无直接尺寸属性时使用空值。",
              "- `imports` 是包声明的导入表，不是完整逐对象依赖图；未扫描所有对象 native payload 的引用。",
              "- 同 Mod key 的包内容变体需结合 disc/path/sha256 选定；包内容不同不一定意味着每个 export 的内容不同。", "",
              "```powershell", 'python -B tools/asset_inventory/inventory.py query --db "<output>/catalog.sqlite" --category models --limit 30',
              'python -B tools/asset_inventory/inventory.py query --db "<output>/catalog.sqlite" --search UI_MAIN_00 --ui',
              'python -B tools/asset_inventory/inventory.py query --db "<output>/catalog.sqlite" --search rpmenurescommon --class-name Texture2D', "```", ""]
    (output / "REPORT.md").write_text("\n".join(lines), encoding="utf-8")
    return summary


def scan(args):
    root, output = args.game.resolve(), args.output.resolve()
    require(root.is_dir(), "game root does not exist")
    require(not output.is_relative_to(root), "output must be outside the game data root")
    require(not output.exists(), "output already exists; choose a new directory")
    require(args.decoder.is_file(), "decoder executable is missing")
    discs = [root] if (root / "LO.fpi").is_file() else sorted(d for d in root.glob("disc[1-4]") if d.is_dir())
    require(discs, "no disc*/LO.fpi found")
    entries, disc_info = [], []
    for disc in discs:
        items, info = read_fpi(disc)
        entries.extend(items)
        disc_info.append(info)
    require(len({d['disc_number'] for d in disc_info}) == len(disc_info), "duplicate disc numbers")
    require(args.allow_partial or {d['disc_number'] for d in disc_info} == {1, 2, 3, 4},
            "complete inventory requires discs 1..4; use --allow-partial for explicitly bounded scans")
    output.mkdir(parents=True)
    db = sqlite3.connect(output / "catalog.sqlite")
    db.row_factory = sqlite3.Row
    db.executescript(SCHEMA)
    def meta(key, value):
        db.execute("INSERT OR REPLACE INTO metadata VALUES(?,?)", (key, json.dumps(value, ensure_ascii=False)))
    started = time.monotonic()
    meta("schema_version", 1)
    meta("started_utc", datetime.now(timezone.utc).isoformat())
    meta("game_root", str(root))
    meta("discs", disc_info)
    meta("four_disc_set", {d['disc_number'] for d in disc_info} == {1, 2, 3, 4})
    meta("command", sys.argv)
    meta("source_commit", subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip())
    meta("tool_sha256", {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in (Path(__file__), Path(__file__).with_name("decode.cpp"), args.decoder)})
    originals, loose_files = {}, []
    for disc in discs:
        for path in sorted(disc.rglob("*")):
            if not path.is_file():
                continue
            stat = path.stat()
            originals[path] = (stat.st_size, stat.st_mtime_ns)
            relative = path.relative_to(root).as_posix()
            digest = ""
            if path.name.lower() not in {n.lower() for n in ARCHIVES}:
                with path.open("rb") as file:
                    digest, magic = digest_extent(file, 0, stat.st_size)
                loose_files.append(dict(path=relative, length=stat.st_size, sha256=digest, magic=magic))
            db.execute("INSERT INTO sources VALUES(?,?,?,?)", (relative, stat.st_size, stat.st_mtime_ns, digest))
    (output / "loose_files.json").write_text(json.dumps(loose_files, indent=2) + "\n", encoding="utf-8")
    by_disc = {d.name: d for d in discs}
    seen, handles = set(), {}
    process = subprocess.Popen([str(args.decoder.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               text=True, encoding="utf-8", errors="strict", bufsize=1)
    try:
        for number, entry in enumerate(entries, 1):
            source = by_disc[entry['disc']] / entry['archive']
            if source not in handles:
                handles[source] = source.open("rb")
            digest, magic = digest_extent(handles[source], entry['offset'], entry['length'])
            ext = Path(entry['path']).suffix.lower()
            if digest not in seen:
                result = dict(status="not_package", encoding="raw", exports=[], imports=[])
                if magic.startswith("637078") or magic in {"9e2a83c1", "c1832a9e"} or ext in {".xxx", ".upk", ".umap", ".u"}:
                    require("\t" not in str(source) and "\n" not in str(source), "unsupported source path")
                    process.stdin.write(f"{source}\t{entry['offset']}\t{entry['length']}\n")
                    process.stdin.flush()
                    response = process.stdout.readline()
                    require(response, f"decoder terminated unexpectedly at {entry['path']}")
                    result = json.loads(response)
                    if result['status'] == "not_package" and ext in {".xxx", ".upk", ".umap", ".u"}:
                        result.update(status="error", error="expected UE3 package magic")
                require(result['status'] in {"ok", "error", "not_package"}, "invalid decoder status")
                db.execute("INSERT INTO payloads VALUES(?,?,?,?,?,?,?,?)", (digest, entry['length'], magic,
                           result.get('encoding', ''), result.get('decoded_size'), str(result.get('version', '')),
                           result['status'], result.get('error') or ''))
                if result['status'] == "ok":
                    for obj in result['exports']:
                        db.execute("INSERT INTO exports VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)", (
                            digest, obj['index'], obj['name'], obj['class_name'], obj['class_ref'], obj['outer_ref'],
                            obj['object_path'], obj['serial_offset'], obj['serial_size'], classify(obj['class_name'], obj['name']),
                            obj.get('width'), obj.get('height'), obj.get('format'), obj.get('property_error') or ''))
                    for obj in result['imports']:
                        db.execute("INSERT INTO imports VALUES(?,?,?,?,?,?,?)", (digest, obj['index'], obj['object_name'],
                                   obj['class_name'], obj['class_package'], obj['outer_ref'], obj['object_path']))
                seen.add(digest)
            db.execute("INSERT INTO files VALUES(?,?,?,?,?,?,?,?,?,?,?)", (number, entry['disc'], entry['archive'], entry['path'],
                       entry['offset'], entry['length'], digest, ext, family(entry['path']), language(entry['path']), ui_reason(entry['path'])))
            if number % 1000 == 0:
                db.commit()
                print(f"{number}/{len(entries)} file occurrences; {len(seen)} distinct payloads; {time.monotonic()-started:.1f}s", flush=True)
        process.stdin.close()
        require(process.wait(timeout=30) == 0, "decoder exited unsuccessfully")
        for path, original in originals.items():
            stat = path.stat()
            require((stat.st_size, stat.st_mtime_ns) == original, f"source changed during scan: {path}")
        # One row per virtual path and complete package content, retaining same-key variants.
        for row in db.execute("SELECT DISTINCT f.path,e.sha256,e.export_index,e.name,e.class_name,e.category FROM files f JOIN exports e USING(sha256)"):
            try:
                key, error = make_key(row['path'], row['export_index'], row['name']), ""
            except ValueError as exc:
                key, error = "", str(exc)
            db.execute("INSERT INTO assets(path,sha256,export_index,category,ui_reason,mod_key,key_error) VALUES(?,?,?,?,?,?,?)", (
                row['path'], row['sha256'], row['export_index'], row['category'], ui_reason(row['path'], row['class_name']), key, error))
        require(not db.execute("PRAGMA foreign_key_check").fetchall(), "catalog relationship mismatch")
        meta("source_stat_check", "passed")
        meta("finished_utc", datetime.now(timezone.utc).isoformat())
        meta("elapsed_seconds", round(time.monotonic() - started, 3))
        meta("complete", True)
        db.commit()
        summary = report(db, output)
        print(json.dumps({k: v for k, v in summary.items() if not isinstance(v, list)}, ensure_ascii=False, indent=2))
        return 2 if summary['failed_payloads'] or summary['property_errors'] or summary['invalid_mod_keys'] else 0
    finally:
        for handle in handles.values():
            handle.close()
        if process.poll() is None:
            process.kill()
            process.wait()
        db.close()


def reparse(args):
    """Recheck package metadata after decoder changes, without rehashing movies/audio."""
    require(args.db.is_file() and args.decoder.is_file(), "existing catalog and decoder are required")
    db = sqlite3.connect(args.db)
    db.row_factory = sqlite3.Row
    process = None
    try:
        metadata = {r[0]: json.loads(r[1]) for r in db.execute("SELECT * FROM metadata")}
        require(metadata.get('schema_version') == 1, "unsupported catalog schema")
        require(sum(d['files'] for d in metadata['discs']) == db.execute("SELECT count(*) FROM files").fetchone()[0],
                "initial file scan is incomplete; reparse cannot discover missing files")
        root = Path(metadata['game_root'])
        sources = list(db.execute("SELECT * FROM sources"))
        def check_sources():
            for row in sources:
                source = root / row['path']
                stat = source.stat()
                require((stat.st_size, stat.st_mtime_ns) == (row['size'], row['mtime_ns']), f"source changed: {source}")
        check_sources()
        pending = "(p.status='error' OR (p.status='not_package' AND p.magic LIKE '637078%' AND p.encoding!='cpx'))"
        condition = pending if args.pending_only else f"(p.status='ok' OR {pending})"
        candidates = list(db.execute("SELECT f.*,p.status previous_status FROM files f JOIN (SELECT sha256,min(id) id FROM files GROUP BY sha256) first USING(id) JOIN payloads p ON p.sha256=f.sha256 WHERE " + condition + " ORDER BY f.id"))
        process = subprocess.Popen([str(args.decoder.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   text=True, encoding="utf-8", bufsize=1)
        db.execute("INSERT OR REPLACE INTO metadata VALUES('complete','false')")
        db.execute("CREATE INDEX IF NOT EXISTS exports_class ON exports(class_name)")
        db.execute("CREATE INDEX IF NOT EXISTS assets_sha ON assets(sha256)")
        db.commit()
        changed = 0
        for number, row in enumerate(candidates, 1):
            source = root / row['disc'] / row['archive'] if not (root / 'LO.fpi').is_file() else root / row['archive']
            require('\t' not in str(source) and '\n' not in str(source), "unsupported source path")
            with source.open('rb') as file:
                digest, _ = digest_extent(file, row['offset'], row['length'])
            require(digest == row['sha256'], f"content no longer matches catalog: {row['path']}")
            process.stdin.write(f"{source}\t{row['offset']}\t{row['length']}\n")
            process.stdin.flush()
            response = process.stdout.readline()
            require(response, "decoder terminated during reparse")
            result = json.loads(response)
            if result['status'] == 'not_package' and (row['previous_status'] != 'not_package' or row['extension'] in {'.xxx', '.upk', '.umap', '.u'}):
                result.update(status='error', error='previous package candidate is not recognized')
            require(result['status'] in {'ok', 'error', 'not_package'}, "invalid decoder status")
            new_exports, new_imports = [], []
            if result['status'] == 'ok':
                for obj in result['exports']:
                    new_exports.append((digest, obj['index'], obj['name'], obj['class_name'], obj['class_ref'], obj['outer_ref'],
                                        obj['object_path'], obj['serial_offset'], obj['serial_size'], classify(obj['class_name'], obj['name']),
                                        obj.get('width'), obj.get('height'), obj.get('format'), obj.get('property_error') or ''))
                for obj in result['imports']:
                    new_imports.append((digest, obj['index'], obj['object_name'], obj['class_name'], obj['class_package'], obj['outer_ref'], obj['object_path']))
            old_exports = [tuple(r) for r in db.execute("SELECT * FROM exports WHERE sha256=? ORDER BY export_index", (digest,))]
            old_imports = [tuple(r) for r in db.execute("SELECT * FROM imports WHERE sha256=? ORDER BY import_index", (digest,))]
            if new_exports != old_exports or new_imports != old_imports:
                changed += 1
                db.execute("DELETE FROM assets WHERE sha256=?", (digest,))
                db.execute("DELETE FROM exports WHERE sha256=?", (digest,))
                db.execute("DELETE FROM imports WHERE sha256=?", (digest,))
                db.executemany("INSERT INTO exports VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)", new_exports)
                db.executemany("INSERT INTO imports VALUES(?,?,?,?,?,?,?)", new_imports)
                paths = [r[0] for r in db.execute("SELECT DISTINCT path FROM files WHERE sha256=?", (digest,))]
                for path in paths:
                    for obj in new_exports:
                        try:
                            key, error = make_key(path, obj[1], obj[2]), ''
                        except ValueError as exc:
                            key, error = '', str(exc)
                        db.execute("INSERT INTO assets(path,sha256,export_index,category,ui_reason,mod_key,key_error) VALUES(?,?,?,?,?,?,?)",
                                   (path, digest, obj[1], obj[9], ui_reason(path, obj[3]), key, error))
            db.execute("UPDATE payloads SET encoding=?,decoded_size=?,version=?,status=?,error=? WHERE sha256=?",
                       (result.get('encoding'), result.get('decoded_size'), str(result.get('version', '')), result['status'], result.get('error') or '', digest))
            if number % 500 == 0:
                db.commit()
                print(f"reparsed {number}/{len(candidates)} packages; {changed} changed metadata", flush=True)
        process.stdin.close()
        require(process.wait(timeout=30) == 0, "decoder exited unsuccessfully")
        check_sources()
        require(not db.execute("PRAGMA foreign_key_check").fetchall(), "catalog relationship mismatch")
        evidence = dict(finished_utc=datetime.now(timezone.utc).isoformat(), packages=len(candidates), changed_metadata=changed,
                        decoder_sha256=hashlib.sha256(args.decoder.read_bytes()).hexdigest(),
                        source_sha256=hashlib.sha256(Path(__file__).with_name('decode.cpp').read_bytes()).hexdigest(),
                        inventory_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), source_stat_check='passed')
        db.execute("INSERT OR REPLACE INTO metadata VALUES('reparse',?)", (json.dumps(evidence),))
        db.execute("INSERT OR REPLACE INTO metadata VALUES('complete','true')")
        db.commit()
        print(json.dumps(evidence, indent=2))
        return 2 if db.execute("SELECT count(*) FROM payloads WHERE status='error'").fetchone()[0] else 0
    finally:
        if process and process.poll() is None:
            process.kill()
            process.wait()
        db.close()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    scan_parser = sub.add_parser("scan")
    scan_parser.add_argument("--game", type=Path, required=True)
    scan_parser.add_argument("--output", type=Path, required=True)
    scan_parser.add_argument("--decoder", type=Path, required=True)
    scan_parser.add_argument("--allow-partial", action="store_true", help="explicitly allow fewer than all four discs")
    report_parser = sub.add_parser("report")
    report_parser.add_argument("--db", type=Path, required=True)
    report_parser.add_argument("--output", type=Path, required=True)
    reparse_parser = sub.add_parser("reparse", help="revalidate all package payloads with an updated decoder, retaining non-package hashes")
    reparse_parser.add_argument("--db", type=Path, required=True)
    reparse_parser.add_argument("--decoder", type=Path, required=True)
    reparse_parser.add_argument("--pending-only", action="store_true", help="retry failures and previously undecoded CPX signatures only")
    query_parser = sub.add_parser("query")
    query_parser.add_argument("--db", type=Path, required=True)
    query_parser.add_argument("--category")
    query_parser.add_argument("--class-name")
    query_parser.add_argument("--search", default="")
    query_parser.add_argument("--ui", action="store_true")
    query_parser.add_argument("--limit", type=int, default=30)
    args = parser.parse_args(argv)
    try:
        if args.command == "scan":
            return scan(args)
        if args.command == "reparse":
            return reparse(args)
        with closing(open_readonly(args.db)) as db:
            if args.command == "report":
                require(not args.output.exists(), "report output exists; choose a new directory")
                report(db, args.output)
                return 0
            require(1 <= args.limit <= 10000, "limit must be 1..10000")
            where, params = [], []
            for name in ("category", "class_name"):
                value = getattr(args, name)
                if value:
                    where.append(("cls" if name == "class_name" else name) + "=?")
                    params.append(value)
            if args.ui:
                where.append("ui_reason!=''")
            if args.search:
                where.append("(instr(lower(package),lower(?))>0 OR instr(lower(object_path),lower(?))>0)")
                params.extend([args.search] * 2)
            sql = "SELECT * FROM catalog" + (" WHERE " + " AND ".join(where) if where else "")
            for row in db.execute(sql + " ORDER BY package,export_index,sha256 LIMIT ?", [*params, args.limit]):
                value = dict(row)
                value['sources'] = [dict(r) for r in db.execute("SELECT disc,archive,offset,length FROM files WHERE path=? AND sha256=? ORDER BY disc", (row['package'], row['sha256']))]
                print(json.dumps(value, ensure_ascii=False))
            return 0
    except (ValueError, OSError, sqlite3.Error, subprocess.SubprocessError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
