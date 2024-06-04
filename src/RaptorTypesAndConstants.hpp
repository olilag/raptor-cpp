#ifndef RAPTOR_TYPES_AND_CONSTANTS_HPP_
#define RAPTOR_TYPES_AND_CONSTANTS_HPP_

#include <limits>
#include <vector>
#include <string>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <UnorderedBimap.hpp>
#include <just_gtfs.h>
#include <sstream>
#include <stdexcept>
#include <utility>

/**
 * @brief Function to combine two hashes
 *
 * From boost
 * @param lhs First hash
 * @param rhs Second hash
 * @return size_t Combined hash
 */
inline size_t hash_combine(size_t lhs, size_t rhs)
{
	if constexpr (sizeof(size_t) >= 8)
	{
		lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
	}
	else
	{
		lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
	}
	return lhs;
}

namespace raptor
{
	/**
	 * @brief Templated class to distinguish route, stop, trip and service indexes
	 *
	 * Provides almost the same methods as size_t and provides a conversion to size_t
	 * 
	 * @tparam size_t Only do distinguish two numbers
	 */
	template<size_t>
	class Id
	{
	private:
		size_t id_;
	public:
		Id() : id_(std::numeric_limits<size_t>::max()) { }
		Id(size_t id) : id_(id) { }
		Id(const Id& other) : id_(other.id_) { }
		Id(Id&& other) noexcept : id_(std::numeric_limits<size_t>::max())
		{
			 std::swap(id_, other.id_);
		}
		size_t getId() const
		{
			return id_;
		}
		Id& operator++()
		{
			++id_;
			return *this;
		}
		Id operator++(int)
		{
			auto old = *this;
			++id_;
			return old;
		}
		operator size_t() const
		{
			return id_;
		}
		Id& operator=(const Id& other)
		{
			if (this == &other)
			{
				return *this;
			}
			id_ = other.id_;
			return *this;
		}
		Id& operator=(Id&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}
			id_ = std::numeric_limits<size_t>::max();
			std::swap(id_, other.id_);
			return *this;
		}
	};

	using RouteId = Id<0>;
	using StopId = Id<1>;
	using TripId = Id<2>;
	using ServiceId = Id<3>;
	
	/**
	 * @brief Seconds since midnight
	 * 
	 */
	using Time_t = int;
	constexpr Time_t undefined_time = std::numeric_limits<Time_t>::min();
	constexpr Time_t inf_time = std::numeric_limits<Time_t>::max();
	constexpr double inf_distance = std::numeric_limits<double>::max();

	enum class RouteDirection
	{
		DefaultDirection = 0,
		OppositeDirection = 1
	};

	struct InternalRouteId
	{
		std::string rId;
		RouteDirection direction;
		InternalRouteId(const gtfs::Route& route, const gtfs::Trip& trip);
		InternalRouteId(const std::string& id, const gtfs::Trip& trip);
		InternalRouteId(const std::string& id, const RouteDirection& dir);
		bool operator==(const InternalRouteId& other) const;
	};
}

/**
 * @brief Parses a string with time to seconds from midnight
 * @throws std::invalid_argument Invalid `time_string` was provided
 * @param time_string String in hh:mm:ss format
 * @relatesalso raptor::Time_t
 * @return Time as seconds since midnight
 */
inline raptor::Time_t toTime(const std::string& time_string)
{
	std::istringstream str(time_string);
	int hours;
	int minutes;
	std::string part;
	size_t p;
	std::getline(str, part, ':');
	hours = std::stoi(part, &p);
	if (p != part.length())
		throw std::invalid_argument("Invalid hour part");
	std::getline(str, part, ':');
	minutes = std::stoi(part, &p);
	if (p != part.length())
		throw std::invalid_argument("Invalid minutes part");
	if (hours < 0 || hours > 23)
		throw std::invalid_argument("Invalid hour part");
	if (minutes < 0 || minutes > 59)
		throw std::invalid_argument("Invalid minutes part");
	return hours*3600 + minutes*60;
}

/**
 * @brief Specialization of `std::hash` for `raptor::Id`
 * 
 * @tparam I Parameter for `raptor::Id`
 */
template<size_t I>
struct std::hash<raptor::Id<I>>
{
	size_t operator()(const raptor::Id<I>& id) const noexcept
	{
		return std::hash<size_t>{}(id.getId());
	}
};

/**
 * @brief Stream write operator for `raptor::Id`
 * 
 * @tparam I Parameter for `raptor::Id`
 * @param stream Output stream
 * @param id Object to write
 * @relatesalso raptor::Id
 * @return Output stream `stream`
 */
template<size_t I>
std::ostream& operator<<(std::ostream& stream, const raptor::Id<I>& id)
{
	stream << id.getId();
	return stream;
}

template<>
struct std::hash<raptor::InternalRouteId>
{
	size_t operator()(const raptor::InternalRouteId& id) const noexcept
	{
		return hash_combine(std::hash<std::string>{}(id.rId), std::hash<raptor::RouteDirection>{}(id.direction));
	}
};

inline bool operator==(const raptor::InternalRouteId& a, const raptor::InternalRouteId& b)
{
	return a.rId == b.rId && a.direction == b.direction;
}

template<>
struct std::hash<std::pair<raptor::StopId, double>>
{
	size_t operator()(const std::pair<raptor::StopId, double>& a) const noexcept
	{
		return hash_combine(std::hash<raptor::StopId>{}(a.first), std::hash<double>{}(a.second));
	}
};

inline bool operator==(const std::pair<raptor::StopId, raptor::Time_t>& a, const std::pair<raptor::StopId, raptor::Time_t>& b)
{
	return a.first == b.first && a.second == b.second;
}

/**
 * @brief Add leading zero to time in string if it has less then 2 digits
 * 
 * @param str String with time
 * @return Modified string
 */
inline std::string addLeadingZeros(const std::string& str)
{
	if (str.size() == 2)
		return str;
	return '0' + str;
}

/**
 * @brief Convert seconds since midnight to string representation hh:mm:ss
 * 
 * @param time Seconds since midnight
 * @relatesalso raptor::Time_t
 * @return String representation of `time`
 */
inline std::string toString(raptor::Time_t time)
{
	uint16_t days = time / (24*3600);
    time %= 24*3600;
    uint16_t hours = time / 3600;
    time %= 3600;
    uint16_t minutes = time / 60;
    time %= 60;
    uint16_t seconds = time;
    std::string day_str;
    switch (days)
    {
   	case 0:
    	day_str = "";
    	break;
   	case 1:
    	day_str = " the next day";
    	break;
   	case 2:
    	day_str = " the 2nd day";
    	break;
   	case 3:
    	day_str = " the 3rd day";
    	break;
   	default:
    	day_str = " the " + std::to_string(days) + "th day";
    	break;
    }
    return std::to_string(hours) + ':' + addLeadingZeros(std::to_string(minutes)) + ':' + addLeadingZeros(std::to_string(seconds)) + day_str;
}

namespace raptor
{
	struct TripBlock
	{
		StopId sId;
		ServiceId service;
		Time_t arrival;
		Time_t departure;
		TripBlock(StopId idS, ServiceId serv, Time_t arr, Time_t dep)
			: sId(idS), service(serv), arrival(arr), departure(dep) { }
	};
	
	using RouteRawData = std::unordered_map<TripId, std::vector<TripBlock>>;
	
	struct StopData
	{
		std::vector<std::pair<StopId, double>> transfers;
		std::vector<RouteId> routes;
	};
	using StopRawData = std::pair<StopId, StopData>;
	
	using RTData = std::tuple<std::vector<std::pair<RouteId, std::vector<std::pair<TripId, RouteRawData::mapped_type>>>>, size_t, size_t>;
	using SData = std::tuple<std::vector<StopRawData>, size_t, size_t>;
	using Data = std::pair<RTData, SData>;
	
	/**
	 * @brief Singleton class for mapping `std::string` id's used in `gtfs::Feed` to `raptor::Id` used in algorithm
	 * 
	 */
	class IdTranslator
	{
	private:
		UnorderedBimap<std::string, StopId> stopIds_;
		UnorderedBimap<InternalRouteId, RouteId> routeIds_;
		UnorderedBimap<std::string, TripId> tripIds_;
		UnorderedBimap<std::string, ServiceId> serviceIds_;
		StopId next_stop_id_ = 0;
		RouteId next_route_id_ = 0;
		TripId next_trip_id_ = 0;
		ServiceId next_service_id_ = 0;
		bool locked_ = false;
		IdTranslator();
	public:
		IdTranslator(const IdTranslator& other) = delete;
		IdTranslator& operator=(const IdTranslator& other) = delete;
		IdTranslator(IdTranslator&& other) = delete;
		IdTranslator& operator=(IdTranslator&& other) = delete;
		
		/**
		 * @brief Get the instance of object
		 * 
		 * @return Instance
		 */
		static IdTranslator& getInstance();

		/**
		 * @brief Locks the object meaning no additional elements can be inserted when calling insert methods
		 * 
		 */
		void lock()
		{
			locked_ = true;
		}
		
		size_t stop_count() const;
		size_t route_count() const;
		size_t trip_count() const;

		void insert(const gtfs::Stop& element);
		void insert(const gtfs::Route& element);
		void insert(const gtfs::Trip& element);
		void insert(const gtfs::CalendarItem& element);

		StopId at(const gtfs::Stop& element) const;
		RouteId at(const InternalRouteId& element) const;
		TripId at(const gtfs::Trip& element) const;
		ServiceId at(const gtfs::CalendarItem& element) const;

		StopId operator[](const gtfs::Stop& element) const;
		RouteId operator[](const InternalRouteId& element) const;
		TripId operator[](const gtfs::Trip& element) const;

		struct StopTag { };
		struct RouteTag { };
		struct TripTag { };
		struct ServiceTag { };
		
		StopId at(const std::string& id, StopTag) const;
		TripId at(const std::string& id, TripTag) const;
		ServiceId at(const std::string& id, ServiceTag) const;

		const std::string& at(const StopId id) const;
		const InternalRouteId& at(const RouteId id) const;
		const std::string& at(const TripId id) const;
		const std::string& at(const ServiceId id) const;
	};
}

#endif // !RAPTOR_TYPES_AND_CONSTANTS_HPP_
