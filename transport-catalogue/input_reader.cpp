#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace transport_catalogue {
namespace input_reader {

namespace detail {

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

std::unordered_map<std::string_view, int> ParseDistances(std::string_view string) {
    std::unordered_map<std::string_view, int> result;

    std::vector<std::string_view> ways = Split(string, ',');
    for (auto way : ways) {
        std::string_view distance_str = way.substr(0, way.find_first_of('m'));
        std::string_view stop = way.substr(way.find_first_of('o') + 1);
        result[Trim(stop)] = std::stoi(std::string(distance_str));
    }

    return result;
}

StopDescription ParseStopDescription(std::string_view string, ParseStopCommand command) {
    auto comma1 = string.find(',');
    auto comma2 = string.find(',', comma1 + 1);
    switch (command) {
    case ParseStopCommand::COORDINATES:
        return {ParseCoordinates(string.substr(0, comma2)), {}};
    case ParseStopCommand::DISTANCES:
        return {{}, ParseDistances(string.substr(comma2 + 1))};
    default:
        return {{}, {}};
    }
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = detail::ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    using namespace std::literals;

    for (auto command_description : commands_) {
        if (command_description.command == "Stop"s) {
            StopDescription stop_description = detail::ParseStopDescription(command_description.description, ParseStopCommand::COORDINATES);
            catalogue.AddStop({command_description.id, stop_description.coordinates});
        }
    }

    for (auto command_description : commands_) {
        if (command_description.command == "Stop"s) {
            StopDescription stop_description = detail::ParseStopDescription(command_description.description, ParseStopCommand::DISTANCES);
            for (auto [way, distance] : stop_description.distances) {
                catalogue.AddDistance(command_description.id, way, distance);
            }
        }
    }

    for (auto command_description : commands_) {
        if (command_description.command == "Bus"s) {
            std::vector<std::string_view> route = detail::ParseRoute(command_description.description);
            std::vector<Stop*> stops;
            for (auto stop : route) {
                stops.push_back(catalogue.FindStop(stop));
            }
            catalogue.AddBus({command_description.id, stops});
        }
    }
}

}
}