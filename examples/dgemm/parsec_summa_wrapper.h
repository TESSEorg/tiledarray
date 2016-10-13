#ifndef PARSEC_SUMMA_WRAPPER_H__INCLUDED
#define PARSEC_SUMMA_WRAPPER_H__INCLUDED

#include "dague.h"
#include "core_blas.h"
#include "summa_s.h"
#include "summa_d.h"
#include "summa_c.h"
#include "summa_z.h"

#include "irregular_tiled_matrix_wrapper.h"

#include <tiledarray.h>

namespace Parsec {

    namespace {
        template <typename Real>
        void summa_destruct(dague_handle_t * handle);
        
        template <>
        void summa_destruct<double>(dague_handle_t * handle) {
            summa_dsumma_Destruct( handle );
        }
        template <>
        void summa_destruct<float>(dague_handle_t * handle) {
            summa_ssumma_Destruct( handle );
        }
        template <>
        void summa_destruct<std::complex<double>>(dague_handle_t * handle) {
            summa_zsumma_Destruct( handle );
        }
        template <>
        void summa_destruct<std::complex<float>>(dague_handle_t * handle) {
            summa_csumma_Destruct( handle );
        }
    }
    
    template<typename BaseType, typename Tile, typename Policy, typename Op>
    class Summa {
    private:
        dague_handle_t  *dague_handle_;
        dague_context_t *dague_context_;

    public:
        Summa(dague_context_t *context) :
            dague_context_(context),
            dague_handle_(NULL) 
        {
        }

        Summa(dague_context_t *context,
              PLASMA_enum transA, PLASMA_enum transB,
              BaseType alpha,
              IrregularTiledMatrix<Tile, Policy, Op> &A,
              IrregularTiledMatrix<Tile, Policy, Op> &B,
              IrregularTiledMatrix<Tile, Policy, Op> &C):
            dague_context_(context),
            dague_handle_(NULL)
        {
            
        }

        ~Summa() {
            if( NULL != dague_handle_ ) {
                summa_destruct<typename Tile::value_type>(dague_handle_);
            }
        }
    }; // class Summa
    
} //namespace Parsec

#endif /* PARSEC_SUMMA_WRAPPER_H__INCLUDED */
