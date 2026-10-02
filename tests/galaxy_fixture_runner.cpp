#include "galaxy_sector.h"
#include "star_properties.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--emit")) {
        std::cerr << "usage: galaxy_fixture_runner FILE [--emit]\n";
        return 2;
    }

    const bool emit = argc == 3;
    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "cannot open fixture file: " << argv[1] << '\n';
        return 2;
    }

    int cases    = 0;
    int failures = 0;
    std::string line;
    for (int line_number = 1; std::getline(input, line); ++line_number) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream row(line);
        std::string id;
        int32_t sector_x, sector_y, sector_z;
        int rarity;
        if (!(row >> id >> sector_x >> sector_y >> sector_z >> rarity) || rarity < 0 || rarity > 32767) {
            std::cerr << "malformed galaxy input at line " << line_number << '\n';
            return 2;
        }

        const auto star = galaxy_star_at(sector_x, sector_y, sector_z, static_cast<int16_t>(rarity));
        ++cases;
        if (emit) {
            std::cout << id << '\t' << sector_x << '\t' << sector_y << '\t' << sector_z << '\t' << rarity;
            if (star) {
                const auto properties = derive_star_properties(star->x, star->y, star->z);
                std::cout << "\t1\t" << star->x << '\t' << star->y << '\t' << star->z << '\t' << properties.star_class
                          << '\t' << properties.radius_milli << '\t' << static_cast<int>(properties.red) << '\t'
                          << static_cast<int>(properties.green) << '\t' << static_cast<int>(properties.blue) << '\t'
                          << static_cast<int>(properties.spin);
            } else {
                std::cout << "\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0";
            }
            std::cout << '\n';
            continue;
        }

        int present, x, y, z, star_class, radius_milli, red, green, blue, spin;
        std::string extra;
        if (!(row >> present >> x >> y >> z >> star_class >> radius_milli >> red >> green >> blue >> spin) ||
            (row >> extra) || (present != 0 && present != 1)) {
            std::cerr << "malformed galaxy expectations at line " << line_number << '\n';
            return 2;
        }

        if (static_cast<bool>(star) != static_cast<bool>(present)) {
            std::cerr << id << " (line " << line_number << "): star presence differs from fixture\n";
            ++failures;
            continue;
        }
        if (!star) {
            continue;
        }

        const auto properties = derive_star_properties(star->x, star->y, star->z);
        if (star->x != x || star->y != y || star->z != z || properties.star_class != star_class ||
            properties.radius_milli != radius_milli || properties.red != red || properties.green != green ||
            properties.blue != blue || properties.spin != spin) {
            std::cerr << id << " (line " << line_number << "): galaxy/star output differs from fixture\n";
            ++failures;
        }
    }

    if (input.bad() || cases == 0) {
        std::cerr << "fixture file could not be read or has no cases\n";
        return 2;
    }
    if (!emit) {
        std::cout << "checked " << cases << " galaxy sectors; " << failures << " mismatches\n";
    }
    return failures == 0 ? 0 : 1;
}
