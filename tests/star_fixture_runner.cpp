#include "star_properties.h"
#include "noctis-d.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--emit")) {
        std::cerr << "usage: star_fixture_runner FILE [--emit]\n";
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
    std::array<bool, star_classes> class_seen{};
    std::string line;
    for (int line_number = 1; std::getline(input, line); ++line_number) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream row(line);
        std::string id;
        double x, y, z;
        if (!(row >> id >> x >> y >> z)) {
            std::cerr << "malformed fixture input at line " << line_number << '\n';
            return 2;
        }

        const auto actual = derive_star_properties(x, y, z);
        class_seen[actual.star_class] = true;
        ++cases;
        if (emit) {
            std::cout << std::setprecision(17) << id << '\t' << x << '\t' << y << '\t' << z << '\t' << actual.star_class
                      << '\t' << actual.radius_milli << '\t' << static_cast<int>(actual.red) << '\t'
                      << static_cast<int>(actual.green) << '\t' << static_cast<int>(actual.blue) << '\t'
                      << static_cast<int>(actual.spin) << '\n';
            continue;
        }

        int star_class, radius_milli, red, green, blue, spin;
        std::string extra;
        if (!(row >> star_class >> radius_milli >> red >> green >> blue >> spin) || (row >> extra)) {
            std::cerr << "malformed expected values at line " << line_number << '\n';
            return 2;
        }
        if (actual.star_class != star_class || actual.radius_milli != radius_milli || actual.red != red ||
            actual.green != green || actual.blue != blue || actual.spin != spin) {
            std::cerr << id << " (line " << line_number << "): expected " << star_class << '/' << radius_milli << '/'
                      << red << '/' << green << '/' << blue << '/' << spin << ", got " << actual.star_class << '/'
                      << actual.radius_milli << '/' << static_cast<int>(actual.red) << '/'
                      << static_cast<int>(actual.green) << '/' << static_cast<int>(actual.blue) << '/'
                      << static_cast<int>(actual.spin) << '\n';
            ++failures;
        }
    }

    if (input.bad() || cases == 0) {
        std::cerr << "fixture file could not be read or has no cases\n";
        return 2;
    }
    if (!emit) {
        for (int star_class = 0; star_class < star_classes; ++star_class) {
            if (!class_seen[star_class]) {
                std::cerr << "fixture does not cover star class " << star_class << '\n';
                ++failures;
            }
        }
    }
    if (!emit) {
        std::cout << "checked " << cases << " star cases; " << failures << " mismatches\n";
    }
    return failures == 0 ? 0 : 1;
}
