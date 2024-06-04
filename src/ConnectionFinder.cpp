#include <Algorithm.hpp>
#include <iostream>
#include <sstream>
#include <optional>
#include <unordered_set>

using namespace std;
using namespace raptor;

/**
 * @brief Short name of the application in terminal
 * 
 */
constexpr char term_name[] = "(cf)";

/**
 * @brief Enum with values for each defined command
 * 
 */
enum class TermCommand
{
    FindRoute,
    Help,
    ListStops,
    Quit,
    Nop,
    Unrecognized,
    SetOptions,
    ListServices
};

/**
 * @brief Type for command and arguments
 * 
 */
using com_args_t = pair<TermCommand, optional<vector<string>>>;

/**
 * @brief Parses a line
 * 
 * @param line Line to parse
 * @return Pair of command and arguments
 */
com_args_t parse_line(const string& line)
{
    istringstream line_stream(line);
    string command;
    getline(line_stream, command, ' ');
    if (command == "" && !line_stream)
    {
        return pair(TermCommand::Nop, nullopt);
    }
    else if (command == "fr" || command == "findroute")
    {
        vector<string> args;
        string arg;
        while (getline(line_stream, arg, '-'))
        {
            args.push_back(arg);
        }
        return pair(TermCommand::FindRoute, args);
    }
    else if (command == "h" || command == "help")
    {
        return pair(TermCommand::Help, nullopt);
    }
    else if (command == "ls" || command == "liststops")
    {
        vector<string> args;
        string arg;
        while (getline(line_stream, arg, ' '))
        {
            args.push_back(arg);
        }
        return pair(TermCommand::ListStops, args);
    }
    else if (command == "q" || command == "quit")
    {
        return pair(TermCommand::Quit, nullopt);
    }
    else if (command == "s" || command == "set")
    {
        vector<string> args;
        string arg;
        while (getline(line_stream, arg, ' '))
        {
            args.push_back(arg);
        }
        return pair(TermCommand::SetOptions, args);
    }
    else if (command == "ser" || command == "services")
    {
        return pair(TermCommand::ListServices, nullopt);
    }
    else
    {
        return pair(TermCommand::Unrecognized, nullopt);
    }
}

/**
 * @brief Finds all stops in `feed` that have the name `stop_name`
 * 
 * @param stop_name Desired name
 * @param feed A `gtfs::Feed` with data
 * @return Vector with `raptor::StopId` representing the found stops
 */
vector<StopId> find_stops_by_name(const std::string& stop_name, const gtfs::Feed& feed)
{
    vector<StopId> result;
    auto tr = IdTranslator::getInstance;
    for (auto&& stop : feed.get_stops())
    {
        if (stop.stop_name == stop_name)
            result.push_back(tr().at(stop));
    }
    return result;
}

/**
 * @brief Finds a connection based on arguments passed in args
 * 
 * If some input data is invalid, it won't calculate anything and print error message
 * 
 * @param args First position start stop, second end stop, third departure time
 * @param rf 
 * @param feed 
 */
void find_route(com_args_t::second_type& args, const RouteFinder& rf, const gtfs::Feed& feed)
{
    if (!args || args->size() < 3)
    {
        cout << "Missing arguments for 'findroute' command!\n";
        return;
    }
    if (args->size() > 3)
    {
        cout << "Provided too many arguments for 'findroute' command!\n";
        return;
    }
    auto arguments(std::move(args.value()));
    auto start_stops = find_stops_by_name(arguments[0], feed);
    if (start_stops.size() == 0)
    {
        cout << "Unrecognized start stop '" << arguments[0] << "'!\n";
        return;
    }
    auto end_stops = find_stops_by_name(arguments[1], feed);
    if (end_stops.size() == 0)
    {
        cout << "Unrecognized end stop '" << arguments[1] << "'!\n";
        return;
    }
    Time_t departure_time;
    try
    {
        departure_time = toTime(arguments[2]);
    }
    catch (exception&)
    {
        cout << "Invalid departure time!\n";
        return;
    }
    try
    {
        auto result = rf.findRoute(start_stops, end_stops, departure_time);
        visit([&](auto&& arg)
        {
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, RouteFinder::result_t>)
            {
                const tuple x(arg, feed, departure_time);
                cout << x;
            }
            else if constexpr (is_same_v<T, string>)
                cout << arg;            
        }, result);
    }
    catch (const IdException& e)
    {
        cout << "Service with id '" << e.what() << "' is not in feed!\n";
        cout << "Please set another service id using the command 'set'\n";
    }
}

/**
 * @brief Prints a help message to `std::cout`
 * 
 */
void print_help()
{
    constexpr char prefix[] = "  ";
    cout << prefix << "Usage...\n";
    cout << prefix << "At startup you need to type full path to a directory containing a GTFS feed.\n\n";
    cout << prefix << "Commands... 'name'|'alias' (arguments) \n";
    cout << prefix << "'findroute'|'fr' (start stop, end stop, departure time - hh:mm) --- Find route between specified 'stops' starting at 'departure time'. ";
    cout << "Arguments should be separated by '-'.\n";
    cout << prefix << "'help'|'h' --- Prints this help message.\n";
    cout << prefix << "'liststops'|'ls' (optional: prefix) --- Print a list of all/stops starting with 'prefix' stops in feed.\n";
    cout << prefix << "'quit'|'q' --- Exits.\n";
    cout << prefix << "'set'|'s' (walking speed - 'Fast'|'Normal'|'Slow', service id) --- Sets preferred walking speed and which trips to use. If service id is left empty, it wont modify it.\n";
    cout << prefix << "'services'|'ser' --- Print a list of all services in feed.\n";
}

/**
 * @brief Print all stops in `feed`
 * 
 * @param args If empty, lists all stops, if one argument is present, it print all stops starting with that string
 * @param feed A `gtfs::Feed` with data
 */
void list_stops(com_args_t::second_type& args, const gtfs::Feed& feed)
{
    if (args->size() > 1)
    {
        cout << "Provided too many arguments for 'liststops' command!\n";
        return;
    }
    auto arguments(std::move(args.value()));
    cout << "Stops in feed...\n";
    constexpr char prefix[] = " ∟ ";
    unordered_set<string> found_stops;
    for (auto&& stop : feed.get_stops())
    {
        if (found_stops.contains(stop.stop_name))
            continue;
        if (arguments.size() == 0 || arguments[0] == " " || stop.stop_name.starts_with(arguments[0]))
        {
            cout << prefix << stop.stop_name << '\n';
            found_stops.insert(stop.stop_name);
        }
    }
}

/**
 * @brief Set options in `rf`
 * 
 * If either value in `args` is invalid, it won't set the options
 * 
 * @param args Parsed arguments from terminal. Valid content is string representation of `raptor::WalkingSpeed` and new service id
 * @param rf 
 */
void set_options(com_args_t::second_type& args, RouteFinder& rf)
{
    if (!args || args->size() < 1)
    {
        cout << "Missing arguments for 'set' command!\n";
        return;
    }
    if (args->size() > 2)
    {
        cout << "Provided too many arguments for 'set' command!\n";
        return;
    }
    auto arguments(std::move(args.value()));
    WalkingSpeed new_speed;
    string wanted_service = "";
    if (arguments[0] == "Slow")
        new_speed = WalkingSpeed::Slow;
    else if (arguments[0] == "Normal")
        new_speed = WalkingSpeed::Normal;
    else if (arguments[0] == "Fast")
        new_speed = WalkingSpeed::Fast;
    else
    {
        cout << "Unrecognized walking speed\n";
        cout << "Options not set\n";
        return;
    }
    if (arguments.size() == 2)
        wanted_service = arguments[1];
    try
    {
        rf.setOptions(new_speed, wanted_service);
    }
    catch (const IdException& e)
    {
        cout << "Service with id '" << e.what() << "' is not in feed!\n";
        cout << "Options not set\n";
        return;
    }
    cout << "Options set\n";
}

/**
 * @brief Print all service ids in `feed`
 * 
 * @param feed A `gtfs::Feed` with data
 */
void list_services(const gtfs::Feed& feed)
{
    cout << "Services in feed...\n";
    constexpr char prefix[] = " ∟ ";
    for (auto&& service : feed.get_calendar())
    {
        cout << prefix << service.service_id << '\n';
    }
}

/**
 * @brief Calls appropriate function based on parsed command in `com_args`
 * 
 * @param com_args Parsed command and optionally arguments for said command
 * @param rf A `raptor::RouteFinder` object
 * @param feed A `gtfs::Feed`
 * 
 * @see main_loop()
 * @return true Main loop should end
 * @return false Main loop should not end
 */
bool execute_command(com_args_t& com_args, RouteFinder& rf, const gtfs::Feed& feed)
{
    auto&& [command, args] = com_args;
    switch (command)
    {
    case TermCommand::FindRoute:
        find_route(args, rf, feed);
        return false;
    case TermCommand::Help:
        print_help();
        return false;
    case TermCommand::ListStops:
        list_stops(args, feed);
        return false;
    case TermCommand::Nop:
        return false;
    case TermCommand::Quit:
        return true;
    case TermCommand::SetOptions:
        set_options(args, rf);
        return false;
    case TermCommand::ListServices:
        list_services(feed);
        return false;
    case TermCommand::Unrecognized:
        cout << "Undefined command. Try 'help'.\n";
        return false;
    default:
        throw invalid_argument("Command not implemented!");
    }
}

/**
 * @brief Returns exit codes based on error in `std::cin`
 * 
 * @return 1 If end of life is read
 * @return 2 If badbit or failbit is set
 * @return 3 Other error
 */
int handle_cin_error()
{
    if (cin.eof())
        return 1;
    if (cin.fail())
        return 2;
    return 3;
    
}

/**
 * @brief Main loop of the terminal application
 * 
 * In each loops it reads a line from `std::cin` and decides what to do next
 * 
 * @param rf A `raptor::RouteFinder`
 * @param feed A `gtfs::Feed`
 * @see handle_cin_error()
 * @return Exit code for the application
 */
int main_loop(RouteFinder& rf, const gtfs::Feed& feed)
{
    bool end = false;
    while (!end)
    {
        if (!cin)
            return handle_cin_error();
        cout << term_name << " ";
        string line;
        getline(cin, line);
        auto com_args = parse_line(line);
        end = execute_command(com_args, rf, feed);
    }
    return 0;
}

/**
 * @brief Initializes data structures needed for application and starts main loop
 * 
 * @see main_loop()
 * 
 * @return Exit code
 */
int main()
{
   cout << "Connection Finder\n";
   cout << "This is a term project by Oliver Lago for NPRG041 Programming in C++ class.\n";
   cout << "It can find the fastest connection between a start and an end stop from a specified GTFS Feed.\n";
   constexpr char link_to_repo[] = "https://gitlab.mff.cuni.cz/teaching/nprg041/2023-24/svoboda-1040/lagoo/-/tree/master/project";
   cout << "You can read more information here " << link_to_repo << '\n';
   cout << "Specify path to a folder with GTFS feed.\n";
   cout << term_name << " ";
   string feed_location;
   cin >> feed_location;
   cout << "Parsing feed, this step could take a while...\n";
   gtfs::Feed feed(feed_location);
   while (feed.read_feed() != gtfs::OK)
   {
       if (!cin)
           return handle_cin_error();
       cerr << "Invalid feed, enter a path again...\n";
       cout << term_name << " ";
       cin >> feed_location;
       cout << "Parsing feed, this step could take a while...\n";
       feed = gtfs::Feed(feed_location);
   }
   getline(cin, feed_location);
   cout << "Feed OK, proceeding to generate required data structures. This step might take a while...\n";
   RouteFinder rf(&feed);
   cout << "Data structures generated. You may enter your queries now.\n" << "Type 'h' or 'help' to show query syntax.\n";
   return main_loop(rf, feed);
}
