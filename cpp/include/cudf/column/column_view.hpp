/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cudf/types.hpp>

#include <vector>

namespace cudf {

static constexpr size_type UNKNOWN_NULL_COUNT{-1};

namespace detail {
/**---------------------------------------------------------------------------*
 * @brief A non-owning, immutable view of device data as a column of elements,
 * some of which may be null as indicated by a bitmask.
 *
 * A `column_view_base` can be constructed implicitly from a `cudf::column`, or
 *may be constructed explicitly from a pointer to pre-existing device memory.
 *
 * Unless otherwise noted, the memory layout of the `column_view_base`'s data
 *and bitmask is expected to adhere to the Arrow Physical Memory Layout
 * Specification: https://arrow.apache.org/docs/memory_layout.html
 *
 * Because `column_view_base` is non-owning, no device memory is allocated nor
 *free'd when `column_view_base` objects are created or destroyed.
 *
 * To enable zero-copy slicing, a `column_view_base` has an `offset` that
 *indicates the index of the first element in the column relative to the base
 *device memory allocation. By default, `offset()` is zero.
 *
 *---------------------------------------------------------------------------**/
class column_view_base {
 public:
  column_view_base() = default;
  ~column_view_base() = default;
  column_view_base(column_view_base const&) = default;
  column_view_base(column_view_base&&) = default;
  column_view_base& operator=(column_view_base const&) = default;
  column_view_base& operator=(column_view_base&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Construct a `column_view_base` from pointers to device memory for
   *the elements and bitmask of the column.
   *
   * If `null_count()` is zero, `null_mask` is optional.
   *
   * If the null count of the `null_mask` is not specified, it defaults to
   * `UNKNOWN_NULL_COUNT`. The first invocation of `null_count()` will then
   * compute the null count if `null_mask` exists.
   *
   * If `type` is `EMPTY`, the specified `null_count` will be ignored and
   * `null_count()` will always return the same value as `size()`
   *
   * @throws `cudf::logic_error` if `size < 0`
   * @throws `cudf::logic_error` if `size > 0` but `data == nullptr`
   * @throws `cudf::logic_error` if `type.id() == EMPTY` but `data != nullptr`
   *or `null_mask != nullptr`
   * @throws `cudf::logic_error` if `null_count > 0`, but `null_mask == nullptr`
   * @throws `cudf::logic_error` if `offset < 0`
   *
   * @param type The element type
   * @param size The number of elements
   * @param data Pointer to device memory containing the column elements
   * @param null_mask Optional, pointer to device memory containing the null
   * indicator bitmask
   * @param null_count Optional, the number of null elements.
   * @param offset optional, index of the first element
   * @param children optional, depending on the element type, child columns may
   * contain additional data
   *---------------------------------------------------------------------------**/
  column_view_base(data_type type, size_type size, void const* data,
                   bitmask_type const* null_mask = nullptr,
                   size_type null_count = UNKNOWN_NULL_COUNT,
                   size_type offset = 0);

  /**---------------------------------------------------------------------------*
   * @brief Returns pointer to the base device memory allocation casted to
   * the specified type.
   *
   * @note If `offset() == 0`, then `head<T>() == data<T>()`
   *
   * @note It should be rare to need to access the `head<T>()` allocation of
   *a column, and instead, accessing the elements should be done via
   *`data<T>()`.
   *
   * @tparam The type to cast to
   * @return T const* Typed pointer to underlying data
   *---------------------------------------------------------------------------**/
  template <typename T = void>
  T const* head() const noexcept {
    return static_cast<T const*>(_data);
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns the underlying data casted to the specified type, plus the
   * offset.
   *
   * @note If `offset() == 0`, then `head<T>() == data<T>()`
   *
   * @TODO Clarify behavior for variable-width types.
   *
   * @tparam T The type to cast to
   * @return T const* Typed pointer to underlying data, including the offset
   *---------------------------------------------------------------------------**/
  template <typename T>
  T const* data() const noexcept {
    return head<T>() + _offset;
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns the number of elements in the column
   *---------------------------------------------------------------------------**/
  size_type size() const noexcept { return _size; }

  /**---------------------------------------------------------------------------*
   * @brief Returns the element `data_type`
   *---------------------------------------------------------------------------**/
  data_type type() const noexcept { return _type; }

  /**---------------------------------------------------------------------------*
   * @brief Indicates if the column can contain null elements, i.e., if it has
   * an allocated bitmask.
   *
   * @note If `null_count() > 0`, this function must always return `true`.
   *
   * @return true The bitmask is allocated
   * @return false The bitmask is not allocated
   *---------------------------------------------------------------------------**/
  bool nullable() const noexcept { return nullptr != _null_mask; }

  /**---------------------------------------------------------------------------*
   * @brief Returns the count of null elements
   *---------------------------------------------------------------------------**/
  size_type null_count() const noexcept;

  /**---------------------------------------------------------------------------*
   * @brief Indicates if the column contains null elements,
   * i.e., `null_count() > 0`
   *
   * @return true One or more elements are null
   * @return false All elements are valid
   *---------------------------------------------------------------------------**/
  bool has_nulls() const noexcept { return _null_count > 0; }

  /**---------------------------------------------------------------------------*
   * @brief Returns raw pointer to the underlying bitmask allocation.
   *
   * @note This function does *not* account for the `offset()`.
   *
   * @note If `null_count() == 0`, this may return `nullptr`.
   *---------------------------------------------------------------------------**/
  bitmask_type const* null_mask() const noexcept { return _null_mask; }

  /**---------------------------------------------------------------------------*
   * @brief Returns the index of the first element relative to the base memory
   * allocation, i.e., what is returned from `head<T>()`.
   *---------------------------------------------------------------------------**/
  size_type offset() const noexcept { return _offset; }

 protected:
  data_type _type{EMPTY};   ///< Element type
  cudf::size_type _size{};  ///< Number of elements
  void const* _data{};      ///< Pointer to device memory containing elements
  bitmask_type const* _null_mask{};  ///< Pointer to device memory containing
                                     ///< bitmask representing null elements.
                                     ///< Optional if `null_count() == 0`
  size_type _null_count{};           ///< The number of null elements
  size_type _offset{};               ///< Index position of the first element.
                                     ///< Enables zero-copy slicing
};
}  // namespace detail

/**---------------------------------------------------------------------------*
 * @brief A non-owning, immutable view of device data as a column of elements,
 * some of which may be null as indicated by a bitmask.
 *
 * A `column_view` can be constructed implicitly from a `cudf::column`, or may
 * be constructed explicitly from a pointer to pre-existing device memory.
 *
 * Unless otherwise noted, the memory layout of the `column_view`'s data and
 * bitmask is expected to adhere to the Arrow Physical Memory Layout
 * Specification: https://arrow.apache.org/docs/memory_layout.html
 *
 * Because `column_view` is non-owning, no device memory is allocated nor free'd
 * when `column_view` objects are created or destroyed.
 *
 * To enable zero-copy slicing, a `column_view` has an `offset` that indicates
 * the index of the first element in the column relative to the base device
 * memory allocation. By default, `offset()` is zero.
 *
 *---------------------------------------------------------------------------**/
class column_view : public detail::column_view_base {
 public:
  column_view() = default;
  ~column_view() = default;
  column_view(column_view const&) = default;
  column_view(column_view&&) = default;
  column_view& operator=(column_view const&) = default;
  column_view& operator=(column_view&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Construct a `column_view` from pointers to device memory for the
   * elements and bitmask of the column.
   *
   * If `null_count()` is zero, `null_mask` is optional.
   *
   * If the null count of the `null_mask` is not specified, it defaults to
   * `UNKNOWN_NULL_COUNT`. The first invocation of `null_count()` will then
   * compute the null count if `null_mask` exists.
   *
   * If `type` is `EMPTY`, the specified `null_count` will be ignored and
   * `null_count()` will always return the same value as `size()`
   *
   * @throws `cudf::logic_error` if `size < 0`
   * @throws `cudf::logic_error` if `size > 0` but `data == nullptr`
   * @throws `cudf::logic_error` if `type.id() == EMPTY` but `data != nullptr`
   *or `null_mask != nullptr`
   * @throws `cudf::logic_error` if `null_count > 0`, but `null_mask == nullptr`
   * @throws `cudf::logic_error` if `offset < 0`
   *
   * @param type The element type
   * @param size The number of elements
   * @param data Pointer to device memory containing the column elements
   * @param null_mask Optional, pointer to device memory containing the null
   * indicator bitmask
   * @param null_count Optional, the number of null elements.
   * @param offset optional, index of the first element
   * @param children optional, depending on the element type, child columns may
   * contain additional data
   *---------------------------------------------------------------------------**/
  column_view(data_type type, size_type size, void const* data,
              bitmask_type const* null_mask = nullptr,
              size_type null_count = UNKNOWN_NULL_COUNT, size_type offset = 0,
              std::vector<column_view> const& children = {});

  /**---------------------------------------------------------------------------*
   * @brief Returns the specified child
   *
   * @param child_index The index of the desired child
   * @return column_view The requested child `column_view`
   *---------------------------------------------------------------------------**/
  column_view child(size_type child_index) const noexcept {
    return _children[child_index];
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns the number of child columns.
   *---------------------------------------------------------------------------**/
  size_type num_children() const noexcept { return _children.size(); }

 private:
  std::vector<column_view> _children{};  ///< Based on element type, children
                                         ///< may contain additional data
};                                       // namespace cudf

/**---------------------------------------------------------------------------*
 * @brief A non-owning, mutable view of device data as a column of elements,
 * some of which may be null as indicated by a bitmask.
 *
 * A `mutable_column_view` can be constructed implicitly from a `cudf::column`,
 *or may be constructed explicitly from a pointer to pre-existing device memory.
 *
 * Unless otherwise noted, the memory layout of the `mutable_column_view`'s data
 *and bitmask is expected to adhere to the Arrow Physical Memory Layout
 * Specification: https://arrow.apache.org/docs/memory_layout.html
 *
 * Because `mutable_column_view` is non-owning, no device memory is allocated
 *nor free'd when `mutable_column_view` objects are created or destroyed.
 *
 * To enable zero-copy slicing, a `mutable_column_view` has an `offset` that
 *indicates the index of the first element in the column relative to the base
 *device memory allocation. By default, `offset()` is zero.
 *
 *---------------------------------------------------------------------------**/
class mutable_column_view : public detail::column_view_base {
 public:
  mutable_column_view() = default;
  ~mutable_column_view() = default;
  mutable_column_view(mutable_column_view const&) = default;
  mutable_column_view(mutable_column_view&&) = default;
  mutable_column_view& operator=(mutable_column_view const&) = default;
  mutable_column_view& operator=(mutable_column_view&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Construct a `mutable_column_view` from pointers to device memory for
   *the elements and bitmask of the column.

   * If the null count of the `null_mask` is not specified, it defaults to
   * `UNKNOWN_NULL_COUNT`. The first invocation of `null_count()` will then
   * compute the null count.
   *
   * If `type` is `EMPTY`, the specified `null_count` will be ignored and
   * `null_count()` will always return the same value as `size()`
   *
   * @throws `cudf::logic_error` if `size < 0`
   * @throws `cudf::logic_error` if `size > 0` but `data == nullptr`
   * @throws `cudf::logic_error` if `type.id() == EMPTY` but `data != nullptr`
   *or `null_mask != nullptr`
   * @throws `cudf::logic_error` if `null_count > 0`, but `null_mask == nullptr`
   * @throws `cudf::logic_error` if `offset < 0`
   *
   * @param type The element type
   * @param size The number of elements
   * @param data Pointer to device memory containing the column elements
   * @param null_mask Optional, pointer to device memory containing the null
   indicator
   * bitmask
   * @param null_count Optional, the number of null elements.
   * @param offset optional, index of the first element
   * @param children optional, depending on the element type, child columns may
   * contain additional data
   *---------------------------------------------------------------------------**/
  mutable_column_view(data_type type, size_type size, void* data,
                      bitmask_type* null_mask = nullptr,
                      size_type null_count = cudf::UNKNOWN_NULL_COUNT,
                      size_type offset = 0,
                      std::vector<mutable_column_view> const& children = {});

  /**---------------------------------------------------------------------------*
   * @brief Returns pointer to the base device memory allocation casted to
   * the specified type.
   *
   * @note If `offset() == 0`, then `head<T>() == data<T>()`
   *
   * @note It should be rare to need to access the `head<T>()` allocation of a
   * column, and instead, accessing the elements should be done via `data<T>()`.
   *
   * @tparam The type to cast to
   * @return T const* Typed pointer to underlying data
   *---------------------------------------------------------------------------**/
  template <typename T = void>
  T* head() const noexcept {
    return const_cast<T*>(detail::column_view_base::head());
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns the underlying data casted to the specified type, plus the
   * offset.
   *
   * @note If `offset() == 0`, then `head<T>() == data<T>()`
   *
   * @TODO Clarify behavior for variable-width types.
   *
   * @tparam T The type to cast to
   * @return T const* Typed pointer to underlying data, including the offset
   *---------------------------------------------------------------------------**/
  template <typename T>
  T* data() const noexcept {
    return head<T>() + _offset;
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns raw pointer to the underlying bitmask allocation.
   *
   * @note This function does *not* account for the `offset()`.
   *
   * @note If `null_count() == 0`, this may return `nullptr`.
   *---------------------------------------------------------------------------**/
  bitmask_type* null_mask() const noexcept {
    return const_cast<bitmask_type*>(detail::column_view_base::null_mask());
  }

  /**---------------------------------------------------------------------------*
   * @brief Set the null count
   *
   * @param new_null_count The new null count
   *---------------------------------------------------------------------------**/
  void set_null_count(size_type new_null_count) noexcept {
    _null_count = new_null_count;
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns a reference to the specified child
   *
   * @param child_index The index of the desired child
   * @return mutable_column_view The requested child `mutable_column_view`
   *---------------------------------------------------------------------------**/
  mutable_column_view& child(size_type child_index) noexcept {
    return mutable_children[child_index];
  }

  /**---------------------------------------------------------------------------*
   * @brief Returns the number of child columns.
   *---------------------------------------------------------------------------**/
  size_type num_children() const noexcept { return mutable_children.size(); }

  /**---------------------------------------------------------------------------*
   * @brief Converts a mutable view into an immutable view
   *
   * @return column_view An immutable view of the mutable view's elements
   *---------------------------------------------------------------------------**/
  operator column_view() const;

 private:
  std::vector<mutable_column_view> mutable_children;
};

}  // namespace cudf