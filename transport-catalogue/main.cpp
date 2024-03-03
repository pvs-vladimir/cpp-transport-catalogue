#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <iostream>

int main() {
    using namespace transport_catalogue;

    TransportCatalogue catalogue;
    json_reader::JsonReader json_reader(std::cin);
    json_reader.LoadCatalogueData(catalogue);
    map_renderer::MapRenderer renderer(json_reader.LoadRenderSettings());
    request_handler::RequestHandler request_handler(catalogue, renderer);
    auto answers_json = request_handler.RenderAnswersJson(json_reader.LoadStatRequests());
    json::Print(answers_json, std::cout);
}