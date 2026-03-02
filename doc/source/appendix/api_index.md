# API 速查索引

本文档按模块分组列出 EmbeddedGUI 框架的公开 API 函数，方便快速查阅。

## Core API

框架核心功能，包括初始化、屏幕管理、脏区域、Activity/Dialog/Toast 管理。

### 初始化与生命周期

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_init(config)` | 初始化 EGUI 框架 | egui_core.h |
| `egui_polling_work()` | 主循环轮询（处理定时器、动画、输入、刷新） | egui_core.h |
| `egui_polling_refresh_display()` | 轮询刷新显示 | egui_core.h |
| `egui_check_need_refresh()` | 检查是否需要刷新屏幕 | egui_core.h |
| `egui_screen_on()` | 开启屏幕（清屏、恢复核心和定时器） | egui_core.h |
| `egui_screen_off()` | 关闭屏幕（暂停核心、停止定时器、关闭显示） | egui_core.h |

### 电源与挂起

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_power_on()` | 核心上电 | egui_core.h |
| `egui_core_power_off()` | 核心断电 | egui_core.h |
| `egui_core_suspend()` | 挂起 GUI 刷新 | egui_core.h |
| `egui_core_resume()` | 恢复 GUI 刷新 | egui_core.h |
| `egui_core_is_suspended()` | 查询是否处于挂起状态 | egui_core.h |

### 屏幕与 PFB 管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_set_screen_size(w, h)` | 设置屏幕尺寸 | egui_core.h |
| `egui_core_set_pfb_buffer_ptr(pfb)` | 设置 PFB 缓冲区指针 | egui_core.h |
| `egui_core_get_pfb_buffer_ptr()` | 获取 PFB 缓冲区指针 | egui_core.h |
| `egui_core_pfb_set_buffer(pfb, w, h)` | 设置 PFB 缓冲区及尺寸 | egui_core.h |
| `egui_pfb_add_buffer(buf)` | 添加额外 PFB 缓冲区到环形队列 | egui_core.h |
| `egui_pfb_notify_flush_complete()` | 通知 DMA 刷新完成（可在 ISR 中调用） | egui_core.h |
| `egui_pfb_bus_acquire()` | 获取 SPI 总线（非显示访问） | egui_core.h |
| `egui_pfb_bus_release()` | 释放 SPI 总线 | egui_core.h |

### 脏区域管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_force_refresh()` | 强制全屏刷新 | egui_core.h |
| `egui_core_update_region_dirty(region)` | 更新指定脏区域 | egui_core.h |
| `egui_core_update_region_dirty_all()` | 标记全屏为脏区域 | egui_core.h |
| `egui_core_clear_region_dirty()` | 清除所有脏区域 | egui_core.h |
| `egui_core_check_region_dirty_intersect(region)` | 检查区域是否与脏区域相交 | egui_core.h |
| `egui_core_get_region_dirty_arr()` | 获取脏区域数组 | egui_core.h |
| `egui_core_clear_screen()` | 用 PFB 分块清屏（黑色填充） | egui_core.h |

### 根视图管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_get_root_view()` | 获取系统根视图组 | egui_core.h |
| `egui_core_add_root_view(view)` | 添加视图到系统根视图组 | egui_core.h |
| `egui_core_get_user_root_view()` | 获取用户根视图组 | egui_core.h |
| `egui_core_add_user_root_view(view)` | 添加视图到用户根视图组 | egui_core.h |
| `egui_core_remove_user_root_view(view)` | 从用户根视图组移除视图 | egui_core.h |
| `egui_core_layout_childs_user_root_view(h, align)` | 布局用户根视图组的子视图 | egui_core.h |
| `egui_core_get_unique_id()` | 获取唯一 ID | egui_core.h |

### 输入处理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_process_input_motion(event)` | 处理触摸/运动输入事件 | egui_core.h |
| `egui_core_process_input_key(event)` | 处理按键输入事件（需启用 KEY 支持） | egui_core.h |

### Activity 管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_activity_get_current()` | 获取当前 Activity | egui_core.h |
| `egui_core_activity_start(self, prev)` | 启动 Activity（指定前一个） | egui_core.h |
| `egui_core_activity_start_with_current(self)` | 启动 Activity（以当前为前一个） | egui_core.h |
| `egui_core_activity_finish(self)` | 结束 Activity | egui_core.h |
| `egui_core_activity_force_finish_all()` | 强制结束所有 Activity | egui_core.h |
| `egui_core_activity_force_finish_to_activity(act)` | 强制结束到指定 Activity | egui_core.h |
| `egui_core_activity_check_in_process(act)` | 检查 Activity 是否在处理中 | egui_core.h |
| `egui_core_activity_append(act)` | 追加 Activity 到列表 | egui_core.h |
| `egui_core_activity_remove(act)` | 从列表移除 Activity | egui_core.h |
| `egui_core_activity_set_start_anim(open, close)` | 设置 Activity 启动动画 | egui_core.h |
| `egui_core_activity_set_finish_anim(open, close)` | 设置 Activity 结束动画 | egui_core.h |
| `egui_core_activity_get_by_view(view)` | 通过视图查找所属 Activity | egui_core.h |

### Dialog 管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_dialog_get()` | 获取当前 Dialog | egui_core.h |
| `egui_core_dialog_start(activity, dialog)` | 在指定 Activity 上启动 Dialog | egui_core.h |
| `egui_core_dialog_start_with_current(dialog)` | 在当前 Activity 上启动 Dialog | egui_core.h |
| `egui_core_dialog_check_in_process(dialog)` | 检查 Dialog 是否在处理中 | egui_core.h |
| `egui_core_dialog_finish(dialog)` | 结束 Dialog | egui_core.h |
| `egui_core_dialog_set_anim(open, close)` | 设置 Dialog 动画 | egui_core.h |

### Toast 管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_toast_get()` | 获取当前 Toast | egui_core.h |
| `egui_core_toast_set(toast)` | 设置 Toast 实例 | egui_core.h |
| `egui_core_toast_show_info(text)` | 显示信息提示 | egui_core.h |

---

## View API

视图基类，所有控件的公共接口。

### 初始化与生命周期

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_init(self)` | 初始化视图 | egui_view.h |
| `egui_view_draw(self)` | 绘制视图 | egui_view.h |
| `egui_view_on_draw(self)` | 视图绘制回调 | egui_view.h |
| `egui_view_on_attach_to_window(self)` | 视图附加到窗口回调 | egui_view.h |
| `egui_view_on_detach_from_window(self)` | 视图从窗口分离回调 | egui_view.h |
| `egui_view_invalidate(self)` | 标记视图需要重绘 | egui_view.h |

### 属性设置

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_set_position(self, x, y)` | 设置视图位置 | egui_view.h |
| `egui_view_set_size(self, w, h)` | 设置视图尺寸 | egui_view.h |
| `egui_view_set_alpha(self, alpha)` | 设置视图透明度 | egui_view.h |
| `egui_view_set_visible(self, visible)` | 设置可见性 | egui_view.h |
| `egui_view_get_visible(self)` | 获取可见性 | egui_view.h |
| `egui_view_set_gone(self, gone)` | 设置是否隐藏（不占布局空间） | egui_view.h |
| `egui_view_get_gone(self)` | 获取隐藏状态 | egui_view.h |
| `egui_view_set_enable(self, enable)` | 设置启用状态 | egui_view.h |
| `egui_view_get_enable(self)` | 获取启用状态 | egui_view.h |
| `egui_view_set_clickable(self, clickable)` | 设置是否可点击 | egui_view.h |
| `egui_view_get_clickable(self)` | 获取可点击状态 | egui_view.h |
| `egui_view_set_pressed(self, pressed)` | 设置按下状态 | egui_view.h |
| `egui_view_get_pressed(self)` | 获取按下状态 | egui_view.h |
| `egui_view_set_background(self, bg)` | 设置背景 | egui_view.h |
| `egui_view_set_view_name(self, name)` | 设置视图调试名称 | egui_view.h |
| `egui_view_set_shadow(self, shadow)` | 设置阴影效果 | egui_view.h |

### 布局与间距

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_layout(self, region)` | 设置视图布局区域 | egui_view.h |
| `egui_view_request_layout(self)` | 请求重新布局 | egui_view.h |
| `egui_view_calculate_layout(self)` | 计算布局 | egui_view.h |
| `egui_view_set_padding(self, l, r, t, b)` | 设置内边距 | egui_view.h |
| `egui_view_set_padding_all(self, padding)` | 设置统一内边距 | egui_view.h |
| `egui_view_set_margin(self, l, r, t, b)` | 设置外边距 | egui_view.h |
| `egui_view_set_margin_all(self, margin)` | 设置统一外边距 | egui_view.h |
| `egui_view_set_parent(self, parent)` | 设置父视图 | egui_view.h |
| `egui_view_get_raw_pos(self, location)` | 获取原始位置 | egui_view.h |
| `egui_view_get_work_region(self, region)` | 获取工作区域 | egui_view.h |

### 滚动

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_scroll_to(self, x, y)` | 滚动到指定位置 | egui_view.h |
| `egui_view_scroll_by(self, x, y)` | 相对滚动 | egui_view.h |
| `egui_view_compute_scroll(self)` | 计算滚动 | egui_view.h |

### 事件处理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_set_on_click_listener(self, listener)` | 设置点击监听器 | egui_view.h |
| `egui_view_set_on_touch_listener(self, listener)` | 设置触摸监听器 | egui_view.h |
| `egui_view_dispatch_touch_event(self, event)` | 分发触摸事件 | egui_view.h |
| `egui_view_on_touch_event(self, event)` | 触摸事件回调 | egui_view.h |
| `egui_view_on_intercept_touch_event(self, event)` | 拦截触摸事件 | egui_view.h |
| `egui_view_perform_click(self)` | 执行点击 | egui_view.h |

### 按键支持（需启用 EGUI_CONFIG_FUNCTION_SUPPORT_KEY）

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_dispatch_key_event(self, event)` | 分发按键事件 | egui_view.h |
| `egui_view_on_key_event(self, event)` | 按键事件回调 | egui_view.h |
| `egui_view_set_on_key_listener(self, listener)` | 设置按键监听器 | egui_view.h |

### 焦点支持（需启用 EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS）

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_set_focusable(self, focusable)` | 设置是否可聚焦 | egui_view.h |
| `egui_view_get_focusable(self)` | 获取可聚焦状态 | egui_view.h |
| `egui_view_request_focus(self)` | 请求焦点 | egui_view.h |
| `egui_view_clear_focus(self)` | 清除焦点 | egui_view.h |
| `egui_view_set_on_focus_change_listener(self, l)` | 设置焦点变化监听器 | egui_view.h |

### 图层支持（需启用 EGUI_CONFIG_FUNCTION_SUPPORT_LAYER）

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_set_layer(self, layer)` | 设置图层（值越大越靠前） | egui_view.h |
| `egui_view_get_layer(self)` | 获取图层值 | egui_view.h |

---

## Group API

视图组（容器），管理子视图的添加、移除和布局。

### 初始化

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_group_init(self)` | 初始化视图组 | egui_view_group.h |
| `egui_view_group_init_with_params(self, params)` | 使用参数初始化视图组 | egui_view_group.h |
| `egui_view_group_apply_params(self, params)` | 应用参数到视图组 | egui_view_group.h |

### 子视图管理

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_group_add_child(self, child)` | 添加子视图 | egui_view_group.h |
| `egui_view_group_remove_child(self, child)` | 移除子视图 | egui_view_group.h |
| `egui_view_group_clear_childs(self)` | 清除所有子视图 | egui_view_group.h |
| `egui_view_group_get_child_count(self)` | 获取子视图数量 | egui_view_group.h |
| `egui_view_group_get_first_child(self)` | 获取第一个子视图 | egui_view_group.h |
| `EGUI_VIEW_GROUP_ADD_CHILD_TREE(group, tree)` | 批量添加子视图树（宏） | egui_view_group.h |

### 布局计算

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_group_layout_childs(self, h, aw, ah, align)` | 布局子视图 | egui_view_group.h |
| `egui_view_group_calculate_all_child_width(self, w)` | 计算所有子视图总宽度 | egui_view_group.h |
| `egui_view_group_calculate_all_child_height(self, h)` | 计算所有子视图总高度 | egui_view_group.h |
| `egui_view_group_get_max_child_width(self, w)` | 获取子视图最大宽度 | egui_view_group.h |
| `egui_view_group_get_max_child_height(self, h)` | 获取子视图最大高度 | egui_view_group.h |
| `egui_view_group_calculate_layout(self)` | 计算视图组布局 | egui_view_group.h |
| `egui_view_group_request_layout(self)` | 请求重新布局 | egui_view_group.h |

### 触摸事件

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_group_dispatch_touch_event(self, event)` | 分发触摸事件到子视图 | egui_view_group.h |
| `egui_view_group_on_touch_event(self, event)` | 视图组触摸事件回调 | egui_view_group.h |
| `egui_view_group_on_intercept_touch_event(self, event)` | 拦截触摸事件 | egui_view_group.h |
| `egui_view_group_set_disallow_process_touch_event(self, d)` | 禁止处理触摸事件 | egui_view_group.h |
| `egui_view_group_request_disallow_intercept_touch_event(self, d)` | 请求禁止拦截触摸事件 | egui_view_group.h |

### 绘制与窗口

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_group_draw(self)` | 绘制视图组及子视图 | egui_view_group.h |
| `egui_view_group_compute_scroll(self)` | 计算滚动 | egui_view_group.h |
| `egui_view_group_on_attach_to_window(self)` | 附加到窗口回调 | egui_view_group.h |
| `egui_view_group_on_detach_from_window(self)` | 从窗口分离回调 | egui_view_group.h |

### 图层排序（需启用 EGUI_CONFIG_FUNCTION_SUPPORT_LAYER）

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_view_group_reorder_child(self, child)` | 按图层值重新排序子视图 | egui_view_group.h |
| `egui_view_group_bring_child_to_front(self, child)` | 将子视图移到最前 | egui_view_group.h |
| `egui_view_group_send_child_to_back(self, child)` | 将子视图移到最后 | egui_view_group.h |

---

## Animation API

动画系统，支持属性动画、重复模式和插值器。

### 初始化与控制

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_animation_init(self)` | 初始化动画 | egui_animation.h |
| `egui_animation_start(self)` | 启动动画 | egui_animation.h |
| `egui_animation_stop(self)` | 停止动画 | egui_animation.h |
| `egui_animation_update(self, time)` | 更新动画（由框架调用） | egui_animation.h |

### 参数配置

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_animation_duration_set(self, ms)` | 设置动画时长（毫秒） | egui_animation.h |
| `egui_animation_target_view_set(self, view)` | 设置动画目标视图 | egui_animation.h |
| `egui_animation_interpolator_set(self, interp)` | 设置插值器 | egui_animation.h |
| `egui_animation_repeat_mode_set(self, mode)` | 设置重复模式（RESTART/REVERSE） | egui_animation.h |
| `egui_animation_repeat_count_set(self, count)` | 设置重复次数 | egui_animation.h |
| `egui_animation_is_fill_before_set(self, fill)` | 设置动画开始前是否应用初始值 | egui_animation.h |
| `egui_animation_is_fill_after_set(self, fill)` | 设置动画结束后是否保持最终值 | egui_animation.h |

### 回调

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_animation_handle_set(self, handle)` | 设置动画事件回调（start/repeat/end） | egui_animation.h |
| `egui_animation_notify_start(self)` | 通知动画开始 | egui_animation.h |
| `egui_animation_notify_end(self)` | 通知动画结束 | egui_animation.h |
| `egui_animation_notify_repeat(self)` | 通知动画重复 | egui_animation.h |

### 核心动画管理（通过 egui_core）

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_core_animation_append(anim)` | 将动画添加到全局动画列表 | egui_core.h |
| `egui_core_animation_remove(anim)` | 从全局动画列表移除动画 | egui_core.h |

---

## Timer API

软件定时器，支持单次和周期触发。

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_timer_init()` | 初始化定时器子系统 | egui_timer.h |
| `egui_timer_init_timer(handle, data, cb)` | 初始化定时器实例 | egui_timer.h |
| `egui_timer_start_timer(handle, ms, period)` | 启动定时器（ms=延迟，period=周期，0=单次） | egui_timer.h |
| `egui_timer_stop_timer(handle)` | 停止定时器 | egui_timer.h |
| `egui_timer_check_timer_start(handle)` | 检查定时器是否已启动 | egui_timer.h |
| `egui_timer_get_current_time()` | 获取当前时间（毫秒） | egui_timer.h |
| `egui_timer_polling_work()` | 轮询处理到期定时器 | egui_timer.h |
| `egui_timer_force_refresh_timer()` | 强制刷新定时器 | egui_timer.h |

---

## Canvas API

画布绘图接口，提供基本图形、文本和图片绘制。

### 初始化与状态

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_init(pfb, region)` | 初始化画布 | egui_canvas.h |
| `egui_canvas_set_alpha(alpha)` | 设置全局透明度 | egui_canvas.h |
| `egui_canvas_get_alpha()` | 获取全局透明度 | egui_canvas.h |
| `egui_canvas_mix_alpha(alpha)` | 混合透明度 | egui_canvas.h |
| `egui_canvas_set_mask(mask)` | 设置遮罩 | egui_canvas.h |
| `egui_canvas_clear_mask()` | 清除遮罩 | egui_canvas.h |
| `egui_canvas_calc_work_region(region)` | 计算工作区域 | egui_canvas.h |
| `egui_canvas_register_spec_circle_info(cnt, arr)` | 注册预计算圆形信息 | egui_canvas.h |

### 基本图形

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_draw_point(x, y, color, alpha)` | 绘制点 | egui_canvas.h |
| `egui_canvas_draw_line(x1, y1, x2, y2, w, color, a)` | 绘制直线 | egui_canvas.h |
| `egui_canvas_draw_line_segment(x1, y1, x2, y2, w, c, a)` | 绘制线段 | egui_canvas.h |
| `egui_canvas_draw_rectangle(x, y, w, h, sw, color, a)` | 绘制矩形边框 | egui_canvas.h |
| `egui_canvas_draw_rectangle_fill(x, y, w, h, color, a)` | 绘制填充矩形 | egui_canvas.h |
| `egui_canvas_draw_round_rectangle(...)` | 绘制圆角矩形边框 | egui_canvas.h |
| `egui_canvas_draw_round_rectangle_fill(...)` | 绘制填充圆角矩形 | egui_canvas.h |
| `egui_canvas_draw_round_rectangle_corners(...)` | 绘制独立圆角矩形边框 | egui_canvas.h |
| `egui_canvas_draw_round_rectangle_corners_fill(...)` | 绘制独立圆角填充矩形 | egui_canvas.h |

### 圆形与弧形

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_draw_circle_basic(cx, cy, r, w, c, a)` | 绘制圆形边框（基础） | egui_canvas.h |
| `egui_canvas_draw_circle_fill_basic(cx, cy, r, c, a)` | 绘制填充圆形（基础） | egui_canvas.h |
| `egui_canvas_draw_arc_basic(cx, cy, r, s, e, w, c, a)` | 绘制弧形（基础） | egui_canvas.h |
| `egui_canvas_draw_arc_fill_basic(cx, cy, r, s, e, c, a)` | 绘制填充弧形（基础） | egui_canvas.h |
| `egui_canvas_draw_circle_hq(cx, cy, r, w, c, a)` | 绘制圆形边框（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_circle_fill_hq(cx, cy, r, c, a)` | 绘制填充圆形（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_arc_hq(cx, cy, r, s, e, w, c, a)` | 绘制弧形（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_arc_fill_hq(cx, cy, r, s, e, c, a)` | 绘制填充弧形（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_arc_round_cap_hq(...)` | 绘制圆帽弧形（高质量） | egui_canvas.h |

### 三角形、椭圆与多边形

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_draw_triangle(...)` | 绘制三角形边框 | egui_canvas.h |
| `egui_canvas_draw_triangle_fill(...)` | 绘制填充三角形 | egui_canvas.h |
| `egui_canvas_draw_ellipse(cx, cy, rx, ry, w, c, a)` | 绘制椭圆边框 | egui_canvas.h |
| `egui_canvas_draw_ellipse_fill(cx, cy, rx, ry, c, a)` | 绘制填充椭圆 | egui_canvas.h |
| `egui_canvas_draw_polygon(pts, cnt, w, c, a)` | 绘制多边形边框 | egui_canvas.h |
| `egui_canvas_draw_polygon_fill(pts, cnt, c, a)` | 绘制填充多边形 | egui_canvas.h |
| `egui_canvas_draw_polyline(pts, cnt, w, c, a)` | 绘制折线 | egui_canvas.h |

### 高质量线条

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_draw_line_hq(...)` | 绘制直线（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_line_segment_hq(...)` | 绘制线段（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_line_round_cap_hq(...)` | 绘制圆帽直线（高质量） | egui_canvas.h |
| `egui_canvas_draw_polyline_hq(...)` | 绘制折线（高质量抗锯齿） | egui_canvas.h |
| `egui_canvas_draw_polyline_round_cap_hq(...)` | 绘制圆帽折线（高质量） | egui_canvas.h |

### 贝塞尔曲线

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_draw_bezier_quad(...)` | 绘制二次贝塞尔曲线 | egui_canvas.h |
| `egui_canvas_draw_bezier_cubic(...)` | 绘制三次贝塞尔曲线 | egui_canvas.h |

### 文本与图片

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_canvas_draw_text(font, str, x, y, c, a)` | 绘制文本 | egui_canvas.h |
| `egui_canvas_draw_text_in_rect(font, str, rect, align, c, a)` | 在矩形区域内绘制文本 | egui_canvas.h |
| `egui_canvas_draw_text_in_rect_with_line_space(...)` | 在矩形区域内绘制文本（带行间距） | egui_canvas.h |
| `egui_canvas_draw_image(img, x, y)` | 绘制图片 | egui_canvas.h |
| `egui_canvas_draw_image_resize(img, x, y, w, h)` | 绘制缩放图片 | egui_canvas.h |
| `egui_canvas_draw_image_color(img, x, y, c, a)` | 绘制着色图片 | egui_canvas.h |
| `egui_canvas_draw_image_resize_color(img, x, y, w, h, c, a)` | 绘制缩放着色图片 | egui_canvas.h |

---

## Activity API

Activity 生命周期管理，类 Android 的页面管理模式。

### 初始化与配置

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_activity_init(self)` | 初始化 Activity | egui_activity.h |
| `egui_activity_set_layout(self, layout)` | 设置 Activity 布局区域 | egui_activity.h |
| `egui_activity_add_view(self, view)` | 向 Activity 添加视图 | egui_activity.h |
| `egui_activity_set_name(self, name)` | 设置 Activity 名称（调试用） | egui_activity.h |

### 生命周期回调

| 函数 | 说明 | 头文件 |
|------|------|--------|
| `egui_activity_on_create(self)` | 创建回调 | egui_activity.h |
| `egui_activity_on_start(self)` | 启动回调 | egui_activity.h |
| `egui_activity_on_resume(self)` | 恢复回调 | egui_activity.h |
| `egui_activity_on_pause(self)` | 暂停回调 | egui_activity.h |
| `egui_activity_on_stop(self)` | 停止回调 | egui_activity.h |
| `egui_activity_on_destroy(self)` | 销毁回调 | egui_activity.h |
