#include <RaptorTypesAndConstants.hpp>

namespace raptor
{
	IdTranslator::IdTranslator() : stopIds_(), routeIds_(), tripIds_(), serviceIds_() { }

	size_t IdTranslator::stop_count() const
	{
		return next_stop_id_;
	}

	size_t IdTranslator::route_count() const
	{
		return next_route_id_;
	}

	size_t IdTranslator::trip_count() const
	{
		return next_trip_id_;
	}
	
	IdTranslator& IdTranslator::getInstance()
	{
		static IdTranslator instance_;
		return instance_;
	}

	void IdTranslator::insert(const gtfs::Stop& element)
	{
		if (locked_)
			return;
		stopIds_.insert(element.stop_id, next_stop_id_);
		next_stop_id_++;
	}

	void IdTranslator::insert(const gtfs::Route& element)
	{
		if (locked_)
			return;
		routeIds_.insert(InternalRouteId(element.route_id, RouteDirection::DefaultDirection), next_route_id_);
		next_route_id_++;
		routeIds_.insert(InternalRouteId(element.route_id, RouteDirection::OppositeDirection), next_route_id_);
		next_route_id_++;
	}

	void IdTranslator::insert(const gtfs::Trip& element)
	{
		if (locked_)
			return;
		tripIds_.insert(element.trip_id, next_trip_id_);
		next_trip_id_++;
	}
	
	void IdTranslator::insert(const gtfs::CalendarItem& element)
	{
		if (locked_)
			return;
		serviceIds_.insert(element.service_id, next_service_id_);
		++next_service_id_;
	}

	StopId IdTranslator::at(const gtfs::Stop& element) const
	{
		return stopIds_[element.stop_id];
	}

	RouteId IdTranslator::at(const InternalRouteId& element) const
	{
		return routeIds_[element];
	}

	TripId IdTranslator::at(const gtfs::Trip& element) const
	{
		return tripIds_[element.trip_id];
	}
	
	ServiceId IdTranslator::at(const gtfs::CalendarItem& element) const
	{
		return serviceIds_[element.service_id];
	}

	StopId IdTranslator::operator[](const gtfs::Stop& element) const
	{
		return at(element);
	}

	RouteId IdTranslator::operator[](const InternalRouteId& element) const
	{
		return at(element);
	}

	TripId IdTranslator::operator[](const gtfs::Trip& element) const
	{
		return at(element);
	}

	StopId IdTranslator::at(const std::string& id, StopTag) const
	{
		return stopIds_[id];
	}

	TripId IdTranslator::at(const std::string& id, TripTag) const
	{
		return tripIds_[id];
	}
	
	ServiceId IdTranslator::at(const std::string& id, ServiceTag) const
	{
		return serviceIds_[id];
	}

	const std::string& IdTranslator::at(const StopId id) const
	{
		return stopIds_[id];
	}

	const InternalRouteId& IdTranslator::at(const RouteId id) const
	{
		return routeIds_[id];
	}

	const std::string& IdTranslator::at(const TripId id) const
	{
		return tripIds_[id];
	}
	
	const std::string& IdTranslator::at(const ServiceId id) const
	{
		return serviceIds_[id];
	}
}
