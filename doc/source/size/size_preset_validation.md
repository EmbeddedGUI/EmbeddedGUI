# Size Preset Validation

- Commit: `1fe189eb`
- Date: 2026-05-21T16:09:03.950909
- Validation target: `APP=HelloSizeAnalysis APP_SUB=preset_validation PORT=qemu CPU_ARCH=cortex-m0plus`
- Validation method: rewrite the app-local preset override header from each template, then build the qemu target and collect ELF section sizes.
- Purpose: verify that preset template files themselves are syntactically valid and can be used as direct app-local config bodies.

## Result

| Preset | Build | Text | Rodata | Data | Bss | Total ROM | Template |
|--------|-------|-----:|-------:|-----:|----:|----------:|----------|
| tiny_rom | PASS | 12680 | 612 | 1368 | 6188 | 13292 | `example/HelloSizeAnalysis/ConfigProfiles/tiny_rom/app_egui_config.h` |
| basic_ui | PASS | 14584 | 596 | 1368 | 6508 | 15180 | `example/HelloSizeAnalysis/ConfigProfiles/basic_ui/app_egui_config.h` |
| dashboard | PASS | 19016 | 660 | 1368 | 7780 | 19676 | `example/HelloSizeAnalysis/ConfigProfiles/dashboard/app_egui_config.h` |
| full_feature | PASS | 30688 | 3980 | 1368 | 8068 | 34668 | `example/HelloSizeAnalysis/ConfigProfiles/full_feature/app_egui_config.h` |
## Reproduce

```bash
python scripts/size_analysis/main.py size-preset-validation-to-doc
```

Raw JSON is written to `output/size_preset_validation_results.json`.
