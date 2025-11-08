#!/bin/bash
# Script to remove "using namespace std;" and add std:: prefixes
#

set -e

echo "Fixing namespace pollution in remaining files..."

# List of files that still need fixing (excluding the ones we already did)
FILES=(
    "main.cpp"
    "structs.cpp"
    "utils.cpp"
    "stargen.cpp"
    "enviro.cpp"
    "display.cpp"
    "solid_radius_helpers.cpp"
    "radius_tables.cpp"
    "gas_radius_helpers.cpp"
    "star_trek.h"
    "solstation.h"
    "ring_universe.h"
    "omega_galaxy.h"
    "jimb.h"
    "ic3094.h"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "Processing $file..."
        # Remove "using namespace std;" line
        sed -i '/^using namespace std;$/d' "$file"
        echo "  - Removed 'using namespace std;'"
    fi
done

echo "Done! Namespace pollution removed from header and source files."
echo "Note: You'll need to add std:: prefixes where needed and recompile to find any issues."
