#ifndef _DMACROS_H
#define _DMACROS_H
#include <boost/preprocessor/seq/fold_right.hpp>

#define PRE_DECLARE_NAMESPACE(s, state, x) namespace x{ state }
#define PRE_DECLARE_CLASS_SIMPLE(c) class c;
#define PRE_DECLARE_CLASS(c, seq) BOOST_PP_SEQ_FOLD_RIGHT(PRE_DECLARE_NAMESPACE,PRE_DECLARE_CLASS_SIMPLE(c), seq)

#define PRE_DECLARE_STRUCT_SIMPLE(c) class c;
#define PRE_DECLARE_STRUCT(c, seq) BOOST_PP_SEQ_FOLD_RIGHT(PRE_DECLARE_NAMESPACE,PRE_DECLARE_STRUCT_SIMPLE(c), seq)

#endif
