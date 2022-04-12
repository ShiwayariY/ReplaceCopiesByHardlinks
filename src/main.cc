#include <iostream>
#include <filesystem>
#include <map>
#include <vector>
#include <list>
#include <ranges>
#include <concepts>
#include <iterator>
#include <algorithm>
#include <fstream>

template<typename R, typename V>
concept RangeOfUsableAs = std::ranges::range<R>&& requires(R r) {
	r.size();
	requires std::is_convertible_v<std::ranges::range_value_t<R>, V>;
};

using SameFilenameGroups = std::vector<std::list<std::string>>;

SameFilenameGroups find_filename_groups(const std::filesystem::path& root) {
	using namespace std::filesystem;

	if (!is_directory(root)) return {};

	std::map<std::string, SameFilenameGroups::value_type> filename_groups;
	for (const path& p : recursive_directory_iterator{ root }) {
		if (!is_regular_file(p)) continue;
		filename_groups[p.filename()].push_back(p);
	}

	SameFilenameGroups groups;
	std::ranges::move(filename_groups | std::views::values, std::back_inserter(groups));
	return groups;
}

bool identical_file(const std::string& f1, const std::string& f2) {
	using namespace std::filesystem;
	if (!is_regular_file(f1) || !is_regular_file(f2)) return false;
	std::ifstream ifs1{ f1, std::ios::binary | std::ios::ate };
	std::ifstream ifs2{ f2, std::ios::binary | std::ios::ate };
	if (ifs1.tellg() != ifs2.tellg() || ifs1.fail() || ifs2.fail()) return false;

	ifs1.seekg(0, std::ios::beg);
	ifs2.seekg(0, std::ios::beg);
	return std::equal(
	  std::istreambuf_iterator<char>{ ifs1 }, std::istreambuf_iterator<char>{},
	  std::istreambuf_iterator<char>{ ifs2 });
}

std::list<std::string> extract_next_identical_file_group(std::list<std::string>& files) {
	std::list<std::string> identical;
	identical.push_back(std::move(files.front()));
	files.pop_front();

	for (auto it = files.begin(); it != files.end();) {
		if (identical_file(identical.front(), *it)) {
			identical.push_back(std::move(*it));
			it = files.erase(it);
		} else
			++it;
	}

	return identical;
}

std::vector<std::list<std::string>> group_identical_files(std::list<std::string> paths) {
	std::vector<std::list<std::string>> identical_file_groups;
	while (paths.size() > 1) {
		auto identical_files = extract_next_identical_file_group(paths);
		if (identical_files.size() > 1)
			identical_file_groups.push_back(std::move(identical_files));
	}
	return identical_file_groups;
}

void delete_all_except_first(const RangeOfUsableAs<std::filesystem::path> auto& paths) {
	if (paths.size() < 2) return;
	for (auto it = std::next(paths.begin()); it != paths.end(); ++it)
		std::filesystem::remove(*it);
}

void hardlink_first_to_all(const RangeOfUsableAs<std::filesystem::path> auto& paths) {
	if (paths.size() < 2) return;

	const auto& src = *paths.begin();
	if (!std::filesystem::is_regular_file(src)) return;

	for (auto it = std::next(paths.begin()); it != paths.end(); ++it) {
		if (std::filesystem::exists(*it)) continue;
		std::filesystem::create_hard_link(src, *it);
	}
}

void print_rows(const std::ranges::range auto& list) {
	for (const auto& s : list)
		std::cout << s << "\n";
}

void replace_copies_by_hardlinks(const std::list<std::string>& files) {
	for (const auto& copies : group_identical_files(files)) {
		std::cout << "Replacing copies by hardlinks:\n";
		print_rows(copies);

		delete_all_except_first(copies);
		hardlink_first_to_all(copies);
	}
}

bool contains_non_hardlinks(const std::list<std::string>& files) {
	// does not check if all hardlinks link to the same file
	// to avoid false positives, 'files' must *all* existing hardlinks to the same file
	const auto file_num = files.size();
	return std::ranges::find_if_not(files,
			 [&file_num](const auto& file) {
				 return std::filesystem::hard_link_count(file) == file_num;
			 }) != std::ranges::end(files);
}

int main() {
	const auto filename_groups = find_filename_groups(std::filesystem::current_path());

	auto has_multiple_elements = [](const auto& e) { return e.size() > 1; };
	auto files_with_multiple_paths = filename_groups | std::views::filter(has_multiple_elements);
	auto copy_candidate_groups = files_with_multiple_paths | std::views::filter(contains_non_hardlinks);

	for (const auto& group : copy_candidate_groups)
		replace_copies_by_hardlinks(group);
}