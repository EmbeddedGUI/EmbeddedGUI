#!/usr/bin/env python
# -*- coding: utf-8 -*-

import hashlib
import json
import os
from pathlib import Path


def normalize_user_cflags(user_cflags):
    return " ".join((user_cflags or "").split())


def get_variant_build_output_dir(project_root, report_key, variant_name, user_cflags=""):
    payload = {
        "report": report_key,
        "variant": variant_name,
        "user_cflags": normalize_user_cflags(user_cflags),
    }
    signature = hashlib.sha1(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()[:10]
    return Path(project_root) / "output" / "size_variant_builds" / report_key / signature


def get_variant_obj_suffix(prefix, variant_suffix):
    return "%s_%s" % (prefix, variant_suffix)


def get_auto_parallel_jobs(total_variants):
    if total_variants <= 1:
        return 1
    cpu_count = os.cpu_count() or 1
    return max(1, min(total_variants, 4, cpu_count // 4 or 1))


def resolve_parallel_jobs(requested_jobs, total_variants):
    if requested_jobs < 0:
        raise ValueError("--jobs must be >= 0")
    if total_variants <= 1:
        return 1
    if requested_jobs > 0:
        return max(1, min(total_variants, requested_jobs))
    return get_auto_parallel_jobs(total_variants)


def resolve_make_jobs(parallel_jobs):
    cpu_count = os.cpu_count() or 1
    return max(1, cpu_count // max(1, parallel_jobs))
