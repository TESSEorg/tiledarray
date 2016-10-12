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

        void destruct(void) {
#if 0
            // How do we do that?
            switch( BaseType ) {
            case double:
                summa_dsumma_Destruct( dague_handle_ );
                break;
            case double complex:
                summa_zsumma_Destruct( dague_handle_ );
                break;
            case float:
                summa_ssumma_Destruct( dague_handle_ );
                break;
            case float complex:
                summa_csumma_Destruct( dague_handle_ );
                break;
            default:
            }
#endif
        }
        
        ~Summa() {
            if( NULL != dague_handle_ ) {
                destruct();
            }
        }
    }; // class Summa
    
} //namespace Parsec

#endif /* PARSEC_SUMMA_WRAPPER_H__INCLUDED */
