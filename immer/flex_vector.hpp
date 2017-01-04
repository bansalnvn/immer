//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <immer/detail/rbts/rrbtree.hpp>
#include <immer/detail/rbts/rrbtree_iterator.hpp>
#include <immer/memory_policy.hpp>

namespace immer {

template <typename T,
          typename MP,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class vector;

template <typename T,
          typename MP,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class flex_vector_transient;

/*!
 * Immutable sequential container supporting both random access,
 * structural sharing and efficient concatenation and slicing.
 *
 * @tparam T The type of the values to be stored in the container.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *         memory_policy.
 *
 * @rst
 *
 * This container is very similar to `vector`_ but also supports
 * :math:`O(log(size))` *concatenation*, *slicing* and *insertion* at
 * any point. Its performance characteristics are almost identical
 * until one of these operations is performed.  After that,
 * performance is degraded by a constant factor that usually oscilates
 * in the range :math:`[1, 2)` depending on the operation and the
 * amount of flexible operations that have been performed.
 *
 * .. tip:: A `vector`_ can be converted to a `flex_vector`_ in
 *    constant time without any allocation.  This is so because the
 *    internal structure of a *vector* is a strict subset of the
 *    internal structure of a *flexible vector*.  You can take
 *    advantage of this property by creating normal vectors as long as
 *    the flexible operations are not needed, and convert later in
 *    your processing pipeline once and if these are needed.
 *
 * @endrst
 */
template <typename T,
          typename MemoryPolicy   = default_memory_policy,
          detail::rbts::bits_t B  = default_bits,
          detail::rbts::bits_t BL = detail::rbts::derive_bits_leaf<T, MemoryPolicy, B>>
class flex_vector
{
    using impl_t = detail::rbts::rrbtree<T, MemoryPolicy, B, BL>;

public:
    static constexpr auto bits = B;
    static constexpr auto bits_leaf = BL;
    using memory_policy = MemoryPolicy;

    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::rbts::rrbtree_iterator<T, MemoryPolicy, B, BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using transient_type   = flex_vector_transient<T, MemoryPolicy, B, BL>;

    /*!
     * Default constructor.  It creates a flex_vector of `size() == 0`.
     * It does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    flex_vector() = default;

    /*!
     * Default constructor.  It creates a flex_vector with the same
     * contents as `v`.  It does not allocate memory and is
     * @f$ O(1) @f$.
     */
    flex_vector(vector<T, MemoryPolicy, B, BL> v)
        : impl_ { v.impl_.size, v.impl_.shift,
                  v.impl_.root->inc(), v.impl_.tail->inc() }
    {}

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing at the first element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rbegin() const { return reverse_iterator{end()}; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing after the last element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    std::size_t size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    bool empty() const { return impl_.size == 0; }

    /*!
     * Returns a `const` reference to the element at position `index`.
     * Undefined for `index >= size()`.
     * It does not allocate memory and its complexity
     * is *effectively* @f$ O(1) @f$.
     */
    reference operator[] (size_type index) const
    { return impl_.get(index); }

    /*!
     * Returns a flex_vector with `value` inserted at the end.  It may
     * allocate memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    flex_vector push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    /*!
     * Returns a flex_vector with `value` inserted at the frony.  It may
     * allocate memory and its complexity is @f$ O(log(size)) @f$.
     */
    flex_vector push_front(value_type value) const
    { return flex_vector{}.push_back(value) + *this; }

    /*!
     * Returns a flex_vector containing value `value` at position `index`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    flex_vector set(std::size_t index, value_type value) const
    { return { impl_.assoc(index, std::move(value)) }; }

    /*!
     * Returns a vector containing the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    template <typename FnT>
    flex_vector update(std::size_t index, FnT&& fn) const
    { return { impl_.update(index, std::forward<FnT>(fn)) }; }

    /*!
     * Returns a vector containing only the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    flex_vector take(std::size_t elems) const
    { return { impl_.take(elems) }; }

    /*!
     * Returns a vector without the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    flex_vector drop(std::size_t elems) const
    { return { impl_.drop(elems) }; }

    /*!
     * Apply operation `fn` for every *chunk* of data in the vector
     * sequentially.  Each time, `Fn` is passed two `value_type`
     * pointers describing a range over a part of the vector.  This
     * allows iterating over the elements in the most efficient way.
     *
     * @rst
     *
     * .. tip:: This is a low level method. Most of the time,
     *    :doc:`wrapper algorithms <algorithms>` should be used
     *    instead.
     *
     * @endrst
     */
    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    { impl_.for_each_chunk(std::forward<Fn>(fn)); }

    /*!
     * Concatenation operator. Returns a flex_vector with the contents
     * of `l` followed by those of `r`.  It may allocate memory
     * and its complexity is @f$ O(log(max(size_r, size_l))) @f$
     */
    friend flex_vector operator+ (const flex_vector& l, const flex_vector& r)
    { return l.impl_.concat(r.impl_); }

    /*!
     * Returns a `transient` form of this container.
     */
    transient_type transient() const&
    { return transient_type{ impl_ }; }
    transient_type transient() &&
    { return transient_type{ std::move(impl_) }; }

#if IMMER_DEBUG_PRINT
    void debug_print() const
    { impl_.debug_print(); }
#endif

private:
    friend transient_type;

    flex_vector(impl_t impl)
        : impl_(std::move(impl))
    {
#if IMMER_DEBUG_PRINT
        // force the compiler to generate debug_print, so we can call
        // it from a debugger
        [](volatile auto){}(&flex_vector::debug_print);
#endif
    }
    impl_t impl_ = impl_t::empty;
};

} // namespace immer
