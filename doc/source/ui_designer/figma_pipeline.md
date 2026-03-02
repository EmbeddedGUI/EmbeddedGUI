# Figma Make -> EGUI 全自动转换管道

## 概述

Figma Make -> EGUI 全自动转换管道是一条端到端的设计转换流水线，将 Figma Make 项目（React + Tailwind CSS + Framer Motion）自动转换为 EGUI C 代码，支持全量动效还原和像素级回归验证。

核心原则：**以转换驱动框架演进** -- 遇到不支持的特效或控件时，设计并实现新的 EGUI 能力，而非跳过或降级。

## 完整流程

```
Stage 1: CAPTURE    Playwright -> Figma Make dev server -> 参考帧序列
Stage 2: CONVERT    TSX 解析 -> 布局/动效提取 -> XML -> C 代码
Stage 3: BUILD&RUN  make -> PC 模拟器 -> recording 截帧
Stage 4: VERIFY     参考帧 vs 实际帧 -> SSIM 比对 -> HTML 报告
```

统一入口：

```bash
python scripts/figmamake/figmamake2egui.py \
    --figma-url "https://www.figma.com/make/{fileKey}/..." \
    --app HelloBattery --width 320 --height 240
```

## Stage 1: CAPTURE（参考帧采集）

工具：`scripts/figmamake/figmamake_capture.py`

### 流程

1. 通过 Figma MCP 读取 TSX 源码到本地临时目录
2. `npm install` + `npm run dev` 启动 dev server
3. Playwright 打开 `localhost:5173`（viewport = 目标分辨率）
4. 截取 Boot 动画序列帧（每 100ms 一帧）
5. 依次导航到每个路由
6. 每个页面：截取动画关键帧（0%/50%/100%）+ 静态终态

### 关键帧采样策略

| 类型 | 策略 | 说明 |
|------|------|------|
| 静态终态 | 必采 | 等待所有动画完成后 500ms |
| 动画关键帧 | 0%, 50%, 100% | 三帧 |
| Boot 序列 | 每 100ms 一帧 | 逐步显示动画 |

### 输出结构

```
example/{App}/.eguiproject/reference_frames/
├── boot/
│   ├── frame_0000.png
│   ├── frame_0010.png
│   └── frame_final.png
├── dashboard/
│   ├── frame_0000.png
│   ├── frame_0005.png
│   └── frame_final.png
├── cells/
│   └── ...
└── settings/
    └── ...
```

## Stage 2: CONVERT（TSX -> EGUI）

### 2a. TSX 语义解析

工具：`scripts/figmamake/figmamake_parser.py`

从 TSX 源码中提取：

- 组件树结构（嵌套关系）
- 布局属性（Tailwind class -> 位置/尺寸/颜色）
- 文本内容和字体样式
- 图标引用（Lucide icon name -> 导出为 PNG）
- 图片引用（下载 Figma Make 中的图片资源）

输出：`layout_description.json`

也可以使用 `html2egui_helper.py figmamake-extract` 子命令：

```bash
# 列出所有页面
python scripts/html2egui_helper.py figmamake-extract \
    --input figma_make_project/ --list-pages

# 提取特定页面
python scripts/html2egui_helper.py figmamake-extract \
    --input figma_make_project/ --page Home

# 提取所有页面
python scripts/html2egui_helper.py figmamake-extract \
    --input figma_make_project/
```

输出 JSON 格式：

```json
{
  "source": "figma-make",
  "screen": {"width": 320, "height": 240},
  "root_bg_color": "0A0E1A",
  "colors": {"cyan-400": "22D3EE", "gray-400": "9CA3AF"},
  "icons": [
    {"lucide": "Battery", "material": "battery_full", "mapped": true,
     "color": "cyan-400", "size_px": 20}
  ],
  "pages": [{
    "name": "Home",
    "route": "/",
    "sections": [...]
  }],
  "navigation": [
    {"from": "/", "to": "/battery", "trigger": "section_1"}
  ],
  "text_chars": "..."
}
```

## Figma Make 导出配置

### TSX 源文件位置

Figma Make 项目的 TSX 源文件存放在：

```
example/{App}/.eguiproject/figmamake_src/
├── src/
│   ├── app/
│   │   ├── page.tsx              # 主页面/路由
│   │   └── components/
│   │       ├── Dashboard.tsx     # 各页面组件
│   │       ├── CellDetails.tsx
│   │       ├── Settings.tsx
│   │       └── ...
│   └── ...
├── package.json
└── tailwind.config.ts
```

### 主题色提取

从 `tailwind.config.ts` 中提取自定义颜色：

```typescript
// tailwind.config.ts
module.exports = {
  theme: {
    extend: {
      colors: {
        'bms-cyan': '#00f3ff',
        'bms-amber': '#ffaa00',
        'bms-red': '#ff3e3e',
        'bms-green': '#00ff9d',
      }
    }
  }
}
```

映射为 EGUI 颜色常量：

| 名称 | 色值 | EGUI |
|------|------|------|
| bms-cyan | #00f3ff | `EGUI_COLOR_HEX(0x00F3FF)` |
| bms-amber | #ffaa00 | `EGUI_COLOR_HEX(0xFFAA00)` |
| bms-red | #ff3e3e | `EGUI_COLOR_HEX(0xFF3E3E)` |
| bms-green | #00ff9d | `EGUI_COLOR_HEX(0x00FF9D)` |

## TSX 源文件分析

### 动效提取

工具：`scripts/figmamake/figmamake_anim_extractor.py`

从 TSX 中提取所有 `motion.*` 和 `animate` 属性，生成动效描述 JSON。

需要分析的 TSX 模式：

```tsx
// 1. motion 组件 + initial/animate
<motion.div
    initial={{ opacity: 0, y: -8 }}
    animate={{ opacity: 1, y: 0 }}
    transition={{ duration: 0.6, ease: "easeOut" }}
>

// 2. 带延迟的级联动画
<motion.div
    initial={{ width: 0 }}
    animate={{ width: `${percentage}%` }}
    transition={{ duration: 1, delay: index * 0.05, ease: "easeOut" }}
/>

// 3. useState + onClick 交互
const [isActive, setIsActive] = useState(false);
<div onClick={() => setIsActive(!isActive)}>

// 4. useEffect + setInterval 周期更新
useEffect(() => {
    const interval = setInterval(() => {
        setTime(new Date());
    }, 1000);
    return () => clearInterval(interval);
}, []);

// 5. AnimatePresence 页面切换
<AnimatePresence mode="wait">
    <motion.div
        key={currentPage}
        initial={{ opacity: 0, x: -20 }}
        animate={{ opacity: 1, x: 0 }}
        exit={{ opacity: 0, x: 20 }}
    />
</AnimatePresence>
```

### 动态特效映射

| Figma Make 动效 | EGUI 映射 | 实现方式 |
|----------------|-----------|---------|
| `opacity: 0 -> 1` | alpha 动画 | `egui_animation_alpha_t` |
| `x: -8 -> 0` / `y` | translate 动画 | `egui_animation_translate_t` |
| `scale: 1.02 -> 1` | scale 动画 | `egui_animation_scale_size_t` |
| `width: 0 -> N%` | resize 动画 | `egui_animation_resize_t` |
| `color transition` | color 动画 | `egui_animation_color_t` |
| `spring physics` | overshoot 插值器 | `egui_interpolator_overshoot_t` |
| `stagger delay` | 多动画 + 递增 delay | 定时器回调中按 id 延迟 |
| `AnimatePresence` | 页面切换 alpha+translate | 组合动画 |
| `setInterval(fn, 1000)` | 周期定时器 | `egui_timer_start_timer(&t, 1000, 1)` |
| `animate-pulse` | 周期 alpha 切换 | `egui_timer_t` 回调 |
| `whileTap: { scale }` | 按压反馈 | Background pressed 状态 |

### TSX -> EGUI 动效映射规则

详细映射规则定义在 `.claude/skills/dynamic-effects.md` 中：

| TSX (React/Framer Motion) | EGUI 对应实现 |
|---------------------------|---------------|
| `motion.div initial/animate width` | `egui_timer_t` + `egui_view_progress_bar_set_process()` |
| `transition: { duration, ease: "easeOut" }` | decelerate easing: `100 - (t_inv * t_inv) / 100` |
| `transition: { delay: N * 0.05 }` | 定时器回调中按 id 延迟帧数 |
| `animate-pulse` | `egui_timer_t` 周期性 alpha 切换 |
| `useState + onClick toggle` | `onClick` handler + 状态变量 |
| `motion layout spring` | `egui_timer_t` 模拟弹簧位移 |

### 不支持特效的处理流程

当遇到无法直接映射的动效时：

1. 标记为 `NEEDS_EXTENSION`
2. 生成扩展提案 `extension_proposals/{type}.md`
3. 实现扩展（新增 `src/anim/egui_animation_{type}.h/c`）
4. 更新 widget 注册系统 + XML schema
5. 重新生成代码

扩展提案模板：

```markdown
# Extension Proposal: egui_animation_{type}

## 需求来源
{哪个 Figma Make 项目的哪个组件需要此能力}

## API 设计
- 结构体: egui_animation_{type}_t
- 初始化: egui_animation_{type}_init(anim, params...)
- 参数宏: EGUI_ANIMATION_{TYPE}_PARAMS_INIT(...)
- 复用现有 interpolator 系统

## 文件变更
- 新增: src/anim/egui_animation_{type}.h/c
- 修改: src/anim/build.mk
- 新增: custom_widgets/ 动画注册
```

## 2c. XML + C 代码生成

利用现有 `generate-code` 管道，增强点：

- XML Animation 元素支持更多类型
- 生成 `uicode_{page}.c` 中的动效初始化代码
- Boot 序列 -> 定时器驱动状态机代码
- 不支持的控件 -> 生成控件扩展提案

```bash
python scripts/html2egui_helper.py generate-code --app HelloBattery
```

## Stage 3: BUILD & RUN（构建运行截帧）

复用 `code_runtime_check.py`，增强 recording 模式：

```bash
# 构建
make all APP=HelloBattery PORT=pc BITS=64

# 运行时验证（带截图）
python scripts/code_runtime_check.py --app HelloBattery --bits64 --keep-screenshots

# 或一步完成
python scripts/html2egui_helper.py verify --app HelloBattery
```

### Recording 增强

- recording action 加入精确时间控制
- 每个页面的 recording 序列：
  1. 切换到该页面
  2. 在动画 0%/50%/100% 时间点各截一帧
  3. 等待动画完成后截终态帧
- 帧命名与参考帧一一对应

### 多页面验证

代码生成器自动为多页面项目生成 `egui_port_get_recording_action()` 录制动作，通过 `uicode_switch_page()` 依次切换所有页面。验证时需检查截图确认每个页面都有正确渲染输出。

## Stage 4: VERIFY（像素级回归验证）

工具：`scripts/figmamake/figmamake_regression.py`

### 验证方法

1. SSIM 替代简单像素 diff（scikit-image structural_similarity）
2. 分区域评分：画面分 N x M 网格，每区域独立评分
3. 动画帧比对：参考帧序列 vs 实际帧序列逐帧 SSIM
4. HTML 报告：
   - 总览：每页 pass/fail + 总体得分
   - 每页详情：参考 | 实际 | diff | SSIM 分数
   - 动画关键帧比对（3帧）
   - 问题区域热力图
   - 扩展提案列表

### 通过标准

| 类型 | SSIM 阈值 | 判定 |
|------|----------|------|
| 静态终态 | >= 0.85 | PASS |
| 动画关键帧 | >= 0.70 | PASS |
| 任何页面 | < 0.60 | FAIL |

## 端到端示例：Battery BMS

### 项目信息

- 目标设计：Figma Make Battery BMS 项目
- 分辨率：320 x 240
- 页面数：5（Boot, Dashboard, CellDetails, TempMonitor, Settings）

### 页面清单

| 页面 | 路由 | 关键控件 | 关键动效 |
|------|------|---------|---------|
| Boot | / (初始) | 文本列表、进度条 | 逐行渐入+滑入、进度条增长 |
| Dashboard | / | SOC大数字、3列指标、进度条 | SOC条宽度增长、数值渐入 |
| CellDetails | /battery | 8个电池单元格、电压条 | 电压条交错增长(stagger) |
| TempMonitor | /temperature | 双通道温度卡片、折线图 | 折线图绘制动画 |
| Settings | /settings | 参数网格、开关列表 | 弹簧开关动画、按钮缩放 |

### 执行步骤

```bash
# 1. 采集参考帧
python scripts/figmamake/figmamake_capture.py \
    --figma-url "https://www.figma.com/make/Fw9h77LjlAxt4l9Hlt5XP9/" \
    --app HelloBattery --width 320 --height 240

# 2. 创建项目骨架
python scripts/html2egui_helper.py scaffold --app HelloBattery \
    --width 320 --height 240 \
    --pages boot,dashboard,cell_details,temp_monitor,settings

# 3. 提取布局 + 动效
python scripts/html2egui_helper.py figmamake-extract \
    --input example/HelloBattery/.eguiproject/figmamake_src/

# 4. 导出图标
python scripts/html2egui_helper.py export-icons \
    --input design.html --app HelloBattery --size 24 --auto-color

# 5. 编写 XML 布局（每个页面）
# 编辑 .eguiproject/layout/boot.xml
# 编辑 .eguiproject/layout/dashboard.xml
# ...

# 6. 生成 C 代码
python scripts/html2egui_helper.py generate-code --app HelloBattery

# 7. 添加动态特效（在 *.c 用户文件中实现）
# 参考 .eguiproject/figmamake_src/src/app/components/*.tsx

# 8. 生成资源
python scripts/html2egui_helper.py gen-resource --app HelloBattery

# 9. 构建验证
python scripts/html2egui_helper.py verify --app HelloBattery

# 10. 回归验证
python scripts/figmamake/figmamake_regression.py \
    --app HelloBattery --reference-dir .eguiproject/reference_frames/
```

## 框架扩展驱动机制

转换过程中遇到不支持的特效或控件时，自动触发扩展流程：

```
解析器标记 NEEDS_EXTENSION
    |
    v
生成 extension_proposals/{type}.md
    |
    v
实现 src/anim/egui_animation_{type}.h/c
  或 src/widget/egui_view_{type}.h/c
    |
    v
更新 custom_widgets/{type}.py 注册
    |
    v
重新走 Stage 2-4
```

### Battery BMS 需要的扩展（示例）

| 扩展 | 说明 | 状态 |
|------|------|------|
| `egui_animation_resize_t` | 宽度/高度动画（进度条增长） | 已实现 |
| ChartLine 动画支持 | 折线图数据点逐帧增长 | 已注册 |
| Boot 序列状态机 | 定时器驱动的逐步显示模式 | 定时器实现 |
| 发光文字效果 | text-shadow 降级为颜色高亮 | 颜色替代 |

## 动态特效实现参考

静态布局转换完成后，必须为每个页面补充动态特效。实现参考：

- `example/HelloBattery/dashboard.c` -- 完整动效参考（growth/pulse/clock 三定时器）
- `example/HelloBattery/cell_details.c` -- 级联延迟动画
- `example/HelloBattery/settings.c` -- toggle 开关交互
- `example/HelloStyleDemo/uicode_dashboard.c` -- 另一个完整动效参考

关键 API：

```c
// 定时器
egui_timer_init_timer(&timer, user_data, callback);
egui_timer_start_timer(&timer, interval_ms, period);
egui_timer_stop_timer(&timer);

// Label 更新
egui_view_label_set_text(EGUI_VIEW_OF(&label), buf);

// 进度条
egui_view_progress_bar_set_process(EGUI_VIEW_OF(&bar), value);

// 颜色
egui_view_set_color(EGUI_VIEW_OF(&view), EGUI_COLOR_HEX(0xRRGGBB));
```
