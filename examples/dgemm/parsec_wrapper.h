#ifndef PARSEC_WRAPPER_H__INCLUDED
#define PARSEC_WRAPPER_H__INCLUDED

#include "dague.h"
#include "dague/class/dague_object.h"

#include <tiledarray.h>

namespace Parsec {
    class Handle {
    protected:
        dague_handle_t  *dague_handle_;
        dague_context_t *dague_context_;
        bool             handle_scheduled;

    public:
        Handle(dague_context_t *context) :
            dague_context_(context),
            dague_handle_(NULL),
            handle_scheduled(false)
        {
        }

        ~Handle() {
            if( NULL != dague_handle_ ) {
                /** For a reason, probably because it was not
                 *  scheduled, the handle was not destructed by
                 *  the specialized class. Try to memory collect
                 *  using generic dague destructor */
                OBJ_RELEASE(dague_handle_);
            }
            dague_handle_ = NULL;
            dague_context_ = NULL;
        }
    };
    
    class Parsec {
    private:
        dague_context_t *dague_context_;
        int              nb_cores_;
        void initialize(std::vector<std::string> &v) {
            int argc = v.size();
            char **argv = new char*[argc+2];
            char **p = argv;
            *p++ = strdup("parsec_wrapper");
            for(auto s = v.begin(); s != v.end(); ++s) {
                *p++ = strdup(s->c_str());
            }
            *p = NULL;
            dague_context_ = dague_init(nb_cores_, &argc, &argv);
            for(int a = 0; a < argc; a++)
                free(argv[a]);
            delete[] argv;
        }

    public:
        Parsec(void) {
            std::vector<std::string> empty_args;
            nb_cores_ = -1;
            initialize(empty_args);
        }
        Parsec(int nb_cores) {
            std::vector<std::string> empty_args;
            nb_cores_ = nb_cores;
            initialize(empty_args);
        }
        Parsec(std::vector<std::string> &v) {
            nb_cores_ = -1;
            initialize(v);
        }
        Parsec(int nb_cores, std::vector<std::string> &v) {
            nb_cores_ = nb_cores;
            initialize(v);
        }

        dague_context_t *context(void) {
            return dague_context_;
        }
        
        ~Parsec() {
            dague_fini(&dague_context_);
        }
    }; // class Parsec
    
} //namespace Parsec

#endif /* PARSEC_WRAPPER_H__INCLUDED */
