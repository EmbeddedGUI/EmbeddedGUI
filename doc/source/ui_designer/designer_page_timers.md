# UI Designer 页面定时器

## 概览

`Page Timers` 停靠面板用于管理 `Page.timers` 这类页面级定时器元数据。

每个定时器包含：

- `name`
- `callback`
- `delay_ms`
- `period_ms`
- `auto_start`

其中 `delay_ms` / `period_ms` 按原始 C 表达式处理，不在设计器里做语义求值。

## 代码生成

通过校验的定时器会参与如下生成：

- `{page}.h`
  - 生成 `egui_timer_t` 成员
  - 声明 `egui_{page}_timers_init()`
  - 声明 `egui_{page}_timers_start_auto()`
  - 声明 `egui_{page}_timers_stop()`
- `{page}_layout.c`
  - 生成 timer callback 的 `extern` 声明
  - 生成上述三个 helper 的实现

## 用户页 skeleton

`{page}.c` 现在使用带 `USER CODE` 区块的 skeleton，后续设计器在补充新框架钩子时会保留这些区块。

对于已存在的页面实现文件：

- 旧版设计器 skeleton 会自动迁移
- 当前无 `USER CODE` 区块但可识别为设计器 skeleton 的文件，也会自动迁移
- 纯手写文件保持不动

## 校验规则

以下情况会被 `Page Timers` 面板拒绝提交，同时 `Diagnostics` 会显示错误：

- 定时器名为空
- 定时器名不是合法 C 标识符
- 定时器名与控件 / 动画 / 页面字段冲突
- 定时器名重复
- 回调函数名为空
- 回调函数名不是合法 C 标识符
