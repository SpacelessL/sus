#pragma once

#include <string>
#include <unordered_map>
#include <any>

namespace spaceless {

class arguments_manager {
public:
	arguments_manager(int argc, char **argv);

	template<typename T>
	[[nodiscard]] T *get(std::string_view name) noexcept {
		return std::any_cast<T>(get_any(name));
	}

	[[nodiscard]] std::any *get_any(std::string_view name) noexcept {
		auto p = get_item(name);
		return p ? &p->value : nullptr;
	}

private:
	struct item {
		std::any value, default_value;
		std::string_view desc;
	};

	item *get_item(std::string_view name) noexcept {
		if (auto it = values_.find(name); it != values_.end())
			return &it->second;
		return nullptr;
	}

	std::unordered_map<std::string_view, item> values_, defaults_;
};

}
