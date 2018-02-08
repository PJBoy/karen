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

struct Subtitle
{
    std::chrono::milliseconds time_begin, time_end;
    std::string text;
};

struct Episode
{
    std::string name;
    std::deque<Subtitle> subtitles;
};

struct EpisodeNameAndOffset
{
    std::string name;
    std::chrono::milliseconds offset;
};

using offsets_t = std::unordered_map<decltype(Episode::name), std::chrono::milliseconds>;

std::vector<std::string> split(std::string text, std::string delimiter)
{
    std::vector<std::string> ret;
    {
        auto it_from(std::begin(text));
        for (;;)
        {
            auto it_to(std::search(it_from, std::end(text), std::begin(delimiter), std::end(delimiter)));
            if (it_to == std::end(text))
                break;

            ret.push_back(std::string(it_from, it_to));
            it_from = it_to + std::size(delimiter);
        }

        ret.push_back(std::string(it_from, std::end(text)));
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
    for (std::string line; std::getline(in, line);)
    {
        std::smatch match;
        std::regex_match(line, match, std::regex(R"((.*): (\d+)\s*)"));
        ret[match[1]] = std::stoul(match[2]) * 1ms;
    }

    return ret;
}

std::chrono::milliseconds loadTime(const std::string& in)
{
    std::smatch match;
    std::regex_match(in, match, std::regex(R"((\d+):(\d+):(\d+)\.(\d+))"));

    return std::stoul(match[1]) * 1h + std::stoul(match[2]) * 1min + std::stoul(match[3]) * 1s + std::stoul(match[4]) * 1ms;
}

Subtitle loadSubtitle(const std::string& in)
{
    Subtitle subtitle;
    std::smatch match;
    std::regex_match(in, match, std::regex(R"((\d+:\d+:\d+\.\d+), (\d+:\d+:\d+\.\d+), *(.*))"));
    subtitle.time_begin = loadTime(match[1]);
    subtitle.time_end = loadTime(match[2]);
    subtitle.text = match[3];
    return subtitle;
}

std::vector<Subtitle> loadSubtitles(const std::experimental::filesystem::path& filepath)
{
    std::vector<Subtitle> ret;
    std::ifstream subtitleFile(filepath);
    for (std::string line; std::getline(subtitleFile, line);)
        if (!line.empty())
            ret.push_back(loadSubtitle(line));

    return ret;
}

std::vector<EpisodeNameAndOffset> pairEpisodeNameAndOffsets(std::vector<std::string> episodeNames, const offsets_t& offsets)
{
    std::vector<EpisodeNameAndOffset> ret;
    for (const std::string name : episodeNames)
        ret.push_back(EpisodeNameAndOffset{name, offsets.at(name)});

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
        episodes.splice(std::end(episodes), loadMultiEpisode(path, offsets));

    return episodes;
}

Episode searchEpisode(const Episode& episode, const std::string& query)
{
    Episode results{episode.name};
    for (const Subtitle& subtitle : episode.subtitles)
        if (std::search(std::begin(subtitle.text), std::end(subtitle.text), std::begin(query), std::end(query)) != std::end(subtitle.text))
            results.subtitles.push_back(subtitle);

    return results;
}

std::vector<Episode> searchEpisodes(const std::list<Episode>& episodes, const std::string& query)
{
    std::vector<Episode> results;
    for (const Episode& episode : episodes)
        if (Episode&& result(searchEpisode(episode, query)); !result.subtitles.empty())
            results.push_back(result);

    return results;
}

void handleQuery(const std::list<Episode>& episodes, const std::string& query)
{
    const std::vector<Episode> results(searchEpisodes(episodes, query));
    std::cout << std::size(results) << '\n' << std::flush;
    for (const Episode& episode : results)
    {
        std::cout << std::size(episode.subtitles) << ", " << episode.name << '\n' << std::flush;
        for (const Subtitle& subtitle : episode.subtitles)
            std::cout << subtitle.time_begin.count() << ", " << subtitle.time_end.count() << ", " << subtitle.text << '\n';

        std::cout << '\n' << std::flush;
    }
}

void handleQueries(const std::list<Episode>& episodes)
{
    for (std::string query; std::getline(std::cin, query);)
        handleQuery(episodes, query);
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
