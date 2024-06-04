#include <DataStructures.hpp>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include <cmath>
#include <numbers>

namespace raptor
{	
	double GTFSFeedParser::toRadians(const double degree)
	{
		double one_deg = std::numbers::pi / 180;
	    return (one_deg * degree);
	}
	
	double GTFSFeedParser::distance(double lat1, double long1,
	                                     double lat2, double long2)
	{
		lat1 = toRadians(lat1);
	    long1 = toRadians(long1);
	    lat2 = toRadians(lat2);
	    long2 = toRadians(long2);
	    
	    double dlong = long2 - long1;
        double dlat = lat2 - lat1;
        double ans = std::pow(sin(dlat / 2), 2) + std::cos(lat1) * std::cos(lat2) * std::pow(sin(dlong / 2), 2);
        ans = 2 * std::asin(std::sqrt(ans));
        // Earth radius
        constexpr double R = 6371;
        return ans * R;
	}
	
	std::vector<std::pair<size_t, size_t>> GTFSFeedParser::findLongestTrips(const std::tuple_element_t<0, RTData>& data)
	{
		std::vector<std::pair<size_t, size_t>> result(data.size(), std::pair(0, 0));
		for (auto&& [rId, trips] : data)
		{
			size_t index = 0;
			for (auto&& [tId, tripBlock] : trips)
			{
				if (tripBlock.size() > result[rId].first)
					result[rId] = std::pair(tripBlock.size(), index);
				++index;
			}
		}
		return result;
	}
	
	std::tuple_element_t<0, RTData> GTFSFeedParser::removeBadTrips(std::tuple_element_t<0, RTData>& data)
	{
		auto result = std::tuple_element_t<0, RTData>();
		result.reserve(data.size());
		auto max_trip_sizes = findLongestTrips(data);
		for (auto&& [rId, trips] : data)
		{
			result.emplace_back(rId, std::tuple_element_t<0, RTData>::value_type::second_type());
			auto condition = [&](const std::pair<TripId, RouteRawData::mapped_type> item)
			{
				return item.second.size() != max_trip_sizes[rId].first;
			};
			auto trips_copy = trips;
			std::remove_copy_if(trips_copy.begin(), trips_copy.end(), std::back_inserter(result.back().second), condition);
		}
		return result;
	}
	
	std::tuple_element_t<0, RTData> GTFSFeedParser::sortRouteRawData(std::unordered_map<RouteId, RouteRawData>&& data)
	{
		std::tuple_element_t<0, RTData> result;
		result.reserve(IdTranslator::getInstance().route_count());
		for (size_t i = 0; i < IdTranslator::getInstance().route_count(); ++i)
		{
			result.emplace_back(i, std::vector<std::pair<TripId, RouteRawData::mapped_type>>());
		}
		// sort individual blocks
		for (auto&& [route, rd] : data)
		{
			for (auto&& [trip, td] : rd)
			{
				result[route].second.emplace_back(trip, std::move(td));
			}
		}
		
		auto CompareBlock = [](const TripBlock& a, const TripBlock& b)
		{
			return a.departure < b.departure;
		};
		
		auto CompareTrips = [](const std::pair<TripId, std::vector<TripBlock>>& lhs, const std::pair<TripId, std::vector<TripBlock>>& rhs)
		{
			return lhs.second[0].arrival < rhs.second[0].arrival;
		};
		for (auto&& [routeId, routeData] : result)
		{
			for (auto&& [tripId, blocks] : routeData)
			{
				std::sort(blocks.begin(), blocks.end(), CompareBlock);
			}
			std::sort(routeData.begin(), routeData.end(), CompareTrips);
		}
		auto CompareRoute = [](const std::tuple_element_t<0, RTData>::value_type& x1, const std::tuple_element_t<0, RTData>::value_type& x2)
		{ 
			return x1.first < x2.first;
		};
		std::sort(result.begin(), result.end(), CompareRoute);
		result = removeBadTrips(result);
		return result;
	}

	std::vector<StopRawData> GTFSFeedParser::sortStopRawData(std::unordered_map<StopId, StopData>&& data)
	{
		std::vector<StopRawData> result;
		for (auto&& d : data)
		{
			result.push_back(std::move(d));
		}
		auto CompareStops = [](const std::pair<StopId, StopData>& a, const std::pair<StopId, StopData>& b)
		{
			return a.first < b.first;
		};
		std::sort(result.begin(), result.end(), CompareStops);
		return result;
	}
	
	const Data GTFSFeedParser::parseFeed(const gtfs::Feed &feed)
	{
		GTFSFeedParser::prepareTranslator(feed);
		std::unordered_map<RouteId, RouteRawData> result1;
		std::unordered_map<RouteId, std::unordered_set<StopId>> stopsVisited;
		std::unordered_map<RouteId, std::unordered_set<TripId>> tripsVisited;
		std::unordered_map<StopId, std::unordered_set<RouteId>> stopRoutes;
		auto&& stop_times = feed.get_stop_times();
		auto tr = IdTranslator::getInstance;
		for (auto&& stop_time : stop_times)
		{
			TripId tId = IdTranslator::getInstance().at(stop_time.trip_id, IdTranslator::TripTag());
			auto&& trip = feed.get_trips()[tId];        // I am indexing them based on this vector, so this is the correct trip
			RouteId rId = IdTranslator::getInstance().at(InternalRouteId(trip.route_id, trip));
			StopId sId = IdTranslator::getInstance().at(stop_time.stop_id, IdTranslator::StopTag());
			ServiceId service = IdTranslator::getInstance().at(trip.service_id, IdTranslator::ServiceTag());
			Time_t arrival = stop_time.arrival_time.get_total_seconds();
			Time_t departure = stop_time.departure_time.get_total_seconds();
			if (!stopsVisited.contains(rId))
			{
				stopsVisited.insert(std::make_pair(rId, std::unordered_set<StopId>(sId)));
			}
			stopsVisited[rId].insert(sId);
		
			if (!tripsVisited.contains(rId))
			{
				tripsVisited.insert(std::make_pair(rId, std::unordered_set<TripId>(tId)));
			}
			tripsVisited[rId].insert(tId);
			
			if (!stopRoutes.contains(sId))
			{
				stopRoutes.insert(std::make_pair(sId, std::unordered_set<RouteId>()));
			}
			stopRoutes[sId].insert(rId);
			
			if (!result1.contains(rId))
			{
				result1.insert(std::make_pair(rId, RouteRawData()));
			}
			if (!result1[rId].contains(tId))
			{
				result1[rId].insert(std::make_pair(tId, std::vector<TripBlock>()));
			}
			result1[rId][tId].emplace_back(sId, service, arrival, departure);
		}
		size_t stopsCount = 0;
		size_t tripsCount = 0;
		for (auto&& [rId, stops] : stopsVisited)
		{
			stopsCount += stops.size();
		}
		for (auto&& [rId, trips] : tripsVisited)
		{
			tripsCount += trips.size() * stopsVisited[rId].size();
		}
		RTData d1{ sortRouteRawData(std::move(result1)), stopsCount, tripsCount };
		
		std::unordered_map<StopId, StopData> result2;
		std::unordered_map<StopId, std::unordered_set<std::pair<StopId, double>>> transfers;
		std::unordered_map<StopId, std::unordered_set<RouteId>> routes;
		
		size_t lower_bound = 0;
		auto&& stops = feed.get_stops();
		for (auto&& from_id : std::views::iota(0ul, tr().stop_count()))
		{
			for (auto&& to_id : std::views::iota(lower_bound, tr().stop_count()))
			{
				if (from_id == to_id)
					continue;
				auto&& from_stop = stops[from_id];
				auto&& to_stop = stops[to_id];
				auto dist = distance(from_stop.stop_lat, from_stop.stop_lon, to_stop.stop_lat, to_stop.stop_lon);
				if (dist >= 1)
					continue;
				if (!transfers.contains(from_id))
				{
					transfers.emplace(from_id, std::unordered_set<std::pair<StopId, double>>());
				}
				transfers[from_id].insert(std::pair(to_id, dist));
				if (!transfers.contains(to_id))
				{
					transfers.emplace(to_id, std::unordered_set<std::pair<StopId, double>>());
				}
				transfers[to_id].insert(std::pair(from_id, dist));
			}
			++lower_bound;
		}
		
		size_t transfersCount = 0;
		size_t routesCount = 0;
		for (auto&& [sId, trans] : transfers)
		{
			transfersCount += trans.size();
			result2[sId].transfers = std::vector(trans.begin(), trans.end());
		}
		for (auto&& [sId, route] : stopRoutes)
		{
			routesCount += route.size();
			result2[sId].routes = std::vector<RouteId>(route.begin(), route.end());
		}
		SData d2{ sortStopRawData(std::move(result2)), transfersCount, routesCount };
		return { d1, d2 };
	}

	void GTFSFeedParser::hashStops(const gtfs::Feed& feed)
	{
		for (auto&& stop : feed.get_stops())
		{
			IdTranslator::getInstance().insert(stop);
		}
	}

	void GTFSFeedParser::hashRoutes(const gtfs::Feed& feed)
	{
		for (auto&& route : feed.get_routes())
		{
			IdTranslator::getInstance().insert(route);
		}
	}

	void GTFSFeedParser::hashTrips(const gtfs::Feed& feed)
	{
		for (auto&& trip : feed.get_trips())
		{
			IdTranslator::getInstance().insert(trip);
		}
	}

	void GTFSFeedParser::hashServices(const gtfs::Feed& feed)
	{
		for (auto&& service : feed.get_calendar())
		{
			IdTranslator::getInstance().insert(service);
		}
	}

	void GTFSFeedParser::prepareTranslator(const gtfs::Feed& feed)
	{
		hashStops(feed);
		hashRoutes(feed);
		hashTrips(feed);
		hashServices(feed);
	}

	InternalRouteId::InternalRouteId(const gtfs::Route& route, const gtfs::Trip& trip) : rId(route.route_id)
	{
		direction = trip.direction_id == gtfs::TripDirectionId::DefaultDirection ? RouteDirection::DefaultDirection : RouteDirection::OppositeDirection;
	}

	InternalRouteId::InternalRouteId(const std::string& id, const gtfs::Trip& trip) : rId(id)
	{
		direction = trip.direction_id == gtfs::TripDirectionId::DefaultDirection ? RouteDirection::DefaultDirection : RouteDirection::OppositeDirection;
	}

	InternalRouteId::InternalRouteId(const std::string& id, const RouteDirection& dir) : rId(id), direction(dir) { }

	bool InternalRouteId::operator==(const InternalRouteId& other) const
	{
		return this->rId == other.rId && this->direction == other.direction;
	}

}
