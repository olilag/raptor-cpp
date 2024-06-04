#include <DataStructures.hpp>

namespace raptor
{
	const StopId undefined::stop = StopId();
	const Trip undefined::trip = Trip();
	const RouteId undefined::route = RouteId();
	const Transfer undefined::transfer = Transfer();
	const ServiceId undefined::service = ServiceId();
	const TripBlock undefined::tripBlock{ undefined::stop, service, undefined_time, undefined_time };
	
	Trip::Trip() : tId(), arrival(inf_time), departure(inf_time) { }

	//Trip::Trip(const Trip& other) : tId(other.tId), arrival(other.arrival), departure(other.departure) { }

	Trip::Trip(const TripId tid, const StopId stopid, const ServiceId sid, const Time_t arr, const Time_t dep) : tId(tid), stopId(stopid), sId(sid), arrival(arr), departure(dep) { }

	/*Trip& Trip::operator=(const Trip& other)
	{
		tId = other.tId;
		sId = other.sId;
		arrival = other.arrival;
		departure = other.departure;
		return *this;
	}*/
	
	bool operator==(const Trip& lhs, const Trip& rhs)
	{
		return lhs.tId == rhs.tId;
	}

	Route::Route(const StopId* rs_ptr, const Trip* st_ptr,
		const size_t st_count, const size_t tr_count) :
		route_stops_ptr(rs_ptr), stop_times_ptr(st_ptr), trip_count(tr_count), stops_count(st_count) { }
	
	RouteTraversal::RouteTraversal(RouteTraversal&& other) noexcept : route_stops_(nullptr), stop_times_(nullptr)
	{
		routes_.clear();
		std::swap(routes_, other.routes_);
		std::swap(rs_size_, other.rs_size_);
		std::swap(st_size_, other.st_size_);
		std::swap(route_stops_, other.route_stops_);
		std::swap(stop_times_, other.stop_times_);
	}

	RouteTraversal::RouteTraversal(const RTData& raw_data) : routes_()
	{
		auto&& [data, stopCount, tripCount] = raw_data;
		auto longest_trips = GTFSFeedParser::findLongestTrips(data);
		unsigned char* rs_raw_memory = new unsigned char[stopCount * sizeof(StopId)];
		unsigned char* st_raw_memory = new unsigned char[tripCount * sizeof(Trip)];
		route_stops_ = (StopId*)rs_raw_memory;
		stop_times_ = (Trip*)st_raw_memory;
		size_t next_rs_index = 0;
		size_t next_st_index = 0;

		size_t prev_rs_count = 0;
		size_t prev_st_count = 0;
		for (auto&& [routeId, sData] : data)
		{
			size_t index = 0;
			for (auto&& [tripId, blocks] : sData)
			{
				for (auto&& block : blocks)
				{
					if (longest_trips[routeId].second == index)
					{
						route_stops_[next_rs_index] = block.sId;
						++next_rs_index;
					}
					new (stop_times_ + next_st_index) Trip(tripId, block.sId, block.service, block.arrival, block.departure);
					++next_st_index;
				}
				++index;
			}
			
			routes_.emplace_back(route_stops_ + prev_rs_count, stop_times_ + prev_st_count, next_rs_index - prev_rs_count, next_st_index - prev_st_count);
			auto&& prev = routes_.back();
			prev_rs_count += prev.stops_count;
			prev_st_count += prev.trip_count;
		}
		rs_size_ = prev_rs_count;
		st_size_ = prev_st_count;
		routes_.emplace_back(route_stops_ + rs_size_, stop_times_ + st_size_, 0, 0);
	}

	size_t RouteTraversal::size() const 
	{
		return routes_.size() - 1;
	}
	
	RouteTraversal& RouteTraversal::operator=(RouteTraversal&& other) noexcept
	{
		if (this == &other)
			return *this;
		
		routes_.clear();
		rs_size_ = 0;
		st_size_ = 0;
		if (route_stops_ != nullptr)
			delete[] route_stops_;
		if  (stop_times_ != nullptr)
			delete[] stop_times_;
		
		route_stops_ = nullptr;
		stop_times_ = nullptr;
		std::swap(routes_, other.routes_);
		std::swap(rs_size_, other.rs_size_);
		std::swap(st_size_, other.st_size_);
		std::swap(route_stops_, other.route_stops_);
		std::swap(stop_times_, other.stop_times_);
		return *this;
	}
	
	RouteTraversal& RouteTraversal::operator=(RTData&& raw_data)
	{
		routes_ = std::vector<Route>();
		auto&& [data, stopCount, tripCount] = raw_data;
		unsigned char* rs_raw_memory = new unsigned char[stopCount * sizeof(StopId)];
		unsigned char* st_raw_memory = new unsigned char[tripCount * sizeof(Trip)];
		route_stops_ = (StopId*)rs_raw_memory;
		stop_times_ = (Trip*)st_raw_memory;
		size_t next_rs_index = 0;
		size_t next_st_index = 0;
		
		size_t prev_rs_count = 0;
		size_t prev_st_count = 0;
		for (auto&& [routeId, sData] : data)
		{
			bool saveStops = true;
			for (auto&& [tripId, blocks] : sData)
			{
				for (auto&& block : blocks)
				{
					if (saveStops)
					{
						route_stops_[next_rs_index] = block.sId;
						++next_rs_index;
					}
					new (stop_times_ + next_st_index) Trip(tripId, block.sId, block.service, block.arrival, block.departure);
					++next_st_index;
				}
				saveStops = false;
			}
			
			routes_.emplace_back(route_stops_ + prev_rs_count, stop_times_ + prev_st_count, next_rs_index - prev_rs_count, next_st_index - prev_st_count);
			auto&& prev = routes_.back();
			prev_rs_count += prev.stops_count;
			prev_st_count += prev.trip_count;
		}
		rs_size_ = prev_rs_count;
		st_size_ = prev_st_count;
		routes_.emplace_back(route_stops_ + rs_size_, stop_times_ + st_size_, 0, 0);
		return *this;
	}

	const Route& RouteTraversal::operator[](size_t index) const
	{
		return routes_[index];
	}

	RouteTraversal::~RouteTraversal() noexcept
	{
		if (route_stops_ != nullptr)
			delete[] route_stops_;
		if  (stop_times_ != nullptr)
			delete[] stop_times_;
	}

	Transfer::Transfer() : target_stop(), distance(inf_distance) { }

	Transfer::Transfer(const StopId tar_stop, const double dist) : target_stop(tar_stop), distance(dist) { }

	Stop::Stop(const RouteId* sr_ptr, const Transfer* tr_ptr) : stop_routes_ptr(sr_ptr), transfers_ptr(tr_ptr) { }
	
	Stops::Stops(Stops&& other) noexcept : stop_routes_(nullptr), transfers_(nullptr)
	{
		std::swap(stops_, other.stops_);
		std::swap(stop_routes_, other.stop_routes_);
		std::swap(transfers_, other.transfers_);
	}
	
	Stops::Stops(const SData& raw_data) : stops_()
	{
		auto&& [data, tr_count, r_count] = raw_data;
		unsigned char* tr_raw_memory = new unsigned char[(tr_count) * sizeof(Transfer)];
		unsigned char* r_raw_memory = new unsigned char[(r_count) * sizeof(RouteId)];
		Transfer* tr_ptr = (Transfer*)tr_raw_memory;
		RouteId* r_ptr = (RouteId*)r_raw_memory;
		stop_routes_ = r_ptr;
		transfers_ = tr_ptr;
		Transfer* prev_tr = tr_ptr;
		RouteId* prev_r = r_ptr;
		for (auto&& [sId, sData] : data)
		{
			for (auto&& [from_sId, dist] : sData.transfers)
			{
				new (tr_ptr) Transfer(from_sId, dist);
				++tr_ptr;
			}
			for (auto&& route : sData.routes)
			{
				*r_ptr = route;
				++r_ptr;
			}
			stops_.emplace_back(prev_r, prev_tr);
			prev_r = r_ptr;
			prev_tr = tr_ptr;
		}
		stops_.emplace_back(r_ptr, tr_ptr);
	}

	size_t Stops::size() const
	{
		return stops_.size() - 1;
	}
	
	Stops& Stops::operator=(Stops&& other) noexcept
	{
		if (this == &other)
			return *this;
		
		stops_.clear();
		if (stop_routes_ != nullptr)
			delete[] stop_routes_;
		if (transfers_ != nullptr)
			delete[] transfers_;
		stop_routes_ = nullptr;
		transfers_ = nullptr;
		
		std::swap(stops_, other.stops_);
		std::swap(stop_routes_, other.stop_routes_);
		std::swap(transfers_, other.transfers_);
		return *this;
	}
	
	Stops& Stops::operator=(SData&& raw_data)
	{
		stops_ = std::vector<Stop>();
		auto&& [data, tr_count, r_count] = raw_data;
		unsigned char* tr_raw_memory = new unsigned char[(tr_count) * sizeof(Transfer)];
		unsigned char* r_raw_memory = new unsigned char[(r_count) * sizeof(RouteId)];
		Transfer* tr_ptr = (Transfer*)tr_raw_memory;
		RouteId* r_ptr = (RouteId*)r_raw_memory;
		stop_routes_ = r_ptr;
		transfers_ = tr_ptr;
		Transfer* prev_tr = tr_ptr;
		RouteId* prev_r = r_ptr;
		for (auto&& [sId, sData] : data)
		{
			for (auto&& [from_sId, dist] : sData.transfers)
			{
				new (tr_ptr) Transfer(from_sId, dist);
				++tr_ptr;
			}
			for (auto&& route : sData.routes)
			{
				*r_ptr = route;
				++r_ptr;
			}
			stops_.emplace_back(prev_r, prev_tr);
			prev_r = r_ptr;
			prev_tr = tr_ptr;
		}
		stops_.emplace_back(r_ptr, tr_ptr);
		return *this;
	}

	const Stop& Stops::operator[](const size_t index) const
	{
		return stops_[index];
	}

	Stops::~Stops() noexcept
	{
		if (stop_routes_ != nullptr)
			delete[] stop_routes_;
		if (transfers_ != nullptr)
			delete[] transfers_;
	}
}
