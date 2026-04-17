# HelloSVGSpec

`HelloSVGSpec` 是 EmbeddedGUI 的 SVG 规范对比基座。

它的目标不是做展示型 Demo，而是把一组可重复执行的 SVG fixture 放进统一验证链路里，对比 `egui_image_svg` 的运行时输出和外部参考渲染结果。

## 默认流程

当前默认流程只有一条主链路：

1. 读取 `fixtures/manifest.json`。
2. 生成 `svg_spec_cases_gen.h`。
3. `HelloSVGSpec` 直接加载 `fixtures/` 里的大视口 SVG 文档。
4. 运行时把整张 SVG 文档渲染到统一 `320x320` image box。
5. 参考渲染器直接对同一份 fixture 做 large-viewport rerender。
6. 对整张大视口输出做像素和几何统计，生成 preview、compare、overview、report。

默认流程已经不再使用：

- canonical 小图主判定
- 第二条“只给 review 看”的额外大图 rerender
- 运行时或参考侧临时再包一层 wrapper SVG

现在 preview、report、人工审查看到的就是主判定产物本身。

## fixture 约定

`fixtures/*.svg` 现在就是统一的大视口文档：

- root 固定为 `320x320`
- 白底背景直接放在 fixture 里
- 被测 SVG 内容作为内层 `<svg>` 铺满整个大视口

也就是说，仓库里的 fixture 本体就是默认比对输入，不再保留一份“小图 spec 输入”再在运行时临时放大。

## 运行时侧

`HelloSVGSpec` 运行时会把 fixture 直接放进统一 image box：

- 使用 `HELLO_SVG_SPEC_VIEWPORT_X/Y/WIDTH/HEIGHT` 控制 image box
- `egui_view_image_set_image_type(..., EGUI_VIEW_IMAGE_TYPE_RESIZE)` 负责大视口图像盒渲染

当前默认画布：

- 运行画布 `320 x 320`
- PFB `80 x 80`

## 参考渲染器

支持的参考后端：

- `auto`
  优先使用本机 `Edge/Chrome`，但只有在当前环境确实支持 large-viewport SVG capture 时才会选它；否则自动回退到 `CairoSVG`
- `edge`
  显式强制使用本机 `Edge/Chrome`
- `cairosvg`
  显式强制使用 `CairoSVG`

在当前 Windows headless Edge 环境里，如果浏览器无法稳定输出 large-viewport SVG 截图，`auto` 会自动降级到 `CairoSVG`，避免默认流程走一条会产出白图的链路。

## 主判定

默认主判定基于 full-page large-viewport parity 结果，主要统计：

- `mean_abs`
- `rms`
- `hot_pixel_ratio`
- `bbox_edge_delta`
- `bbox_area_delta_ratio`
- `max_difference`
- `diff_pixels`
- `diff_pixel_ratio`

报告里仍然保留部分历史字段名，例如 `review_*`、`canonical_*`，主要是为了兼容现有报告消费方；默认语义已经统一到单条 large-viewport parity pass。

## manifest

`manifest.json` 仍然保留 case 元数据和阈值配置。

其中这些字段现在主要用于元数据、阈值和试验参数：

- `render_width / render_height / composite_x / composite_y`
- `review_*`
- `reference_engine_overrides`

默认 fixture 本体已经是 full-page large-viewport 文档，所以主流程不再依赖 manifest 去临时生成另一份大图输入。

## 常用命令

运行默认校验：

```bash
python scripts/checks/svg_validation_check.py
```

运行单个 case：

```bash
python scripts/checks/svg_validation_check.py --case path_smooth_quad_fill
```

运行单个 case 并带内部单测：

```bash
python scripts/checks/svg_validation_check.py --with-unit --case path_smooth_quad_fill
```

显式指定参考后端：

```bash
python scripts/checks/svg_validation_check.py --reference-engine edge
python scripts/checks/svg_validation_check.py --reference-engine cairosvg
```

对单个 case 临时试验 image box：

```bash
python scripts/checks/svg_validation_check.py --case rect_fill_basic --trial-render-box 120x120
python scripts/checks/svg_validation_check.py --case rect_fill_basic --trial-render-box 120x120@8,8
```

## 输出产物

默认输出目录：

- `runtime_check_output/HelloSVGSpec/{output_subdir}/`

常见产物：

- `svg_validation_report.json`
- `svg_validation_artifacts_summary.json`
- `svg_validation_overview.png`
- `svg_validation_overview_all_cases.png`
- `svg_validation_review_pages/`
- `compare/`
- `reference/`

其中 `reference/` 目录里保存的是从 repo fixture 复制出来的大视口 SVG 及其参考 PNG，`compare/` 目录里保存的是 reference / actual / diff 的并排图。

## 结论

当前 `HelloSVGSpec` 的默认链路已经收敛为：

- repo 内大视口 fixture
- 单一 large-viewport rerender
- 单一主判定产物
- preview 和 report 与主判定完全同源

这就是后续继续补 `egui_image_svg` 行为缺口时的默认回归基线。
