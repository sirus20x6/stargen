#!/bin/bash
# Script to add std:: prefix to common standard library types

set -e

echo "Adding std:: prefixes to common standard library identifiers..."

# List of .cpp files to fix
CPP_FILES=(
    "accrete.cpp"
    "andromeda.cpp"
    "andromeda2.cpp"
    "display.cpp"
    "dole.cpp"
    "elements.cpp"
    "enviro.cpp"
    "gas_radius_helpers.cpp"
    "ic3094.cpp"
    "ic3094_2.cpp"
    "jimb.cpp"
    "main.cpp"
    "omega_galaxy.cpp"
    "omega_galaxy2.cpp"
    "Planetary_Habitability_Laboratory.cpp"
    "planets.cpp"
    "radius_tables.cpp"
    "ring_universe.cpp"
    "ring_universe2.cpp"
    "solid_radius_helpers.cpp"
    "solstation.cpp"
    "stargen.cpp"
    "star_temps.cpp"
    "star_trek.cpp"
    "structs.cpp"
    "utils.cpp"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "Processing $file..."

        # Fix common patterns - be careful not to double-prefix
        # string (but not std::string, string& after ::, or inside angle brackets)
        sed -i 's/\([^:]\|^\)string\([^a-zA-Z_]\)/\1std::string\2/g' "$file"

        # Stream objects
        sed -i 's/\([^:]\|^\)cout\([^a-zA-Z_]\)/\1std::cout\2/g' "$file"
        sed -i 's/\([^:]\|^\)cerr\([^a-zA-Z_]\)/\1std::cerr\2/g' "$file"
        sed -i 's/\([^:]\|^\)cin\([^a-zA-Z_]\)/\1std::cin\2/g' "$file"
        sed -i 's/\([^:]\|^\)endl\([^a-zA-Z_]\)/\1std::endl\2/g' "$file"

        # Common containers
        sed -i 's/\([^:]\|^\)vector\([^a-zA-Z_]\)/\1std::vector\2/g' "$file"
        sed -i 's/\([^:]\|^\)map\([^a-zA-Z_]\)/\1std::map\2/g' "$file"

        # Streams
        sed -i 's/\([^:]\|^\)ofstream\([^a-zA-Z_]\)/\1std::ofstream\2/g' "$file"
        sed -i 's/\([^:]\|^\)ifstream\([^a-zA-Z_]\)/\1std::ifstream\2/g' "$file"
        sed -i 's/\([^:]\|^\)fstream\([^a-zA-Z_]\)/\1std::fstream\2/g' "$file"
        sed -i 's/\([^:]\|^\)stringstream\([^a-zA-Z_]\)/\1std::stringstream\2/g' "$file"
        sed -i 's/\([^:]\|^\)ostream\([^a-zA-Z_]\)/\1std::ostream\2/g' "$file"
        sed -i 's/\([^:]\|^\)istream\([^a-zA-Z_]\)/\1std::istream\2/g' "$file"

        echo "  - Added std:: prefixes"
    fi
done

echo "Done! std:: prefixes added. Recompiling to check..."
