/*
 * QwParitySchemaRow.h
 *
 *  Created on: Sept 19, 2025
 *      Author: wdconinc
 */

#ifndef QWPARITYSCHEMAROW_H_
#define QWPARITYSCHEMAROW_H_

/*
 * QwParitySchemaRow.h
 *
 * Template-based row structures that automatically generate from sqlpp11 schema definitions.
 * This eliminates the need to manually duplicate field names and types while providing
 * clean, type-safe access using the schema column definitions.
 *
 * Usage:
 *   QwParitySchema::beam_optics table;
 *   QwParitySchema::beam_optics_row row;
 *   
 *   // Clean syntax using schema columns
 *   row[table.analysis_id] = analysis_id_value;
 *   row[table.monitor_id] = monitor_id_value;
 *   
 *   // Or method-based access
 *   row.set(table.amplitude, amplitude_value);
 *   auto amplitude = row.get(table.amplitude);
 *   
 *   // Generate insert query automatically
 *   auto query = row.insert_query();
 */

// System headers
#include <tuple>
#include <type_traits>
#include <utility>

// Qweak headers
#ifdef __USE_DATABASE__
#include "QwParitySchema.h"
#include <sqlpp11/sqlpp11.h>
#endif // __USE_DATABASE__

#ifdef __USE_DATABASE__

namespace QwParitySchema {

namespace detail {
    // Helper to extract C++ value types from sqlpp11 column types
    template<typename Column>
    using column_value_t = sqlpp::cpp_value_type_of<typename Column::_traits::_value_type>;
    
    // Extract all column value types into a tuple
    template<typename Tuple>
    struct extract_value_types;
    
    template<typename... Columns>
    struct extract_value_types<std::tuple<Columns...>> {
        using type = std::tuple<column_value_t<Columns>...>;
    };
    
    // Recursive helper to find column index - base case
    template<typename Target>
    constexpr std::size_t find_index_impl() {
        static_assert(sizeof(Target) == 0, "Column type not found in table");
        return 0; // This should never be reached due to static_assert
    }
    
    // Recursive helper to find column index - recursive case
    template<typename Target, typename First, typename... Rest>
    constexpr std::size_t find_index_impl() {
        if constexpr (std::is_same_v<Target, First>) {
            return 0;
        } else {
            return 1 + find_index_impl<Target, Rest...>();
        }
    }
    
    // Helper to find the index of a specific column type in the tuple
    template<typename TargetColumn, typename Tuple>
    struct column_index;
    
    template<typename TargetColumn, typename... Columns>
    struct column_index<TargetColumn, std::tuple<Columns...>> {
        static constexpr std::size_t value = find_index_impl<TargetColumn, Columns...>();
    };
} // namespace detail

/**
 * @brief Template-based row generator that automatically extracts column information
 *        from sqlpp11 table definitions.
 * 
 * This class provides a type-safe, schema-synchronized way to create row structures
 * without manually duplicating field names. It uses template metaprogramming to
 * extract column types and provides clean access syntax using the schema column
 * definitions.
 * 
 * @tparam Table The sqlpp11 table type (e.g., QwParitySchema::beam_optics)
 */
template<typename Table>
class row {
private:
    // Extract the column tuple type from the table
    using column_tuple_t = typename Table::_column_tuple_t;
    using values_tuple_t = typename detail::extract_value_types<column_tuple_t>::type;
    
public:
    using table_type = Table;
    
    /**
     * @brief Storage for all column values as a tuple
     * 
     * The tuple contains one element for each column in the table,
     * with types matching the sqlpp11 column value types.
     */
    values_tuple_t values;
    
    /**
     * @brief Default constructor
     */
    row() = default;
    
    /**
     * @brief Copy constructor
     */
    row(const row&) = default;
    
    /**
     * @brief Move constructor
     */
    row(row&&) = default;
    
    /**
     * @brief Copy assignment operator
     */
    row& operator=(const row&) = default;
    
    /**
     * @brief Move assignment operator
     */
    row& operator=(row&&) = default;
    
    /**
     * @brief Destructor
     */
    ~row() = default;
    
    /**
     * @brief Set a column value using the column specification type
     * 
     * @tparam ColumnSpec The column specification type from the schema
     * @tparam T The value type
     * @param value The value to set
     */
    template<typename ColumnSpec, typename T>
    void set(T&& value) {
        using column_t = sqlpp::column_t<Table, ColumnSpec>;
        constexpr auto idx = detail::column_index<column_t, column_tuple_t>::value;
        std::get<idx>(values) = std::forward<T>(value);
    }
    
    /**
     * @brief Get a column value using the column specification type
     * 
     * @tparam ColumnSpec The column specification type from the schema
     * @return const reference to the column value
     */
    template<typename ColumnSpec>
    const auto& get() const {
        using column_t = sqlpp::column_t<Table, ColumnSpec>;
        constexpr auto idx = detail::column_index<column_t, column_tuple_t>::value;
        return std::get<idx>(values);
    }
    
    /**
     * @brief Get a mutable column value using the column specification type
     * 
     * @tparam ColumnSpec The column specification type from the schema
     * @return mutable reference to the column value
     */
    template<typename ColumnSpec>
    auto& get() {
        using column_t = sqlpp::column_t<Table, ColumnSpec>;
        constexpr auto idx = detail::column_index<column_t, column_tuple_t>::value;
        return std::get<idx>(values);
    }
    
    /**
     * @brief Set a column value using a table column instance (auto-deducing)
     * 
     * @tparam Column The column type (auto-deduced from parameter)
     * @tparam T The value type
     * @param column The table column instance
     * @param value The value to set
     */
    template<typename Table2, typename ColumnSpec, typename T>
    void set(const sqlpp::column_t<Table2, ColumnSpec>& /*column*/, T&& value) {
        static_assert(std::is_same_v<Table, Table2>, "Column must be from the same table");
        set<ColumnSpec>(std::forward<T>(value));
    }
    
    /**
     * @brief Get a column value using a table column instance (auto-deducing)
     * 
     * @tparam Column The column type (auto-deduced from parameter)
     * @param column The table column instance
     * @return const reference to the column value
     */
    template<typename Table2, typename ColumnSpec>
    const auto& get(const sqlpp::column_t<Table2, ColumnSpec>& /*column*/) const {
        static_assert(std::is_same_v<Table, Table2>, "Column must be from the same table");
        return get<ColumnSpec>();
    }
    
    /**
     * @brief Get a mutable column value using a table column instance (auto-deducing)
     * 
     * @tparam Column The column type (auto-deduced from parameter)
     * @param column The table column instance
     * @return mutable reference to the column value
     */
    template<typename Table2, typename ColumnSpec>
    auto& get(const sqlpp::column_t<Table2, ColumnSpec>& /*column*/) {
        static_assert(std::is_same_v<Table, Table2>, "Column must be from the same table");
        return get<ColumnSpec>();
    }
    
    /**
     * @brief Array-style access operator for setting/getting column values
     * 
     * @tparam Column The column type (auto-deduced from parameter)
     * @param column The table column instance
     * @return mutable reference to the column value
     */
    template<typename Table2, typename ColumnSpec>
    auto& operator[](const sqlpp::column_t<Table2, ColumnSpec>& column) {
        return get(column);
    }
    
    /**
     * @brief Const array-style access operator for getting column values
     * 
     * @tparam Column The column type (auto-deduced from parameter)
     * @param column The table column instance
     * @return const reference to the column value
     */
    template<typename Table2, typename ColumnSpec>
    const auto& operator[](const sqlpp::column_t<Table2, ColumnSpec>& column) const {
        return get(column);
    }
    
    /**
     * @brief Generate an sqlpp11 insert query from the row data
     * 
     * This method automatically maps all row values to their corresponding
     * table columns and creates a properly typed insert statement.
     * 
     * @return sqlpp11 insert query object
     */
    auto insert_into() const {
        Table table;
        return generate_insert_impl(table, std::make_index_sequence<std::tuple_size_v<values_tuple_t>>{});
    }
    
    /**
     * @brief Reset all column values to their default-constructed state
     */
    void reset() {
        values = values_tuple_t{};
    }
    
    /**
     * @brief Get the number of columns in this row
     * 
     * @return number of columns
     */
    static constexpr std::size_t column_count() {
        return std::tuple_size_v<values_tuple_t>;
    }
    
private:
    /**
     * @brief Implementation helper for generating insert queries
     * 
     * Uses index sequences to map tuple elements to table columns.
     * 
     * @tparam Is Index sequence for the tuple elements
     * @param table The table instance
     * @param Index sequence (unused parameter)
     * @return sqlpp11 insert query
     */
    template<std::size_t... Is>
    auto generate_insert_impl(Table& table, std::index_sequence<Is...>) const {
        auto columns = sqlpp::all_of(table);
        return sqlpp::insert_into(table).set(
            std::get<Is>(columns) = std::get<Is>(values)...
        );
    }
};

// Convenience type aliases for common tables
using beam_optics_row = row<beam_optics>;
using md_data_row = row<md_data>;
using lumi_data_row = row<lumi_data>;
using beam_row = row<beam>;
using beam_errors_row = row<beam_errors>;
using lumi_errors_row = row<lumi_errors>;
using md_errors_row = row<md_errors>;
using general_errors_row = row<general_errors>;

} // namespace QwParitySchema

#endif // __USE_DATABASE__

#endif // QWPARITYSCHEMAROW_H_