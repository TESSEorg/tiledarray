#ifndef PARSEC_SUMMA_WRAPPER_H__INCLUDED
#define PARSEC_SUMMA_WRAPPER_H__INCLUDED

#include "parsec.h"
#include "core_blas.h"
#include "summa_s.h"
#include "summa_d.h"
#include "summa_c.h"
#include "summa_z.h"

#include "irregular_tiled_matrix_wrapper.h"
#include <madness/world/parsec_madness.h>
#include <tiledarray.h>

namespace Parsec {

    namespace {
        template <typename Real>
        void summa_destruct(parsec_handle_t * handle);
        
        template <>
        void summa_destruct<double>(parsec_handle_t * handle) {
            summa_dsumma_Destruct( handle );
        }
        template <>
        void summa_destruct<float>(parsec_handle_t * handle) {
            summa_ssumma_Destruct( handle );
        }
        template <>
        void summa_destruct<std::complex<double>>(parsec_handle_t * handle) {
            summa_zsumma_Destruct( handle );
        }
        template <>
        void summa_destruct<std::complex<float>>(parsec_handle_t * handle) {
            summa_csumma_Destruct( handle );
        }

        template <typename Real, typename TileA, typename TileB, typename TileC, typename Policy, typename Op>
        struct summa_handle_new_fct {
            static parsec_handle_t *create(PLASMA_enum transA, PLASMA_enum transB,
                                          Real alpha,
                                          IrregularTiledMatrix<TileA, Policy, Op> &A,
                                          IrregularTiledMatrix<TileB, Policy, Op> &B,
                                          IrregularTiledMatrix<TileC, Policy, Op> &C)
            {
                return NULL;
            }
        };

        template <typename TileA, typename TileB, typename TileC, typename Policy, typename Op>
        struct summa_handle_new_fct<double, TileA, TileB, TileC, Policy, Op> {
            static parsec_handle_t *create(PLASMA_enum transA, PLASMA_enum transB,
                                          double alpha,
                                          IrregularTiledMatrix<TileA, Policy, Op> &A,
                                          IrregularTiledMatrix<TileB, Policy, Op> &B,
                                          IrregularTiledMatrix<TileC, Policy, Op> &C)
            {
                return summa_dsumma_New(transA, transB, alpha, A.desc(), B.desc(), C.desc());
            }
        };

        template <typename TileA, typename TileB, typename TileC, typename Policy, typename Op>
        struct summa_handle_new_fct<float, TileA, TileB, TileC, Policy, Op> {
            static parsec_handle_t *create(PLASMA_enum transA, PLASMA_enum transB,
                                          float alpha,
                                          IrregularTiledMatrix<TileA, Policy, Op> &A,
                                          IrregularTiledMatrix<TileB, Policy, Op> &B,
                                          IrregularTiledMatrix<TileC, Policy, Op> &C)
            {
                return summa_ssumma_New(transA, transB, alpha, A.desc(), B.desc(), C.desc());
            }
        };

        template <typename TileA, typename TileB, typename TileC, typename Policy, typename Op>
        struct summa_handle_new_fct<std::complex<double>, TileA, TileB, TileC, Policy, Op> {
            static parsec_handle_t *create(PLASMA_enum transA, PLASMA_enum transB,
                                          std::complex<double> alpha,
                                          IrregularTiledMatrix<TileA, Policy, Op> &A,
                                          IrregularTiledMatrix<TileB, Policy, Op> &B,
                                          IrregularTiledMatrix<TileC, Policy, Op> &C)
            {
                return summa_zsumma_New(transA, transB, alpha, A.desc(), B.desc(), C.desc());
            }
        };

        template <typename TileA, typename TileB, typename TileC, typename Policy, typename Op>
        struct summa_handle_new_fct<std::complex<float>, TileA, TileB, TileC, Policy, Op> {
            static parsec_handle_t *create(PLASMA_enum transA, PLASMA_enum transB,
                                          std::complex<float> alpha,
                                          IrregularTiledMatrix<TileA, Policy, Op> &A,
                                          IrregularTiledMatrix<TileB, Policy, Op> &B,
                                          IrregularTiledMatrix<TileC, Policy, Op> &C)
            {
                return summa_csumma_New(transA, transB, alpha, A.desc(), B.desc(), C.desc());
            }
        };        
    }
    
    template<typename TileA, typename TileB, typename TileC, typename Policy, typename Op>
    class Summa : Handle {
    public:
        typedef typename TileC::value_type value_type;
        
    protected:
        void destruct_handle(void) {
            summa_destruct<value_type>(parsec_handle_);
        }

    public:
        Summa(parsec_context_t *context) : Handle(context)
        {
        }

        Summa(parsec_context_t *context,
              PLASMA_enum transA, PLASMA_enum transB,
              value_type alpha,
              IrregularTiledMatrix<TileA, Policy, Op> &A,
              IrregularTiledMatrix<TileB, Policy, Op> &B,
              IrregularTiledMatrix<TileC, Policy, Op> &C):
            Handle(context)
        {
            parsec_handle_ = summa_handle_new_fct<value_type, TileA, TileB, TileC, Policy, Op>
                ::create(transA, transB, alpha, A, B, C);            
        }

        void run(void) {
            if( NULL == parsec_handle_ || NULL == parsec_context_)
                return;
            /*
            parsec_handle_t *madness_handle = &madness::madness_handle;
            int remaining = parsec_atomic_add_32b(&madness_handle->nb_tasks, -1);
            parsec_check_complete_cb(madness_handle, parsec_context_, remaining);
            parsec_context_wait(parsec_context_);
            std::cout << "Completed MADNESS context" << std::endl;
            */
            parsec_enqueue(parsec_context_, parsec_handle_);
            parsec_context_start(parsec_context_);
            handle_scheduled = true;
            parsec_ptg_handle_wait(parsec_context_, parsec_handle_);
            std::cout << "Completed SUMMA context" << std::endl;
            /*
            if( 0 != parsec_enqueue(parsec_context_, madness_handle) ) {
                std::cerr << "ERROR: parsec_enqueue!!" << std::endl;
            }
            parsec_atomic_add_32b(&madness_handle->nb_tasks, 1);
            if( 0 != parsec_context_start(parsec_context_) ) {
                std::cerr << "ERROR: parsec_context_start!!" << std::endl;
            }
            */
        }

        ~Summa() {
            if( NULL != parsec_handle_ && handle_scheduled ) {
                summa_destruct<value_type>(parsec_handle_);
                parsec_handle_ = NULL;
            }
        }
    }; // class Summa
    
} //namespace Parsec

#endif /* PARSEC_SUMMA_WRAPPER_H__INCLUDED */
