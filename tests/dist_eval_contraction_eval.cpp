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
 *  dist_eval_contraction_eval.h
 *  Oct 8, 2013
 *
 */

#include "TiledArray/dist_eval/contraction_eval.h"
#include "unit_test_config.h"
#include "TiledArray/array.h"
#include "TiledArray/dist_eval/array_eval.h"
#include "TiledArray/dense_shape.h"
#include "TiledArray/tile_op/contract_reduce.h"
#include "TiledArray/eigen.h"
#include "TiledArray/proc_grid.h"
#include "array_fixture.h"

using namespace TiledArray;

struct ContractionEvalFixture : public TiledRangeFixture {
  typedef Array<int, GlobalFixture::dim> ArrayN;
  typedef math::Noop<ArrayN::value_type::eval_type,
      ArrayN::value_type::eval_type, true> array_op_type;
  typedef detail::DistEval<detail::LazyArrayTile<ArrayN::value_type, array_op_type>,
      DensePolicy> array_eval_type;
  typedef math::ContractReduce<ArrayN::value_type, ArrayN::value_type, ArrayN::value_type> op_type;
  typedef detail::ContractionEvalImpl<array_eval_type, array_eval_type, op_type, DensePolicy> impl_type;
  typedef Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrix_type;

  ContractionEvalFixture() :
    left(*GlobalFixture::world, tr),
    right(*GlobalFixture::world, tr),
    proc_grid(*GlobalFixture::world, tr.tiles().size().front(), tr.tiles().size().back(),
        tr.elements().size().front(), tr.elements().size().back()),
    left_arg(detail::make_array_eval(left, left.get_world(), DenseShape(),
        proc_grid.make_row_phase_pmap(tr.tiles().volume() / tr.tiles().size().front()),
        Permutation(), array_op_type())),
    right_arg(detail::make_array_eval(right, right.get_world(), DenseShape(),
        proc_grid.make_col_phase_pmap(tr.tiles().volume() / tr.tiles().size().back()),
        Permutation(), array_op_type())),
    result_tr(),
    op(madness::cblas::NoTrans, madness::cblas::NoTrans, 1, 2u, tr.tiles().dim(), tr.tiles().dim())
  {
    // Fill arrays with random data
    rand_fill_array(left);
    rand_fill_array(right);

    std::array<TiledRange1, 2ul> tranges =
        {{ left.trange().data().front(), right.trange().data().back() }};
    result_tr = typename impl_type::trange_type(tranges.begin(), tranges.end());

    pmap.reset(new detail::BlockedPmap(* GlobalFixture::world, result_tr.tiles().volume()));
  }

  ~ContractionEvalFixture() {
    GlobalFixture::world->gop.fence();
  }

  template <typename T, unsigned int DIM, typename Tile, typename Policy>
  static void rand_fill_array(Array<T, DIM, Tile, Policy>& array) {
    // Iterate over local, non-zero tiles
    for(typename Array<T, DIM, Tile, Policy>::iterator it = array.begin(); it != array.end(); ++it) {
      // Construct a new tile with random data
      typename Array<T, DIM, Tile, Policy>::value_type tile(array.trange().make_tile_range(it.index()));
      for(typename Array<T, DIM, Tile, Policy>::value_type::iterator tile_it = tile.begin(); tile_it != tile.end(); ++tile_it)
        *tile_it = GlobalFixture::world->rand() % 27;

      // Set array tile
      *it = tile;
    }
  }

  template <typename T, unsigned int DIM, typename Tile, typename Policy>
  static matrix_type copy_to_matrix(const Array<T, DIM, Tile, Policy>& array, const int middle) {

    // Compute the number of rows and columns in the matrix, and a new weight
    // that is bisected the row and column dimensions.
    std::vector<std::size_t> weight(array.range().dim(), 0ul);
    std::size_t MN[2] = { 1ul, 1ul };
    int i = array.range().dim() - 1;
    for(; i >= middle; --i) {
      weight[i] = MN[1];
      MN[1] *= array.trange().elements().size()[i];
    }
    for(; i >= 0; --i) {
      weight[i] = MN[0];
      MN[0] *= array.trange().elements().size()[i];
    }

    // Construct the result matrix
    matrix_type matrix(MN[0], MN[1]);
    matrix.fill(0);

    // Copy tiles from array to matrix
    for(std::size_t index = 0ul; index < array.size(); ++index) {
      if(array.is_zero(index))
        continue;

      // Get tile for index
      const ArrayN::value_type tile = array.find(index);

      // Compute block start and size
      std::size_t start[2] = { 0ul, 0ul }, size[2] = { 1ul, 1ul };
      for(i = 0ul; i < middle; ++i) {
        start[0] += tile.range().start()[i] * weight[i] * size[0];
        size[0] *= tile.range().size()[i];
      }
      for(; i < array.range().dim(); ++i) {
        start[1] += tile.range().start()[i] * weight[i] * size[1];
        size[1] *= tile.range().size()[i];
      }

      // Copy tile into matrix sub-block
      matrix.block(start[0], start[1], size[0], size[1]) = eigen_map(tile, size[0], size[1]);
    }

    return matrix;
  }

  static SparseShape<float> make_shape(const Range& range, const float fill_percent, const int seed) {
    GlobalFixture::world->srand(seed);
    float max = 0.0f;
    const float threshold = fill_percent * 27.0f;
    Tensor<float> shape_data(range);
    for(std::size_t i = 0ul; i < range.volume(); ++i) {
      shape_data[i] = GlobalFixture::world->rand();
      max = std::max(max, shape_data[i]);
    }

    shape_data *= 27.0f / max;

    return SparseShape<float>(shape_data, threshold);
  }

  ArrayN left;
  ArrayN right;
  detail::ProcGrid proc_grid;
  array_eval_type left_arg;
  array_eval_type right_arg;
  typename impl_type::trange_type result_tr;
  std::shared_ptr<impl_type::pmap_interface> pmap;
  op_type op;

}; // ContractionEvalFixture

BOOST_FIXTURE_TEST_SUITE( dist_eval_contraction_eval_suite, ContractionEvalFixture )

BOOST_AUTO_TEST_CASE( constructor )
{
  typedef detail::DistEval<op_type::result_type, DensePolicy> dist_eval_type1;

  BOOST_REQUIRE_NO_THROW(detail::make_contract_eval(left_arg, right_arg, left.get_world(),
      DenseShape(), pmap, Permutation(), op));


  dist_eval_type1 contract = detail::make_contract_eval(left_arg, right_arg,
      left_arg.get_world(), DenseShape(), pmap, Permutation(), op);

  BOOST_CHECK_EQUAL(& contract.get_world(), GlobalFixture::world);
  BOOST_CHECK(contract.pmap() == pmap);
  BOOST_CHECK_EQUAL(contract.range(), result_tr.tiles());
  BOOST_CHECK_EQUAL(contract.trange(), result_tr);
  BOOST_CHECK_EQUAL(contract.size(), result_tr.tiles().volume());
  BOOST_CHECK(contract.is_dense());
  for(std::size_t i = 0; i < result_tr.tiles().volume(); ++i)
    BOOST_CHECK(! contract.is_zero(i));

}


BOOST_AUTO_TEST_CASE( eval )
{
  typedef detail::DistEval<op_type::result_type, DensePolicy> dist_eval_type1;

  dist_eval_type1 contract = detail::make_contract_eval(left_arg, right_arg,
      left_arg.get_world(), DenseShape(), pmap, Permutation(), op);

  // Check evaluation
  BOOST_REQUIRE_NO_THROW(contract.eval());
  BOOST_REQUIRE_NO_THROW(contract.wait());


  // Compute the reference contraction
  const matrix_type l = copy_to_matrix(left, 1), r = copy_to_matrix(right, GlobalFixture::dim - 1);
  const matrix_type reference = l * r;

  dist_eval_type1::pmap_interface::const_iterator it = contract.pmap()->begin();
  const dist_eval_type1::pmap_interface::const_iterator end = contract.pmap()->end();

  // Check that each tile has been properly scaled.
  for(; it != end; ++it) {

    // Get the array evaluator tile.
    madness::Future<dist_eval_type1::value_type> tile;
    BOOST_REQUIRE_NO_THROW(tile = contract.move(*it));

    // Force the evaluation of the tile
    dist_eval_type1::eval_type eval_tile;
    BOOST_REQUIRE_NO_THROW(eval_tile = tile.get());
    BOOST_CHECK(! eval_tile.empty());

    if(!eval_tile.empty()) {
      // Check that the result tile is correctly modified.
      BOOST_CHECK_EQUAL(eval_tile.range(), contract.trange().make_tile_range(*it));
      BOOST_CHECK(eigen_map(eval_tile) == reference.block(eval_tile.range().start()[0],
          eval_tile.range().start()[1], eval_tile.range().size()[0], eval_tile.range().size()[1]));
    }
  }

}


BOOST_AUTO_TEST_CASE( perm_eval )
{
  typedef detail::DistEval<op_type::result_type, DensePolicy> dist_eval_type1;
  Permutation perm(1,0);
  op_type pop(madness::cblas::NoTrans, madness::cblas::NoTrans, 1, 2u, tr.tiles().dim(), tr.tiles().dim(), perm);

  dist_eval_type1 contract = detail::make_contract_eval(left_arg, right_arg,
      left_arg.get_world(), DenseShape(), pmap, perm, pop);

  // Check evaluation
  BOOST_REQUIRE_NO_THROW(contract.eval());
  BOOST_REQUIRE_NO_THROW(contract.wait());


  // Compute the reference contraction
  const matrix_type l = copy_to_matrix(left, 1), r = copy_to_matrix(right, GlobalFixture::dim - 1);
  const matrix_type reference = (l * r).transpose();

  dist_eval_type1::pmap_interface::const_iterator it = contract.pmap()->begin();
  const dist_eval_type1::pmap_interface::const_iterator end = contract.pmap()->end();

  // Check that each tile has been properly scaled.
  for(; it != end; ++it) {

    // Get the array evaluator tile.
    madness::Future<dist_eval_type1::value_type> tile;
    BOOST_REQUIRE_NO_THROW(tile = contract.move(*it));

    // Force the evaluation of the tile
    dist_eval_type1::eval_type eval_tile;
    BOOST_REQUIRE_NO_THROW(eval_tile = tile.get());
    BOOST_CHECK(! eval_tile.empty());

    if(!eval_tile.empty()) {
      // Check that the result tile is correctly modified.
      BOOST_CHECK_EQUAL(eval_tile.range(), contract.trange().make_tile_range(*it));
      BOOST_CHECK(eigen_map(eval_tile) == reference.block(eval_tile.range().start()[0],
          eval_tile.range().start()[1], eval_tile.range().size()[0], eval_tile.range().size()[1]));
    }
  }

}

BOOST_AUTO_TEST_CASE( sparse_eval )
{
  typedef detail::DistEval<op_type::result_type, SparsePolicy> dist_eval_type1;
  typedef Array<int, GlobalFixture::dim, Tensor<int>, SparsePolicy> array_type;
  typedef detail::DistEval<detail::LazyArrayTile<array_type::value_type, array_op_type>,
      SparsePolicy> array_eval_type;

  array_type left(*GlobalFixture::world, tr, make_shape(tr.tiles(), 0.1, 23));

  array_type right(*GlobalFixture::world, tr, make_shape(tr.tiles(), 0.1, 42));

  // Fill arrays with random data
  rand_fill_array(left);
  rand_fill_array(right);

  array_eval_type left_arg(detail::make_array_eval(left, left.get_world(), left.get_shape(),
      proc_grid.make_row_phase_pmap(tr.tiles().volume() / tr.tiles().size().front()),
      Permutation(), array_op_type()));
  array_eval_type right_arg(detail::make_array_eval(right, right.get_world(), right.get_shape(),
      proc_grid.make_col_phase_pmap(tr.tiles().volume() / tr.tiles().size().back()),
      Permutation(), array_op_type()));

  const SparseShape<float> result_shape = left_arg.shape().gemm(right_arg.shape(), 1, op.gemm_helper());

  dist_eval_type1 contract = detail::make_contract_eval(left_arg, right_arg,
      left_arg.get_world(), result_shape, pmap, Permutation(), op);

  // Check evaluation
  BOOST_REQUIRE_NO_THROW(contract.eval());
  BOOST_REQUIRE_NO_THROW(contract.wait());

  // Compute the reference contraction
  const matrix_type l = copy_to_matrix(left, 1), r = copy_to_matrix(right, GlobalFixture::dim - 1);
  const matrix_type reference = l * r;

  dist_eval_type1::pmap_interface::const_iterator it = contract.pmap()->begin();
  const dist_eval_type1::pmap_interface::const_iterator end = contract.pmap()->end();

  // Check that each tile has been properly scaled.
  for(; it != end; ++it) {
    // Skip zero tiles
    if(contract.is_zero(*it)) {
      dist_eval_type1::range_type range = contract.trange().make_tile_range(*it);

      BOOST_CHECK((reference.block(range.start()[0], range.start()[1],
          range.size()[0], range.size()[1]).array() == 0).all());

    } else {
      // Get the array evaluator tile.
      madness::Future<dist_eval_type1::value_type> tile;
      BOOST_REQUIRE_NO_THROW(tile = contract.move(*it));

      // Force the evaluation of the tile
      dist_eval_type1::eval_type eval_tile;
      BOOST_REQUIRE_NO_THROW(eval_tile = tile.get());
      BOOST_CHECK(! eval_tile.empty());

      if(!eval_tile.empty()) {
        // Check that the result tile is correctly modified.
        BOOST_CHECK_EQUAL(eval_tile.range(), contract.trange().make_tile_range(*it));
        BOOST_CHECK(eigen_map(eval_tile) == reference.block(eval_tile.range().start()[0],
            eval_tile.range().start()[1], eval_tile.range().size()[0], eval_tile.range().size()[1]));
      }
    }
  }

}

BOOST_AUTO_TEST_SUITE_END()
