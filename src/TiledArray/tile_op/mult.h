/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2013  Virginia Tech
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Justus Calvin
 *  Department of Chemistry, Virginia Tech
 *
 *  mult.h
 *  May 8, 2013
 *
 */

#ifndef TILEDARRAY_TILE_OP_MULT_H__INCLUDED
#define TILEDARRAY_TILE_OP_MULT_H__INCLUDED

#include <TiledArray/error.h>
#include <TiledArray/tile_op/tile_interface.h>
#include <TiledArray/zero_tensor.h>

namespace TiledArray {
  namespace detail {

    /// Tile multiplication operation

    /// This multiplication will multiply the content two tiles, and accepts
    /// an optional permute argument.
    /// \tparam Result The result tile type
    /// \tparam Left The left-hand argument type
    /// \tparam Right The right-hand argument type
    /// \tparam LeftConsumable If `true`, the left-hand tile is a temporary and
    /// may be consumed
    /// \tparam RightConsumable If `true`, the right-hand tile is a temporary
    /// and may be consumed
    /// \note Input tiles can be consumed only if their type matches the result
    /// type.
    template <typename Result, typename Left, typename Right,
        bool LeftConsumable, bool RightConsumable>
    class Mult {
    public:

      typedef Mult<Result, Left, Right, LeftConsumable, RightConsumable> Mult_;
      typedef Left left_type; ///< Left-hand argument base type
      typedef Right right_type; ///< Right-hand argument base type
      typedef Result result_type; ///< The result tile type

      /// Indicates whether it is *possible* to consume the left tile
      static constexpr bool left_is_consumable =
          LeftConsumable && std::is_same<result_type, left_type>::value;
      /// Indicates whether it is *possible* to consume the right tile
      static constexpr bool right_is_consumable =
          RightConsumable && std::is_same<result_type, right_type>::value;

    private:

      // Permuting tile evaluation function
      // These operations cannot consume the argument tile since this operation
      // requires temporary storage space.

      static result_type eval(const left_type& first, const right_type& second,
          const Permutation& perm)
      {
        using TiledArray::mult;
        return mult(first, second, perm);
      }

      static result_type eval(ZeroTensor, const right_type& second,
          const Permutation& perm)
      {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      static result_type eval(const left_type& first, ZeroTensor,
          const Permutation& perm)
      {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      // Non-permuting tile evaluation functions
      // The compiler will select the correct functions based on the
      // consumability of the arguments.

      template <bool LC, bool RC,
          typename std::enable_if<!(LC || RC)>::type* = nullptr>
      static result_type
      eval(const left_type& first, const right_type& second) {
        using TiledArray::mult;
        return mult(first, second);
      }

      template <bool LC, bool RC,
          typename std::enable_if<LC>::type* = nullptr>
      static result_type eval(left_type& first, const right_type& second) {
        using TiledArray::mult_to;
        return mult_to(first, second);
      }

      template <bool LC, bool RC,
          typename std::enable_if<!LC && RC>::type* = nullptr>
      static result_type eval(const left_type& first, right_type& second) {
        using TiledArray::mult_to;
        return mult_to(second, first);
      }

      template <bool LC, bool RC,
          typename std::enable_if<!RC>::type* = nullptr>
      static result_type eval(ZeroTensor, const right_type& second) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC,
          typename std::enable_if<RC>::type* = nullptr>
      static result_type eval(ZeroTensor, right_type& second) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC,
          typename std::enable_if<!LC>::type* = nullptr>
      static result_type eval(const left_type& first, ZeroTensor) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC,
          typename std::enable_if<LC>::type* = nullptr>
      static result_type eval(left_type& first, ZeroTensor) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

    public:

      /// Multiply-and-permute operator

      /// Compute the product of two tiles and permute the result.
      /// \tparam L The left-hand tile argument type
      /// \tparam R The right-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \param perm The permutation applied to the result tile
      /// \return The permuted and scaled product of `left` and `right`.
      template <typename L, typename R>
      result_type
      operator()(L&& left, R&& right, const Permutation& perm) const {
        return eval(std::forward<L>(left), std::forward<R>(right), perm);
      }

      /// Multiply operator

      /// Compute the product of two tiles.
      /// \tparam L The left-hand tile argument type
      /// \tparam R The right-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \return The scaled product of `left` and `right`.
      template <typename L, typename R>
      result_type operator()(L&& left, R&& right) const {
        return Mult_::template eval<left_is_consumable, right_is_consumable>(
            std::forward<L>(left), std::forward<R>(right));
      }

      /// Multiply right to left

      /// Multiply the right tile to the left.
      /// \tparam R The right-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \return The product of `left` and `right`.
      template <typename R>
      result_type consume_left(left_type& left, R&& right) const {
        constexpr bool can_consume_left =
            is_consumable_tile<left_type>::value &&
            std::is_same<result_type, left_type>::value;
        constexpr bool can_consume_right = right_is_consumable &&
            ! (std::is_const<R>::value || can_consume_left);
        return Mult_::template eval<can_consume_left, can_consume_right>(left,
            std::forward<R>(right));
      }

      /// Multiply left to right

      /// Multiply the left tile to the right.
      /// \tparam L The left-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \return The product of `left` and `right`.
      template <typename L>
      result_type consume_right(L&& left, right_type& right) const {
        constexpr bool can_consume_right =
            is_consumable_tile<right_type>::value &&
            std::is_same<result_type, right_type>::value;
        constexpr bool can_consume_left = left_is_consumable &&
            ! (std::is_const<L>::value || can_consume_right);
        return Mult_::template eval<can_consume_left, can_consume_right>(
            std::forward<L>(left), right);
      }

    }; // class Mult

    /// Tile scale-multiplication operation

    /// This multiplication operation will multiply the content two tiles and
    /// apply a permutation to the result tensor. If no permutation is given or
    /// the permutation is null, then the result is not permuted.
    /// \tparam Result The result tile type
    /// \tparam Left The left-hand argument type
    /// \tparam Right The right-hand argument type
    /// \tparam Scalar The scaling factor type
    /// \tparam LeftConsumable If `true`, the left-hand tile is a temporary and
    /// may be consumed
    /// \tparam RightConsumable If `true`, the right-hand tile is a temporary
    /// and may be consumed
    /// \note Input tiles can be consumed only if their type matches the result
    /// type.
    template <typename Result, typename Left, typename Right, typename Scalar,
        bool LeftConsumable, bool RightConsumable>
    class ScalMult {
    public:

      typedef ScalMult<Result, Left, Right, Scalar, LeftConsumable,
          RightConsumable> ScalMult_; ///< This class type
      typedef Left left_type; ///< Left-hand argument base type
      typedef Right right_type; ///< Right-hand argument base type
      typedef Scalar scalar_type; ///< Scaling factor type
      typedef Result result_type; ///< Result tile type

      /// Indicates whether it is *possible* to consume the left tile
      static constexpr bool left_is_consumable =
          LeftConsumable && std::is_same<result_type, left_type>::value;
      /// Indicates whether it is *possible* to consume the right tile
      static constexpr bool right_is_consumable =
          RightConsumable && std::is_same<result_type, right_type>::value;

    private:

      scalar_type factor_; ///< The scaling factor

      // Permuting tile evaluation function
      // These operations cannot consume the argument tile since this operation
      // requires temporary storage space.

      result_type eval(const left_type& first, const right_type& second,
          const Permutation& perm) const
      {
        using TiledArray::mult;
        return mult(first, second, factor_, perm);
      }

      result_type eval(ZeroTensor, const right_type& second,
          const Permutation& perm) const
      {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      result_type eval(const left_type& first, ZeroTensor,
          const Permutation& perm) const
      {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      // Non-permuting tile evaluation functions
      // The compiler will select the correct functions based on the
      // consumability of the arguments.

      template <bool LC, bool RC,
          typename std::enable_if<!(LC || RC)>::type* = nullptr>
      result_type eval(const left_type& first, const right_type& second) const {
        using TiledArray::mult;
        return mult(first, second, factor_);
      }

      template <bool LC, bool RC,
          typename std::enable_if<LC>::type* = nullptr>
      result_type eval(left_type& first, const right_type& second) const {
        using TiledArray::mult_to;
        return mult_to(first, second, factor_);
      }

      template <bool LC, bool RC,
          typename std::enable_if<!LC && RC>::type* = nullptr>
      result_type eval(const left_type& first, right_type& second) const {
        using TiledArray::mult_to;
        return mult_to(second, first, factor_);
      }

      template <bool LC, bool RC,
          typename std::enable_if<!RC>::type* = nullptr>
      result_type eval(ZeroTensor, const right_type& second) const {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC,
          typename std::enable_if<RC>::type* = nullptr>
      result_type eval(ZeroTensor, right_type& second) const {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC,
          typename std::enable_if<!LC>::type* = nullptr>
      result_type eval(const left_type& first, ZeroTensor) const {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC,
          typename std::enable_if<LC>::type* = nullptr>
      result_type eval(left_type& first, ZeroTensor) const {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

    public:

      // Compiler generated functions
      ScalMult(const ScalMult_&) = default;
      ScalMult(ScalMult_&&) = default;
      ~ScalMult() = default;
      ScalMult_& operator=(const ScalMult_&) = default;
      ScalMult_& operator=(ScalMult_&&) = default;

      /// Constructor

      /// \param factor The scaling factor applied to result tiles
      explicit ScalMult(const Scalar factor) : factor_(factor) { }

      /// Scale-multiply-and-permute operator

      /// Compute the scaled product of two tiles and permute the result.
      /// \tparam L The left-hand tile argument type
      /// \tparam R The right-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \param perm The permutation applied to the result tile
      /// \return The permuted and scaled product of `left` and `right`.
      template <typename L, typename R>
      result_type
      operator()(L&& left, R&& right, const Permutation& perm) const {
        return eval(std::forward<L>(left), std::forward<R>(right), perm);
      }

      /// Scale-and-multiply operator

      /// Compute the scaled product of two tiles.
      /// \tparam L The left-hand tile argument type
      /// \tparam R The right-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \return The scaled product of `left` and `right`.
      template <typename L, typename R>
      result_type operator()(L&& left, R&& right) const {
        return ScalMult_::template eval<left_is_consumable,
            right_is_consumable>(std::forward<L>(left), std::forward<R>(right));
      }

      /// Multiply right to left and scale the result

      /// Multiply the right tile to the left.
      /// \tparam R The right-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \return The product of `left` and `right`.
      template <typename R>
      result_type consume_left(left_type& left, R&& right) const {
        constexpr bool can_consume_left =
            is_consumable_tile<left_type>::value &&
            std::is_same<result_type, left_type>::value;
        constexpr bool can_consume_right = right_is_consumable &&
            ! (std::is_const<R>::value || can_consume_left);
        return ScalMult_::template eval<can_consume_left, can_consume_right>(
            left, std::forward<R>(right));
      }

      /// Multiply left to right and scale the result

      /// Multiply the left tile to the right, and scale the resulting left
      /// tile.
      /// \tparam L The left-hand tile argument type
      /// \param left The left-hand tile argument
      /// \param right The right-hand tile argument
      /// \return The product of `left` and `right`.
      template <typename L>
      result_type consume_right(L&& left, right_type& right) const {
        constexpr bool can_consume_right =
            is_consumable_tile<right_type>::value &&
            std::is_same<result_type, right_type>::value;
        constexpr bool can_consume_left = left_is_consumable &&
            ! (std::is_const<L>::value || can_consume_right);
        return ScalMult_::template eval<can_consume_left, can_consume_right>(
            std::forward<L>(left), right);
      }

    }; // class ScalMult

  } // namespace detail
} // namespace TiledArray

#endif // TILEDARRAY_TILE_OP_MULT_H__INCLUDED
