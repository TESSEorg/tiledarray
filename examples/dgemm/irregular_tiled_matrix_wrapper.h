#ifndef IRREGULAR_TILED_MATRIX_WRAPPER_H__INCLUDED
#define IRREGULAR_TILED_MATRIX_WRAPPER_H__INCLUDED

#include "irregular_tiled_matrix.h"

#include <tiledarray.h>

extern "C" {
    void *tilearray_future_get_tile(void *f);
}

namespace Parsec {
    
    template <typename Tile, typename Policy, typename Op>
    class IrregularTiledMatrix {
    public:
        typedef typename TiledArray::detail::DistEval<Tile, Policy>::trange_type trange_type; ///< Tiled range type for this object

        /// @return true if all tiles are ready
        bool probe_all() const {
          for(const auto& t: _tiles) {
            if (t.probe() == false) return false;
          }
          return true;
        }

    private:
        irregular_tiled_matrix_desc_t                             _ddesc;
        std::vector< TiledArray::Future < TiledArray::Tensor< double, Eigen::aligned_allocator<double> > > > _tiles;
        
    public:
        IrregularTiledMatrix(TiledArray::detail::DistEval<Tile, Policy> &de, std::size_t P) {
            trange_type tr = de.trange();
            int lm = 0;
            std::vector<std::size_t>idx;
            idx.resize(4);
            std::vector<int> mbs;
            mbs.resize((tr.tiles_range().upbound_data()[0]-tr.tiles_range().lobound_data()[0])*(tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1]));
            for(int i = tr.tiles_range().lobound_data()[0]; i < tr.tiles_range().upbound_data()[0]; i++) {
                idx[0] = i;
                for(int j = tr.tiles_range().lobound_data()[1]; j < tr.tiles_range().upbound_data()[1]; j++) {
                    idx[1] = j;
                    idx[2] = tr.tiles_range().lobound_data()[2];
                    idx[3] = tr.tiles_range().lobound_data()[3];
                    auto t = de.trange().make_tile_range(idx);
                    mbs[i*(tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1])+j] = (t.upbound_data()[0]-t.lobound_data()[0]) * (t.upbound_data()[1]-t.lobound_data()[1]);
                    lm += mbs[i*(tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1])+j];
                }
            }
            int ln = 0;
            std::vector<int> nbs;
            nbs.resize((tr.tiles_range().upbound_data()[2]-tr.tiles_range().lobound_data()[2])*(tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3]));
            idx[0] = tr.tiles_range().lobound_data()[0];
            idx[1] = tr.tiles_range().lobound_data()[1];
            for(int i = tr.tiles_range().lobound_data()[2]; i < tr.tiles_range().upbound_data()[2]; i++) {
                idx[2] = i;
                for(int j = tr.tiles_range().lobound_data()[3]; j < tr.tiles_range().upbound_data()[3]; j++) {
                    idx[3] = j;
                    auto t = de.trange().make_tile_range(idx);
                    nbs[i*(tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3])+j] = (t.upbound_data()[2]-t.lobound_data()[2]) * (t.upbound_data()[3]-t.lobound_data()[3]);
                    ln += nbs[i*(tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3])+j];
                }
            }

            irregular_tiled_matrix_desc_init(&_ddesc, tile_coll_RealDouble,
                                             de.world().nproc(), de.world().rank(),
                                             lm, ln,
                                             (tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1]) * (tr.tiles_range().upbound_data()[0]-tr.tiles_range().lobound_data()[0]),
                                             (tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3]) * (tr.tiles_range().upbound_data()[2]-tr.tiles_range().lobound_data()[2]),
                                             (unsigned int*)mbs.data(), (unsigned int*)nbs.data(),
                                             0, 0,
                                             (tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1]) * (tr.tiles_range().upbound_data()[0]-tr.tiles_range().lobound_data()[0]),
                                             (tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3]) * (tr.tiles_range().upbound_data()[2]-tr.tiles_range().lobound_data()[2]),
                                             P, tilearray_future_get_tile);
            _tiles.resize(de.pmap()->size());
            int ltile = 0;
            for(int i = tr.tiles_range().lobound_data()[0]; i < tr.tiles_range().upbound_data()[0]; i++) {
                idx[0] = i;
                for(int j = tr.tiles_range().lobound_data()[1]; j < tr.tiles_range().upbound_data()[1]; j++) {
                    idx[1] = j;
                    for(int c = tr.tiles_range().lobound_data()[2]; c < tr.tiles_range().upbound_data()[2]; c++) {
                        idx[2] = c;
                        for(int d = tr.tiles_range().lobound_data()[3]; d < tr.tiles_range().upbound_data()[3]; d++) {
                            idx[3] = d;
                            std::size_t tileid = tr.tiles_range().ordinal(idx);
                            if( de.world().rank() == de.pmap()->owner(tileid) ) {
                                // N.B. this can potentially schedule work if tiles are lazy
                                // since there is no way for Parsec to drive this computation
                                // (but this is something we will need ... do Summa on data that
                                // is not ready yet, Parsec will have to drive its evaluation)
                                // let's not worry about this for now
                                _tiles[ltile] = get_tile(de, tileid);
                                irregular_tiled_matrix_desc_set_data(&_ddesc, &_tiles[ltile],
                                                                     i*(tr.tiles_range().upbound_data()[1] - tr.tiles_range().lobound_data()[1]) + j,
                                                                     c*(tr.tiles_range().upbound_data()[3] - tr.tiles_range().lobound_data()[3]) + d,
                                                                     mbs[i*(tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1])+j],
                                                                     nbs[c*(tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3])+d],
                                                                     0, de.world().rank());
                                ltile++;
                            } else {
                                irregular_tiled_matrix_desc_set_data(&_ddesc, NULL, 
                                                                     i*(tr.tiles_range().upbound_data()[1] - tr.tiles_range().lobound_data()[1]) + j,
                                                                     c*(tr.tiles_range().upbound_data()[3] - tr.tiles_range().lobound_data()[3]) + d,
                                                                     mbs[i*(tr.tiles_range().upbound_data()[1]-tr.tiles_range().lobound_data()[1])+j],
                                                                     nbs[c*(tr.tiles_range().upbound_data()[3]-tr.tiles_range().lobound_data()[3])+d],
                                                                     0, de.world().rank());
                            }
                        }
                    }
                }
            }
        }

        irregular_tiled_matrix_desc_t *desc(void) { return &_ddesc; }
        
        ~IrregularTiledMatrix() {
            _tiles.clear();
        }
        
    private:
        /// Returns a Future to the data. This specialization for non-lazy tiles,
        /// so it does nothing
        /// \tparam Arg The type of the argument that holds the input tiles
        /// \param arg The argument that holds the tiles
        /// \param index The tile index of arg
        /// \return \c tile
        template <typename Arg>
        static
        typename std::enable_if<!TiledArray::is_lazy_tile<typename Arg::value_type>::value,
                                TiledArray::Future<typename Arg::eval_type>>::type
            get_tile(Arg& arg, typename Arg::size_type index) {
            return arg.get(index);
        }

        /// Returns a Future to the data.
        /// This function spawns a task that will convert a lazy tile from the
        /// tile type to the evaluated tile type.
        /// \tparam Arg The type of the argument that holds the input tiles
        /// \param arg The argument that holds the tiles
        /// \param index The tile index of arg
        /// \return A future to the evaluated tile
        template <typename Arg>
        static
        typename std::enable_if<TiledArray::is_lazy_tile<typename Arg::value_type>::value,
                                TiledArray::Future<typename Arg::eval_type>>::type
            get_tile(Arg& arg, typename Arg::size_type index) {
            return arg.world().taskq.add(&convert_tile_task,
                                         arg.get(index), madness::TaskAttributes::hipri());
        }

        static typename TiledArray::eval_trait<Tile>::type convert_tile_task(const Tile& tile) { return tile; }

    }; // class IrregularTiledMatrix

} //namespace Parsec

#endif /* IRREGULAR_TILED_MATRIX_WRAPPER_H__INCLUDED */
