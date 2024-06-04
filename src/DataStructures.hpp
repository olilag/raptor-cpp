#ifndef DATA_STRUCTURES_HPP_
#define DATA_STRUCTURES_HPP_

#include <vector>
#include <ranges>
#include <algorithm>
#include <iterator>
#include <just_gtfs.h>
#include <RaptorTypesAndConstants.hpp>

namespace raptor
{
	/**
	 * @brief Stores data for one trip leaving from one stop
	 * 
	 */
	struct Trip
	{
		Trip();
		Trip(const TripId tid, const StopId stopid, const ServiceId sid, const Time_t arr, const Time_t dep);
		Trip(const Trip& other) = default;
		Trip& operator=(const Trip& other) = default;
		Trip(Trip&& other) = default;
		Trip& operator=(Trip&& other) = default;
		TripId tId;
		StopId stopId;
		ServiceId sId;
		Time_t arrival;
		Time_t departure;
	};
	
	/**
	 * @brief Stores data for a transfer between two stops
	 *
	 */
	struct Transfer
	{
		Transfer();
		Transfer(const StopId tar_stop, const double dist);
		const StopId target_stop;
		const double distance;
	};

	bool operator==(const Trip& lhs, const Trip& rhs);
	
	/**
	 * @brief Struct containing undefined values of certain classes
	 * 
	 */
	struct undefined
	{
		static const StopId stop;
		static const Trip trip;
		static const RouteId route;
		static const ServiceId service;
		static const Transfer transfer;
		static const TripBlock tripBlock;
		static const StopId& get(StopId) { return undefined::stop; }
		static const Trip& get(Trip) { return undefined::trip; }
		static const RouteId& get(RouteId) { return undefined::route; }
		static const Transfer& get(Transfer) { return undefined::transfer; }
	};
	
	/**
	 * @brief Iterator class for `raptor::RouteTraversal` and `raptor::Stops`
	 * 
	 * @tparam T Type of element iterating over
	 */
	template<typename T>
	class iterator
	{
	private:
		const T* item_;
	public:
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = const T;
		using reference = const T&;
		using pointer = const T*;
		iterator() : item_(nullptr) { }
		iterator(const T* item_ptr) : item_(item_ptr) { }
		iterator(const iterator& other) = default;
		iterator(iterator&& other) = default;
		iterator& operator=(const iterator& other) = default;
		iterator& operator=(iterator&& other) = default;
		
		iterator& operator++()
		{
			if (item_ == nullptr)
				return *this;
			++item_;
			return *this;
		}
		
		iterator operator++(int)
		{
			if (item_ == nullptr)
				return *this;
			auto tmp = *this;
			++*this;
			return tmp;
		}
		
		iterator operator+(size_t diff)
		{
			if (item_ == nullptr)
				return *this;
			return iterator(item_ + diff);
		}
		
		iterator operator-(size_t diff)
		{
			if (item_ == nullptr)
				return *this;
			return iterator(item_ - diff);
		}
		
		size_t operator-(const iterator& other) const
		{
			if (other.item_ == nullptr || item_ == nullptr)
				return 0;
			return item_ > other.item_ ? item_ - other.item_ : other.item_ - item_;
		}
		
		reference operator*() const
		{
			if (item_ == nullptr)
				return undefined::get(value_type());
			return *item_;
		}
		
		pointer operator->() const
		{
			if (item_ == nullptr)
				return &undefined::get(value_type());
			return item_;
		}
		
		bool operator==(const iterator& other) const
		{
			return item_ == other.item_;
		}
		
		bool operator!=(const iterator& other) const
		{
			return !(*this == other);
		}
		
		bool operator<(const iterator& other) const
		{
			return item_ < other.item_;
		}
		
		bool operator<=(const iterator& other) const
		{
			return *this == other || *this < other;
		}
	};
	
	/**
	 * @brief Iterates over `raptor::Trip`s departing from one specific stop
	 * 
	 */
	class jumping_trip_iterator
	{
	private:
		const Trip* item_;
		size_t diff_;
	public:
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = const Trip;
		using reference = const Trip&;
		using pointer = const Trip*;
		jumping_trip_iterator() : item_(nullptr), diff_() { }
		jumping_trip_iterator(const jumping_trip_iterator& other) = default;
		jumping_trip_iterator(jumping_trip_iterator&& other) = default;
		jumping_trip_iterator(const Trip* item, const size_t diff) : item_(item), diff_(diff) { }
		jumping_trip_iterator& operator=(const jumping_trip_iterator& other) = default;
		jumping_trip_iterator& operator=(jumping_trip_iterator&& other) = default;
		
		jumping_trip_iterator& operator++()
		{
			if (item_ == nullptr)
				return *this;
			item_ += diff_;
			return *this;
		}
		
		jumping_trip_iterator operator++(int)
		{
			if (item_ == nullptr)
				return *this;
			auto tmp = *this;
			++*this;
			return tmp;
		}
		
		jumping_trip_iterator operator+(size_t diff)
		{
			if (item_ == nullptr)
				return *this;
			return jumping_trip_iterator(item_ + diff * diff_, diff_);
		}
		
		jumping_trip_iterator operator-(size_t diff)
		{
			if (item_ == nullptr)
				return *this;
			return jumping_trip_iterator(item_ - diff * diff_, diff_);
		}
		
		size_t operator-(const jumping_trip_iterator& other) const
		{
			return item_ > other.item_ ? item_ - other.item_ : other.item_ - item_;
		}
		
		reference operator*() const
		{
			if (item_ == nullptr)
				return undefined::get(value_type());
			return *item_;
		}
		
		pointer operator->() const
		{
			if (item_ == nullptr)
				return &undefined::get(value_type());
			return item_;
		}
		
		bool operator==(const jumping_trip_iterator& other) const
		{
			return item_ == other.item_;
		}
		
		bool operator!=(const jumping_trip_iterator& other) const
		{
			return !(*this == other);
		}
		
		bool operator<(const jumping_trip_iterator& other) const
		{
			return item_ < other.item_;
		}
		
		bool operator<=(const jumping_trip_iterator& other) const
		{
			return *this == other || *this < other;
		}
		
		operator iterator<Trip>() const
		{
			return iterator<Trip>(item_);
		}
	};

	/**
	 * @brief Points to all stops `raptor::StopId` and trips `raptor::Trip` for a route in feed
	 * 
	 */
	struct Route
	{
		Route(const StopId* rs_ptr, const Trip* st_ptr, 
			const size_t tr_count, const size_t st_count);
		const StopId* route_stops_ptr;
		const Trip* stop_times_ptr;
		size_t trip_count;
		size_t stops_count;
	};
	
	/**
	 * @brief Responsible for allocation and storage of data needed to traverse routes in algorithm
	 * 
	 */
	class RouteTraversal
	{
	private:
		/**
		 * @brief Stored pointers for each route
		 * 
		 * Last element stores pointers to end+1 in `route_stops_` and `stop_times_`
		 * 
		 */
		std::vector<Route> routes_;
		StopId* route_stops_;
		Trip* stop_times_;
		size_t rs_size_ = 0;
		size_t st_size_ = 0;
	public:
		size_t size() const;
		RouteTraversal() : route_stops_(nullptr), stop_times_(nullptr) { }
		RouteTraversal(const RouteTraversal& other) = delete;
		RouteTraversal(RouteTraversal&& other) noexcept;
		RouteTraversal(const RTData& raw_data);
		RouteTraversal& operator=(const RouteTraversal& other) = delete;
		RouteTraversal& operator=(RTData&& raw_data);
		RouteTraversal& operator=(RouteTraversal&& other) noexcept;
		const Route& operator[](size_t index) const;
		~RouteTraversal() noexcept;

		using stop_iterator = iterator<StopId>;

		/**
		 * @brief Returns begin() a end() iterators to all stops for `route`
		 * 
		 * @param route A route
		 * @return A `std::ranges::subrange(begin(), end())` of stop_iterators
		 */
		std::ranges::subrange<stop_iterator> getStops(RouteId route) const
		{
			return std::ranges::subrange(stop_iterator(routes_[route].route_stops_ptr), stop_iterator(routes_[route+1].route_stops_ptr));
		}

		using trip_iterator = iterator<Trip>;

		/**
		 * @brief Returns begin() a end() iterators to all trips for `route`
		 * 
		 * @param route A route
		 * @return A `std::ranges::subrange(begin(), end())` of trip_iterators
		 */
		std::ranges::subrange<trip_iterator> getTrips(RouteId route) const
		{
			return std::ranges::subrange(trip_iterator(routes_[route].stop_times_ptr), trip_iterator(routes_[route+1].stop_times_ptr));
		}

		using stop_trip_iterator = jumping_trip_iterator;

		/**
		 * @brief Returns begin() a end() iterators to all trips for `route` from `stop`
		 * 
		 * @param route A route
		 * @param stop A stop
		 * @return A `std::ranges::subrange(begin(), end())` of stop_trip_iterators
		 */
		std::ranges::subrange<stop_trip_iterator> getTripsFromStop(RouteId route, StopId stop) const
		{
			size_t diff = routes_[route].stops_count;
			auto is_stop = [&stop](const StopId s){ return s == stop; };
			auto&& [first, last] = getStops(route);
			auto stop_iter = std::find_if(first, last, is_stop);
			size_t stop_diff = stop_iter - first;
			return std::ranges::subrange(stop_trip_iterator(routes_[route].stop_times_ptr + stop_diff, diff),
			                             stop_trip_iterator(routes_[route+1].stop_times_ptr + (stop_diff % diff), diff));
		}
		
		/**
		 * @brief Returns begin() a end() iterators to all trips for `route` from `stop`
		 * 
		 * @param route A route
		 * @param stop_iter A stop
		 * @return A `std::ranges::subrange(begin(), end())` of stop_trip_iterators
		 */
		std::ranges::subrange<stop_trip_iterator> getTripsFromStop(RouteId route, stop_iterator stop_iter) const
		{
			size_t diff = routes_[route].stops_count;
			auto&& [first, last] = getStops(route);
			size_t stop_diff = stop_iter - first;
			return std::ranges::subrange(stop_trip_iterator(routes_[route].stop_times_ptr + stop_diff, diff),
			                             stop_trip_iterator(routes_[route+1].stop_times_ptr + (stop_diff % diff), diff));
		}
	};
    
    const RouteTraversal::trip_iterator undefined_trip;
	
	/**
	 * @brief Points to all routes `raptor::RouteId` and transfers `raptor::Transfer` for a stop in feed
	 * 
	 */
	struct Stop
	{
		Stop(const RouteId* sr_ptr, const Transfer* tr_ptr);
		const RouteId* stop_routes_ptr;
		const Transfer* transfers_ptr;
	};
	
	/**
	 * @brief Responsible for allocation and storage of data needed to traverse stops in algorithm
	 * 
	 */
	class Stops
	{
	private:
		/**
		 * @brief Stored pointers for each stop
		 * 
		 * Last element stores pointers to end+1 in `stop_routes_` and `transfers_`
		 * 
		 */
		std::vector<Stop> stops_;
		RouteId* stop_routes_;
		Transfer* transfers_;
	public:
		Stops() : stop_routes_(nullptr), transfers_(nullptr) { }
		Stops(const Stops& other) = delete;
		Stops(Stops&& other) noexcept;
		Stops(const SData& raw_data);
		size_t size() const;
		Stops& operator=(SData&& raw_data);
		Stops& operator=(const Stops& other) = delete;
		Stops& operator=(Stops&& other) noexcept;
		const Stop& operator[](const size_t index) const;
		~Stops() noexcept;

		using route_iterator = iterator<RouteId>;

		/**
		 * @brief Returns begin() a end() iterators to all routes from `stop`
		 * 
		 * @param stop A stop
		 * @return A `std::ranges::subrange(begin(), end())` of route_iterators
		 */
		std::ranges::subrange<route_iterator> getRoutes(StopId stop) const
		{
			return std::ranges::subrange(route_iterator(stops_[stop].stop_routes_ptr), route_iterator(stops_[stop+1].stop_routes_ptr));
		}

		using transfer_iterator = iterator<Transfer>;

		/**
		 * @brief Returns begin() a end() iterators to all transfers from `stop`
		 * 
		 * @param stop A stop
		 * @return A `std::ranges::subrange(begin(), end())` of transfer_iterators
		 */
		std::ranges::subrange<transfer_iterator> getTransfers(StopId stop) const
		{
			return std::ranges::subrange(transfer_iterator(stops_[stop].transfers_ptr), transfer_iterator(stops_[stop+1].transfers_ptr));
		}
	};

	/**
	 * @brief A static helper class to parse data from `gtfs::Feed`
	 * 
	 */
	class GTFSFeedParser
	{
	private:
		friend class RouteTraversal;

		/**
		 * @brief Converts degrees to radians
		 * 
		 * @param degree Value
		 * @return `degree` in radians
		 */
		static double toRadians(const double degree);

		/**
		 * @brief Calculates distance between two point on Earth from GPS coordinates
		 * 
		 * @param lat1 Latitude of first point
		 * @param long1 Longitude of first point
		 * @param lat2 Latitude of second point
		 * @param long2 Longitude of second point
		 * @return Distance in kilometers
		 */
		static double distance(double lat1, double long1,
		                            double lat2, double long2);

		/**
		 * @brief Sorts data for `raptor::RouteTraversal` to correct order
		 * 
		 * @param data Data to sort
		 * @return Sorted `data`
		 */
		static std::tuple_element_t<0, RTData> sortRouteRawData(std::unordered_map<RouteId, RouteRawData>&& data);

		/**
		 * @brief Sorts data for `raptor::Stops` to correct order
		 * 
		 * @param data Data to sort
		 * @return Sorted `data`
		 */
		static std::vector<StopRawData> sortStopRawData(std::unordered_map<StopId, StopData>&& data);

		/**
		 * @brief Finds the longest trip for each route
		 * 
		 * @param data Data
		 * @return `std::vector` of `std::pair<size_t, size_t>` where first = length, second = index of longest trip
		 */
		static std::vector<std::pair<size_t, size_t>> findLongestTrips(const std::tuple_element_t<0, RTData>& data);

		/**
		 * @brief For each route removes trips which have different length than the longest trip
		 * 
		 * @param data Data
		 * @return `data` without the trips
		 */
		static std::tuple_element_t<0, RTData> removeBadTrips(std::tuple_element_t<0, RTData>& data);

		/**
		 * @brief Inserts all stops from a `gtfs::Feed` to `raptor::IdTranslator`
		 * 
		 * @param feed A `gtfs::Feed` feed with stops
		 */
		static void hashStops(const gtfs::Feed& feed);

		/**
		 * @brief Inserts all routes from a `gtfs::Feed` to `raptor::IdTranslator`
		 * 
		 * @param feed A `gtfs::Feed` feed with routes
		 */
		static void hashRoutes(const gtfs::Feed& feed);

		/**
		 * @brief Inserts all trips from a `gtfs::Feed` to `raptor::IdTranslator`
		 * 
		 * @param feed A `gtfs::Feed` feed with trips
		 */
		static void hashTrips(const gtfs::Feed& feed);
		
		/**
		 * @brief Inserts all services from a `gtfs::Feed` to `raptor::IdTranslator`
		 * 
		 * @param feed A `gtfs::Feed` feed with services
		 */
		static void hashServices(const gtfs::Feed& feed);
		
		/**
		 * @brief Prepares `raptor::IdTranslator` by calling all hash* member functions above
		 * 
		 * @param feed A `gtfs::Feed` feed with desired data
		 */
		static void prepareTranslator(const gtfs::Feed& feed);
	public:
		/**
		 * @brief 
		 * 
		 * @param feed A `gtfs::Feed` feed with desired data
		 * @return Sorted data for `raptor::RouteTraversal` and `raptor::Stops`
		 */
		static const Data parseFeed(const gtfs::Feed& feed);
	};
}

#endif // !DATA_STRUCTURES_HPP_
