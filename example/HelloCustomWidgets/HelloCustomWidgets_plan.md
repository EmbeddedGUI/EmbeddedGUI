# HelloCustomWidgets Plan

> 当前版本改为“参考体系优先”的长期方案说明。`HelloCustomWidgets` 仍然保留现有 showcase 控件，但后续新增控件默认不再走强设定原创路线。

## 当前状态

- `HelloCustomWidgets` 继续使用 `category/widget_name` 的两级 `APP_SUB` 结构
- 截至 `2026-04-07`，仓库中已落地 `94` 个控件目录，覆盖 `chart`、`decoration`、`display`、`feedback`、`input`、`layout`、`media`、`navigation` 八个分类
- 其中前期已完成的 `50` 个控件继续视为 **Showcase Track** 历史基线，保留用于 demo、实验和展示，不再默认作为后续标准控件模板
- 从 `2026-03-14` 起，后续新增控件切换到 **Reference Track**，统一基于 `Fluent 2 + WPF UI`；当前目录总数 `94` 已包含这部分后续落地项

## 当前基线

详细方案见：`.claude/2026-03-14-hcw-fluent-reference-plan.md`

### 主参考体系

- 设计规范：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充：`ModernWpf`
- 缺口补充：`MahApps.Metro`

### 基本原则

- 新控件先找开源原型，再做 EGUI 适配
- 优先做通用控件，不优先做行业控件
- 优先做未来可能沉入 `src/widget/` 的控件
- 不再继续扩展 demo 风格的自创视觉零件

## 目录结构

```text
example/HelloCustomWidgets/<category>/<widget>/
```

每个控件目录通常包含：

- `egui_view_<widget>.h`
- `egui_view_<widget>.c`
- `test.c`
- `readme.md`
- `iteration_log/`

## 新增控件的额外要求

除既有 workflow 外，新增以下约束：

### 1. 必须有参考来源

每个新控件的 `readme.md` 顶部必须增加：

- 参考设计系统
- 参考开源库
- 对应组件名
- 保留状态
- 删除效果
- EGUI 简化说明

### 2. 历史白名单（已完成基线）

以下白名单用于说明当时的首批 Reference Track 基线来源，当前条目均已落地；后续选题应以 `.claude/2026-03-14-hcw-fluent-reference-plan.md` 和 `.claude/workflow/widget_progress_tracker.md` 的未完成项为准：

- `feedback/message_bar`
- `navigation/breadcrumb_bar`
- `feedback/skeleton_loader`
- `navigation/tab_strip`
- `input/number_box`
- `navigation/nav_panel`
- `feedback/toast_stack`
- `display/card_panel`

### 3. 存量控件不再当作主线模板

像下面这类控件继续保留，但不再作为后续风格母本：

- `chart/*`
- `media/*`
- `input/clip_launcher_grid`
- `input/scene_crossfader`
- `input/step_sequencer`
- `input/piano_roll_editor`
- `navigation/dock_launcher`
- `navigation/coverflow_strip`
- `navigation/radial_menu`
- `display/server_rack`
- `display/signal_matrix`

## 构建入口

- Make 默认入口仍可使用真实控件，例如：`input/xy_pad`
- CMake 仍支持两级 `APP_SUB`
- 示例：`input/xy_pad`、`feedback/alert_banner`、`navigation/breadcrumb_trail`

## 推荐验证命令

### 构建 / 运行

```bash
make all APP=HelloCustomWidgets APP_SUB=input/xy_pad PORT=pc
cmake -B build_cmake/HelloCustomWidgets_input_xy_pad -DAPP=HelloCustomWidgets -DAPP_SUB=input/xy_pad -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloCustomWidgets_input_xy_pad -j
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/xy_pad --timeout 10 --keep-screenshots
```

### 编译检查

```bash
python scripts/code_compile_check.py --custom-widgets
```

### Web

```bash
python scripts/web/wasm_build_demos.py
```

## 当前开发流程

1. 先读 `.claude/workflow/widget_acceptance_workflow.md`
2. 再读 `.claude/workflow/widget_progress_tracker.md`
3. 再读 `.claude/2026-03-14-hcw-fluent-reference-plan.md`
4. 优先从 `.claude/2026-03-14-hcw-fluent-reference-plan.md` 与 tracker 中尚未落地的项里选下一个控件
5. 在 `example/HelloCustomWidgets/<category>/<widget>/` 下完成实现、readme、iteration_log 和 runtime 验证
6. 完成后更新 tracker，并单独提交一个 commit

## 维护说明

- `HelloCustomWidgets` 继续承担“样例池 + 探索池”职责
- 但后续要区分 **Showcase** 与 **Reference** 两条线，不再混用
- 如果某个新控件未来要沉入框架层，必须优先来自 `Reference Track`
