# 2026-03-23 脏区域接口文档补充计划

## 目标

- 补一页专门说明 dirty-region 接口分层与推荐用法
- 配套运行截图，帮助用户直观看到整控件刷新与局部刷新差异
- 只提交本轮相关代码、测试、文档和图片

## 执行项

1. 梳理 `egui_view_invalidate`、`egui_view_invalidate_region`、`egui_view_invalidate_sub_region`、`egui_canvas_is_region_active`
2. 整理 `egui_view_circle_dirty.h` 在圆弧类控件中的用法
3. 从 `button_matrix` 和 `arc_slider` 运行结果中提取截图
4. 新增 `doc/source/performance/dirty_region_interfaces.md`
5. 更新性能文档索引，并在 `dirty_rect.md` 增加跳转
6. 做单测、运行截图和文档构建验证

## 结果

- 新增 dirty-region 接口设计说明页
- 增加 3 张正式文档截图
- 文档入口补齐
- 本轮提交包含 canvas transform helper、对应单测和 dirty-region 文档
