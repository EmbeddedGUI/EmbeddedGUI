架构原理
========

本章节用于理解 EmbeddedGUI 的核心设计思想。建议按下面顺序阅读：

1. `pfb_design` - 了解局部帧缓冲设计
2. `dirty_rect` - 了解脏矩形机制
3. `view_system` - 了解视图模型
4. `layout_system` 与 `flexlayout` - 了解布局系统
5. `event_system` 与 `event_lite` - 了解输入与事件分发
6. `virtual_stage` - 了解虚拟容器与按需实例化
7. `rendering_pipeline` - 了解渲染流水线

.. toctree::
   :maxdepth: 2

   pfb_design
   core_ram_guide
   dirty_rect
   punch_region
   oop_pattern
   view_system
   property_system
   subject_observer
   event_lite
   event_system
   layout_system
   flexlayout
   ../multi-display
   virtual_stage
   rendering_pipeline
   theme_system
