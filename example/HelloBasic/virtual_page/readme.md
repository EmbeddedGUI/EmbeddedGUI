# Virtual Page 示例

## 这个示例展示什么

这个例程用 `egui_view_virtual_page_t` 演示“长页面 / 多 section 容器”的真实用法，不再只是列表行复用：

- 280 个 section 的长页面
- 不同 section 高度和视觉样式
- 点击 section 后展开，确认选中了哪一项
- `Add / Del / Patch / Jump` 模拟页面内容变更
- `stable_id` 定位与滚动
- 脉冲动画 + keepalive + 状态袋恢复

适合的业务场景：

- dashboard
- 设置页
- 表单页
- 详情页里的多 section 内容流

## 怎么运行

```bash
make -j1 all APP=HelloBasic APP_SUB=virtual_page PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_page --keep-screenshots
```

## 代码结构

- 顶部固定区域：
  - 状态卡片，显示当前选中 section 和最后一次动作
  - 工具栏按钮，触发增删改跳转
- 中间滚动区域：
  - `virtual_page`
  - 每个 section 只有进入可见区时才真正创建 view

核心文件：

- `example/HelloBasic/virtual_page/test.c`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_page.c`
- `src/widget/egui_view_virtual_viewport.h`

## 最小接入方式

```c
static egui_view_virtual_page_t dashboard_page;

static const egui_view_virtual_page_data_source_t dashboard_sections = {
        .get_count = dashboard_get_count,
        .get_stable_id = dashboard_get_stable_id,
        .find_index_by_stable_id = dashboard_find_index,
        .measure_section_height = dashboard_measure_height,
        .create_section_view = dashboard_create_view,
        .bind_section_view = dashboard_bind_view,
        .unbind_section_view = dashboard_unbind_view,
        .should_keep_alive = dashboard_should_keep_alive,
        .save_section_state = dashboard_save_state,
        .restore_section_state = dashboard_restore_state,
};

EGUI_VIEW_VIRTUAL_PAGE_PARAMS_INIT(dashboard_page_params, 8, 48, 224, 264);

egui_view_virtual_page_init_with_params(EGUI_VIEW_OF(&dashboard_page), &dashboard_page_params);
egui_view_virtual_page_set_data_source(EGUI_VIEW_OF(&dashboard_page), &dashboard_sections, &dashboard_ctx);
egui_view_virtual_page_set_estimated_section_height(EGUI_VIEW_OF(&dashboard_page), 90);
egui_view_virtual_page_set_keepalive_limit(EGUI_VIEW_OF(&dashboard_page), 4);
egui_view_virtual_page_set_state_cache_limits(EGUI_VIEW_OF(&dashboard_page), 64, 64 * sizeof(my_section_state_t));
```

## 使用建议

### 1. section 必须有稳定 `stable_id`

这样才能安全使用：

- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`
- `egui_view_virtual_page_write_section_state()`
- `egui_view_virtual_page_read_section_state()`

### 2. 高度会变时要主动通知

如果点击后 section 会展开、折叠或切换样式，业务层要显式通知：

```c
egui_view_virtual_page_notify_section_resized(EGUI_VIEW_OF(&dashboard_page), index);
```

### 3. keepalive 和状态袋分工不同

- `keepalive`：短时间内必须保留同一个 view 实例
- 状态袋：允许 view 回收，但希望动画进度或临时状态能恢复

这个示例里：

- 选中的 section 进入 keepalive
- 脉冲动画进度存进状态袋，离屏回收后还能接着恢复

## 和 `virtual_list` 的区别

- `virtual_list` 更适合“很多行”
- `virtual_page` 更适合“很多 section”
- 底层都复用 `virtual_viewport`
- 当你的业务语义已经是 page / dashboard / settings 时，直接用 `virtual_page` 更顺手
