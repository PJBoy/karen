#include "k-mismatches/kangaroo.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <experimental/filesystem>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <numeric>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::literals;

const static unsigned maxMismatches = 16;

using episodeName_t = std::string;

struct Subtitle
{
    std::chrono::milliseconds time_begin, time_end;
    std::string text;
};

struct Episode
{
    episodeName_t name;
    std::deque<Subtitle> subtitles;
};

struct EpisodeNameAndOffset
{
    std::string name;
    std::chrono::milliseconds offset;
};

struct QueryResult
{
    unsigned mismatches;
    episodeName_t episodeName;
    Subtitle subtitle;
};

using offsets_t = std::unordered_map<episodeName_t, std::chrono::milliseconds>;

std::vector<std::string> split(std::string text, std::string delimiter)
{
    std::vector<std::string> ret;
    auto it_from(std::begin(text));
    for (;;)
    {
        auto it_to(std::search(it_from, std::end(text), std::begin(delimiter), std::end(delimiter)));
        ret.push_back(std::string(it_from, it_to));
            
        if (it_to == std::end(text))
            break;

        it_from = it_to + std::size(delimiter);
    }

    return ret;
}

std::string usage(std::string program)
{
    if (program.empty())
        program = "<this executable>"s;

    return program + " <videos directory> <subtitles directory> <offsets filepath>\n"s;
}

offsets_t loadOffsets(const std::experimental::filesystem::path& filepath)
{
    offsets_t ret;
    std::ifstream in(filepath);
    for (auto [i_line, line](std::tuple<unsigned, std::string>{}); std::getline(in, line); ++i_line)
    {
        if (std::empty(line))
            continue;

        std::smatch match;
        std::regex_match(line, match, std::regex(R"((.*): (-?\d+)\s*)"));
        if (std::size(match) != 3)
        {
            std::clog
                << "Warning: syntax error on line "s << i_line + 1 << " of map file "s << filepath << ":\n"s
                << line << '\n';
            
            continue;
        }

        ret[match[1]] = std::stoll(match[2]) * 1ms;
    }

    return ret;
}

std::chrono::milliseconds loadTime(const std::string& in)
{
    std::smatch match;
    std::regex_match(in, match, std::regex(R"((\d+):(\d+):(\d+)\.(\d+))"));
    if (std::size(match) != 5)
        throw std::runtime_error("Time '"s + in + "' does not match expected format"s);

    return std::stoul(match[1]) * 1h + std::stoul(match[2]) * 1min + std::stoul(match[3]) * 1s + std::stoul(match[4]) * 1ms;
}

Subtitle loadSubtitle(const std::string& in)
{
    Subtitle subtitle;
    std::smatch match;
    std::regex_match(in, match, std::regex(R"((\d+:\d+:\d+\.\d+), (\d+:\d+:\d+\.\d+), *(.*))"));
    if (std::size(match) != 4)
        throw std::runtime_error("Subtitle '"s + in + "' does not match expected format"s);

    try
    {
        subtitle.time_begin = loadTime(match[1]);
        subtitle.time_end = loadTime(match[2]);
        subtitle.text = match[3];
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Error parsing subtitle '"s + in + "': "s + e.what());
    }

    return subtitle;
}

std::vector<Subtitle> loadSubtitles(const std::experimental::filesystem::path& filepath)
{
    std::vector<Subtitle> ret;
    std::ifstream subtitleFile(filepath);
    for (auto [i_line, line](std::tuple<unsigned, std::string>{}); std::getline(subtitleFile, line); ++i_line)
        try
        {
            if (!line.empty())
                ret.push_back(loadSubtitle(line));
        }
        catch (const std::exception& e)
        {
            std::clog
                << "Warning: error on line "s << i_line + 1 << " of subtitle file "s << filepath << ":\n"s
                << e.what() << '\n';

            continue;
        }

    return ret;
}

std::vector<EpisodeNameAndOffset> pairEpisodeNameAndOffsets(std::vector<std::string> episodeNames, const offsets_t& offsets)
{
    std::vector<EpisodeNameAndOffset> ret;
    for (const std::string name : episodeNames)
        try
        {
            ret.push_back(EpisodeNameAndOffset{name, offsets.at(name)});
        }
        catch (const std::exception& e)
        {
            std::clog
                << "Warning: error getting offset for episode '"s << name << "'\n:"s
                << e.what() << '\n';
        }

    return ret;
}

std::list<Episode> loadMultiEpisode(const std::experimental::filesystem::path& filepath, const offsets_t& offsets)
{
    std::clog << "Loading episodes: "s << filepath.stem().u8string() << '\n';
    
    std::vector<std::string> episodeNames(split(filepath.stem().u8string(), " - "s));
    std::vector<EpisodeNameAndOffset> episodeNameAndOffsets(pairEpisodeNameAndOffsets(episodeNames, offsets));
    std::sort(std::begin(episodeNameAndOffsets), std::end(episodeNameAndOffsets), [](const EpisodeNameAndOffset& lhs, const EpisodeNameAndOffset& rhs){ return lhs.offset < rhs.offset; });

    std::vector<Subtitle> subtitles(loadSubtitles(filepath));
    std::list<Episode> ret;
    {
        auto it_episodeAndOffset(std::rbegin(episodeNameAndOffsets)), it_end_episodeAndOffset(std::rend(episodeNameAndOffsets));
        if (it_episodeAndOffset == it_end_episodeAndOffset)
            return ret;

        Episode episode{it_episodeAndOffset->name};
        for (auto it(std::rbegin(subtitles)), it_end(std::rend(subtitles)); it != it_end; ++it)
        {
            if (it->time_begin < it_episodeAndOffset->offset)
            {
                ++it_episodeAndOffset;
                if (it_episodeAndOffset == it_end_episodeAndOffset)
                    break;

                ret.push_front(episode);
                episode = Episode{it_episodeAndOffset->name};
            }

            episode.subtitles.push_front(Subtitle{it->time_begin - it_episodeAndOffset->offset, it->time_end - it_episodeAndOffset->offset, it->text});
        }

        ret.push_front(episode);
    }

    return ret;
}

std::list<Episode> loadEpisodes(const std::experimental::filesystem::path& subtitlesDirectory, const offsets_t& offsets)
{
    std::list<Episode> episodes;
    for (std::experimental::filesystem::path path : std::experimental::filesystem::directory_iterator(subtitlesDirectory))
        try
        {
            episodes.splice(std::end(episodes), loadMultiEpisode(path, offsets));
        }
        catch (const std::exception& e)
        {
            std::clog
                << "Warning: could not load episodes for subtitles file '"s << path << "'\n"s
                << e.what() << '\n';
        }

    return episodes;
}

std::vector<QueryResult> searchEpisode(const Episode& episode, const std::string& query)
{
    std::vector<QueryResult> results;
    for (const Subtitle& subtitle : episode.subtitles)
        if (Mismatches minMismatches(minKangaroo(std::size(query) / 4, query, subtitle.text)); minMismatches)
            results.push_back({minMismatches, episode.name, subtitle});

    return results;
}

std::vector<QueryResult> searchEpisodes(const std::list<Episode>& episodes, const std::string& query)
{
    std::vector<QueryResult> results;
    for (const Episode& episode : episodes)
        if (std::vector<QueryResult> result(searchEpisode(episode, query)); !result.empty())
            results.insert(std::end(results), std::make_move_iterator(std::begin(result)), std::make_move_iterator(std::end(result)));

    return results;
}

void handleQuery(const std::list<Episode>& episodes, const std::string& query)
{
    std::vector<QueryResult> results(searchEpisodes(episodes, query));
    std::sort(std::begin(results), std::end(results), [](const QueryResult& lhs, const QueryResult& rhs){ return lhs.mismatches < rhs.mismatches; });
    std::cout << std::size(results) << '\n';
    for (const QueryResult& result : results)
    {
        std::cout
            << 1 - float(result.mismatches) / (maxMismatches + 1) << '\n'
            << result.episodeName << '\n'
            << result.subtitle.time_begin.count() << ", " << result.subtitle.time_end.count() << ", " << result.subtitle.text << '\n'
            << '\n';
    }
}

void handleQueries(const std::list<Episode>& episodes)
{
    for (std::string query; std::getline(std::cin, query);)
        try
        {
            handleQuery(episodes, query);
        }
        catch (const std::exception& e)
        {
            std::clog
                << "Warning: error handling query '"s << query << "':\n"s
                << e.what() << '\n';
        }
}

int main(int argc, char* argv[])
{
    const std::vector<std::string> args(argv, argv + argc);
    if (std::size(args) != 4)
        return std::cerr << usage(args[0]), EXIT_FAILURE;

    const std::experimental::filesystem::path videoDirectory(args[1]), subtitlesDirectory(args[2]), offsetsFilepath(args[3]);

    offsets_t offsets(loadOffsets(offsetsFilepath));
    const std::list<Episode> episodes(loadEpisodes(subtitlesDirectory, offsets));
    handleQueries(episodes);
}
