/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2015  Virginia Tech
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
 *  kernels.h
 *  Jun 1, 2015
 *
 */

#ifndef TILEDARRAY_TENSOR_KENERLS_H__INCLUDED
#define TILEDARRAY_TENSOR_KENERLS_H__INCLUDED

#include <TiledArray/tensor/utility.h>
#include <TiledArray/tensor/permute.h>
#include <TiledArray/math/eigen.h>

namespace TiledArray {

  template <typename, typename> class Tensor;

  namespace detail {

    // -------------------------------------------------------------------------
    // Tensor kernel operations that generate a new tensor

    /// Tensor operations with contiguous data

    /// This function sets the elements of the result tensor with
    /// \c op(tensor1[i], tensors[i]...)
    /// \tparam TR The tensor result type
    /// \tparam Op The element-wise operation type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param op The result tensor element initialization operation
    /// \param tensor1 The first argument tensor
    /// \param tensors The remaining argument tensors
    template <typename TR, typename Op, typename T1, typename... Ts,
        enable_if_t<is_tensor<TR, T1, Ts...>::value
            || is_tensor_of_tensor<TR, T1, Ts...>::value>* = nullptr>
    inline TR tensor_op(Op&& op, const T1& tensor1, const Ts&... tensors) {
      return TR(tensor1, tensors..., std::forward<Op>(op));
    }


    /// Tensor permutation operations with contiguous data

    /// This function sets the elements of the result tensor with
    /// \c op(tensor1[i],tensors[i]...)
    /// \tparam TR The tensor result type
    /// \tparam Op The element-wise operation type
    /// \tparam T1 The result tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The operation that is used to compute the result
    /// value from the input arguments
    /// \param[in] perm The permutation applied to the argument tensors
    /// \param[in] tensor1 The first argument tensor
    /// \param[in] tensors The remaining argument tensors
    template <typename TR, typename Op, typename T1, typename... Ts,
        enable_if_t<(is_tensor<T1, Ts...>::value
            || is_tensor_of_tensor<TR, T1, Ts...>::value)
            && is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    inline TR tensor_op(Op&& op, const Permutation& perm, const T1& tensor1,
        const Ts&... tensors)
    {
      return TR(tensor1, tensors..., std::forward<Op>(op), perm);
    }


    // -------------------------------------------------------------------------
    // Tensor kernel operations with in-place memory operations

    /// In-place tensor operations with contiguous data

    /// This function sets the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[in,out] result The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename... Ts,
        enable_if_t<is_tensor<TR, Ts...>::value
                 && is_contiguous_tensor<TR, Ts...>::value>* = nullptr>
    inline void inplace_tensor_op(Op&& op, TR& result, const Ts&... tensors) {
      TA_ASSERT(! empty(result, tensors...));
      TA_ASSERT(is_range_set_congruent(result, tensors...));

      const auto volume = result.range().volume();

      math::vector_op(std::forward<Op>(op), volume, result.data(),
          tensors.data()...);
    }

    /// In-place tensor of tensors operations with contiguous data

    /// This function sets the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[in,out] result The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename... Ts,
        enable_if_t<is_tensor_of_tensor<TR, Ts...>::value
                 && is_contiguous_tensor<TR, Ts...>::value>* = nullptr>
    inline void inplace_tensor_op(Op&& op, TR& result, const Ts&... tensors) {
      TA_ASSERT(! empty(result, tensors...));
      TA_ASSERT(is_range_set_congruent(result, tensors...));

      const auto volume = result.range().volume();

      for(decltype(result.range().volume()) i = 0ul; i < volume; ++i) {
        inplace_tensor_op(op, result[i], tensors[i]...);
      }
    }

    /// In-place tensor permutation operations with contiguous data

    /// This function sets the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// The expected signature of the input operations is:
    /// \code
    /// Result::value_type op(const T1::value_type, const Ts::value_type...)
    /// \endcode
    /// The expected signature of the output operations is:
    /// \code
    /// void op(TR::value_type*, const TR::value_type)
    /// \endcode
    /// \tparam InputOp The input operation type
    /// \tparam OutputOp The output operation type
    /// \tparam TR The result tensor type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param[in] input_op The operation that is used to generate the output
    /// value from the input arguments
    /// \param[in] output_op The operation that is used to set the value of the
    /// result tensor given the element pointer and the result value
    /// \param[in] perm The permutation applied to the argument tensors
    /// \param[in,out] result The result tensor
    /// \param[in] tensor1 The first argument tensor
    /// \param[in] tensors The remaining argument tensors
    template <typename InputOp, typename OutputOp, typename TR, typename T1, typename... Ts,
        enable_if_t<is_tensor<TR, T1, Ts...>::value
               && is_contiguous_tensor<TR, T1, Ts...>::value>* = nullptr>
    inline void inplace_tensor_op(InputOp&& input_op, OutputOp&& output_op,
        const Permutation& perm, TR& result, const T1& tensor1,
        const Ts&... tensors)
    {
      TA_ASSERT(! empty(result, tensor1, tensors...));
      TA_ASSERT(is_range_congruent(result, tensor1, perm));
      TA_ASSERT(is_range_set_congruent(tensor1, tensors...));
      TA_ASSERT(perm);
      TA_ASSERT(perm.dim() == tensor1.range().rank());

      permute(std::forward<InputOp>(input_op), std::forward<OutputOp>(output_op),
          result, perm, tensor1, tensors...);
    }


    /// In-place tensor of tensors permutation operations with contiguous data

    /// This function sets the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// The expected signature of the input operations is:
    /// \code
    /// Result::value_type op(const T1::value_type::value_type, const Ts::value_type::value_type...)
    /// \endcode
    /// The expected signature of the output operations is:
    /// \code
    /// void op(TR::value_type::value_type*, const TR::value_type::value_type)
    /// \endcode
    /// \tparam InputOp The input operation type
    /// \tparam OutputOp The output operation type
    /// \tparam TR The result tensor type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param[in] input_op The operation that is used to generate the output
    /// value from the input arguments
    /// \param[in] output_op The operation that is used to set the value of the
    /// result tensor given the element pointer and the result value
    /// \param[in] perm The permutation applied to the argument tensors
    /// \param[in,out] result The result tensor
    /// \param[in] tensor1 The first argument tensor
    /// \param[in] tensors The remaining argument tensors
    template <typename InputOp, typename OutputOp, typename TR, typename T1, typename... Ts,
        enable_if_t<is_tensor_of_tensor<TR, T1, Ts...>::value
               && is_contiguous_tensor<TR, T1, Ts...>::value>* = nullptr>
    inline void inplace_tensor_op(InputOp&& input_op, OutputOp&& output_op,
        const Permutation& perm, TR& result, const T1& tensor1,
        const Ts&... tensors)
    {
      TA_ASSERT(! empty(result, tensor1, tensors...));
      TA_ASSERT(is_range_congruent(result, tensor1, perm));
      TA_ASSERT(is_range_set_congruent(tensor1, tensors...));
      TA_ASSERT(perm);
      TA_ASSERT(perm.dim() == tensor1.range().rank());

      auto wrapper_input_op = [=] (typename T1::const_reference restrict value1,
        typename Ts::const_reference restrict... values) ->
        typename T1::value_type
        { return tensor_op<TR::value_type>(input_op, value1, values...); };

      auto wrapper_output_op = [=] (typename T1::pointer restrict const result_value,
                                    const typename TR::value_type value)
      { inplace_tensor_op(output_op, *result_value, value); };

      permute(wrapper_input_op, wrapper_output_op, result, perm, tensor1,
          tensors...);
    }

    /// In-place tensor operations with non-contiguous data

    /// This function sets the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[in,out] result The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename... Ts,
        enable_if_t<is_tensor<TR, Ts...>::value
               && ! (is_contiguous_tensor<TR, Ts...>::value)>* = nullptr>
    inline void inplace_tensor_op(Op&& op, TR& result, const Ts&... tensors) {
      TA_ASSERT(! empty(result, tensors...));
      TA_ASSERT(is_range_congruent(result, tensors...));

      const auto stride = inner_size(result, tensors...);
      const auto volume = result.range().volume();

      for(decltype(result.range().volume()) i = 0ul; i < volume; i += stride)
        math::vector_op(op, stride, result.data() + result.range().ord(i),
          (tensors.data() + tensors.range().ord(i))...);
    }

    /// In-place tensor of tensors operations with non-contiguous data

    /// This function sets the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam Ts The remaining argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[in,out] result The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename... Ts,
        enable_if_t<is_tensor_of_tensor<TR, Ts...>::value
               && ! (is_contiguous_tensor<TR, Ts...>::value)>* = nullptr>
    inline void inplace_tensor_op(Op&& op, TR& result, const Ts&... tensors) {
      TA_ASSERT(! empty(result, tensors...));
      TA_ASSERT(is_range_congruent(result, tensors...));

      const auto stride = inner_size(result, tensors...);
      const auto volume = result.range().volume();

      auto inplace_tensor_range =
          [=] (typename TR::pointer restrict const result_data,
          typename Ts::const_pointer restrict const... tensors_data)
          {
            for(decltype(result.range().volume()) i = 0ul; i < stride; ++i)
              inplace_tensor_op(op, result_data[i], tensors_data[i]...);
          };

      for(decltype(result.range().volume()) i = 0ul; i < volume; i += stride)
        inplace_tensor_range(result.data() + result.range().ord(i),
            (tensors.data() + tensors.range().ord(i))...);
    }

    // -------------------------------------------------------------------------
    // Tensor initialization functions for argument tensors with contiguous
    // memory layout

    /// Initialize tensor with contiguous tensor arguments

    /// This function initializes the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \pre The memory of \c tensor1 has been allocated but not initialized.
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[out] result The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename... Ts,
        enable_if_t<is_tensor<TR, Ts...>::value
               && is_contiguous_tensor<TR, Ts...>::value>* = nullptr>
    inline void tensor_init(Op&& op, TR& result, const Ts&... tensors) {
      TA_ASSERT(! empty(result, tensors...));
      TA_ASSERT(is_range_set_congruent(result, tensors...));

      const auto volume = result.range().volume();

      auto wrapper_op = [=] (typename TR::pointer restrict result,
              typename Ts::const_reference restrict... ts)
          { new(result) typename TR::value_type(op(ts...)); };

      math::vector_ptr_op(wrapper_op, volume, result.data(), tensors.data()...);
    }

    /// Initialize tensor of tensors with contiguous tensor arguments

    /// This function initializes the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \pre The memory of \c tensor1 has been allocated but not initialized.
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[out] result The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename... Ts,
        enable_if_t<is_tensor_of_tensor<TR, Ts...>::value
               && is_contiguous_tensor<TR, Ts...>::value>* = nullptr>
    inline void tensor_init(Op&& op, TR& result, const Ts&... tensors) {
      TA_ASSERT(! empty(result, tensors...));
      TA_ASSERT(is_range_set_congruent(result, tensors...));

      const auto volume = result.range().volume();

      for(decltype(result.range().volume()) i = 0ul; i < volume; ++i) {
        new(result.data() + i)
            typename TR::value_type(tensor_op<typename TR::value_type>(op, tensors[i]...));
      }
    }


    /// Initialize tensor with permuted tensor arguments

    /// This function initializes the elements of \c tensor1 with the result of
    /// \c op(tensor1[i], tensors[i]...)
    /// \pre The memory of \c result has been allocated but not initialized.
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[in] perm The permutation that will be applied to tensor2
    /// \param[out] result The result tensor
    /// \param[in] tensor1 The first argument tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename T1, typename... Ts,
        enable_if_t<is_tensor<TR, T1, Ts...>::value
               && is_contiguous_tensor<TR, T1, Ts...>::value>* = nullptr>
    inline void tensor_init(Op&& op, const Permutation& perm, TR& result,
        const T1& tensor1, const Ts&... tensors)
    {
      TA_ASSERT(! empty(result, tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(perm, result, tensor1, tensors...));
      TA_ASSERT(perm);
      TA_ASSERT(perm.dim() == result.range().rank());

      auto output_op = [=] (typename TR::pointer restrict result,
          typename TR::const_reference  restrict temp)
          { new(result) typename TR::value_type(temp); };

      permute(std::forward<Op>(op), output_op, result, perm, tensor1, tensors...);
    }


    /// Initialize tensor of tensors with permuted tensor arguments

    /// This function initializes the elements of \c tensor1 with the result of
    /// \c op(tensor1[i], tensors[i]...)
    /// \pre The memory of \c result has been allocated but not initialized.
    /// \tparam Op The element initialization operation type
    /// \tparam TR The result tensor type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[in] perm The permutation that will be applied to tensor2
    /// \param[out] result The result tensor
    /// \param[in] tensor1 The first argument tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename T1, typename... Ts,
        enable_if_t<is_tensor_of_tensor<TR, T1, Ts...>::value
               && is_contiguous_tensor<TR, T1, Ts...>::value>* = nullptr>
    inline void tensor_init(Op&& op, const Permutation& perm, TR& result,
        const T1& tensor1, const Ts&... tensors)
    {
      TA_ASSERT(! empty(result, tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(perm, result, tensor1, tensors...));
      TA_ASSERT(perm);
      TA_ASSERT(perm.dim() == result.range().rank());

      auto output_op = [=] (typename TR::pointer restrict result,
          typename TR::const_reference  restrict temp)
          { new(result) typename TR::value_type(temp); };
      auto tensor_input_op = [=] (typename T1::const_reference restrict value1,
          typename Ts::const_reference restrict... values) ->
          typename TR::value_type
          { return tensor_op<typename TR::value_type>(op, value1, values...); };

      permute(tensor_input_op, output_op, result, perm, tensor1, tensors...);
    }


    /// Initialize tensor with one or more non-contiguous tensor arguments

    /// This function initializes the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \pre The memory of \c tensor1 has been allocated but not initialized.
    /// \tparam Op The element initialization operation type
    /// \tparam T1 The result tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[out] tensor1 The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename T1, typename... Ts,
        enable_if_t<is_tensor<TR, T1, Ts...>::value
                 && is_contiguous_tensor<TR>::value
                 && ! is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    inline void tensor_init(Op&& op, TR& result, const T1& tensor1,
        const Ts&... tensors)
    {
      TA_ASSERT(! empty(result, tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(result, tensor1, tensors...));

      const auto stride = inner_size(tensor1, tensors...);
      const auto volume = tensor1.range().volume();

      auto wrapper_op = [=] (typename TR::pointer restrict result_ptr,
              const typename T1::value_type value1,
              const typename Ts::value_type... values)
          { new(result_ptr) typename T1::value_type(op(value1, values...)); };

      for(decltype(tensor1.range().volume()) i = 0ul; i < volume; i += stride)
        math::vector_ptr_op(wrapper_op, stride, result.data() + i,
            (tensor1.data() + tensor1.range().ord(i)),
            (tensors.data() + tensors.range().ord(i))...);
    }

    /// Initialize tensor with one or more non-contiguous tensor arguments

    /// This function initializes the elements of \c tensor1 with the result of
    /// \c op(tensors[i]...)
    /// \pre The memory of \c tensor1 has been allocated but not initialized.
    /// \tparam Op The element initialization operation type
    /// \tparam T1 The result tensor type
    /// \tparam Ts The argument tensor types
    /// \param[in] op The result tensor element initialization operation
    /// \param[out] tensor1 The result tensor
    /// \param[in] tensors The argument tensors
    template <typename Op, typename TR, typename T1, typename... Ts,
        enable_if_t<is_tensor_of_tensor<TR, T1, Ts...>::value
                 && is_contiguous_tensor<TR>::value
                 && ! is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    inline void tensor_init(Op&& op, TR& result, const T1& tensor1,
        const Ts&... tensors)
    {
      TA_ASSERT(! empty(result, tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(result, tensor1, tensors...));

      const auto stride = inner_size(tensor1, tensors...);
      const auto volume = tensor1.range().volume();


      auto inplace_tensor_range =
          [=] (typename TR::pointer restrict const result_data,
              typename T1::const_pointer restrict const tensor1_data,
              typename Ts::const_pointer restrict const... tensors_data)
          {
            for(decltype(result.range().volume()) i = 0ul; i < stride; ++i)
              new(result_data + i)
                  typename TR::value_type(tensor_op<typename TR::value_type>(op,
                      tensor1_data[i], tensors_data[i]...));
          };

      for(decltype(volume) i = 0ul; i < volume; i += stride)
        inplace_tensor_range(result.data() + i,
            (tensor1.data() + tensor1.range().ord(i)),
            (tensors.data() + tensors.range().ord(i))...);
    }


    // -------------------------------------------------------------------------
    // Tensor reduction kernels for argument tensors

    /// Tensor reduction operation for contiguous tensors

    /// Perform an element-wise reduction of the tensors.
    /// \tparam ReduceOp The element-wise reduction operation type
    /// \tparam JoinOp The result operation type
    /// \tparam Scalar A scalar type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The argument tensor types
    /// \param reduce_op The element-wise reduction operation
    /// \param identity The initial value for the reduction and the result
    /// \param tensor1 The first tensor to be reduced
    /// \param tensors The other tensors to be reduced
    /// \return The reduced value of the tensor(s)
    template <typename ReduceOp, typename JoinOp, typename Scalar, typename T1, typename... Ts,
    enable_if_t<is_numeric<Scalar>::value && is_tensor<T1, Ts...>::value
             && is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    Scalar tensor_reduce(ReduceOp&& reduce_op, JoinOp&&,
        Scalar identity, const T1& tensor1, const Ts&... tensors)
    {
      TA_ASSERT(! empty(tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(tensor1, tensors...));

      const auto volume = tensor1.range().volume();

      math::reduce_op(std::forward<ReduceOp>(reduce_op), volume, identity,
          tensor1.data(), tensors.data()...);

      return identity;
    }

    /// Tensor of tensor reduction operation for contiguous tensors

    /// Perform an element-wise reduction of the tensors.
    /// \tparam ReduceOp The element-wise reduction operation type
    /// \tparam JoinOp The result operation type
    /// \tparam Scalar A scalar type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The argument tensor types
    /// \param reduce_op The element-wise reduction operation
    /// \param join_op The result join operation
    /// \param identity The initial value for the reduction and the result
    /// \param tensor1 The first tensor to be reduced
    /// \param tensors The other tensors to be reduced
    /// \return The reduced value of the tensor(s)
    template <typename ReduceOp, typename JoinOp, typename Scalar, typename T1, typename... Ts,
        enable_if_t<is_numeric<Scalar>::value
            && is_tensor_of_tensor<T1, Ts...>::value
            && is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    Scalar tensor_reduce(ReduceOp&& reduce_op, JoinOp&& join_op,
        Scalar identity, const T1& tensor1, const Ts&... tensors)
    {
      TA_ASSERT(! empty(tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(tensor1, tensors...));

      const auto volume = tensor1.range().volume();

      auto result = identity;
      for(decltype(tensor1.range().volume()) i = 0ul; i < volume; ++i) {
        auto temp = tensor_reduce(reduce_op, join_op, identity, tensor1[i],
            tensors[i]...);
        join_op(result, temp);
      }

      return result;
    }

    /// Tensor reduction operation for non-contiguous tensors

    /// Perform an element-wise reduction of the tensors.
    /// \tparam ReduceOp The element-wise reduction operation type
    /// \tparam JoinOp The result operation type
    /// \tparam Scalar A scalar type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The argument tensor types
    /// \param reduce_op The element-wise reduction operation
    /// \param join_op The result join operation
    /// \param identity The initial value for the reduction and the result
    /// \param tensor1 The first tensor to be reduced
    /// \param tensors The other tensors to be reduced
    /// \return The reduced value of the tensor(s)
    template <typename ReduceOp, typename JoinOp, typename Scalar, typename T1, typename... Ts,
        enable_if_t<is_numeric<Scalar>::value && is_tensor<T1, Ts...>::value
            && ! is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    Scalar tensor_reduce(ReduceOp&& reduce_op, JoinOp&& join_op,
        const Scalar identity, const T1& tensor1, const Ts&... tensors)
    {
      TA_ASSERT(! empty(tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(tensor1, tensors...));

      const auto stride = inner_size(tensor1, tensors...);
      const auto volume = tensor1.range().volume();

      Scalar result = identity;
      for(decltype(tensor1.range().volume()) i = 0ul; i < volume; i += stride) {
        Scalar temp = identity;
        math::reduce_op(reduce_op, stride, temp,
            tensor1.data() + tensor1.range().ord(i),
            (tensors.data() + tensors.range().ord(i))...);
        join_op(result, temp);
      }

      return result;
    }

    /// Tensor of tensors reduction operation for non-contiguous tensors

    /// Perform an element-wise reduction of the tensors.
    /// \tparam ReduceOp The element-wise reduction operation type
    /// \tparam JoinOp The result operation type
    /// \tparam Scalar A scalar type
    /// \tparam T1 The first argument tensor type
    /// \tparam Ts The argument tensor types
    /// \param reduce_op The element-wise reduction operation
    /// \param join_op The result join operation
    /// \param identity The initial value for the reduction and the result
    /// \param tensor1 The first tensor to be reduced
    /// \param tensors The other tensors to be reduced
    /// \return The reduced value of the tensor(s)
    template <typename ReduceOp, typename JoinOp, typename Scalar, typename T1, typename... Ts,
        enable_if_t<is_numeric<Scalar>::value
            && is_tensor_of_tensor<T1, Ts...>::value
            && ! is_contiguous_tensor<T1, Ts...>::value>* = nullptr>
    Scalar tensor_reduce(ReduceOp&& reduce_op, JoinOp&& join_op,
        const Scalar identity, const T1& tensor1, const Ts&... tensors)
    {
      TA_ASSERT(! empty(tensor1, tensors...));
      TA_ASSERT(is_range_set_congruent(tensor1, tensors...));

      const auto stride = inner_size(tensor1, tensors...);
      const auto volume = tensor1.range().volume();

      auto wrapper_op = [=] (Scalar& restrict result,
          typename T1::const_reference restrict value1,
          typename Ts::const_reference restrict... values)
          { join_op(result, tensor_reduce(reduce_op, join_op, identity, value1, values...)); };

      Scalar result = identity;
      for(decltype(tensor1.range().volume()) i = 0ul; i < volume; i += stride) {
        Scalar temp = identity;
        math::reduce_op(reduce_op, stride, temp,
            tensor1.data() + tensor1.range().ord(i),
            (tensors.data() + tensors.range().ord(i))...);
        join_op(result, temp);
      }

      return identity;
    }

  }  // namespace detail
} // namespace TiledArray

#endif // TILEDARRAY_TENSOR_KENERLS_H__INCLUDED
