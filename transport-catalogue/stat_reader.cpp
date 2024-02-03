#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <string>

namespace transport_catalogue {
namespace stat_reader {

namespace detail {

std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

std::pair<std::string_view, std::string_view> ParseRequest(std::string_view request) {
    auto trimed_request = Trim(request);
    const auto space = trimed_request.find(' ');
    if (space == trimed_request.npos) {
        return {};
    }
    auto command = trimed_request.substr(0, space);
    auto id = trimed_request.substr(space + 1);
    return {command, id.substr(id.find_first_not_of(' '))};
}

void PrintBusStat(std::string_view bus_name, std::optional<Info> info, std::ostream& output) {
    using namespace std::literals;

    output << "Bus "s << std::string{bus_name} << ": "s;
    if (info.has_value()) {
        output << info.value().stops_count << " stops on route, "s;
        output << info.value().unique_stops_count << " unique stops, "s;
        output << std::setprecision(6) << info.value().route_length << " route length"s << std::endl;
    } else {
        output << "not found"s << std::endl;
    }
}

void PrintStopStat(std::string_view stop_name, std::optional<std::set<std::string_view>> buses, std::ostream& output) {
    using namespace std::literals;
    
    output << "Stop "s << std::string{stop_name} << ": "s;
    if (!buses.has_value()) {
        output << "not found"s << std::endl;
    } else if (buses.value().empty()) {
        output << "no buses"s << std::endl;
    } else {
        output << "buses"s;
        for (auto bus : buses.value()) {
            output << " "s << std::string{bus};
        }
        output << std::endl;
    }
}

}

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    using namespace std::literals;

    auto parsed_request = detail::ParseRequest(request);
    if (std::string{parsed_request.first} == "Bus"s) {
        detail::PrintBusStat(parsed_request.second, transport_catalogue.GetBusInfo(parsed_request.second), output);
    } else if (std::string{parsed_request.first} == "Stop"s) {
        detail::PrintStopStat(parsed_request.second, transport_catalogue.GetStopInfo(parsed_request.second), output);
    }
}

}
}