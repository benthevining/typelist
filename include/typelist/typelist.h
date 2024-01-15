#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <memory>
#include <functional>

/** @defgroup typelist Typelist library
	A library providing metaprogramming features, primarily a compile-time typelist.
 */

/** @file
	The main include file of the typelist library.
	@ingroup typelist
 */

/** This namespace contains all the typelist library code.
	@ingroup typelist
 */
namespace meta
{

/** @concept Function
	This concept simply requires that an object is invocable.
	@ingroup typelist
 */
template <typename T>
concept Function = requires (T f)
{
	{ std::invoke (f) };
};

/** This typedef evaluates to the result type returned by calling the given function with the given arguments.
	@ingroup typelist
 */
template <Function Func, typename... Args>
using func_result_type = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

/** A dummy type needed in some places.
	@ingroup typelist
 */
struct NullType final {};

/*----------------------------------------------------------------------------------------------------------------------*/

/** This namespace contains internal implementation code that should not
	be used directly by third party projects.
	@ingroup typelist
 */
namespace impl
{
/// @cond

template <size_t Value>
using SizeConstant = std::integral_constant<size_t, Value>;

static constinit const size_t invalid_index = static_cast<size_t> (-1);

template <typename... Types>
struct InternalTypeList final
{
	template <template <typename...> class TypelistType>
	using makeTypelist = TypelistType<Types...>;
};

template <template <typename> class Condition>
struct TypeErasedUnaryPredicate final
{
	template <typename Type>
	static constexpr const bool met = Condition<Type>();
};

/*-----------------------------------------------------------*/

template <class LHS, class RHS>
struct are_same final : std::false_type
{
};

template <template <typename...> class LHS, template <typename...> class RHS, typename... Args>
struct are_same<LHS<Args...>, RHS<Args...>> final : std::true_type
{
};

template <class LHS, class RHS>
static constexpr const bool are_same_v = are_same<LHS, RHS>::value;

/*-----------------------------------------------------------*/

template <class Typelist, typename... TypesToFind>
struct contains final
{
private:
	template <class TypeList, typename TypeToFind>
	struct contains_one;

	template <template <typename...> class TypeList, typename TypeToFind, typename... Args>
	struct contains_one<TypeList<Args...>, TypeToFind> final : std::bool_constant<(std::is_same_v<TypeToFind, Args> || ...)>
	{
	};

public:
	static constexpr const bool value = (contains_one<Typelist, TypesToFind>::value && ...);
};

template <class Typelist, typename... TypesToFind>
static constexpr const bool contains_v = contains<Typelist, TypesToFind...>::value;

template <class Typelist, typename... TypesToFind>
struct contains_or final : std::bool_constant<(contains_v<Typelist, TypesToFind> || ...)>
{
};

template <class Typelist, typename... TypesToFind>
static constexpr const bool contains_or_v = contains_or<Typelist, TypesToFind...>::value;

/*-----------------------------------------------------------*/

template <class Typelist>
struct size final : SizeConstant<0>
{
};

template <template <typename...> class Typelist, typename... Args>
struct size<Typelist<Args...>> final : SizeConstant<sizeof...(Args)>
{
};

template <class Typelist>
static constexpr const size_t size_v = size<Typelist>::value;

template <class Typelist>
static constexpr const bool is_empty = (size_v<Typelist> == 0);

/*-----------------------------------------------------------*/

template <class Typelist, typename... TypesToAdd>
struct add;

template <template <typename...> class Typelist, typename... TypesToAdd, typename... Args>
struct add<Typelist<Args...>, TypesToAdd...> final
{
	using type = Typelist<Args..., TypesToAdd...>;
};

template <class Typelist, typename... TypesToAdd>
using add_t = typename add<Typelist, TypesToAdd...>::type;

/*-----------------------------------------------------------*/

template <class List1, class List2>
struct add_from;

template <template <typename...> class List1, template <typename...> class List2, typename... List1Args, typename... List2Args>
struct add_from<List1<List1Args...>, List2<List2Args...>> final
{
	using type = List1<List1Args..., List2Args...>;
};

template <class List1, class List2>
using add_from_t = typename add_from<List1, List2>::type;

/*-----------------------------------------------------------*/

template <class Typelist, typename... TypesToAdd>
struct addIfAbsent;

template <template <typename...> class Typelist, typename... TypesToAdd, typename... Args>
struct addIfAbsent<Typelist<Args...>, TypesToAdd...> final
{
private:
	template <class ListType, typename Type>
	using add_one = std::conditional_t<
		(! contains_v<Typelist<Args...>, Type>),
		add_t<ListType, Type>,
		ListType>;

	template <class ListType, typename... ListArgs>
	struct adder;

	template <class ListType, typename First, typename... Others>
	struct adder<ListType, First, Others...> final
	{
		using type = typename adder<
			add_one<ListType, First>,
			Others...>::type;
	};

	template <class ListType, typename First>
	struct adder<ListType, First> final
	{
		using type = add_one<ListType, First>;
	};

public:
	using type = typename adder<Typelist<Args...>, TypesToAdd...>::type;
};

template <class Typelist, typename... TypesToAdd>
using addIfAbsent_t = typename addIfAbsent<Typelist, TypesToAdd...>::type;

/*-----------------------------------------------------------*/

template <class List1, class List2>
struct common_with;

template <template <typename...> class List1, template <typename...> class List2, typename... List1Types, typename... List2Types>
struct common_with<List1<List1Types...>, List2<List2Types...>> final
{
private:
	template <class NewList, typename Type>
	using conditional_add = std::conditional_t<
		contains_v<List2<List2Types...>, Type>,
		add_t<NewList, Type>,
		NewList>;

	template <class NewList, typename... Types>
	struct builder;

	template <class NewList, typename First, typename... Others>
	struct builder<NewList, First, Others...> final
	{
		using type = typename builder<
			conditional_add<NewList, First>,
			Others...>::type;
	};

	template <class NewList, typename First>
	struct builder<NewList, First> final
	{
		using type = conditional_add<NewList, First>;
	};

public:
	using type = typename builder<InternalTypeList<>, List1Types...>::type::template makeTypelist<List1>;
};

template <class List1, class List2>
using common_t = typename common_with<List1, List2>::type;

/*-----------------------------------------------------------*/

template <class List1, class List2>
struct not_in;

template <template <typename...> class List1, template <typename...> class List2, typename... List1Types, typename... List2Types>
struct not_in<List1<List1Types...>, List2<List2Types...>> final
{
private:
	template <class NewList, typename Type>
	using conditional_add = std::conditional_t<
		contains_v<List2<List2Types...>, Type>,
		NewList,
		add_t<NewList, Type>>;

	template <class NewList, typename... Types>
	struct builder;

	template <class NewList, typename First, typename... Others>
	struct builder<NewList, First, Others...> final
	{
		using type = typename builder<
			conditional_add<NewList, First>,
			Others...>::type;
	};

	template <class NewList, typename First>
	struct builder<NewList, First> final
	{
		using type = conditional_add<NewList, First>;
	};

public:
	using type = typename builder<InternalTypeList<>, List1Types...>::type::template makeTypelist<List1>;
};

template <class List1, class List2>
using not_in_t = typename not_in<List1, List2>::type;

template <class List1, class List2>
static constexpr const bool same_ignoring_order_v = is_empty<not_in_t<List1, List2>> && is_empty<not_in_t<List2, List1>>;

/*-----------------------------------------------------------*/

template <class Typelist, size_t Index, typename ToInsert>
struct insert_at;

template <template <typename...> class Typelist, size_t Index, typename ToInsert, typename... Args>
struct insert_at<Typelist<Args...>, Index, ToInsert> final
{
private:
	static_assert (Index <= size_v<Typelist<Args...>>, "Insertion index in type list is out of bounds!");

	template <size_t CurrentIndex, class ListType, typename Type>
	using inserter = std::conditional_t<
		CurrentIndex == Index,
		add_t<add_t<ListType, ToInsert>, Type>,
		add_t<ListType, Type>>;

	template <size_t CurrentIndex, class ListType, typename... ListArgs>
	struct rebuilder;

	template <size_t CurrentIndex, class ListType, typename First, typename... Others>
	struct rebuilder<CurrentIndex, ListType, First, Others...> final
	{
		using type = typename rebuilder<CurrentIndex + 1, inserter<CurrentIndex, ListType, First>, Others...>::type;
	};

	template <size_t CurrentIndex, class ListType, typename First>
	struct rebuilder<CurrentIndex, ListType, First> final
	{
	public:
		using type = inserter<CurrentIndex, ListType, First>;
	};

public:
	using type = typename rebuilder<0, InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, size_t Index, typename ToInsert>
using insert_at_t = typename insert_at<Typelist, Index, ToInsert>::type;

template <class Typelist, typename ToPrepend>
using prepend_t = insert_at_t<Typelist, 0, ToPrepend>;

template <class Typelist, typename ToAppend>
using append_t = add_t<Typelist, ToAppend>;

/*-----------------------------------------------------------*/

template <class Typelist, size_t Index>
struct get;

template <template <typename...> class Typelist, size_t Index, typename... Args>
struct get<Typelist<Args...>, Index> final
{
private:
	static_assert (Index <= size_v<Typelist<Args...>>, "Search index in type list is out of bounds!");

	template <size_t SearchIndex, typename First, typename... Others>
	struct getter final
	{
		using type = std::conditional_t<
			SearchIndex == 0,
			First,
			typename getter<SearchIndex - 1, Others...>::type>;
	};

	template <size_t SearchIndex, typename First>
	struct getter<SearchIndex, First> final
	{
		using type = std::conditional_t<
			SearchIndex == 0,
			First,
			meta::NullType>;
	};

public:
	using type = typename getter<Index, Args...>::type;
};

template <class Typelist, size_t Index>
using get_t = typename get<Typelist, Index>::type;

template <class Typelist>
using get_first_t = get_t<Typelist, 0>;

template <class Typelist>
using get_last_t = get_t<Typelist, size_v<Typelist> - 1>;

/*-----------------------------------------------------------*/

template <class Typelist, typename TypeToFind>
struct find;

template <template <typename...> class Typelist, typename TypeToFind, typename... Args>
struct find<Typelist<Args...>, TypeToFind> final
{
private:
	static_assert (contains_v<Typelist<Args...>, TypeToFind>,
				   "TypeToFind is not in the Typelist!");

	template <size_t Index, typename... Types>
	struct finder;

	template <size_t Index, typename First, typename... Others>
	struct finder<Index, First, Others...> final : SizeConstant<std::conditional_t<std::is_same_v<First, TypeToFind>,
																				   std::integral_constant<size_t, Index>,
																				   finder<Index + 1, Others...>>::value>
	{
	};

	template <size_t Index, typename First>
	struct finder<Index, First> final : SizeConstant<Index>
	{
	};

public:
	static constexpr const size_t value = finder<0, Args...>::value;
};

template <class Typelist, typename TypeToFind>
static constexpr const size_t find_v = find<Typelist, TypeToFind>::value;

/*-----------------------------------------------------------*/

template <class Typelist, typename... TypesToRemove>
struct remove;

template <template <typename...> class Typelist, typename... TypesToRemove, typename... Args>
struct remove<Typelist<Args...>, TypesToRemove...>
{
private:
	template <typename Type>
	static constinit const bool isTypeAbsent = (! std::is_same_v<TypesToRemove, Type> && ...);

	template <class ListType, typename Type>
	using remove_one = std::conditional_t<isTypeAbsent<Type>, add_t<ListType, Type>, ListType>;

	template <class ListType, typename... ListArgs>
	struct remover;

	template <class ListType, typename First, typename... Others>
	struct remover<ListType, First, Others...> final
	{
		using type = typename remover<
			remove_one<ListType, First>,
			Others...>::type;
	};

	template <class ListType, typename First>
	struct remover<ListType, First>
	{
		using type = remove_one<ListType, First>;
	};

public:
	using type = typename remover<InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, typename... TypesToRemove>
using remove_t = typename remove<Typelist, TypesToRemove...>::type;

template <class Typelist>
using remove_null_types_t = remove_t<Typelist, meta::NullType>;

/*-----------------------------------------------------------*/

template <class Typelist, size_t Index>
struct remove_at;

template <template <typename...> class Typelist, size_t Index, typename... Args>
struct remove_at<Typelist<Args...>, Index> final
{
private:
	static_assert (Index <= size_v<Typelist<Args...>>, "Cannot remove item out of range of TypeList!");

	template <size_t CurrentIndex, class ListType, typename Type>
	using do_remove = std::conditional_t<CurrentIndex == Index, ListType, add_t<ListType, Type>>;

	template <size_t CurrentIndex, class ListType, typename... ListArgs>
	struct remover;

	template <size_t CurrentIndex, class ListType, typename First, typename... Others>
	struct remover<CurrentIndex, ListType, First, Others...> final
	{
		using type = typename remover<
			CurrentIndex + 1,
			do_remove<CurrentIndex, ListType, First>,
			Others...>::type;
	};

	template <size_t CurrentIndex, class ListType, typename First>
	struct remover<CurrentIndex, ListType, First>
	{
		using type = do_remove<CurrentIndex, ListType, First>;
	};

public:
	using type = typename remover<0, InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, size_t Index>
using remove_at_t = typename remove_at<Typelist, Index>::type;

/*-----------------------------------------------------------*/

template <class Typelist, template <typename> class UnaryPredicate>
struct remove_if;

template <template <typename...> class Typelist, template <typename> class UnaryPredicate, typename... Args>
struct remove_if<Typelist<Args...>, UnaryPredicate> final
{
private:
	using Condition = TypeErasedUnaryPredicate<UnaryPredicate>;

	template <class TypeList, typename Type>
	using conditional_add = std::conditional_t<Condition::template met<Type>,
											   TypeList,
											   add_t<TypeList, Type>>;

	template <class TypeList, typename...>
	struct rebuilder;

	template <class TypeList, typename First, typename... Others>
	struct rebuilder<TypeList, First, Others...> final
	{
		using type = typename rebuilder<
			conditional_add<TypeList, First>,
			Others...>::type;
	};

	template <class TypeList, typename First>
	struct rebuilder<TypeList, First> final
	{
		using type = conditional_add<TypeList, First>;
	};

public:
	using type = typename rebuilder<InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, template <typename> class UnaryPredicate>
using remove_if_t = typename remove_if<Typelist, UnaryPredicate>::type;

/*-----------------------------------------------------------*/

template <class Typelist, template <typename> class UnaryPredicate>
struct remove_if_not;

template <template <typename...> class Typelist, template <typename> class UnaryPredicate, typename... Args>
struct remove_if_not<Typelist<Args...>, UnaryPredicate> final
{
private:
	using Condition = TypeErasedUnaryPredicate<UnaryPredicate>;

	template <class TypeList, typename Type>
	using conditional_add = std::conditional_t<! Condition::template met<Type>,
											   TypeList,
											   add_t<TypeList, Type>>;

	template <class TypeList, typename...>
	struct rebuilder;

	template <class TypeList, typename First, typename... Others>
	struct rebuilder<TypeList, First, Others...> final
	{
		using type = typename rebuilder<
			conditional_add<TypeList, First>,
			Others...>::type;
	};

	template <class TypeList, typename First>
	struct rebuilder<TypeList, First> final
	{
		using type = conditional_add<TypeList, First>;
	};

public:
	using type = typename rebuilder<InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, template <typename> class UnaryPredicate>
using remove_if_not_t = typename remove_if_not<Typelist, UnaryPredicate>::type;

/*-----------------------------------------------------------*/

template <class Typelist, typename ToReplace, typename ReplaceWith>
struct replace;

template <template <typename...> class Typelist, typename ToReplace, typename ReplaceWith, typename... Args>
struct replace<Typelist<Args...>, ToReplace, ReplaceWith> final
{
private:
	static_assert (contains_v<Typelist<Args...>, ToReplace>, "Cannot replace type not in Typelist!");

	template <class TypeList, typename First, typename InPlaceType>
	using do_replace = add_t<TypeList, std::conditional_t<std::is_same_v<First, ToReplace>, InPlaceType, First>>;

	template <class TypeList, typename InPlaceType, typename...>
	struct replacer;

	template <class TypeList, typename InPlaceType, typename First>
	struct replacer<TypeList, InPlaceType, First> final
	{
		using type = do_replace<TypeList, First, InPlaceType>;
	};

	template <class TypeList, typename InPlaceType, typename First, typename... Others>
	struct replacer<TypeList, InPlaceType, First, Others...> final
	{
		using type = typename replacer<
			do_replace<TypeList, First, InPlaceType>,
			InPlaceType,
			Others...>::type;
	};

public:
	using type = typename replacer<InternalTypeList<>, ReplaceWith, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, typename ToReplace, typename ReplaceWith>
using replace_t = typename replace<Typelist, ToReplace, ReplaceWith>::type;

/*-----------------------------------------------------------*/

template <class Typelist, size_t Index, typename ReplaceWith>
struct replace_at;

template <template <typename...> class Typelist, size_t Index, typename ReplaceWith, typename... Args>
struct replace_at<Typelist<Args...>, Index, ReplaceWith> final
{
private:
	static_assert (Index <= size_v<Typelist<Args...>>, "Cannot replace item out of range of TypeList!");

	template <size_t CurrentIndex, class ListType, typename First>
	using do_replace = std::conditional_t<CurrentIndex == Index, add_t<ListType, ReplaceWith>, add_t<ListType, First>>;

	template <size_t CurrentIndex, class ListType, typename... ListArgs>
	struct rebuilder;

	template <size_t CurrentIndex, class ListType, typename First, typename... Others>
	struct rebuilder<CurrentIndex, ListType, First, Others...> final
	{
		using type = typename rebuilder<
			CurrentIndex + 1,
			do_replace<CurrentIndex, ListType, First>,
			Others...>::type;
	};

	template <size_t CurrentIndex, class ListType, typename First>
	struct rebuilder<CurrentIndex, ListType, First>
	{
		using type = do_replace<CurrentIndex, ListType, First>;
	};

public:
	using type = typename rebuilder<0, InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, size_t Index, typename ReplaceWith>
using replace_at_t = typename replace_at<Typelist, Index, ReplaceWith>::type;

/*-----------------------------------------------------------*/

template <class Typelist, size_t Index1, size_t Index2>
struct swap_at;

template <template <typename...> class Typelist, size_t Index1, size_t Index2, typename... Args>
struct swap_at<Typelist<Args...>, Index1, Index2> final
{
private:
	static_assert (Index1 != Index2, "Cannot swap elements at the same index!");

	using OriginalList = Typelist<Args...>;

	static_assert (Index1 <= size_v<OriginalList>, "Swap index out of range of TypeList!");
	static_assert (Index2 <= size_v<OriginalList>, "Swap index out of range of TypeList!");

	template <size_t CurrentIndex, class ListType, typename First>
	using do_swap = std::conditional_t<CurrentIndex == Index1,
									   add_t<ListType, get_t<OriginalList, Index2>>,
									   std::conditional_t<CurrentIndex == Index2,
														  add_t<ListType, get_t<OriginalList, Index1>>,
														  add_t<ListType, First>>>;

	template <size_t CurrentIndex, class ListType, typename...>
	struct rebuilder;

	template <size_t CurrentIndex, class ListType, typename First, typename... Others>
	struct rebuilder<CurrentIndex, ListType, First, Others...> final
	{
		using type = typename rebuilder<
			CurrentIndex + 1,
			do_swap<CurrentIndex, ListType, First>,
			Others...>::type;
	};

	template <size_t CurrentIndex, class ListType, typename First>
	struct rebuilder<CurrentIndex, ListType, First> final
	{
		using type = do_swap<CurrentIndex, ListType, First>;
	};

public:
	using type = typename rebuilder<0, InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist, size_t Index1, size_t Index2>
using swap_at_t = typename swap_at<Typelist, Index1, Index2>::type;

/*-----------------------------------------------------------*/

template <class Typelist, typename Type1, typename Type2>
struct swap;

template <template <typename...> class Typelist, typename Type1, typename Type2, typename... Args>
struct swap<Typelist<Args...>, Type1, Type2> final
{
private:
	static_assert (! std::is_same_v<Type1, Type2>, "Type1 and Type2 need to be different types");

	using OriginalList = Typelist<Args...>;

	static_assert (contains_v<OriginalList, Type1>, "Cannot swap types not in Typelist!");
	static_assert (contains_v<OriginalList, Type2>, "Cannot swap types not in Typelist!");

public:
	using type = swap_at_t<OriginalList,
						   find_v<OriginalList, Type1>,
						   find_v<OriginalList, Type2>>;
};

template <class Typelist, typename Type1, typename Type2>
using swap_t = typename swap<Typelist, Type1, Type2>::type;

/*-----------------------------------------------------------*/

template <class Typelist, typename ToCount>
struct count;

template <template <typename...> class Typelist, typename ToCount, typename... Args>
struct count<Typelist<Args...>, ToCount> final
{
private:
	template <size_t Count, size_t CurrentIndex>
	struct counter final
	{
	private:
		static constexpr const size_t thisValue = std::conditional_t<
			std::is_same_v<ToCount, get_t<Typelist<Args...>, CurrentIndex>>,
			SizeConstant<Count + 1>,
			SizeConstant<Count>>::value;

	public:
		static constexpr const size_t value = std::conditional_t<
			CurrentIndex == size_v<Typelist<Args...>>,
			SizeConstant<thisValue>,
			counter<thisValue, CurrentIndex + 1>>::value;
	};

public:
	static constexpr const size_t value = counter<0, 0>::value;
};

template <class Typelist, typename ToCount>
static constexpr const size_t count_v = count<Typelist, ToCount>::value;

/*-----------------------------------------------------------*/

template <class Typelist, template <typename> class UnaryPredicate>
struct count_if;

template <template <typename...> class Typelist, template <typename> class UnaryPredicate, typename... Args>
struct count_if<Typelist<Args...>, UnaryPredicate> final
{
private:
	using Condition = TypeErasedUnaryPredicate<UnaryPredicate>;

	template <size_t Count, size_t CurrentIndex>
	struct counter final
	{
	private:
		using thisValue = std::conditional_t<
			Condition::template met<get_t<Typelist<Args...>, CurrentIndex>>,
			SizeConstant<Count + 1>,
			SizeConstant<Count>>;

	public:
		static constexpr const size_t value = std::conditional_t<  
			CurrentIndex == size_v<Typelist<Args...>>,
			thisValue,
			counter<thisValue::value, CurrentIndex + 1>>::value;
	};

public:
	static constexpr const size_t value = counter<0, 0>::value;
};

template <class Typelist, template <typename> class UnaryPredicate>
static constexpr const size_t count_if_v = count_if<Typelist, UnaryPredicate>::value;

template <class Typelist, template <typename> class UnaryPredicate>
struct count_if_not : SizeConstant<(size_v<Typelist> - count_if_v<Typelist, UnaryPredicate>)>
{
};

template <class Typelist, template <typename> class UnaryPredicate>
static constexpr const size_t count_if_not_v = count_if_not<Typelist, UnaryPredicate>::value;

/*-----------------------------------------------------------*/

template <class Typelist>
struct remove_duplicates;

template <template <typename...> class Typelist, typename... Args>
struct remove_duplicates<Typelist<Args...>> final
{
private:
	template <size_t CurrentIndex>
	using CurrentType = get_t<Typelist<Args...>, CurrentIndex>;

	template <size_t CurrentIndex, class ListType>
	using remover = std::conditional_t<contains_v<ListType, CurrentType<CurrentIndex>>,
									   ListType,
									   add_t<ListType, CurrentType<CurrentIndex>>>;

	template <size_t CurrentIndex, class ListType, typename...>
	struct rebuilder;

	template <size_t CurrentIndex, class ListType, typename First, typename... Others>
	struct rebuilder<CurrentIndex, ListType, First, Others...> final
	{
		using type = typename rebuilder<
			CurrentIndex + 1,
			remover<CurrentIndex, ListType>,
			Others...>::type;
	};

	template <size_t CurrentIndex, class ListType, typename First>
	struct rebuilder<CurrentIndex, ListType, First> final
	{
		using type = remover<CurrentIndex, ListType>;
	};

public:
	using type = typename rebuilder<0, InternalTypeList<>, Args...>::type::template makeTypelist<Typelist>;
};

template <class Typelist>
using remove_duplicates_t = typename remove_duplicates<Typelist>::type;

template <class Typelist>
struct contains_duplicates;

template <template <typename...> class Typelist, typename... Args>
struct contains_duplicates<Typelist<Args...>> final : std::bool_constant<
														(size_v<Typelist<Args...>> != size_v<remove_duplicates_t<Typelist<Args...>>>)>
{
};

template <class Typelist>
static constexpr const bool contains_duplicates_v = contains_duplicates<Typelist>::value;

/*-----------------------------------------------------------*/

template <class Typelist>
struct reverse;

template <template <typename...> class Typelist, typename... Args>
struct reverse<Typelist<Args...>> final
{
private:
	template <typename>
	struct base;

	template <template <typename...> class T, typename... Types>
	struct base<T<Types...>> final
	{
		using type = T<>;
	};

	template <typename T, typename = typename base<T>::type>
	struct reverse_impl;

	template <template <typename...> class T, typename... Types>
	struct reverse_impl<typename base<T<Types...>>::type, T<Types...>> final
	{
		using type = T<Types...>;
	};

	template <template <typename...> class T, typename First, typename... Rest, typename... done>
	struct reverse_impl<T<First, Rest...>, T<done...>> final
	{
		using type = typename reverse_impl<T<Rest...>, T<First, done...>>::type;
	};

public:
	using type = typename reverse_impl<InternalTypeList<Args...>>::type::template makeTypelist<Typelist>;
};

template <class Typelist>
using reverse_t = typename reverse<Typelist>::type;

/// @endcond
} // namespace impl

/*----------------------------------------------------------------------------------------------------------------------*/

#pragma mark Typelist

/**
	A compile-time list of types that can be manipulated.

	Example usage:
	@code{.cpp}
	using MyTypeList = TypeList<int, float, double>;

	static_assert (MyTypeList::size == 3);
	static_assert (MyTypeList::num_of<int> == 1);

	using FloatingPointTypes = MyTypeList::remove_if_not<std::is_floating_point>;

	static_assert (FloatingPointTypes::equal<TypeList<float, double>>);
	@endcode

	Most operations on TypeLists are done with using statements, as illustrated above. Once declared, 
	a %TypeList is immutable; it can only create permutations of itself as new specializations of %TypeList.

	@tparam Types The list of types for the %TypeList to hold.
	@ingroup meta
	@test This class's API is extensively tested with static assertions at compile-time.

	@todo get most/least derived type
	@todo sort types
	@todo function to get a string containing names of all types?
 */
template <typename... Types>
class TypeList final
{
public:
	/** You can't actually create an instance of one, %TypeLists are only static types. */
	TypeList() = delete;

	/** You can get the fully specialized type of this %TypeList using this alias.

		For example:
		@code
		using MyTypeList = TypeList<int>;

		static_assert (std::is_same_v<MyTypeList::type_id, TypeList<int>>);
		@endcode
	 */
	using type_id = TypeList<Types...>;

	/** Creates an empty %TypeList. */
	using clear = TypeList<>;

	/** @name Adding types */
	///@{

	/** The type of a new %TypeList with the given types appended to the end.

		For an empty TypeList, this is equivalent to \c TypeList<TypesToAdd...> .

		@tparam TypesToAdd Types to add to the end of this %TypeList.
	 */
	template <typename... TypesToAdd>
	using add = impl::add_t<type_id, TypesToAdd...>;

	/** The type of a new %TypeList with types added from another %TypeList.

		For an empty TypeList, this is equivalent to \c Other .

		@tparam Other Another %TypeList to add types from. This should be a fully specialized specialization of TypeList.
	 */
	template <class Other>
	using add_from = impl::add_from_t<type_id, Other>;

	/** For each type in the passed list of types, adds it to the %TypeList if it was not already in the list.

		For an empty TypeList, this is equivalent to \c TypeList<TypesToAdd...>::remove_duplicates .

		@tparam TypesToAdd List of types to add to the %TypeList
	 */
	template <typename... TypesToAdd>
	using add_if_absent = impl::addIfAbsent_t<type_id, TypesToAdd...>;

	/** Inserts a type in the %TypeList at the given \c Index . Types after the given index in the list will be pushed back by 1.

		For an empty TypeList, this is equivalent to \c TypeList<ToInsert> .

		@tparam Index The index in the %TypeList to insert the new type at. Indices start from 0.
		@tparam ToInsert The type to insert in the %TypeList.
	 */
	template <size_t Index, typename ToInsert>
	using insert_at = impl::insert_at_t<type_id, Index, ToInsert>;

	/** Prepends a type to the beginning of the %TypeList.

		For an empty TypeList, this is equivalent to \c TypeList<ToPrepend> .

		@tparam ToPrepend Type to prepend to the list.
	 */
	template <typename ToPrepend>
	using prepend = impl::prepend_t<type_id, ToPrepend>;

	/** Appends a type to the end of the %TypeList.

		This is equivalent to \c add<ToPrepend> , but is provided so that you can be more explicit.

		For an empty TypeList, this is equivalent to \c TypeList<ToPrepend> .

		@tparam ToAppend Type to append to the list.
	 */
	template <typename ToAppend>
	using append = impl::append_t<type_id, ToAppend>;

	///@}

	/** @name Accessing types */
	///@{

	/** Gets the type of an element of the list at the specified index.

		For an empty TypeList, this evaluates to \c NullType .

		@tparam Index The index in the list to retrieve. A compile-time error will be raised if this index is out of bounds for this %TypeList, unless the list is empty -- if the list is empty, this will always evaluate to NullType.
	 */
	template <size_t Index>
	using at = impl::get_t<type_id, Index>;

	/** The type of the first element in the %TypeList.
		This is semantically the same as \c at<0> .

		For an empty TypeList, this evaluates to \c NullType .
	 */
	using front = impl::get_first_t<type_id>;

	/** The type of the last element in the %TypeList.
		This is semantically the same as \c at<size-1> .

		For an empty TypeList, this evaluates to \c NullType .
	 */
	using back = impl::get_last_t<type_id>;

	/** The index of the first occurrence of the specified type in the %TypeList.

		A compile-time error will be raised if the specified type is not in the %TypeList, unless the list is empty -- if the list is empty, this will always evaluate to \c static_cast<size_t>(-1) , regardless of what type was specified.

		For an empty TypeList, this will evaluate to \c invalid_index .

		@tparam Type The type to find in the %TypeList.
	 */
	template <typename Type>
	static constexpr const size_t index_of = impl::find_v<type_id, Type>;

	///@}

	/** @name Counting types */
	///@{

	/** The total size of the %TypeList. */
	static constexpr const size_t size = impl::size_v<type_id>;

	/** True if the %TypeList is empty; false otherwise. */
	static constexpr const bool empty = impl::is_empty<type_id>;

	/** The number of times the given type appears in this %TypeList.
		@tparam Type The type to count occurrences of in this %TypeList.
		@see count_if, contains
	 */
	template <typename Type>
	static constexpr const size_t num_of = impl::count_v<type_id, Type>;

	/** The number of types in the %TypeList that satisfy the predicate.
		@tparam UnaryPredicate A template class that will be instantiated for each type in the %TypeList and must provide a compile-time call operator that will determine if the predicate is met for the type it was instantiated with. Examples of unary predicates are \c std::is_floating_point or \c std::is_enum .
		@see num_of, count_if_not
	 */
	template <template <typename> class UnaryPredicate>
	static constexpr const size_t count_if = impl::count_if_v<type_id, UnaryPredicate>;

	/** The number of types in the %TypeList that do not satisfy the predicate.
		@tparam UnaryPredicate A template class that will be instantiated for each type in the %TypeList and must provide a compile-time call operator that will determine if the predicate is met for the type it was instantiated with. Examples of unary predicates are \c std::is_floating_point or \c std::is_enum .
		@see count_if
	 */
	template <template <typename> class UnaryPredicate>
	static constexpr const size_t count_if_not = impl::count_if_not_v<type_id, UnaryPredicate>;

	/** True if all the passed types are contained in this %TypeList.
		@tparam TypesToFind List of types to check if this %TypeList contains.
		@see num_of, contains_or
	 */
	template <typename... TypesToFind>
	static constexpr const bool contains = impl::contains_v<type_id, TypesToFind...>;

	/** True if this %TypeList contains at least one of the passed types.
		@tparam TypesToFind List of types to check if this %TypeList contains.
		@see contains
	 */
	template <typename... TypesToFind>
	static constexpr const bool contains_or = impl::contains_or_v<type_id, TypesToFind...>;

	/** True if any type appears in the list more than once. */
	static constexpr const bool contains_duplicates = impl::contains_duplicates_v<type_id>;

	///@}

	////** @name Removing types */
	///@{

	/** The type of a new %TypeList with all occurrences of the passed types removed.

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).

	 @tparam TypesToRemove List of types to remove from the list.
	 */
	template <typename... TypesToRemove>
	using remove = impl::remove_t<type_id, TypesToRemove...>;

	/** If any \c NullType elements are present in this %TypeList, removes them.
	 This is semantically the same as \c remove<NullType> .

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).
	 */
	using remove_null_types = impl::remove_null_types_t<type_id>;

	/** The type of a new %TypeList with the type at the specified index removed.
	 All types after \c Index in the list will be moved forward by 1.
	 A compile-time error will be raised if the \c Index is out of range for this %TypeList.

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).

	 @tparam Index Index to remove from the list.
	 */
	template <size_t Index>
	using remove_at = impl::remove_at_t<type_id, Index>;

	/** Removes the first element of the %TypeList.
	 This is semantically the same as \c remove_at<0> .

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).
	 */
	using remove_first = remove_at<0>;

	/** Removes the last element of the %TypeList.
	 This is semantically the same as \c remove_at<size-1> .

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).
	 */
	using remove_last = remove_at<size - 1>;

	/** Evaluates the predicate for each type in the list, and removes it if the predicate is met.

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).

	 @tparam UnaryPredicate A template class that will be instantiated for each type in the %TypeList and must provide a compile-time call operator that will determine if the predicate is met for the type it was instantiated with. Examples of unary predicates are \c std::is_floating_point or \c std::is_enum .

	 @see remove_if_not
	 */
	template <template <typename> class UnaryPredicate>
	using remove_if = impl::remove_if_t<type_id, UnaryPredicate>;

	/** Evaluates the predicate for each type in the list, and removes it if the predicate is not met.

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).

	 @tparam UnaryPredicate A template class that will be instantiated for each type in the %TypeList and must provide a compile-time call operator that will determine if the predicate is met for the type it was instantiated with. Examples of unary predicates are \c std::is_floating_point or \c std::is_enum .

	 @see remove_if
	 */
	template <template <typename> class UnaryPredicate>
	using remove_if_not = impl::remove_if_not_t<type_id, UnaryPredicate>;

	/** The type of a new %TypeList with all duplicates removed -- if a type appeared in the original list multiple times, it will appear in the new list exactly once.

	 For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).
	 */
	using remove_duplicates = impl::remove_duplicates_t<type_id>;

	///@}

	/** @name Swapping and replacing types */
	///@{

	/** Swaps the types at two indices in the list.
		A compile-time error will be raised if \c Index1 and \c Index2 are the same.

		For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).

		@tparam Index1 The first index to be swapped.
		@tparam Index2 The second index to be swapped.

		@see swap
	 */
	template <size_t Index1, size_t Index2>
	using swap_at = impl::swap_at_t<type_id, Index1, Index2>;

	/** Swaps two types in the list.
		This finds the first instances of each type in the list, and uses \c swap_at to swap their indices.
		A compile-time error will be raised if \c Type1 and \c Type2 are the same type.

		For an empty TypeList, this is equivalent to \c TypeList<> (or \c TypeList<>::type_id ).

		@tparam Type1 The first type to be swapped.
		@tparam Type2 The second type to be swapped.

		@see swap_at
	 */
	template <typename Type1, typename Type2>
	using swap = impl::swap_t<type_id, Type1, Type2>;

	/** Replaces all occurrences of \c Replace in the list with \c With.
		It is not an error for there to be no occurrences of \c Replace in the list.

		For an empty TypeList, if \c Replace is \c NullType , this evaluates to \c TypeList<With> , otherwise, this evaluates to
		\c TypeList<> (or \c TypeList<>::type_id ).

		@tparam Replace The type to be replaced in the list. A compile-time error will be raised if the list does not contain this type.
		@tparam With The type to replace \c Replace with.

		@see replace_at
	 */
	template <typename Replace, typename With>
	using replace = impl::replace_t<type_id, Replace, With>;

	/** Replaces the type at the specified \c Index with the new type \c ReplaceWith.
		A compile-time error will be raised if the \c Index is out of range for this %TypeList.

		For an empty TypeList, this evaluates to \c TypeList<ReplaceWith> .

		@tparam Index The index of the element to be replaced in the list.
		@tparam ReplaceWith The new type to put at index \c Index in the list.

		@see replace
	 */
	template <size_t Index, typename ReplaceWith>
	using replace_at = impl::replace_at_t<type_id, Index, ReplaceWith>;

	///@}

	/** @name List comparisons */
	///@{

	/** True if the other %TypeList holds the same types in the same order as this one.
		@tparam Other Another TypeList to compare with.
		@see equal_ignore_order
	 */
	template <class Other>
	static constexpr const bool equal = impl::are_same_v<type_id, Other>;

	/** True if the other %TypeList holds the same types as this one, regardless of their order.
		@tparam Other Another TypeList to compare with.
		@see equal
	 */
	template <class Other>
	static constexpr const bool equal_ignore_order = impl::same_ignoring_order_v<type_id, Other>;

	/** The type of a new %TypeList that contains only types that were present in the original list and the passed list \c Other .

		For example:
		@code
		using TypeListA = TypeList<int, float, double>;

		using TypeListB = TypeList<int, double, size_t>;

		static_assert (TypeListA::common_with<TypeListB>::equal<TypeList<int, double>>);
		@endcode

		@tparam Other Another %TypeList to compare this one with.
		@see not_in
	 */
	template <class Other>
	using common_with = impl::common_t<type_id, Other>;

	/** The type of a new %TypeList that contains only types that were present in the original list and not in the passed list \c Other .

		For example:
		@code
		using TypeListA = TypeList<int, float, double>;

		using TypeListB = TypeList<int, double, size_t>;

		static_assert (TypeListA::not_in<TypeListB>::equal<TypeList<float>>);
		@endcode

		@tparam Other  Another %TypeList to compare this one with.
		@see common_with
	 */
	template <class Other>
	using not_in = impl::not_in_t<type_id, Other>;

	///@}

	/** The type of a new %TypeList with the same types as this one, but in reverse order. */
	using reverse = impl::reverse_t<type_id>;

	/** Applies the types in this %TypeList to a class template, as its template arguments.

		For example:
		@code
		using TypeListA = TypeList<int, size_t, double, float>;

		using ExpectedResult = std::variant<int, size_t, double, float>;

		static_assert (std::is_same_v <TypeListA::apply_to<std::variant>, ExpectedResult>);
		@endcode

		For an empty TypeList, this evaluates to \c T<> .

		@tparam T Template class to specialize using the types in this list.
	 */
	template <template <typename...> class T>
	using apply_to = T<Types...>;

	/** Constructs an object of the type at the given index in the list.
		If the TypeList is empty, this will return a \c NullType object.

		@tparam Index The index in the list of the type to be constructed. A compile-time error will be raised if this index is out of range for the list.
		@tparam Args Constructor arguments for the new object.

		@see make_unique()
	 */
	template <size_t Index, typename... Args>
	static constexpr at<Index> construct (Args&&... args) noexcept (noexcept (at<Index> (std::forward<Args> (args)...)))
	{
		return at<Index> (std::forward<Args> (args)...);
	}

	/** The same as \c construct() , except it returns a \c unique_ptr to the new object.

		@tparam Index The index in the list of the type to be constructed. A compile-time error will be raised if this index is out of range for the list.
		@tparam Args Constructor arguments for the new object.

		@see construct()
	 */
	template <size_t Index, typename... Args>
	static std::unique_ptr<at<Index>> make_unique (Args&&... args) noexcept (noexcept (at<Index> (std::forward<Args> (args)...)))
	{
		return std::make_unique<at<Index>> (std::forward<Args> (args)...);
	}

	/** Calls a templated function for each type in the list.
		If the list is empty, this function does nothing.

		@tparam Func Templated function to call.
		@tparam Args Arguments to forward to the function.

		@see for_all()
	 */
	template <Function Func, typename... Args>
	static constexpr void for_each (Func&& f, Args&&... args) noexcept (noexcept ((std::forward<Func> (f).template operator()<Types> (std::forward<Args> (args)...), ...)))
	{
		(std::forward<Func> (f).template operator()<Types> (std::forward<Args> (args)...), ...);
	}

	/** Calls a templated function once, with the template arguments being the types contained in the list.

		If the list is empty, the function is called with an empty template parameter list, such as \c f<>(); .

		@tparam Func Templated function to call using the types in the list.
		@tparam Args Arguments to forward to the function.

		@return The return type of the function.
		@see for_each()
	 */
	template <Function Func, typename... Args>
	static constexpr auto for_all (Func&& f, Args&&... args) noexcept (noexcept (f().template operator()<Types...> (std::forward<Args> (args)...)))
		-> func_result_type<Func, Args...>
	{
		return f().template operator()<Types...> (std::forward<Args> (args)...);
	}
};

/*----------------------------------------------------------------------------------------------------------------------*/

#pragma mark Empty TypeList specialization

/** The empty specialization of TypeList requires some special implementation. 
	@ingroup typelist
 */
template <>
class TypeList<> final
{
public:
	TypeList() = delete;

	using type_id = TypeList<>;
	using clear	  = type_id;

	static constinit const size_t size = 0;

	static constinit const bool empty = true;

	template <class Other>
	static constexpr const bool equal = impl::is_empty<Other>;

	template <class Other>
	static constexpr const bool equal_ignore_order = impl::is_empty<Other>;

	template <typename...>
	static constinit const bool contains = false;

	template <typename...>
	static constinit const bool contains_or = false;

	template <typename>
	static constinit const size_t num_of = 0;

	template <template <typename> class>
	static constinit const size_t count_if = 0;

	template <template <typename> class UnaryPredicate>
	static constinit const size_t count_if_not = 0;

	template <typename... TypesToAdd>
	using add = TypeList<TypesToAdd...>;

	template <class Other>
	using add_from = Other;

	template <typename... TypesToAdd>
	using add_if_absent = typename TypeList<TypesToAdd...>::remove_duplicates;

	template <size_t, typename ToInsert>
	using insert_at = TypeList<ToInsert>;

	template <typename ToPrepend>
	using prepend = TypeList<ToPrepend>;

	template <typename ToAppend>
	using append = TypeList<ToAppend>;

	template <size_t, size_t>
	using swap_at = type_id;

	template <typename, typename>
	using swap = type_id;

	template <size_t>
	using at = NullType;

	using front = NullType;
	using back	= NullType;

	using reverse = type_id;

	template <typename>
	static constinit const size_t index_of = impl::invalid_index;

	template <typename...>
	using remove = type_id;

	template <size_t>
	using remove_at = type_id;

	using remove_first = type_id;
	using remove_last  = type_id;

	using remove_null_types = type_id;

	template <template <typename> class>
	using remove_if = type_id;

	template <template <typename> class>
	using remove_if_not = type_id;

	template <typename Replace, typename With>
	using replace = std::conditional_t<std::is_same_v<Replace, NullType>, TypeList<With>, type_id>;

	template <size_t, typename With>
	using replace_at = TypeList<With>;

	static constinit const bool contains_duplicates = false;

	using remove_duplicates = type_id;

	template <class>
	using common_with = type_id;

	template <class Other>
	using not_in = type_id;

	template <template <typename...> class T>
	using apply_to = T<>;

	template <size_t, typename... Args>
	static constexpr NullType construct (Args&&...) noexcept
	{
		return {};
	}

	template <size_t, typename... Args>
	static std::unique_ptr<NullType> make_unique (Args&&...) noexcept
	{
		return std::make_unique<NullType>();
	}

	template <Function Func, typename... Args>
	static constexpr void for_each (Func&&, Args&&...) noexcept
	{
	}

	template <Function Func, typename... Args>
	static constexpr auto for_all (Func&& f, Args&&... args) noexcept (noexcept (f().template operator()<> (std::forward<Args> (args)...)))
		-> func_result_type<Func, Args...>
	{
		return f().template operator()<> (std::forward<Args> (args)...);
	}
};

/** A utility typedef for an empty TypeList.
	@ingroup typelist
 */
using Empty = TypeList<>;

/*----------------------------------------------------------------------------------------------------------------------*/

#pragma mark Misc utilities

namespace impl {
/// @cond
template <typename>
struct make_type_list_from
{
	using type = TypeList<>;
};

template <template <typename...> class T, typename... Args>
struct make_type_list_from<T<Args...>> final
{
	using type = TypeList<Args...>;
};
/// @endcond
}

/** Creates a typelist from the template arguments of a template type.
	For example:
	@code
	using OriginalType = std::variant<int, float, double>;

	using TypeList = make_type_list_from_t<OriginalType>;

	static_assert (TypeList::equal<TypeList<int, float, double>>);
	@endcode
	@ingroup typelist
 */
template <typename... Args>
using make_type_list_from_t = typename impl::make_type_list_from<Args...>::type;

namespace impl {
/// @cond
template <class T, template <class...> class Template>
struct is_specialization final : std::false_type
{
};

template <template <class...> class Template, class... Args>
struct is_specialization<Template<Args...>, Template> final : std::true_type
{
};
/// @endcond
}

/** True if the specified type is a specialization of TypeList.
	@ingroup typelist
 */
template <class T>
static constinit const bool is_typelist = impl::is_specialization<T, TypeList>::value;

/** @concept IsTypeList
	@see is_typelist
	@ingroup typelist
 */
template <typename T>
concept IsTypeList = is_typelist<T>;

}
