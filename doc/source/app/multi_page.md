# 多页面应用开发

EmbeddedGUI 提供了三种多页面管理方案，适用于不同的资源预算和交互需求。本文对比各方案的特点，帮助开发者做出合理选型。

## 三种多页面方案

| 特性 | Activity 栈 | Page (union) | ViewPage 横向翻页 |
|------|------------|-------------|-------------------|
| 切换方式 | 全屏切换，支持前进/回退栈 | 全屏切换，手动管理 | 横向滑动翻页 |
| 内存模型 | 每个 Activity 独立内存 | union 共享内存 | 所有页面同时驻留 |
| RAM 开销 | 高（所有 Activity 之和） | 低（最大 Page 的大小） | 高（所有页面之和） |
| 切换动画 | 内置平移动画 | 无内置动画 | 内置滑动动画 |
| 页面共存 | 切换时两个 Activity 短暂共存 | 同时只有一个 Page | 所有页面始终存在 |
| Dialog 支持 | 内置 | 无 | 无 |
| Toast 支持 | 内置 | 需手动初始化 | 需手动初始化 |
| 生命周期 | 6 个状态 | 2 个状态 | 无 |
| 适用场景 | 复杂导航、多层级 | 简单切换、极低 RAM | 平级页面浏览 |

## Activity 栈全屏切换

Activity 模式通过栈管理实现页面的前进和回退，支持完整的生命周期和切换动画。

核心流程：

```c
// 1. 配置切换动画
egui_core_activity_set_start_anim(open_anim, close_anim);
egui_core_activity_set_finish_anim(open_anim, close_anim);

// 2. 启动新 Activity
egui_core_activity_start(new_activity, current_activity);

// 3. 结束当前 Activity（回退）
egui_core_activity_finish(current_activity);
```

适用场景：
- 需要页面栈管理（如设置页 -> 子设置页 -> 详情页）
- 需要切换动画
- 需要 Dialog 弹窗
- RAM 预算 > 4KB（UI 部分）

详细用法参见 [Activity 生命周期管理](activity.md)。

## Page union 轻量切换

Page 模式通过 union 共享内存，同一时刻只有一个 Page 存在，极大节省 RAM。

核心流程：

```c
// 定义 union
union page_array
{
    egui_page_0_t page_0;
    egui_page_1_t page_1;
};
static union page_array g_page_array;

// 切换页面
void uicode_switch_page(int page_index)
{
    if (current_page)
        egui_page_base_close(current_page);

    switch (page_index)
    {
    case 0:
        egui_page_0_init((egui_page_base_t *)&g_page_array.page_0);
        current_page = (egui_page_base_t *)&g_page_array.page_0;
        break;
    // ...
    }
    egui_page_base_open(current_page);
}
```

适用场景：
- RAM 极度紧张（<2KB 可用于 UI）
- 页面间无需共存
- 不需要切换动画
- 简单的线性导航

详细用法参见 [Page 开发模式](easy_page.md)。

## ViewPage 横向翻页

ViewPage 是一个容器控件，将多个子页面水平排列，支持触摸滑动翻页。所有页面在初始化时同时创建，始终驻留在内存中。

核心流程：

```c
// 1. 创建 ViewPage
static egui_view_viewpage_t viewpage;
egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

// 2. 创建页面容器（Group）
static egui_view_group_t page_1, page_2, page_3;
egui_view_group_init_with_params(EGUI_VIEW_OF(&page_1), &page_params);
// ... 初始化页面内容 ...

// 3. 添加页面到 ViewPage
egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_1));
egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_2));
egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_3));

// 4. 布局并添加到根视图
egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));
egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));

// 5. 注册页面切换回调
egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&viewpage), on_page_changed);
```

程序化切换页面：

```c
egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&viewpage), page_index);
```

适用场景：
- 平级页面浏览（如仪表盘的多个面板）
- 需要触摸滑动翻页体验
- 页面数量较少（3-5 个）
- RAM 预算充足

详细用法参见 [HelloStyleDemo 完整解析](style_demo.md)。

## 选型指南

根据以下条件选择合适的方案：

```
RAM 预算 < 2KB?
  -> Page (union) 模式

需要触摸滑动翻页?
  -> ViewPage 模式

需要页面栈（前进/回退）?
  -> Activity 模式

需要 Dialog 弹窗?
  -> Activity 模式

页面数量 <= 5 且平级关系?
  -> ViewPage 模式

简单的线性导航?
  -> Page 模式（最省 RAM）
  -> Activity 模式（有动画）
```

## 混合使用

三种方案可以混合使用。例如 HelloStyleDemo 使用 ViewPage 管理 4 个平级页面，每个页面内部可以再用 Activity 或 Page 管理子页面。

常见组合：
- ViewPage（顶层翻页） + Activity（子页面导航）
- Activity（主导航） + Dialog（弹窗）+ Toast（提示）
- Page（主页面切换） + Toast（提示）

## 相关文件

- `src/app/egui_activity.h` - Activity 模式
- `src/app/egui_page_base.h` - Page 模式
- `src/widget/egui_view_viewpage.h` - ViewPage 控件
- `example/HelloActivity/` - Activity 示例
- `example/HelloEasyPage/` - Page 示例
- `example/HelloStyleDemo/` - ViewPage 示例
