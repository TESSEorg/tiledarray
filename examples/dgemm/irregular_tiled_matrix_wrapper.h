#ifndef IRREGULAR_TILED_MATRIX_WRAPPER_H__INCLUDED
#define IRREGULAR_TILED_MATRIX_WRAPPER_H__INCLUDED

#include "irregular_tiled_matrix.h"

#include <tiledarray.h>

namespace Parsec {
    
    template <typename Tile, typename Policy, typename Op>
    class IrregularTiledMatrix {
    public:
        typedef typename TiledArray::detail::DistEval<Tile, Policy>::trange_type trange_type; ///< Tiled range type for this object

    private:
        irregular_tiled_matrix_desc_t                  _ddesc;
        std::vector<typename TiledArray::Future<Tile>> _tiles;
        
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
                                             P);
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
                                _tiles[ltile] = de.get(tileid);
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
        ~IrregularTiledMatrix() {
            _tiles.clear();
        }
        
    }; // class IrregularTiledMatrix

} //namespace Parsec

#endif /* IRREGULAR_TILED_MATRIX_WRAPPER_H__INCLUDED */
