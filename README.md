# Typelist

A compile-time typelist in C++:
```cpp
#include <typelist/typelist.h>

using OrigList = meta::TypeList<int, float, double, std::string, std::vector<int>>;

using FilteredList = OrigList::remove_if_not<std::is_floating_point>;

static_assert (FilteredList::equal<meta::TypeList<float, double>>);

using VariantType = std::variant<float, double>;

static_assert (make_type_list_from_t<VariantType>::equal<FilteredList>);

static_assert (std::is_same_v<VariantType, FilteredList::apply_to<std::variant>>);
```

Obviously C++20 is required. All the code is in a single header file, so you can just
include it to your build, though a CMake interface library is also provided.
