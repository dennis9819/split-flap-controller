#!/bin/bash

# This file is part of SplitFlapController project.
# Copyright (C) 2024-2025 GuniaLabs (www.dennisgunia.de)
# Author: Dennis Gunia 
#
# This program is licensed under the AGPLv3 license. You can find a copy
# of the license in the LICENSE file in the root directory of this
# project.

# Run the nginx server with the specified configuration file in background
sudo nginx -c $(pwd)/nginx.conf