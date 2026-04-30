---
name: ai-generate-egui-app
description: Use when a user provides a UI reference image/effect image and asset slice path, and wants Codex to generate a new EmbeddedGUI APP or improve/refine an existing half-finished EmbeddedGUI APP with current-state audit, reference-image resync, confirmed target screen size, a written phased Markdown plan, 1:1 visual restoration, pixel-level screenshot comparison, PC preview, runtime_check validation, backend data interfaces, and optional handoff to app-level performance or code-size optimization skills
---

# Effect To EGUI App Skill

用于把“效果图 + 切图资源路径 + 业务需求”转成可运行的 EmbeddedGUI APP，也用于继续完善已经写了一半、视觉还原不足或交互/后端未收口的现有 APP。目标是让 Codex 先对齐输入、现状和差异，再按固定流程完成 APP 结构、资源、页面、数据适配、PC 预览和运行时截图验收。

本 skill 只保留 APP 生成主流程。涉及性能或体积优化时，不在这里展开细则，改为按需读取：

- `.claude/skills/performance-optimization.md`：卡顿、整屏刷新、dirty 区域、PFB/SPI、文本/图片绘制、真机刷新耗时。
- `.claude/skills/code-size-optimization.md`：ROM、Resource、RAM、PFB、heap/stack、资源格式、feature 宏、widget/HQ 路径裁剪。

## 强制门禁

以下门禁不能省略；缺一项时不能进入大规模实现或最终验收：

- **先判断工作模式**：新建 APP 走“新建模式”；APP 目录已存在、用户说“完善/继续/修正/优化现有项目/视觉差异大”时，必须走“现有项目完善模式”。
- **现有项目先读状态**：完善已有 APP 前，必须读取当前 APP 目录、资源配置、构建配置、页面代码、后端 API、recording、已有计划/报告和 git 状态；不能凭记忆继续写。
- **屏幕尺寸必须由用户确认**：可以从效果图推断宽高和方向，但必须把推断值反馈给用户确认；未确认前不得把尺寸写死到 `app_egui_config.h`。
- **计划必须落盘为 Markdown**：实现前必须生成阶段计划文档，推荐路径为 `.claude/plans/{APP}-effect-to-egui-plan.md`；APP 目录创建后可同步保存到 `example/{APP}/docs/effect_to_egui_plan.md`。
- **阶段计划必须有门禁**：每个阶段列出目标、产物、验证方式、阻塞问题和完成条件，不能只给一句“实现 UI、接入资源、运行验证”。
- **视觉验收必须做像素级比对**：`runtime_check` 只能证明程序能跑和截图存在，不能替代效果图对比；必须生成像素 diff 指标和差异图。
- **用户切图默认 1:1 使用**：优先按资源 natural size 放置，不为了“看起来适配”在运行时 resize；确需缩放时必须在计划文档中说明原因、比例、影响并经用户确认。

## 必要输入

如果用户没有给齐以下信息，先只问缺失项，不要直接生成：

- 效果图路径或已上传图片：用于 1:1 视觉还原和截图对比。
- 切图资源路径：包含 PNG/SVG/字体等设计资源。
- APP 名称或放置位置：例如 `example/HelloXxx`；未指定时按功能命名。
- 屏幕尺寸与方向：这是阻塞输入。未指定时只能从效果图尺寸推断候选值，并要求用户确认真实目标屏幕宽高、方向，以及是否要求 APP 画布与效果图画布完全一致。
- 需要动态展示的后端数据：例如播放状态、列表、音量、电量、时间、网络状态、错误状态。
- 交互路径：需要 PC 录制验证的点击、滑动、页面跳转、状态切换。

只问真正阻塞的问题。APP 名称、部分后端字段、交互细节可以在读图反馈后继续确认；屏幕尺寸/方向、效果图路径、切图路径不能静默假设。

## 工作模式

### 新建模式

当 APP 目录不存在，或用户明确要求从零生成时，按“固定工作流”从读图、计划、建骨架开始执行。

### 现有项目完善模式

当 APP 已存在、已有半成品代码，或用户要求“继续完善/修正视觉差异/补功能/按效果图收口”时，先执行现状同步，再列计划：

1. **读取当前状态**
   - 读取 `example/{APP}/build.mk`、`app_egui_config.h`、页面源码、资源配置、生成报告、recording 相关代码和现有 docs/plan。
   - 检查当前 `APP/APP_SUB`、屏幕尺寸、PFB 配置、资源格式、已接入的后端 state/control API。
   - 扫描当前实现中的缩放、硬编码和临时实现，例如 `rg -n "draw_image_resize|set_resize|TODO|mock|hardcode" example/{APP}`。
   - 查看 git 状态，区分已有用户改动、上次生成产物和本轮将要修改的文件；不要重置或覆盖不相关改动。

2. **建立当前基线**
   - 如果能编译，先运行 `make all APP={APP} PORT=pc` 和 `python scripts/code_runtime_check.py --app {APP} --keep-screenshots`，保留当前截图作为 before 基线。
   - 如果不能编译，记录首个阻塞错误、失败命令和最小修复范围；不要在未理解现状时大面积重写。
   - 读取已有 `runtime_check_output/{APP}/` 截图和现有 diff/report，确认当前完成到哪一步。

3. **重新同步效果图和切图**
   - 重新读取用户提供或现有计划记录的效果图、切图路径和资源 natural size；不要默认沿用之前的读图结论。
   - 将“效果图尺寸、用户确认屏幕尺寸、当前 `app_egui_config.h` 尺寸、运行截图尺寸”列在同一张表里，发现不一致时先确认处理策略。
   - 将当前代码的页面/状态/资源使用情况和效果图测量结果对齐，形成差异清单：缺失、错位、尺寸错误、颜色错误、字体错误、滚动缺失、交互缺失、后端字段缺失。

4. **输出完善计划**
   - 在计划文档中增加“当前状态快照”“参考输入同步”“差异清单”“保留范围”“重做范围”“分阶段修复计划”。
   - 明确哪些现有代码保留，哪些局部替换，哪些文件会被新增或修改；默认小步修复，不整体推倒重写。
   - 每个阶段都要有 before/after 截图或 diff 验证点，保证半成品能逐步收口。

## 对话式确认

复杂效果图、多页面 APP 或业务状态较多时，必须先对齐理解，再开始大规模写代码。

### 1. 读图后反馈理解

读取效果图和切图目录后，先向用户反馈自己的理解，等待用户确认或纠正：

```text
我先按效果图理解如下：
- 页面/状态：...
- 屏幕尺寸和主要布局：我从效果图推断为 ...x...，方向为 ...；请确认真实目标屏幕是否就是这个尺寸，APP 是否按效果图画布 1:1 实现。
- 关键视觉元素：...
- 可用切图资源：...
- 切图 natural size 与放置策略：...
- 动态数据字段：...
- 可能的交互入口：...
- 不确定项：...

请确认这些理解是否正确，尤其是 ...。
```

反馈要具体到页面、区域、状态、资源和动态数据，不要只说“这是一个音乐播放器/仪表盘”。如果用户纠正了核心理解，更新理解后再继续；如果只是小细节，可记录假设并进入下一步。

### 2. 规划页面跳转

用户确认读图理解后，再给出页面和跳转规划：

- 页面清单：每个页面的 `page_id`、入口、返回方式、主状态。
- 跳转关系：点击/滑动/定时/后端事件分别触发什么页面变化。
- 状态覆盖：loading、empty、normal、error、disabled、selected 等是否需要单独截图验证。
- recording 覆盖：每个页面至少一个稳定截图，多页面 APP 不能只验证首页。

对跳转关系不确定时，先问用户确认；不要自行发明关键业务路径。

### 3. 抽取公用控件

页面跳转确认后，再规划公用控件和文件结构：

- 重复出现两次以上的区域优先抽取，例如 status bar、标题栏、底部导航、列表项、卡片、进度环、媒体控制区、设备 tile、空态/错误态。
- 只有单个页面使用、且后续变化不明确的元素，先保持页面内实现，避免过早抽象。
- 公用控件要定义清晰的 init、set_state/refresh、event callback 边界；页面只传数据，不直接改控件内部对象。
- 抽取计划需要说明复用理由、输入数据、事件输出和放置文件。

### 4. 实现前计划

开始文件编辑前，必须生成 Markdown 计划文档，并在对话中给用户摘要。推荐计划路径：

- `.claude/plans/{APP}-effect-to-egui-plan.md`
- APP 目录已经存在时，可追加保存 `example/{APP}/docs/effect_to_egui_plan.md`

计划文档必须包含以下阶段，每个阶段都要列出“目标、产物、验证方式、阻塞问题、完成条件”：

1. 当前状态同步：新建模式记录“无现有 APP”；现有项目完善模式记录当前文件、配置、构建/运行结果和 before 截图。
2. 输入确认：效果图、切图路径、APP 名称、用户确认的屏幕尺寸/方向。
3. 视觉测量：页面清单、元素坐标、尺寸、色值、字体、间距、滚动区域和状态。
4. 资源映射：切图文件、natural size、目标放置坐标、是否允许缩放、缺失资源。
5. 差异清单：现有实现与效果图/需求之间的缺口，标注优先级和修复方式。
6. 页面与控件规划：页面跳转、状态覆盖、recording 覆盖、公用控件抽取。
7. 后端与数据分层：PC mock、嵌入式接入 API、动态字段和控制入口。
8. 静态 1:1 还原：先固定首屏和关键状态，不先做泛化布局。
9. 交互与录制：点击、滑动、滚动、状态切换和多页面截图。
10. 像素级比对与迭代：参考图、运行截图、diff 输出、阈值、剩余差异。
11. 构建验收与优化移交：编译、runtime_check、必要时切换性能/体积 skill。

对话摘要至少包括：

- APP 目录和主要文件。
- 当前项目状态：新建/现有、当前是否可编译运行、before 截图路径或首个阻塞错误。
- 页面列表与跳转关系。
- 公用控件列表。
- 后端数据模型和 PC/MCU 分层。
- 现有项目需要保留、局部替换或重做的范围。
- 资源生成、runtime_check、像素级比对和差异报告路径。

用户已经明确要求“直接实现”且计划没有关键不确定项时，可以在写出计划文档和摘要后继续实施；存在屏幕尺寸、页面跳转、后端语义、视觉状态或缩放策略不确定时，先等用户确认。

## 固定工作流

1. **判断模式和同步现状**
   - 先判断是新建模式还是现有项目完善模式。
   - 现有项目完善模式必须先执行“读取当前状态、建立当前基线、重新同步效果图和切图”，再写计划。
   - 如果当前项目已经有计划文档，先读取并更新，不要另起一套相互冲突的计划。

2. **读图和资源**
   - 用视觉能力读取效果图，记录屏幕尺寸、主要区域、坐标、颜色、字体大小、图标位置、间距、状态。
   - 扫描切图目录，建立资源清单：文件名、尺寸、透明度、用途、是否可复用。
   - 将效果图尺寸与切图尺寸分开记录；效果图尺寸不能自动等同于目标屏幕尺寸，必须经用户确认。
   - 不要凭空重画已有切图；优先使用用户提供资源。缺失资源时再用控件绘制或说明缺口。
   - 完成读图后执行“对话式确认”，先让用户判断理解是否正确。

3. **写入阶段计划文档**
   - 在任何 APP 代码编辑前，创建或更新 `.claude/plans/{APP}-effect-to-egui-plan.md`。
   - 现有项目完善模式必须在计划中写入当前状态快照、before 基线、差异清单、保留范围和重做范围。
   - 计划中必须有视觉测量表和资源映射表；每个资源记录 natural width/height、目标 x/y、目标 w/h、是否 1:1、缩放原因。
   - 计划中必须有 pixel compare 表；每个页面/状态记录参考图、运行截图、diff 图、阈值和通过条件。
   - 如果用户还没有确认屏幕尺寸或缩放策略，计划状态标记为 blocked，不进入实现。

4. **规划页面和控件**
   - 在用户确认读图理解后，规划页面跳转、状态覆盖和 recording 覆盖。
   - 规划公用控件，只抽取真实复用或边界清晰的部分。
   - 对多页面 APP，先确认页面图和跳转逻辑，再开始批量写页面代码。
   - 现有项目完善模式要标明复用现有控件还是替换现有实现；不能无说明改动公共 API。

5. **建立或修正 APP 骨架**
   - 新建或更新 `example/{APP}/`，至少包含 `build.mk`、`app_egui_config.h`、页面源码、`resource/src/app_resource_config.json`。
   - 资源目录按仓库规则放在 `example/{APP}/resource/src/`，生成文件由资源管线产生，不手写到 `resource/img` 或 `resource/font`。
   - `build.mk` 必须包含资源源码目录和 include 目录；需要单文件源码时使用 `EGUI_CODE_SRC_FILES`。
   - 现有项目完善模式只补齐缺失骨架或修复错误配置；不要因为局部问题重建整个 APP 目录。

6. **UI 还原**
   - 按效果图先做静态 1:1 还原，再接入动态数据和交互。
   - 坐标、尺寸、色值优先来自效果图测量；避免“看起来差不多”的大概布局。
   - 对文本、图标、卡片、列表、进度条等建立清晰的本地刷新函数，例如 `page_xxx_refresh()`。
   - 保持 EmbeddedGUI C 风格：静态对象、明确 init、页面生命周期中启动/停止 timer，避免运行时大块 malloc。
   - 滚动区域必须在计划和 recording 中单独列出起始位置、滚动后位置、可见 item 范围和截图帧。
   - 现有项目完善模式按差异清单小步修复；每完成一个视觉区域或页面状态，就更新 before/after 记录。

7. **图片资源使用**
   - 用户提供的切图默认按 natural size 1:1 绘制；优先用设计切图本身的尺寸解决视觉还原问题。
   - 不要为了适配布局常规使用 `egui_canvas_draw_image_resize()`、`egui_image_file_set_resize()` 或等价运行时缩放接口处理用户切图。
   - 如果目标屏幕与效果图尺寸不同，先和用户确认缩放/裁剪/重新导出资源策略；不要直接把整页或切图按比例运行时缩放。
   - 确需缩放的例外必须写入计划文档：资源名、原始尺寸、目标尺寸、比例、原因、PC/MCU 性能和体积影响。
   - 最终验收前运行 `rg -n "draw_image_resize|set_resize|resize" example/{APP}`，对命中的用户切图缩放逐项说明或消除。

8. **后端数据适配**
   - UI 页面不得直接依赖硬件通信细节；必须先定义稳定的 UI 数据模型和控制 API。
   - 推荐分层：
     - `app_ui_state_t`：页面只读的缓存状态，包含当前展示需要的所有字段。
     - `uicode_ctrl_*()`：页面发起用户操作的控制入口。
     - `app_backend_*` 或项目现有后端层：负责 PC mock 和嵌入式真实对接。
   - PC 仿真必须提供 mock 数据和 mock 操作结果，使 `make run APP={APP} PORT=pc` 可直接预览。
   - 嵌入式路径只通过同一组控制 API 替换 transport/driver，不改页面逻辑。
   - 参考 `D:\workspace\gitlab\app_audio_module\Code\module_screen` 的模式：
     - 页面读取类似 `g_ui_device_state` 的缓存状态，不直接发协议帧。
     - 用户操作通过 `uicode_ctrl_*` 或高层 node API 进入后端。
     - 后端请求集中排队，GET 去重，SET 合并，高优先级用户操作优先于后台轮询。
     - 页面打开时订阅或 request-now 所需数据，页面关闭时清理订阅和 timer。
     - 后台 polling 要有水位限制，不能在嵌入式端压满 UART/SPI/队列。

9. **资源生成**
   - 使用 `resource-generation` skill 的规则维护 `app_resource_config.json`。
   - 新增中文、图标字体或动态文本后，运行：
     ```bash
     python scripts/tools/extract_font_text.py --app {APP}
     make resource_refresh APP={APP}
     ```
   - 图片格式选择要兼顾 PC 和 MCU：
     - 单色图标优先 `alpha` + 运行时着色。
     - 照片/背景优先 `rgb565`，需要体积优化时考虑 `qoi`。
     - 大图或专辑图一类动态资源要预留外部资源或流式更新方案，不把真实后端数据写死进页面。

10. **录制和运行验证**
   - 必须使用 `runtime-verification` skill。
   - 给新 APP 增加 recording 动作，覆盖首屏、关键交互、页面跳转和动态状态变化。
   - 验证命令：
     ```bash
     python scripts/code_runtime_check.py --app {APP} --keep-screenshots
     ```
   - 有 APP_SUB 时使用：
     ```bash
     python scripts/code_runtime_check.py --app {APP} --app-sub {SUB} --keep-screenshots
     ```
   - 截图必须与效果图逐项对比：位置、尺寸、颜色、文本、图标、留白、裁剪、滚动状态。只通过编译不算完成。

11. **像素级比对**
   - 比对输入必须是同尺寸图片；参考图、目标屏幕和运行截图尺寸不一致时，先回到屏幕尺寸确认，不要通过 resize 参考图掩盖问题。
   - 每个计划中的页面/状态至少有一组 `reference -> runtime frame -> diff image`；滚动页面要包含滚动前和滚动后的稳定截图。
   - 使用仓库现有图像 diff 工具或基于 PIL/ImageChops 的最小脚本计算：`max_diff`、`mean_abs`、`rms`、`diff_pixel_ratio`、`hot_pixel_ratio`、`diff_bbox`。
   - 默认按严格 1:1 目标收口；如果字体抗锯齿、RGB565 量化或控件绘制导致不能零差异，必须在计划和报告中写明允许阈值及原因。
   - 输出目录建议为 `runtime_check_output/{APP}/visual_diff/`，报告建议为 `example/{APP}/docs/effect_to_egui_pixel_compare.md`。
   - `runtime_check` 输出 `ALL PASSED` 后，仍需完成像素比对报告；没有 diff 指标和差异图时不能宣称视觉验收完成。

12. **性能和体积收口**
   - 如果用户提出性能目标，或 APP 包含动画、滚动、频繁后端刷新、大图、外部资源、真机 SPI 刷屏，读取 `performance-optimization` skill，围绕当前 `APP/APP_SUB` 建立应用级场景基线。
   - 如果用户提出 ROM/RAM/heap/stack 预算，或 APP 包含大量资源、字体、压缩图片、重控件、HQ 绘制路径，读取 `code-size-optimization` skill，围绕当前 `APP/APP_SUB` 建立体积基线。
   - 不要在本 skill 中复制优化细则；这里只负责判断是否需要切换到对应优化技能，并把当前 APP、场景、资源和验证结果传递过去。

## PC 与嵌入式兼容要求

- `EGUI_PORT == EGUI_PORT_TYPE_PC` 时：
  - 使用 mock state、mock timer、mock backend response。
  - 可以主动构造多组状态用于 recording，例如空列表、加载中、正常数据、错误态。
  - 不包含 MCU SDK 头文件、寄存器、DMA、真实 UART/SPI 调用。

- `EGUI_PORT == EGUI_PORT_TYPE_MCU` 或非 PC 端时：
  - 后端只暴露同一组 UI state/control API。
  - 真实通信、队列、轮询、DMA、外部资源读取放在 porting/backend 层。
  - 高频操作要做 coalesce，重复读取要 dedupe，后台轮询要能暂停。
  - artwork、大列表、歌词、频谱等大数据要设计为增量/缓存/流式更新，不阻塞 GUI 刷新。

- 任何页面逻辑都不能因为 PC mock 简单就跳过嵌入式边界；从第一版就保留 transport 替换点。

## 验收标准

- `.claude/plans/{APP}-effect-to-egui-plan.md` 已生成，且包含阶段计划、用户确认的屏幕尺寸/方向、视觉测量表、资源映射表和像素比对计划。
- `make all APP={APP} PORT=pc` 通过。
- `python scripts/code_runtime_check.py --app {APP} --keep-screenshots` 输出 `ALL PASSED`。
- `runtime_check_output/{APP}/default/frame_*.png` 覆盖用户指定的关键状态。
- 像素级比对报告已生成，包含同尺寸确认、diff 指标、diff 图、剩余差异和处理结论；只做人工目测不算通过。
- 截图与效果图的差异已逐项检查并修正，不能黑屏、空白、错位、文字贴边、资源缺失、滚动状态缺失。
- 用户切图没有无说明地运行时 resize；所有 `draw_image_resize`、`set_resize` 或等价缩放命中都有计划记录和用户确认。
- PC mock 数据能独立预览；嵌入式后端有清晰 API 接口和替换点。
- 最终回复必须列出：
  - APP 路径和运行命令。
  - 使用的效果图与切图目录。
  - 阶段计划文档路径。
  - 用户确认过的屏幕尺寸/方向、读图理解、页面跳转和公用控件规划。
  - 后端数据模型和 PC/MCU 分层说明。
  - runtime_check 截图目录、像素 diff 报告路径和剩余视觉差异。
  - 如已触发性能或体积优化：对应技能、基线场景、关键指标和验证结果。

## 常见失败处理

- 效果图和目标屏幕尺寸不一致：先确认目标屏幕和处理策略；不要直接 resize 整页或参考图。
- 效果图和切图尺寸不一致：先以效果图坐标为准，记录资源 natural size 和放置策略；确需缩放时先确认。
- 缺少字体字符：补 text 文件，重新 `extract_font_text.py` 和 `make resource_refresh`。
- PC 能跑、MCU 不能编译：检查 `#if EGUI_PORT` 分支，确认 PC mock 没引用嵌入式 SDK，MCU 分支 include 路径在 `build.mk` 中。
- 运行截图状态不全：补 recording action 或 mock 状态切换，不要只截首屏。
- 视觉差异较大但 runtime_check 通过：回到像素比对报告，按 diff_bbox 拆分为坐标、尺寸、颜色、字体、资源、滚动状态问题逐项修正。
- 后端字段临时写死：改为进入 `app_ui_state_t` 默认值或 mock provider，真实后端通过同名 API 更新。
