#include <Algorithm.hpp>
#include <array>
#include <limits>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace raptor
{
    RouteFinder::RouteFinder(const gtfs::Feed* feed) : num_stops_(feed->get_stops().size()), feed_(feed)
    {
        auto [rd, sd] = GTFSFeedParser::parseFeed(*feed_);
        rt_ = std::move(rd);
        stops_ = std::move(sd);
    }
    
    Time_t RouteFinder::distanceToTime(const double distance, WalkingSpeed speed)
    {
        // seconds per km
        short pace;
        // error to account for longer real distance
        constexpr double error = 1.2;
        switch (speed)
        {
        case WalkingSpeed::Slow:
            // 4 km/h
            pace = 15*60;
            break;
        case WalkingSpeed::Normal:
            // 5 km/h
            pace = 12*60;
            break;
        case WalkingSpeed::Fast:
            // 6 km/h
            pace = 10*60;
            break;
        default:
            pace = std::numeric_limits<short>::max();
        }
        return std::round(distance * pace * error);
    }
    
    void RouteFinder::setOptions(const WalkingSpeed new_speed, const std::string& service_id)
    {
        if (service_id != "")
        {
            if (!checkServiceIdInFeed(service_id))
                throw IdException(service_id);
            options_.wanted_service_id = service_id;
        }
        options_.preferred_walking_speed = new_speed;
    }
    
    bool RouteFinder::checkServiceIdInFeed(const std::string& id) const
    {
        return feed_->get_calendar(id).has_value();
    }
    
    std::variant<RouteFinder::result_t, std::string> RouteFinder::findRoute(const std::vector<StopId>& starts, const std::vector<StopId>& ends, const Time_t departure) const
    {
        if (!checkServiceIdInFeed(options_.wanted_service_id))
            throw IdException(options_.wanted_service_id);
        const Time_t new_inf_time = inf_time - departure;
        constexpr Time_t day = 24*60*60;
        const auto w_speed = options_.preferred_walking_speed;
        const Time_t max_travel_time = 10*60;
        std::vector<std::vector<std::tuple<Time_t, StopId, std::optional<RouteTraversal::trip_iterator>>>> labels;
        std::vector<Time_t> earliest_arrival(num_stops_, new_inf_time);
        std::tuple<Time_t, StopId, size_t> earliest_arrival_end(new_inf_time, undefined::stop, 0);
        earliest_arrival.shrink_to_fit();
        std::vector<bool> marked(num_stops_, false);
        marked.shrink_to_fit();
        std::unordered_map<RouteId, StopId> potential_routes;
        labels.emplace_back(num_stops_, std::tuple(new_inf_time, undefined::stop, std::nullopt));
        labels[0].shrink_to_fit();
        size_t num_marked = 0;
        for (auto&& start : starts)
        {
            std::get<0>(labels[0][start]) = 0;
            earliest_arrival[start] = 0;
            marked[start] = true;        // mark starting stop
            ++num_marked;
        }
        labels.emplace_back(labels.back());
        bool end_cond = false;    // end condition
        auto early_end = [&]()
        {
            if (starts.size() != ends.size())
                return false;
            for (size_t i = 0; i < starts.size(); ++i)
            {
                if (starts[i] != ends[i])
                    return false;
            }
            return true;
        };
        if (early_end())
            return "Start and end are the same stop\n";
        for (size_t k = 1; !end_cond; ++k)
        {
            auto recalculate_end_arrival = [&]()
            {
                for (auto&& end : ends)
                {
                    if (earliest_arrival[end] < std::get<0>(earliest_arrival_end))
                        earliest_arrival_end = std::tuple(earliest_arrival[end], end, k);
                }
            };
            potential_routes.clear();
            for (size_t stop = 0; stop < marked.size(); ++stop)
            {
                if (marked[stop])
                {
                    for (auto&& route : stops_.getRoutes(StopId(stop)))
                    {
                        auto&& [it, inserted] = potential_routes.emplace(route, StopId(stop));
                        if (!inserted)
                        {
                            auto&& [first, last] = rt_.getStops(route);
                            auto is_earlier_stop = [&stop, &it](const StopId s) { return s == stop || s == it->second; };
                            // will always find a stop
                            it->second = *std::find_if(first, last, is_earlier_stop);
                        }
                    }
                    marked[stop] = false;
                    --num_marked;
                }
            }
            for (auto&& [route, stop] : potential_routes)
            {
                auto curr_trip = undefined_trip;
                auto is_stop = [&stop](const StopId s){ return s == stop; };
                auto&& [first, last] = rt_.getStops(route);
                auto next = std::find_if(first, last, is_stop);
                auto range = std::ranges::subrange(next, last);
                auto prev_stop = stop;
                size_t diff = 0;
                for (auto&& next_stop : range)
                {
                    auto trip_iter = curr_trip + diff;
                    while (trip_iter != undefined_trip && trip_iter->stopId != next_stop)
                    {
                        ++trip_iter;
                        ++diff;
                        if (trip_iter->tId != curr_trip->tId)
                            trip_iter = undefined_trip;
                    }
                    assert(trip_iter == undefined_trip || (trip_iter->stopId == next_stop && trip_iter->tId == curr_trip->tId));
                    const Time_t next_arrival = earliest_arrival[next_stop] == new_inf_time ? inf_time : (departure + earliest_arrival[next_stop]) % day;
                    const Time_t end_arrival = std::get<0>(earliest_arrival_end) == new_inf_time ? inf_time : (departure + std::get<0>(earliest_arrival_end)) % day;
                    const Time_t iter_arrival = trip_iter->arrival;
                    if (curr_trip != undefined_trip && iter_arrival < std::min(next_arrival, end_arrival))
                    {
                        const Time_t new_arrival = iter_arrival - departure;
                        labels[k][next_stop] = std::tuple(new_arrival, prev_stop, curr_trip);
                        earliest_arrival[next_stop] = new_arrival;
                        recalculate_end_arrival();
                        if (!marked[next_stop])
                            ++num_marked;
                        marked[next_stop] = true;
                    }
                    
                    const Time_t old_arr  = std::get<0>(labels[k-1][next_stop]) == new_inf_time ? inf_time : (departure + std::get<0>(labels[k-1][next_stop])) % day;
                    if (old_arr <= trip_iter->departure)
                    {
                        auto [first_trip, last_trip] = rt_.getTripsFromStop(route, next_stop);
                        const auto arr = departure + std::get<0>(labels[k-1][next_stop]);
                        auto earliest_trip = [&](const Trip& t){ return t.departure > arr && t.sId == IdTranslator::getInstance().at(options_.wanted_service_id, IdTranslator::ServiceTag()); };
                        auto candidate_trip = std::find_if(first_trip, last_trip, earliest_trip);
                        if (candidate_trip != last_trip)
                        {
                            curr_trip = candidate_trip;
                            prev_stop = candidate_trip->stopId;
                            diff = 0;
                        }
                    }
                }
            }
            auto new_marked = marked;
            for (size_t stop = 0; stop < marked.size(); ++stop)
            {
                if (marked[stop])
                {
                    for (auto&& transfer : stops_.getTransfers(stop))
                    {
                        constexpr Time_t transfer_penalty = 60;
                        const auto arrival_with_walking = std::get<0>(labels[k][stop]) + distanceToTime(transfer.distance, w_speed) + transfer_penalty;
                        const auto arrival_without = std::get<0>(labels[k][transfer.target_stop]);
                        if (arrival_with_walking < arrival_without && distanceToTime(transfer.distance, w_speed) < max_travel_time && std::get<2>(labels[k][stop]).has_value())
                        {
                            labels[k][transfer.target_stop] = std::tuple(arrival_with_walking, stop, std::nullopt);
                            earliest_arrival[transfer.target_stop] = arrival_with_walking;
                            if (!new_marked[transfer.target_stop])
                                ++num_marked;
                            new_marked[transfer.target_stop] = true;
                        }
                    }
                }
            }
            recalculate_end_arrival();
            marked = std::move(new_marked);
            end_cond = num_marked == 0;
            if (!end_cond)
            {
                labels.push_back(labels.back());
            }
        }
        labels.pop_back();
        result_t v;
        if (std::get<1>(earliest_arrival_end) == undefined::stop)
            return "End stop unreachable\n";
        auto&& [time, end, last_round] = earliest_arrival_end;
        assert(std::get<0>(labels[last_round][end]) == time);
        v.push_back(std::pair(end, std::get<0>(labels[last_round][end])));
        StopId prev = std::get<1>(labels[last_round][end]);
        if (std::get<2>(labels[last_round][end]).has_value())
            v.push_back(std::get<2>(labels[last_round][end]).value());
        else
        {
            v.push_back(std::pair(prev, std::get<0>(labels[last_round][prev])));
            if (std::get<2>(labels[last_round][prev]).has_value())
                v.push_back(std::get<2>(labels[last_round][prev]).value());
            prev = std::get<1>(labels[last_round][prev]);
        }
        for (auto&& round : std::ranges::subrange(labels.rbegin()+labels.size()-last_round, labels.rend()))
        {
            auto&& [arrival, s, iter] = round[prev];
            v.push_back(std::pair(prev, arrival));
            if (iter.has_value())
            {
                v.push_back(iter.value());
            }
            // I walked to here
            else if (s != undefined::stop)
            {
                auto&& [p_arrival, p_s, p_iter] = round[s];
                v.push_back(std::pair(s, p_arrival));
                if (p_iter.has_value())
                    v.push_back(p_iter.value());
                s = p_s;
            }
            prev = s;
        }
        return result_t(v.rbegin(), v.rend());
    }
}

std::ostream& operator<<(std::ostream& stream, const std::tuple<raptor::RouteFinder::result_t, gtfs::Feed, raptor::Time_t>& data)
{
    constexpr raptor::Time_t day = 24*60*60;
    constexpr char padding[] = "  ";
	auto tr = raptor::IdTranslator::getInstance;
	auto&& [d, feed, departure] = data;
	auto [s, arr] = std::get<0>(*d.begin());
	stream << padding << "Begin on stop '" << feed.get_stop(tr().at(s))->stop_name << "' at " << toString(departure + arr) << '\n';
	auto prev_arr = arr;
	auto prev_dep = raptor::undefined_time;
	int day_offset = 0;
	for (auto&& block : std::ranges::subrange(d.begin()+1, d.end()-1))
	{
	    std::visit([&](auto&& arg)
	    {
	        using T = std::decay_t<decltype(arg)>;
	        if constexpr (std::is_same_v<T, raptor::RouteTraversal::trip_iterator>)
	        {
	        	raptor::Trip trip = *arg;
	        	if (trip.departure - prev_arr - departure + day_offset * day < 0)
	        	    ++day_offset;
	        	const raptor::Time_t wait_time = trip.departure - prev_arr - departure + day_offset * day;
	        	stream << padding << "Wait for " << (wait_time) / 60 << " minutes\n";
	        	stream << padding << "Board line " << feed.get_route(feed.get_trip(tr().at(trip.tId))->route_id)->route_short_name << " at " << toString(trip.departure + day * day_offset) << '\n';
	        	prev_arr = raptor::undefined_time;
	        	prev_dep = trip.departure;
	        }
	        else if constexpr (std::is_same_v<T, std::pair<raptor::StopId, raptor::Time_t>>)
            {
            	auto [stop, arrival] = arg;
            	if (prev_arr != raptor::undefined_time)
            	{
            		stream << padding << "Walk for " << (arrival - prev_arr) / 60 << " minutes to stop " << feed.get_stops()[stop].stop_name << '\n';
            	}
            	else
            	{
            		stream << padding << "Get off at stop " << feed.get_stop(tr().at(stop))->stop_name << " after " << (departure + arrival - (prev_dep + day * day_offset)) / 60 << " minutes at " << toString(arrival + departure) << '\n';
            	}
            	prev_arr = arrival;
            	prev_dep = raptor::undefined_time;
            }
	    }, block);
	}
    auto [stop, arrival] = std::get<0>(d.back());
    if (prev_arr != raptor::undefined_time)
        stream << padding << "Walk for " << (arrival - prev_arr) / 60 << " minutes to stop " << feed.get_stops()[stop].stop_name << '\n';
    stream << padding << "You have arrived to your destination " << feed.get_stops()[stop].stop_name << " at " << toString(arrival + departure) << '\n';
	return stream;
}
