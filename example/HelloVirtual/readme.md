# HelloVirtual

`HelloVirtual` 用来集中承载所有 virtual 相关例程，不再把这类示例混在 `HelloBasic` 里。

## 子例程

基础容器：

- `virtual_viewport_basic`
- `virtual_page_basic`
- `virtual_strip_basic`
- `virtual_grid_basic`
- `virtual_section_list_basic`
- `virtual_tree_basic`
- `virtual_stage_basic`

完整场景：

- `virtual_viewport`
- `virtual_page`
- `virtual_strip`
- `virtual_grid`
- `virtual_section_list`
- `virtual_tree`
- `virtual_stage`
- `virtual_stage_showcase`

高层包装：

- `list_view_basic`
- `list_view`
- `grid_view_basic`
- `grid_view`

## 构建

PC 运行：

```bash
make all APP=HelloVirtual APP_SUB=virtual_stage_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --keep-screenshots
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_basic --keep-screenshots
python scripts/hello_basic_render_workflow.py --app HelloVirtual --suite basic --skip-unit-tests --bits64
```

`stm32g0` 体积对比：

```bash
make -j1 all APP=HelloShowcase PORT=stm32g0
make -j1 all APP=HelloVirtual APP_SUB=virtual_stage_showcase PORT=stm32g0
python scripts/perf_analysis/compare_virtual_showcase_ram.py --skip-build
```

QEMU heap 对比：

```bash
python scripts/perf_analysis/compare_virtual_showcase_heap_qemu.py --mode app-recording
```

## 推荐阅读顺序

1. 先看 `virtual_viewport_basic`、`virtual_page_basic`、`virtual_grid_basic` 这类 basic case。
2. 再看 `list_view_basic`、`grid_view_basic`，理解高层 `ViewHolder + DataModel` 包装。
3. 最后看 `virtual_stage_basic`、`virtual_stage_showcase`、`virtual_stage`。
