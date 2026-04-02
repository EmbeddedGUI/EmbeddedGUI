# Size Preset Validation

- Commit: `09190fa`
- Date: 2026-04-02T11:13:50.598891
- Validation target: `APP=HelloSizeAnalysis APP_SUB=preset_validation PORT=qemu CPU_ARCH=cortex-m0plus`
- Validation method: rewrite the app-local preset override header from each template, then build the qemu target and collect ELF section sizes.
- Purpose: verify that preset template files themselves are syntactically valid and can be used as direct app-local config bodies.

## Result

| Preset | Build | Text | Rodata | Data | Bss | Total ROM | Template |
|--------|-------|-----:|-------:|-----:|----:|----------:|----------|
| tiny_rom | PASS | 9156 | 556 | 24 | 5760 | 9712 | `example/HelloSizeAnalysis/ConfigProfiles/tiny_rom/app_egui_config.h` |
## Reproduce

```bash
python scripts/size_analysis/size_preset_validation_to_doc.py
```

Raw JSON is written to `output/size_preset_validation_results.json`.
