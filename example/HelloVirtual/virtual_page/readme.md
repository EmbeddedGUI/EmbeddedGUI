# Virtual Page 示例

## 这个控件适合什么

`egui_view_virtual_page_t` 适合“由很多异构 section 组成的一整页内容”，而不是重复 row 的长列表。

如果你是第一次接触这个控件，建议先看：

- `example/HelloVirtual/virtual_page_basic/`

`virtual_page_basic` 只保留一个 header、一个操作条和三种 section 类型，重点演示最小接入闭环：`init_with_setup`、点击回查 section、单项 patch、单项 resize 通知和 jump / reset。

这个例程把 280 个 section 组织成 4 类页面模块：

- `Overview`：页首摘要块，接近 dashboard hero
- `Metric`：窄 KPI tile，左右错位摆放
- `Alert`：横向告警条，强调状态和进度
- `Checklist`：表单 / checklist 模块，强调字段骨架和完成度

重点不是“很多 row”，而是“很多 page section 只在进入可视区时才创建和绑定”。

## 这个例程在展示什么

- section 只有进入可视区后才创建、绑定、绘制
- 不同 section 的高度、宽度、横向位置都可以不同
- section 被点击后，可以反查命中的 `index / stable_id`
- `Patch` 会修改 section 的内容、状态和布局
- 通过 `keepalive + state cache` 保住 pulse 动画状态，避免复用时丢失
- 录制动作覆盖了滚动到中段、点击可见模块、再 patch 的过程，方便看滚动后和选中后的渲染

## 为什么它看起来不应该像 list

这个示例故意让不同模块拥有不同的页面语义：

- `Overview` 是宽模块，像页首摘要
- `Metric` 是窄块，强调局部指标，不占满整行
- `Alert` 是横幅式模块，视觉重点是风险条
- `Checklist` 是右偏 / 中偏的表单块，内部是字段骨架

如果你的业务语义已经是 “page / section / module”，优先用 `virtual_page`，不要再硬塞成 `virtual_list` 的 row。

## 什么时候用 `virtual_page`

适合：

- dashboard
- 设置页中的大分区
- 详情页里的多块异构内容
- 很长的配置页 / 表单页

不太适合：

- 只是重复行：优先 `virtual_list`
- 有 group header：优先 `virtual_section_list`
- 有父子层级和折叠展开：优先 `virtual_tree`

## 运行方式

```bash
make -j1 all APP=HelloVirtual APP_SUB=virtual_page PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_page --keep-screenshots
```

截图输出目录：

```bash
runtime_check_output/HelloVirtual_virtual_page/default/
```

## 接入方式

推荐直接用 `setup` 一次性配置参数、数据源和状态缓存：

```c
static egui_view_virtual_page_t page_view;
static app_context_t page_ctx;

static const egui_view_virtual_page_params_t page_params = {
        .region = {{8, 48}, {224, 264}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_section_height = 118,
};

static const egui_view_virtual_page_data_source_t page_source = {
        .get_count = page_get_count,
        .get_stable_id = page_get_stable_id,
        .find_index_by_stable_id = page_find_index,
        .measure_section_height = page_measure_height,
        .create_section_view = page_create_view,
        .bind_section_view = page_bind_view,
        .unbind_section_view = page_unbind_view,
        .should_keep_alive = page_should_keep_alive,
        .save_section_state = page_save_state,
        .restore_section_state = page_restore_state,
};

static const egui_view_virtual_page_setup_t page_setup = {
        .params = &page_params,
        .data_source = &page_source,
        .data_source_context = &page_ctx,
        .state_cache_max_entries = 64,
        .state_cache_max_bytes = 64 * sizeof(my_section_state_t),
};

egui_view_virtual_page_init_with_setup(EGUI_VIEW_OF(&page_view), &page_setup);
```

## 点击、改动和定位

点击 section 后，可以直接反查命中的模块：

```c
static void page_click_cb(egui_view_t *self)
{
    egui_view_virtual_page_entry_t entry;

    if (!egui_view_virtual_page_resolve_section_by_view(EGUI_VIEW_OF(&page_view), self, &entry))
    {
        return;
    }

    focus_section(entry.index, entry.stable_id);
}
```

如果 section 改动会影响高度，需要显式通知：

```c
egui_view_virtual_page_notify_section_resized_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id);
```

如果只是希望把目标模块滚到可视安全区：

```c
egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&page_view), stable_id, inset);
```

## 设计提醒

- `virtual_page` 的最小单元是 section，不是 row
- 不同 variant 的差异应该体现在模块结构、宽度和锚点，而不只是换背景色
- 有短时动画或选中态时，优先把活动 view 放入 `keepalive`
- 可恢复状态放进 `state cache`，避免 view 回收后状态丢失

## 相关文件

- `example/HelloVirtual/virtual_page/test.c`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_page.c`
- `src/widget/egui_view_virtual_viewport.h`
