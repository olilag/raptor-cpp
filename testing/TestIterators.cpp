#include <DataStructures.hpp>
#include <just_gtfs.h>
#include <iostream>
#include <ranges>

int main()
{
    std::string filename = "/home/oliver/Documents/C++/lagoo/project/BA-data";
    gtfs::Feed feed = gtfs::Feed(filename);
    feed.read_feed();
    auto [route_traversal, st] = raptor::GTFSFeedParser::parseFeed(feed);
    raptor::RouteTraversal rt = std::move(route_traversal);
    raptor::Stops stops = std::move(st);
    auto tr = raptor::IdTranslator::getInstance;
    for (auto&& route : std::views::iota(0ul, tr().route_count()))
    {
        std::cout << "Stops for route " << feed.get_route(tr().at(raptor::RouteId(route)).rId)->route_short_name << " (" << route << ")\n";
        for (auto&& stop : rt.getStops(route))
        {
            if (stop != raptor::undefined::stop)
                std::cout << feed.get_stops()[stop].stop_name << " (" << stop << ")\n";
        }
        std::cout << "----------------------\n";
    }
    for (auto&& stop : std::views::iota(0ul, tr().stop_count()))
    {
        /*for (auto&& route : stops.getRoutes(stop))
        {
            std::cout << "Data for stop " << feed.get_stops()[stop].stop_name << " (" << stop << ") and route ";
            std::cout << feed.get_route(tr().at(route).rId)->route_short_name << " (" << route << ")\n";
            for (auto&& trip : rt.getTripsFromStop(route, stop))
            {
                if (trip.sId != raptor::undefined::service && tr().at(trip.sId) == "Prac.dny_0")
                    std::cout << "  " << trip.tId << '-' << "-a" << toString(trip.arrival) << "-d" << toString(trip.departure) << '\n';
            }
        }*/
        std::cout << "Data for stop " << feed.get_stops()[stop].stop_name << " (" << stop << ")\n";
        /*for (auto&& trans : stops.getTransfers(stop))
        {
            std::cout << "  " << feed.get_stops()[trans.target_stop].stop_name << " - distance: " << trans.distance << '\n';
        }
        std::cout << "-----------------------\n";*/
        for (auto&& route : stops.getRoutes(stop))
        {
            std::cout << "  " << feed.get_route(tr().at(route).rId)->route_short_name << '\n';
        }
        std::cout << ">-----------------------<" << std::endl;
    }
}
