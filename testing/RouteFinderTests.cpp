#include <gtest/gtest.h>
#include <just_gtfs.h>
#include <Algorithm.hpp>
#include <fstream>

using namespace raptor;
constexpr char feed_location[] = "example-data";
std::ofstream o("out.txt");

class RouteFinderTest : public testing::TestWithParam<std::pair<std::string, std::string>>
{
protected:
    gtfs::Feed feed_;
    std::ofstream out;
    void SetUp() override
    {
        feed_ = gtfs::Feed(feed_location);
        auto result = feed_.read_feed();
        EXPECT_EQ(result.code, gtfs::OK);
        out.open("out.txt", std::ios::app);
    }
    
    std::vector<StopId> find_stops_by_name(const std::string& stop_name)
    {
        std::vector<StopId> result;
        auto tr = IdTranslator::getInstance;
        for (auto&& stop : feed_.get_stops())
        {
            if (stop.stop_name  == stop_name)
                result.push_back(tr().at(stop));
        }
        return result;
    }
};

auto generateParams()
{
    std::vector<std::pair<std::string, std::string>> result;
    const std::vector<std::string> start_stops{
        "Furnace Creek Resort (Demo)", "Nye County Airport (Demo)",
        "Bullfrog (Demo)", "Stagecoach Hotel & Casino (Demo)",
        "North Ave / D Ave N (Demo)", "North Ave / N A Ave (Demo)",
        "Doing Ave / D Ave N (Demo)", "E Main St / S Irving St (Demo)",
        "Amargosa Valley (Demo)"
        };
    const std::vector<std::string> end_stops{
        "Furnace Creek Resort (Demo)", "Nye County Airport (Demo)",
        "Bullfrog (Demo)", "Stagecoach Hotel & Casino (Demo)",
        "North Ave / D Ave N (Demo)", "North Ave / N A Ave (Demo)",
        "Doing Ave / D Ave N (Demo)", "E Main St / S Irving St (Demo)",
        "Amargosa Valley (Demo)"
        };
    for (auto&& start : start_stops)
    {
        for (auto&& end : end_stops)
        {
            result.emplace_back(start, end);
        }
    }
    return result;
}

TEST_P(RouteFinderTest, TestExampleData)
{
    RouteFinder rf(&feed_);
    rf.setOptions(WalkingSpeed::Normal, "FULLW");
    IdTranslator::getInstance().lock();
    auto&& [start, end] = GetParam();
    const Time_t departure = 5*60*60;
    out << start << '-' << end << '\n';
    auto starts = find_stops_by_name(start);
    auto ends = find_stops_by_name(end);
    auto result = rf.findRoute(starts, ends, departure);
    std::visit([&](auto&& arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, RouteFinder::result_t>)
        {
            const std::tuple x(arg, feed_, departure);
            out << x;
        }
        else if constexpr (std::is_same_v<T, std::string>)
            out << arg;        
    }, result);
    out << '\n';
}

std::string removeSpaces(const std::string& str)
{
    std::string result = "";
    for (auto&& chr : str)
    {
        if (std::isalnum(chr))
            result += chr;
    }
    return result;
}

INSTANTIATE_TEST_SUITE_P(RFTest, RouteFinderTest,
                         testing::ValuesIn(generateParams()),
                         [](const testing::TestParamInfo<RouteFinderTest::ParamType>& tinfo) {
                             std::string name = removeSpaces(tinfo.param.first) + "_" + removeSpaces(tinfo.param.second);
                             return name;
                         });
