#ifndef PARSEC_WRAPPER_H__INCLUDED
#define PARSEC_WRAPPER_H__INCLUDED

#include "dague.h"

#include <tiledarray.h>

namespace Parsec {
    class Parsec {
    private:
        dague_context_t *dague_context_;
        void initialize(std::vector<std::string> &v) {
            int argc = v.size();
            char **argv = new char*[argc+2];
            char **p = argv;
            *p++ = strdup("parsec_wrapper");
            for(auto s = v.begin(); s != v.end(); ++s) {
                *p++ = strdup(s->c_str());
            }
            *p = NULL;
            dague_context_ = dague_init(-1, &argc, &argv);
            for(int a = 0; a < argc; a++)
                free(argv[a]);
            delete[] argv;
        }

    public:
        Parsec(void) {
            std::vector<std::string> empty_args;
            initialize(empty_args);
        }
        Parsec(std::vector<std::string> &v) {
            initialize(v);
        }
        
        ~Parsec() {
            dague_fini(&dague_context_);
        }
    }; // class Parsec
    
} //namespace Parsec

#endif /* PARSEC_WRAPPER_H__INCLUDED */
