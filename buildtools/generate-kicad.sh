#!/bin/bash

## Generate pdf files for schematics
kicad-cli sch export pdf \
    --output ./dist/controller_schematics.pdf \
    ./hardware/module_controller/ModuleController.kicad_sch

