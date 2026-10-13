from __future__ import annotations

import argparse
import csv
import json
import re
import shutil
import zipfile
from collections import Counter
from pathlib import Path

import ctranslate2
import sentencepiece as spm
from lxml import etree


CJK_RE = re.compile(r"[\u3400-\u9fff]")
TARGET_PART_PREFIXES = (
    "ppt/slides/",
    "ppt/notesSlides/",
    "ppt/slideLayouts/",
    "ppt/slideMasters/",
    "ppt/charts/",
    "ppt/diagrams/",
    "ppt/comments/",
)


EXACT_TRANSLATIONS = {
    "灵巧手训练营": "Dexterous Hand Training Camp",
    "灵巧手大揭秘": "Unveiling the Dexterous Hand",
    "灵巧手的": "of the Dexterous Hand",
    "灵巧手": "Dexterous Hand",
    "肌肉": "Muscles",
    "串口与": "Serial Port and",
    "通信控制舵机实战": "Hands-on Servo Control via Communication",
    "串口与I2C通信控制舵机实战": "Hands-on Servo Control via Serial Port and I2C",
    "解锁机械原理的奥秘": "Unlocking the Secrets of Mechanical Principles",
    "平面机构自由度": "Degrees of Freedom of Planar Mechanisms",
    "机械魔法与骨架组装": "Mechanical Magic and Skeleton Assembly",
    "代码实战": "Hands-on Coding",
    "课后作业": "Homework",
    "作业": "Homework",
    "模块": "Module",
    "导师：岳国文老师": "Instructor: Yue Guowen",
    "助手：胡源涛老师": "Assistant: Hu Yuantao",
    "转动副": "Revolute Joint",
    "核心代码": "Core Code",
    "串口通信": "Serial Communication",
    "核心定义": "Core Definition",
    "人体类比：": "Human-body analogy:",
    "小鹅通内部直播  本周四 ": "Internal livestream on Xiaoetong — This Thursday",
    "脉冲": "Pulse",
    "连接": "Connection",
    "与": "and",
    "硬件核心：ESP32（机器人的“大脑”）": "Hardware Core: ESP32 (the Robot's “Brain”)",
    "烧录": "Upload Firmware",
    "使用": "Use",
    "的": "of",
    "手": "Hand",
    "第": "Lesson ",
    "节课：": ":",
    "步骤：": "Steps:",
    "通过": "via",
    "度位置": "° Position",
    "总结与拓展": "Summary and Extension",
    "小伙伴": "Students",
    "系统解刨": "System Anatomy",
    "骨骼": "Skeleton",
    "神经与大脑": "Nerves and Brain",
    "舵机的工作原理简析": "How a Servo Motor Works",
    "平面运动副的约束规律": "Constraint Rules of Planar Kinematic Pairs",
    "机构具有确定运动的条件": "Conditions for Definite Mechanism Motion",
    "到": "to",
    "学习": "Learning",
    "特点：": "Features:",
    "作用：": "Function:",
    "关节": "Joint",
    "控制": "Control",
    "和": "and",
    "年代：": "Era:",
    "指令格式：": "Command Format:",
    "运动副": "Kinematic Pair",
    "思考与探究": "Think and Explore",
    "关键特点": "Key Features",
    "从动件": "Driven Link",
    "机构运动的": "of Mechanism Motion",
    "隐藏密码": "Hidden Code",
    "核心作用": "Core Function",
    "专用对讲机": "Dedicated Intercom",
    "实战准备": "Hands-on Preparation",
    "代码解析": "Code Walkthrough",
    "从幻想走进现实，亲手触摸未来": "From Imagination to Reality—Build the Future with Your Own Hands",
    "童年时代": "Childhood",
    "少年时代": "Adolescence",
    "会拥抱物体的灵巧手": "A Dexterous Hand That Can Embrace Objects",
    "：青年时代": ": Young Adulthood",
    "长出触觉的灵巧手": "A Dexterous Hand with a Sense of Touch",
    "全能冠军：": "All-round Champion:",
    "力量王者：": "Power Champion:",
    "机器人宇航员": "Robot Astronaut",
    "总部": "Headquarters",
    "关节与韧带": "Joints and Ligaments",
    "核心": "Core",
    "我的手": "My Hand",
    "观察报告": "Observation Report",
    "画下你一只手的轮廓。": "Trace the outline of one of your hands.",
    "在图上标出关节的位置。": "Mark the joint locations on the drawing.",
    "在旁边画出灵巧手的一根手指骨架。": "Draw the skeleton of one dexterous-hand finger beside it.",
    "摄像头遥控机器手": "Camera-Controlled Robotic Hand",
    "个舵机（": " servos (",
    "课程的最后，我们来回顾一下今天的知识地图。我们从最基础的零件，到能运动的构件，再到连接它们的运动副，组合成运动链，最后固定机架形成能完成特定功能的机构。这些看似简单的原理，却是现代科技的基石。从航天飞机的机械臂，到工厂里的机器人，再到我们坐的高铁，都离不开它们。希望这次课程，能让大家感受到机械世界的魅力！": "At the end of the course, let's review today's knowledge map. We began with basic parts, moved to movable links, connected them with kinematic pairs to form kinematic chains, and finally fixed a frame to create mechanisms that perform specific functions. These simple principles underpin modern technology—from Space Shuttle robotic arms and factory robots to high-speed trains. I hope this course has shown you the appeal of the mechanical world!",
    "首先是数据类型，比如int代表整数，用来存储角度。然后是变量定义，用来存储程序运行中会变化的值，和固定不变的常量区分开": "First come data types: for example, int represents integers and can store angles. Next are variables, which store values that may change while the program runs, unlike fixed constants.",
}


# Longest terms are masked first so technical vocabulary is not mistranslated.
TERM_TRANSLATIONS = {
    "平面机构自由度": "Degrees of Freedom of Planar Mechanisms",
    "平面运动副": "Planar Kinematic Pair",
    "机械原理": "Mechanical Principles",
    "串口通信": "Serial Communication",
    "通信控制": "Communication Control",
    "灵巧手": "Dexterous Hand",
    "机械手指": "Robotic Finger",
    "机器手": "Robotic Hand",
    "舵机": "Servo Motor",
    "运动副": "Kinematic Pair",
    "转动副": "Revolute Joint",
    "移动副": "Prismatic Joint",
    "平面机构": "Planar Mechanism",
    "自由度": "Degrees of Freedom",
    "连杆机构": "Linkage",
    "连杆": "Link",
    "曲柄": "Crank",
    "摇杆": "Rocker",
    "构件": "Link",
    "高副": "Higher Pair",
    "低副": "Lower Pair",
    "机架": "Frame",
    "主动件": "Input Link",
    "从动件": "Driven Link",
    "约束": "Constraint",
    "串口": "Serial Port",
    "摄像头": "Camera",
    "机器人": "Robot",
    "舵盘": "Servo Horn",
    "杜邦线": "Dupont Wire",
    "面包板": "Breadboard",
    "脉冲宽度": "Pulse Width",
    "脉冲": "Pulse",
    "角度": "Angle",
    "位置": "Position",
    "旋转": "Rotation",
    "指令": "Command",
    "代码": "Code",
    "程序": "Program",
    "硬件": "Hardware",
    "软件": "Software",
    "关节": "Joint",
    "韧带": "Ligament",
    "肌肉": "Muscle",
    "骨骼": "Skeleton",
}


def preserve_outer_whitespace(source: str, translated: str) -> str:
    leading = source[: len(source) - len(source.lstrip())]
    trailing = source[len(source.rstrip()) :]
    return leading + translated.strip() + trailing


def mask_terms(text: str):
    masked = text
    markers = []
    for term, english in sorted(TERM_TRANSLATIONS.items(), key=lambda kv: len(kv[0]), reverse=True):
        if term not in masked:
            continue
        marker = f"XQZ{len(markers):03d}ZXQ"
        masked = masked.replace(term, f" {marker} ")
        markers.append((marker, english))
    masked = re.sub(r"\s+", " ", masked).strip()
    return masked, markers


def restore_terms(text: str, markers):
    restored = text
    for marker, english in markers:
        index = re.search(r"\d{3}", marker).group(0)
        # Marian occasionally drops one boundary letter or inserts spaces inside
        # an all-caps marker. Match the stable numeric core plus surrounding caps.
        spaced_index = r"\s*".join(map(re.escape, index))
        loose_pattern = rf"(?<![A-Za-z0-9])[A-Z]{{1,12}}\s*{spaced_index}\s*[A-Z]{{1,12}}(?![A-Za-z0-9])"
        restored = re.sub(loose_pattern, english, restored, flags=re.IGNORECASE)
    restored = restored.replace(" ⁇ ", " ").replace("⁇", "")
    restored = re.sub(r"\b(rudder|steering gear|steering engine)\b", "servo motor", restored, flags=re.I)
    restored = re.sub(r"\bsmart[- ]hand\b", "dexterous hand", restored, flags=re.I)
    restored = re.sub(r"\bkinmatic\b", "kinematic", restored, flags=re.I)
    restored = re.sub(r"\s+([,.;:!?])", r"\1", restored)
    restored = re.sub(r"\s{2,}", " ", restored).strip()
    return restored


def is_target_part(name: str) -> bool:
    return name.endswith(".xml") and name.startswith(TARGET_PART_PREFIXES)


def collect_texts(pptx_paths):
    counts = Counter()
    occurrences = Counter()
    for path in pptx_paths:
        with zipfile.ZipFile(path) as zf:
            for name in zf.namelist():
                if not is_target_part(name):
                    continue
                try:
                    root = etree.fromstring(zf.read(name))
                except etree.XMLSyntaxError:
                    continue
                for node in root.iter():
                    if node.text and CJK_RE.search(node.text):
                        counts[node.text] += 1
                        occurrences[path.name] += 1
    return counts, occurrences


class OfflineTranslator:
    def __init__(self, model_dir: Path):
        self.model_dir = model_dir
        self.source_sp = spm.SentencePieceProcessor(model_file=str(model_dir / "source.spm"))
        self.target_sp = spm.SentencePieceProcessor(model_file=str(model_dir / "target.spm"))
        self.translator = ctranslate2.Translator(str(model_dir), device="cpu", compute_type="int8")

    def translate_many(self, texts, batch_size=64):
        prepared = []
        for text in texts:
            stripped = text.strip()
            masked, markers = mask_terms(stripped)
            prepared.append((text, stripped, masked, markers))

        outputs = {}
        for start in range(0, len(prepared), batch_size):
            batch = prepared[start : start + batch_size]
            token_batches = [self.source_sp.encode(item[2], out_type=str) + ["</s>"] for item in batch]
            max_length = max(64, min(256, max(len(tokens) * 4 for tokens in token_batches)))
            results = self.translator.translate_batch(
                token_batches,
                beam_size=4,
                max_decoding_length=max_length,
            )
            for (source, stripped, _masked, markers), result in zip(batch, results):
                translated = self.target_sp.decode(result.hypotheses[0])
                translated = restore_terms(translated, markers)
                if not re.search(r"[。！？；：]$", stripped) and len(stripped) <= 28:
                    translated = translated.rstrip(".")
                outputs[source] = preserve_outer_whitespace(source, translated)
            print(f"Translated {min(start + batch_size, len(prepared))}/{len(prepared)} unique strings", flush=True)
        return outputs


def build_mapping(text_counts, model_dir: Path, cache_path: Path):
    cache = {}
    if cache_path.exists():
        cache = json.loads(cache_path.read_text(encoding="utf-8"))

    mapping = {}
    pending = []
    for source in text_counts:
        stripped = source.strip()
        if stripped in EXACT_TRANSLATIONS:
            mapping[source] = preserve_outer_whitespace(source, EXACT_TRANSLATIONS[stripped])
        elif source in cache:
            _masked, markers = mask_terms(stripped)
            fixed = restore_terms(cache[source], markers)
            mapping[source] = preserve_outer_whitespace(source, fixed)
            cache[source] = mapping[source]
        else:
            pending.append(source)

    if pending:
        translator = OfflineTranslator(model_dir)
        new_values = translator.translate_many(pending)
        mapping.update(new_values)
        cache.update(new_values)
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        cache_path.write_text(json.dumps(cache, ensure_ascii=False, indent=2), encoding="utf-8")
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    cache_path.write_text(json.dumps(cache, ensure_ascii=False, indent=2), encoding="utf-8")
    return mapping


def translate_package(source: Path, destination: Path, mapping):
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(source, "r") as zin, zipfile.ZipFile(destination, "w") as zout:
        for info in zin.infolist():
            data = zin.read(info.filename)
            if is_target_part(info.filename):
                try:
                    root = etree.fromstring(data)
                    changed = False
                    for node in root.iter():
                        if node.text in mapping:
                            node.text = mapping[node.text]
                            changed = True
                    if changed:
                        data = etree.tostring(
                            root,
                            encoding="UTF-8",
                            xml_declaration=True,
                            standalone=True,
                        )
                except etree.XMLSyntaxError:
                    pass
            zout.writestr(info, data)


def write_report(path: Path, text_counts, mapping):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f, delimiter="\t", lineterminator="\n")
        writer.writerow(["count", "source", "translation"])
        for source, count in text_counts.most_common():
            writer.writerow([count, source, mapping[source]])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("model_dir", type=Path)
    parser.add_argument("--cache", type=Path, default=Path("work/offline_translation_cache.json"))
    parser.add_argument("--report", type=Path, default=Path("work/offline_translation_report.tsv"))
    args = parser.parse_args()

    pptx_paths = sorted(
        [p for p in args.root.glob("*.pptx") if not p.name.startswith("~$") and "_English" not in p.stem],
        key=lambda p: p.name.casefold(),
    )
    if not pptx_paths:
        raise SystemExit("No source PPTX files found")

    text_counts, occurrences = collect_texts(pptx_paths)
    print(f"Found {len(pptx_paths)} decks and {len(text_counts)} unique Chinese strings", flush=True)
    for name, count in sorted(occurrences.items()):
        print(f"  {name}: {count} Chinese text nodes", flush=True)

    mapping = build_mapping(text_counts, args.model_dir, args.cache)
    write_report(args.report, text_counts, mapping)

    for source in pptx_paths:
        destination = args.output_dir / f"{source.stem}_English.pptx"
        translate_package(source, destination, mapping)
        print(f"Wrote {destination}", flush=True)


if __name__ == "__main__":
    main()
