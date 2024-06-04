#ifndef ALGORITHM_HPP_
#define ALGORITHM_HPP_

#include <DataStructures.hpp>
#include <variant>
#include <iostream>
#include <string>

namespace raptor
{
    /**
     * @brief Enum representing different walking speed
     * 
     */
    enum class WalkingSpeed
    {
        Fast, /**< Fast walking speed, 6 km/h */
        Normal, /**< Normal walking speed, 5 km/h */
        Slow /**< Slow walking speed, 4 km/h*/
    };
    
    /**
     * @brief Options for `raptor::RouteFinder`
     * 
     */
    struct Options
    {
        /**
         * @brief Id from feed of the wanted trip service
         * 
         * Default value is valid for feed in '../BA-data'
         */
        std::string wanted_service_id = "Prac.dny_0";

        /**
         * @brief Preferred walking speed
         * 
         * Default `raptor::WalkingSpeed::Normal`
         * @see raptor::WalkingSpeed
         */
        WalkingSpeed preferred_walking_speed = WalkingSpeed::Normal;
    };
    
    /**
     * @brief Exception for invalid service id
     * 
     */
    class IdException
    {
    private:
        /**
         * @brief Stored message
         * 
         */
        const char* message_;
    public:
        IdException(const char* message) : message_(message) { }
        IdException(const std::string& message) : message_(message.c_str()) { }

        /**
         * @brief Returns message inside exception
         * 
         * @return Stored message
         */
        const char* what() const
        {
            return message_;
        }
    };
    
    /**
     * @brief Class for finding connections between two stops
     * 
     */
    class RouteFinder
    {
    private:
        /**
         * @brief Data for routes
         * 
         * @see raptor::RouteTraversal
         * 
         */
        RouteTraversal rt_;

        /**
         * @brief Data for stops
         * 
         * @see raptor::Stops
         *
         */
        Stops stops_;
        size_t num_stops_;

        /**
         * @brief Pointer to `gtfs::Feed`
         * 
         */
        const gtfs::Feed* feed_;

        /**
         * @brief Options which affect route search
         * 
         * @see raptor::Options
         * 
         */
        Options options_;

        /**
         * @brief Calculates approximate time in which the distance will be covered based on walking speed
         * 
         * @param distance Distance to be covered
         * @param speed Walking speed
         * @return Time to walk `distance`
         */
        static Time_t distanceToTime(const double distance, WalkingSpeed speed);

        /**
         * @brief Checks if `id` is a valid service id in `feed_`
         * 
         * @param id Service id as string
         * @return true The id is valid
         * @return false The id is invalid
         */
        bool checkServiceIdInFeed(const std::string& id) const;
    public:
        /**
         * @brief Type for result of a search
         * 
         * Pair contains data of a visited stop and time of arrival.
         * When two pairs are after each other, it means we walked from the first stop to the second.
         * trip_iterator comes only after a pair, there are data about the trip we used to get to the next stop.
         * Two trip_iterators can't come after each other.
         */
        using result_t = std::vector<std::variant<std::pair<StopId, Time_t>, RouteTraversal::trip_iterator>>;
        RouteFinder() : rt_(), stops_(), num_stops_(), feed_() { }
        RouteFinder(const gtfs::Feed* feed);

        /**
         * @brief Set options for route search
         * 
         * @param new_speed New walking speed
         * @param service_id New service id
         * 
         * @throws raptor::IdException If `service_id` is invalid, it's value will be stored in exception message
         */
        void setOptions(const WalkingSpeed new_speed = WalkingSpeed::Normal, const std::string& service_id = "");

        /**
         * @brief Finds the fastest connection between a start stop and an end stop which leaves from start after `departure`
         * 
         * Uses values in `options_` to modify the search
         * 
         * @param start Start stop (implementation detail: must be in vector, because feed can contain multiple stops with the same name)
         * @param end End stop (implementation detail: must be in vector, because feed can contain multiple stops with the same name)
         * @param departure Time of earliest departure from first stop
         * @throws raptor::IdException If configured `service_id` is invalid, it's value will be stored in exception message
         * @return Data about the connection in a special format
         */
        std::variant<result_t, std::string> findRoute(const std::vector<StopId>& start, const std::vector<StopId>& end, const Time_t departure) const;
    };
}

/**
 * @brief Print the `raptor::RouteFinder::result_t` to the `stream`
 * 
 * @param stream Desired output stream
 * @param data `std::tuple` with needed data `[raptor::RouteFinder::result_t, feed, departure from first stop]`
 * @return `stream`
 */
std::ostream& operator<<(std::ostream& stream, const std::tuple<raptor::RouteFinder::result_t, gtfs::Feed, raptor::Time_t>& data);

#endif // !ALGORITHM_HPP_
