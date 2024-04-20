#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <iostream>

int main() {
    using namespace transport_catalogue;

    TransportCatalogue catalogue;
    json_reader::JsonReader json_reader(std::cin);
    json_reader.LoadCatalogueData(catalogue);
    map_renderer::MapRenderer renderer(json_reader.LoadRenderSettings());
    transport_router::TransportRouter router(catalogue, json_reader.LoadRouterSettings());
    request_handler::RequestHandler request_handler(catalogue, renderer, router);
    auto answers_json = json_reader.RenderAnswersJson(request_handler, json_reader.LoadStatRequests());
    json::Print(answers_json, std::cout);;
}