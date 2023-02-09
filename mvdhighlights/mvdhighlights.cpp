#include <iostream>
#include <string>
#include <iterator>
#include <chrono>
#include <array>
#include <optional>
#include <deque>
#include <ranges>
#include <unordered_set>
#include <string_view>
#include <algorithm>
#include <fstream>
#include "argparse.h"
int fps = 10;
using namespace std::chrono_literals;
//#include "messages.h"
const std::unordered_set<std::string> messages_left{
" had its legs blown off thanks to",
" was knocked out by",
" was John Woo'd by",
"was shredded by",
" got his ass kicked by",
" saw the sniper bullet go through his scope thanks to",
" was trepanned by",
" needs some new kidneys thanks to",
"was popped by",
"was melted by",
"was shoved off the edge by",
"melted thanks to",
" was sniped by",
" had a Bruce Lee put on it by",
" was picked off by",
" had a makeover by",
" was caught by",
" had its legs cut off thanks to",
" got a free facelift by",
"was blasted by",
" was slashed apart by",
" had his legs cut off thanks to",
" had her throat slit by",
" is now shorter thanks to",
" has an upset stomach thanks to",
" was hit by",
" was gutted by",
" had its throat slit by",
" was shot by",
"was thrown through a window by",
" sees the contents of her own stomach thanks to",
" saw the sniper bullet go through its scope thanks to",
" had his legs blown off thanks to",
" had a Bruce Lee put on him by",
"was taught how to fly by",
" loses a vital chest organ thanks to",
" was shot in the legs by",
"was drop-kicked into the lava by",
" feels some heart burn thanks to",
" sees the contents of its own stomach thanks to",
" was stabbed repeatedly in the legs by",
" was minched by",
" had her legs cut off thanks to",
" sees the contents of his own stomach thanks to",
" had a Bruce Lee put on her by",
" got her ass kicked by",
" had her legs blown off thanks to",
" was sniped in the stomach by",
" had his throat slit by",
" won't be able to pass a metal detector anymore thanks to",
" saw the sniper bullet go through her scope thanks to",
" got its ass kicked by" };
using timepoint = std::chrono::duration<int, std::deci>;
timepoint fast_2k_duration = 4s;
timepoint fast_3k_duration = 8s;
struct timepoint_printer {
    std::chrono::duration<int,std::deci> tp;
    friend std::ostream& operator<<(std::ostream& os, const timepoint_printer& printer) {
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(printer.tp);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(printer.tp - minutes);
        std::chrono::duration<int,std::deci> tenths = printer.tp - minutes - seconds;
        os << minutes.count() << ':' << seconds.count() << '.' << tenths.count();
        return os;
    }
};
//parse timepoint from string in [mm:ss:x] format
timepoint parse_timepoint(const std::string& str) {
    struct timepoint_substr_format {
        size_t minutes, seconds, tenths;
    };
    timepoint_substr_format offsets, counts;
    if (str.at(2) == ':') {
        offsets = { 1,3,6 };
        counts = { 1,2,1 };
    }
    else if (std::isdigit(str.at(2))) {
        offsets = { 1,4,7 };
        counts = { 2,2,1 };
    }
    else {
        std::cerr << "Error in MVDTOOL output - expected timespec but got something else\n";
        exit(1);
    }
    int minutes = std::stoi(str.substr(offsets.minutes, counts.minutes));
    int seconds = std::stoi(str.substr(offsets.seconds, counts.seconds));
    int tenths = std::stoi(str.substr(offsets.tenths, counts.tenths));
    return std::chrono::minutes(minutes) + std::chrono::seconds(seconds)
        + timepoint(seconds);
}

struct KillInfo {
    std::string killer;
};

const std::vector<std::string> award_strings {
    "MVD IMPRESSIVE ",
    "MVD ACCURACY ",
    "MVD EXCELLENT "
};

std::vector<std::string> playernames;

size_t mvd_award_index(const std::string& line) {
    for (const auto& award : award_strings) {
        auto index = line.find(award);
        if (index != std::string::npos) {
            return index + award.size() - 1;
        }
    }
    return std::string::npos;
}

struct Highlight {
    timepoint start;
    timepoint end;
    std::string reason;
    friend std::ostream& operator<<(std::ostream& os, const Highlight& hl) {
        os << timepoint_printer{ hl.start } << "->" << timepoint_printer{ hl.end };
        os << ": " << hl.reason;
        return os;
    }
};

struct Line {
    Line(const std::string& line) {
        time = parse_timepoint(line);
        auto award_index = mvd_award_index(line);
        if (award_index != std::string::npos) {
            //[11:12.6] MVD IMPRESSIVE _Misse!
            award_player_name =
                line.substr(award_index + 1);
            award_player_name.value().pop_back();
            return;
        }
        for (auto& msg : messages_left) {
            if (auto msg_it = line.find(msg); msg_it != std::string::npos) {
                auto killer_name_it = msg_it + msg.size();
                auto apostrophe_it = line.substr(killer_name_it).find("'s");
				auto killer_name = line.substr(
                    killer_name_it+1, apostrophe_it);
                killer_name.pop_back();
                kill_info = KillInfo{ killer_name };
                break;
			}
        }
    }
    timepoint time;
    std::optional<std::string> award_player_name;
	std::optional<KillInfo> kill_info;
    bool empty() const {
        return !(
            award_player_name.has_value() || kill_info.has_value()
            );
    }
    friend std::ostream& operator<<(std::ostream& os, const Line& line) {
        os << timepoint_printer{ line.time } << " ";
        if (line.award_player_name.has_value()) {
            os << line.award_player_name.value() << " AWARD ";
        }
        if (line.kill_info.has_value()) {
            os << line.kill_info.value().killer << " got a kill ";
        }
        return os;
    }
};

struct Round {
    timepoint start_time;
    std::vector<Line> lines;
    void add(Line line) {
        if (!line.empty()) lines.push_back(line);
    }
    std::optional<Highlight> get_highlight(std::string playername) const {
        auto player_kill = [&playername](const Line& line) {
            if (line.kill_info) {
                return line.kill_info.value().killer == playername;
            }
            return false;
        };
    
        if (std::ranges::count_if(lines, player_kill) >= 4) {
            return Highlight {
                start_time,
                std::ranges::find_if(std::ranges::reverse_view(lines),player_kill)->time,
                "Killed 4+ people"
            };
        }
        {
            bool give_hl = false;
            Highlight hl;
            std::deque<timepoint> kill_times;
            for (const auto& line : lines) {
                if (line.kill_info) {
                    if (line.kill_info.value().killer != playername) {
                        //std::cout << "COMPARISON -";
                        //std::cout << line.kill_info.value().killer;
                        //std::cout << playername << '\n';
                        continue;
                    }
                    if (kill_times.empty()) kill_times.push_back(line.time);
                    else if (kill_times.size() == 1) {
                        if (line.time - kill_times[0] <= fast_2k_duration) {
                            give_hl = true;
                            hl.reason += "Fast 2k! ";
                            kill_times.push_back(line.time);
                            hl.start = kill_times[0] - 2s;
                            hl.end = line.time + 2s;
                        }
                    }
                    else if (kill_times.size() == 2) {
                        if (line.time - kill_times[1] <= fast_2k_duration) {
                            give_hl = true;
                            hl.reason += "Fast 2k! ";
                            hl.start = kill_times[0] - 2s;
                            hl.end = line.time + 2s;
                        }
                        if (line.time - kill_times[0] <= fast_3k_duration) {
                            give_hl = true;
                            hl.reason += "Fast 3k! ";
                            hl.start = kill_times[0] - 2s;
                            hl.end = line.time + 2s;
                        }
                        kill_times.push_back(line.time);
                        kill_times.pop_front();
                    }
                }
            }
            if (give_hl) return hl;
        }
        for (const auto& line : lines) {
            if (line.award_player_name.has_value()) {
                if (line.award_player_name.value() == playername) {
                    return Highlight{ line.time - 2s, line.time + 2s, "Awarded Accu/Impres/Excellent" };
                }
            }
        }
        return std::nullopt;
    }
    friend std::ostream& operator<<(std::ostream& os, const Round& round) {
        os << "\nNew Round " << timepoint_printer{ round.start_time } << '\n';
        for (const auto& line : round.lines) {
            os << line << '\n';
        }
        return os;
    }
};

bool mvd_action(const std::string& str) {
	return str.find("MVD ACTION") != std::string::npos;
}

std::vector<Round> parse(std::istream& in) {
	std::vector<Round> rounds;
	std::string line_str;
    Round round;
    while (std::getline(in, line_str)) {
      //  std::cout << 'S' << line_str << std::endl;
		round.start_time = parse_timepoint(line_str);
        if (mvd_action(line_str)) break;
    }
    while (std::getline(in, line_str)) {
      //  std::cout << '#' << line_str << std::endl;
        if (mvd_action(line_str)) {
			rounds.push_back(round);
            round = { parse_timepoint(line_str), {} };
        }
        else {
            round.add({ line_str });
        }
    }
    rounds.push_back(round);
    return rounds;
}

void write_highlights(std::vector<Round> rounds, std::string playername, std::ostream& os) {
    auto say_with_dots = [&os](std::string str) {
        for (int i = 0; i < 5; i++) {
            if (i == 2) os << "say " << str << '\n';
            else os << "say ..............\n";
        }
    };
    os << "set hl_old_maxfps $cl_maxfps\n";
    os << "set hl_old_name $name\n";
    os << "cl_maxfps " << fps << '\n';
    os << "name MVD_HIGHLIGHTS\n";
    os << "wait " << fps << '\n';
    for (const auto& round : rounds) {
        auto hl_opt = round.get_highlight(playername);
        if (hl_opt) {
            auto hl = hl_opt.value();
            say_with_dots(hl.reason);
            os << "seek " << timepoint_printer{hl.start-3s} << '\n';
            os << "pause\n";
            os << "wait " << fps << '\n';
            os << "pause\n";
            os << "chase " << playername << '\n';
            os << "wait " << (hl.end - hl.start+2s).count() << '\n';
        }
    }
    say_with_dots("End of highlights");
    os << "pause\n";
    os << "set cl_maxfps $hl_old_maxfps\n";
    os << "set name $hl_old_name\n";
}

int main(int argc, const char* argv[]) {
    using namespace argparse;
    ArgumentParser parser("mvdhighlights", "mvdhighlights");
    parser.add_argument()
        .names({ "-p","--player" })
        .description("Player name")
        .required(true);
    parser.enable_help();
    auto err = parser.parse(argc, argv);
    if (err) {
        std::cerr << err << '\n';
        return -1;
    }
    if (parser.exists("help")) {
        parser.print_help();
        return 0;
    }
    std::string playername = parser.get<std::string>("player");
    std::cout << std::setw(2) << std::setfill('0');
    auto rounds = parse(std::cin);
    for (auto& round : rounds) {
        std::cout << round;
    }
    std::ofstream ofs("highlights.cfg");
    write_highlights(rounds, playername, ofs);
}