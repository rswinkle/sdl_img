/*

File Browser 0.80 MIT licensed library for browsing a file system
https://github.com/rswinkle/file_browser
robertwinkler.com


Do this:
    #define FILE_BROWSER_IMPLEMENTATION
before you include this file in *one* C or C++ file to create the implementation.


TODO NOTES/DOCS/example code

For now see the two example programs in the github repo


The MIT License (MIT)

Copyright (c) 2017-2025 Robert Winkler

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

*/

#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef FILEBROWSER_H
#define FILEBROWSER_H


// pulls in cvector
#ifndef FILE_H
#define FILE_H


#ifndef MYINTTYPES_H
#define MYINTTYPES_H

#include <stdint.h>
#include <inttypes.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#endif



#define CVEC_ONLY_INT
#define CVEC_ONLY_STR
#define CVEC_SIZE_T i64
#define PRIcv_sz PRIiMAX

#ifndef CVECTOR_H
#define CVECTOR_H

#if defined(CVEC_ONLY_INT) || defined(CVEC_ONLY_DOUBLE) || defined(CVEC_ONLY_STR) || defined(CVEC_ONLY_VOID)
   #ifndef CVEC_ONLY_INT
   #define CVEC_NO_INT
   #endif
   #ifndef CVEC_ONLY_DOUBLE
   #define CVEC_NO_DOUBLE
   #endif
   #ifndef CVEC_ONLY_STR
   #define CVEC_NO_STR
   #endif
   #ifndef CVEC_ONLY_VOID
   #define CVEC_NO_VOID
   #endif
#endif

#if defined(CVEC_MALLOC) && defined(CVEC_FREE) && defined(CVEC_REALLOC)
/* ok */
#elif !defined(CVEC_MALLOC) && !defined(CVEC_FREE) && !defined(CVEC_REALLOC)
/* ok */
#else
#error "Must define all or none of CVEC_MALLOC, CVEC_FREE, and CVEC_REALLOC."
#endif

#ifndef CVEC_MALLOC
#include <stdlib.h>
#define CVEC_MALLOC(sz)      malloc(sz)
#define CVEC_REALLOC(p, sz)  realloc(p, sz)
#define CVEC_FREE(p)         free(p)
#endif

#ifndef CVEC_MEMMOVE
#include <string.h>
#define CVEC_MEMMOVE(dst, src, sz)  memmove(dst, src, sz)
#endif

#ifndef CVEC_ASSERT
#include <assert.h>
#define CVEC_ASSERT(x)       assert(x)
#endif

#ifndef CVEC_SIZE_T
#include <stdlib.h>
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CVEC_NO_INT



/** Data structure for int vector. */
typedef struct cvector_i
{
	int* a;            /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_i;

extern cvec_sz CVEC_I_START_SZ;

int cvec_i(cvector_i* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_i(cvector_i* vec, int* vals, cvec_sz num);

cvector_i* cvec_i_heap(cvec_sz size, cvec_sz capacity);
cvector_i* cvec_init_i_heap(int* vals, cvec_sz num);
int cvec_copyc_i(void* dest, void* src);
int cvec_copy_i(cvector_i* dest, cvector_i* src);

int cvec_push_i(cvector_i* vec, int a);
int cvec_pop_i(cvector_i* vec);

int cvec_extend_i(cvector_i* vec, cvec_sz num);
int cvec_insert_i(cvector_i* vec, cvec_sz i, int a);
int cvec_insert_array_i(cvector_i* vec, cvec_sz i, int* a, cvec_sz num);
int cvec_replace_i(cvector_i* vec, cvec_sz i, int a);
void cvec_erase_i(cvector_i* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_i(cvector_i* vec, cvec_sz size);
#define cvec_shrink_to_fit_i(vec) cvec_set_cap_i((vec), (vec)->size)
int cvec_set_cap_i(cvector_i* vec, cvec_sz size);
void cvec_set_val_sz_i(cvector_i* vec, int val);
void cvec_set_val_cap_i(cvector_i* vec, int val);

int* cvec_back_i(cvector_i* vec);

void cvec_clear_i(cvector_i* vec);
void cvec_free_i_heap(void* vec);
void cvec_free_i(void* vec);

#endif

#ifndef CVEC_NO_DOUBLE



/** Data structure for double vector. */
typedef struct cvector_d
{
	double* a;         /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_d;

extern cvec_sz CVEC_D_START_SZ;

int cvec_d(cvector_d* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_d(cvector_d* vec, double* vals, cvec_sz num);

cvector_d* cvec_d_heap(cvec_sz size, cvec_sz capacity);
cvector_d* cvec_init_d_heap(double* vals, cvec_sz num);
int cvec_copyc_d(void* dest, void* src);
int cvec_copy_d(cvector_d* dest, cvector_d* src);

int cvec_push_d(cvector_d* vec, double a);
double cvec_pop_d(cvector_d* vec);

int cvec_extend_d(cvector_d* vec, cvec_sz num);
int cvec_insert_d(cvector_d* vec, cvec_sz i, double a);
int cvec_insert_array_d(cvector_d* vec, cvec_sz i, double* a, cvec_sz num);
double cvec_replace_d(cvector_d* vec, cvec_sz i, double a);
void cvec_erase_d(cvector_d* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_d(cvector_d* vec, cvec_sz size);
#define cvec_shrink_to_fit_d(vec) cvec_set_cap_d((vec), (vec)->size)
int cvec_set_cap_d(cvector_d* vec, cvec_sz size);
void cvec_set_val_sz_d(cvector_d* vec, double val);
void cvec_set_val_cap_d(cvector_d* vec, double val);

double* cvec_back_d(cvector_d* vec);

void cvec_clear_d(cvector_d* vec);
void cvec_free_d_heap(void* vec);
void cvec_free_d(void* vec);

#endif

#ifndef CVEC_NO_STR



/** Data structure for string vector. */
typedef struct cvector_str
{
	char** a;          /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_str;

extern cvec_sz CVEC_STR_START_SZ;

#ifndef CVEC_STRDUP
#define CVEC_STRDUP cvec_strdup
char* cvec_strdup(const char* str);
#endif

int cvec_str(cvector_str* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_str(cvector_str* vec, char** vals, cvec_sz num);

cvector_str* cvec_str_heap(cvec_sz size, cvec_sz capacity);
cvector_str* cvec_init_str_heap(char** vals, cvec_sz num);
int cvec_copyc_str(void* dest, void* src);
int cvec_copy_str(cvector_str* dest, cvector_str* src);

int cvec_push_str(cvector_str* vec, char* a);
void cvec_pop_str(cvector_str* vec, char* ret);

int cvec_pushm_str(cvector_str* vec, char* a);
#define cvec_popm_str(vec) (vec).a[--(vec).size]
int cvec_insertm_str(cvector_str* vec, cvec_sz i, char* a);
int cvec_insert_arraym_str(cvector_str* vec, cvec_sz i, char** a, cvec_sz num);
#define cvec_replacem_str(vec, i, s, ret) ((ret) = (vec).a[i], (vec).a[i] = (s))

int cvec_extend_str(cvector_str* vec, cvec_sz num);
int cvec_insert_str(cvector_str* vec, cvec_sz i, char* a);
int cvec_insert_array_str(cvector_str* vec, cvec_sz i, char** a, cvec_sz num);
void cvec_replace_str(cvector_str* vec, cvec_sz i, char* a, char* ret);
void cvec_erase_str(cvector_str* vec, cvec_sz start, cvec_sz end);
void cvec_remove_str(cvector_str* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_str(cvector_str* vec, cvec_sz size);
#define cvec_shrink_to_fit_str(vec) cvec_set_cap_str((vec), (vec)->size)
int cvec_set_cap_str(cvector_str* vec, cvec_sz size);
void cvec_set_val_sz_str(cvector_str* vec, char* val);
void cvec_set_val_cap_str(cvector_str* vec, char* val);

char** cvec_back_str(cvector_str* vec);

void cvec_clear_str(cvector_str* vec);
void cvec_free_str_heap(void* vec);
void cvec_free_str(void* vec);

#endif

#ifndef CVEC_NO_VOID



typedef unsigned char cvec_u8;

/** Data structure for generic type (cast to void) vectors */
typedef struct cvector_void
{
	cvec_u8* a;                 /**< Array. */
	cvec_sz size;             /**< Current size (amount you should use when manipulating array directly). */
	cvec_sz capacity;         /**< Allocated size of array; always >= size. */
	cvec_sz elem_size;        /**< Size in bytes of type stored (sizeof(T) where T is type). */
	void (*elem_free)(void*);
	int (*elem_init)(void*, void*);
} cvector_void;

extern cvec_sz CVEC_VOID_START_SZ;

#define CVEC_GET_VOID(VEC, TYPE, I) ((TYPE*)&(VEC)->a[(I)*(VEC)->elem_size])

int cvec_void(cvector_void* vec, cvec_sz size, cvec_sz capacity, cvec_sz elem_sz, void(*elem_free)(void*), int(*elem_init)(void*, void*));
int cvec_init_void(cvector_void* vec, void* vals, cvec_sz num, cvec_sz elem_sz, void(*elem_free)(void*), int(*elem_init)(void*, void*));

cvector_void* cvec_void_heap(cvec_sz size, cvec_sz capacity, cvec_sz elem_sz, void (*elem_free)(void*), int(*elem_init)(void*, void*));
cvector_void* cvec_init_void_heap(void* vals, cvec_sz num, cvec_sz elem_sz, void (*elem_free)(void*), int(*elem_init)(void*, void*));

int cvec_copyc_void(void* dest, void* src);
int cvec_copy_void(cvector_void* dest, cvector_void* src);

int cvec_push_void(cvector_void* vec, void* a);
void cvec_pop_void(cvector_void* vec, void* ret);
void* cvec_get_void(cvector_void* vec, cvec_sz i);

int cvec_pushm_void(cvector_void* vec, void* a);
void cvec_popm_void(cvector_void* vec, void* ret);
int cvec_insertm_void(cvector_void* vec, cvec_sz i, void* a);
int cvec_insert_arraym_void(cvector_void* vec, cvec_sz i, void* a, cvec_sz num);
void cvec_replacem_void(cvector_void* vec, cvec_sz i, void* a, void* ret);

int cvec_extend_void(cvector_void* vec, cvec_sz num);
int cvec_insert_void(cvector_void* vec, cvec_sz i, void* a);
int cvec_insert_array_void(cvector_void* vec, cvec_sz i, void* a, cvec_sz num);
int cvec_replace_void(cvector_void* vec, cvec_sz i, void* a, void* ret);
void cvec_erase_void(cvector_void* vec, cvec_sz start, cvec_sz end);
void cvec_remove_void(cvector_void* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_void(cvector_void* vec, cvec_sz size);
#define cvec_shrink_to_fit_void(vec) cvec_set_cap_void((vec), (vec)->size)
int cvec_set_cap_void(cvector_void* vec, cvec_sz size);
int cvec_set_val_sz_void(cvector_void* vec, void* val);
int cvec_set_val_cap_void(cvector_void* vec, void* val);

void* cvec_back_void(cvector_void* vec);

void cvec_clear_void(cvector_void* vec);
void cvec_free_void_heap(void* vec);
void cvec_free_void(void* vec);

#endif


#ifdef __cplusplus
}
#endif


#define CVEC_NEW_DECLS(TYPE)                                                          \
  typedef struct cvector_##TYPE {                                                     \
    TYPE* a;                                                                          \
    cvec_sz size;                                                                     \
    cvec_sz capacity;                                                                 \
  } cvector_##TYPE;                                                                   \
                                                                                      \
  extern cvec_sz CVEC_##TYPE##_SZ;                                                    \
                                                                                      \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity);               \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num);                 \
                                                                                      \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity);                 \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num);                   \
                                                                                      \
  int cvec_copyc_##TYPE(void* dest, void* src);                                       \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src);                    \
                                                                                      \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE a);                                  \
  TYPE cvec_pop_##TYPE(cvector_##TYPE* vec);                                          \
                                                                                      \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num);                           \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a);                     \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num); \
  TYPE cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a);                   \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end);            \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size);                         \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size);                         \
  void cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE val);                         \
  void cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE val);                        \
                                                                                      \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec);                                        \
                                                                                      \
  void cvec_clear_##TYPE(cvector_##TYPE* vec);                                        \
  void cvec_free_##TYPE##_heap(void* vec);                                            \
  void cvec_free_##TYPE(void* vec);

#define CVEC_NEW_DEFS(TYPE, RESIZE_MACRO)                                                   \
  cvec_sz CVEC_##TYPE##_SZ = 50;                                                            \
                                                                                            \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity)                        \
  {                                                                                         \
    cvector_##TYPE* vec;                                                                    \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                    \
      CVEC_ASSERT(vec != NULL);                                                             \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    vec->size     = size;                                                                   \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))          \
                        ? capacity                                                          \
                        : vec->size + CVEC_##TYPE##_SZ;                                     \
                                                                                            \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      CVEC_FREE(vec);                                                                       \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    return vec;                                                                             \
  }                                                                                         \
                                                                                            \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num)                          \
  {                                                                                         \
    cvector_##TYPE* vec;                                                                    \
                                                                                            \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                    \
      CVEC_ASSERT(vec != NULL);                                                             \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                 \
    vec->size     = num;                                                                    \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      CVEC_FREE(vec);                                                                       \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                         \
                                                                                            \
    return vec;                                                                             \
  }                                                                                         \
                                                                                            \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity)                      \
  {                                                                                         \
    vec->size     = size;                                                                   \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))          \
                        ? capacity                                                          \
                        : vec->size + CVEC_##TYPE##_SZ;                                     \
                                                                                            \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      vec->size = vec->capacity = 0;                                                        \
      return 0;                                                                             \
    }                                                                                       \
                                                                                            \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num)                        \
  {                                                                                         \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                 \
    vec->size     = num;                                                                    \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      vec->size = vec->capacity = 0;                                                        \
      return 0;                                                                             \
    }                                                                                       \
                                                                                            \
    CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                         \
                                                                                            \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_copyc_##TYPE(void* dest, void* src)                                              \
  {                                                                                         \
    cvector_##TYPE* vec1 = (cvector_##TYPE*)dest;                                           \
    cvector_##TYPE* vec2 = (cvector_##TYPE*)src;                                            \
                                                                                            \
    vec1->a = NULL;                                                                         \
    vec1->size = 0;                                                                         \
    vec1->capacity = 0;                                                                     \
                                                                                            \
    return cvec_copy_##TYPE(vec1, vec2);                                                    \
  }                                                                                         \
                                                                                            \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src)                           \
  {                                                                                         \
    TYPE* tmp = NULL;                                                                       \
    if (!(tmp = (TYPE*)CVEC_REALLOC(dest->a, src->capacity*sizeof(TYPE)))) {                \
      CVEC_ASSERT(tmp != NULL);                                                             \
      return 0;                                                                             \
    }                                                                                       \
    dest->a = tmp;                                                                          \
                                                                                            \
    CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(TYPE));                                  \
    dest->size = src->size;                                                                 \
    dest->capacity = src->capacity;                                                         \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE a)                                         \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity > vec->size) {                                                        \
      vec->a[vec->size++] = a;                                                              \
    } else {                                                                                \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                 \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a              = tmp;                                                            \
      vec->a[vec->size++] = a;                                                              \
      vec->capacity       = tmp_sz;                                                         \
    }                                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  TYPE cvec_pop_##TYPE(cvector_##TYPE* vec) { return vec->a[--vec->size]; }                 \
                                                                                            \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec) { return &vec->a[vec->size - 1]; }            \
                                                                                            \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num)                                  \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity < vec->size + num) {                                                  \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a        = tmp;                                                                  \
      vec->capacity = tmp_sz;                                                               \
    }                                                                                       \
                                                                                            \
    vec->size += num;                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a)                            \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity > vec->size) {                                                        \
      CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));             \
      vec->a[i] = a;                                                                        \
    } else {                                                                                \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                 \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a = tmp;                                                                         \
      CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));             \
      vec->a[i]     = a;                                                                    \
      vec->capacity = tmp_sz;                                                               \
    }                                                                                       \
                                                                                            \
    vec->size++;                                                                            \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)        \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity < vec->size + num) {                                                  \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a        = tmp;                                                                  \
      vec->capacity = tmp_sz;                                                               \
    }                                                                                       \
                                                                                            \
    CVEC_MEMMOVE(&vec->a[i + num], &vec->a[i], (vec->size - i) * sizeof(TYPE));             \
    CVEC_MEMMOVE(&vec->a[i], a, num * sizeof(TYPE));                                        \
    vec->size += num;                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  TYPE cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a)                          \
  {                                                                                         \
    TYPE tmp  = vec->a[i];                                                                  \
    vec->a[i] = a;                                                                          \
    return tmp;                                                                             \
  }                                                                                         \
                                                                                            \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end)                   \
  {                                                                                         \
    cvec_sz d = end - start + 1;                                                            \
    CVEC_MEMMOVE(&vec->a[start], &vec->a[end + 1], (vec->size - 1 - end) * sizeof(TYPE));   \
    vec->size -= d;                                                                         \
  }                                                                                         \
                                                                                            \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    if (vec->capacity < size) {                                                             \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * (size + CVEC_##TYPE##_SZ)))) { \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a        = tmp;                                                                  \
      vec->capacity = size + CVEC_##TYPE##_SZ;                                              \
    }                                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    if (size < vec->size) {                                                                 \
      vec->size = size;                                                                     \
    }                                                                                       \
                                                                                            \
    if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * size))) {                        \
      CVEC_ASSERT(tmp != NULL);                                                             \
      return 0;                                                                             \
    }                                                                                       \
    vec->a        = tmp;                                                                    \
    vec->capacity = size;                                                                   \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  void cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE val)                                \
  {                                                                                         \
    cvec_sz i;                                                                              \
    for (i = 0; i < vec->size; i++) {                                                       \
      vec->a[i] = val;                                                                      \
    }                                                                                       \
  }                                                                                         \
                                                                                            \
  void cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE val)                               \
  {                                                                                         \
    cvec_sz i;                                                                              \
    for (i = 0; i < vec->capacity; i++) {                                                   \
      vec->a[i] = val;                                                                      \
    }                                                                                       \
  }                                                                                         \
                                                                                            \
  void cvec_clear_##TYPE(cvector_##TYPE* vec) { vec->size = 0; }                            \
                                                                                            \
  void cvec_free_##TYPE##_heap(void* vec)                                                   \
  {                                                                                         \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                             \
    if (!tmp) return;                                                                       \
    CVEC_FREE(tmp->a);                                                                      \
    CVEC_FREE(tmp);                                                                         \
  }                                                                                         \
                                                                                            \
  void cvec_free_##TYPE(void* vec)                                                          \
  {                                                                                         \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                             \
    CVEC_FREE(tmp->a);                                                                      \
    tmp->a        = NULL;                                                                   \
    tmp->size     = 0;                                                                      \
    tmp->capacity = 0;                                                                      \
  }

#define CVEC_NEW_DECLS2(TYPE)                                                                    \
  typedef struct cvector_##TYPE {                                                                \
    TYPE* a;                                                                                     \
    cvec_sz size;                                                                                \
    cvec_sz capacity;                                                                            \
    void (*elem_free)(void*);                                                                    \
    int (*elem_init)(void*, void*);                                                              \
  } cvector_##TYPE;                                                                              \
                                                                                                 \
  extern cvec_sz CVEC_##TYPE##_SZ;                                                               \
                                                                                                 \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity, void (*elem_free)(void*), \
                  int (*elem_init)(void*, void*));                                               \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num, void (*elem_free)(void*),   \
                       int (*elem_init)(void*, void*));                                          \
                                                                                                 \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity, void (*elem_free)(void*),   \
                                     int (*elem_init)(void*, void*));                            \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num, void (*elem_free)(void*),     \
                                          int (*elem_init)(void*, void*));                       \
                                                                                                 \
  int cvec_copyc_##TYPE(void* dest, void* src);                                                  \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src);                               \
                                                                                                 \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE* val);                                          \
  void cvec_pop_##TYPE(cvector_##TYPE* vec, TYPE* ret);                                          \
                                                                                                 \
  int cvec_pushm_##TYPE(cvector_##TYPE* vec, TYPE* a);                                           \
  void cvec_popm_##TYPE(cvector_##TYPE* vec, TYPE* ret);                                         \
  int cvec_insertm_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a);                              \
  int cvec_insert_arraym_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num);           \
  void cvec_replacem_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret);                 \
                                                                                                 \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num);                                      \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a);                               \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num);            \
  int cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret);                   \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end);                       \
  void cvec_remove_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end);                      \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size);                                    \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size);                                    \
  int cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE* val);                                    \
  int cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE* val);                                   \
                                                                                                 \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec);                                                   \
                                                                                                 \
  void cvec_clear_##TYPE(cvector_##TYPE* vec);                                                   \
  void cvec_free_##TYPE##_heap(void* vec);                                                       \
  void cvec_free_##TYPE(void* vec);

#define CVEC_NEW_DEFS2(TYPE, RESIZE_MACRO)                                                       \
  cvec_sz CVEC_##TYPE##_SZ = 20;                                                                 \
                                                                                                 \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity, void (*elem_free)(void*),   \
                                     int (*elem_init)(void*, void*))                             \
  {                                                                                              \
    cvector_##TYPE* vec;                                                                         \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                         \
      CVEC_ASSERT(vec != NULL);                                                                  \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    vec->size     = size;                                                                        \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))               \
                        ? capacity                                                               \
                        : vec->size + CVEC_##TYPE##_SZ;                                          \
                                                                                                 \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      CVEC_FREE(vec);                                                                            \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return vec;                                                                                  \
  }                                                                                              \
                                                                                                 \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num, void (*elem_free)(void*),     \
                                          int (*elem_init)(void*, void*))                        \
  {                                                                                              \
    cvector_##TYPE* vec;                                                                         \
    cvec_sz i;                                                                                   \
                                                                                                 \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                         \
      CVEC_ASSERT(vec != NULL);                                                                  \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                      \
    vec->size     = num;                                                                         \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      CVEC_FREE(vec);                                                                            \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    if (elem_init) {                                                                             \
      for (i = 0; i < num; ++i) {                                                                \
        if (!elem_init(&vec->a[i], &vals[i])) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          CVEC_FREE(vec->a);                                                                     \
          CVEC_FREE(vec);                                                                        \
          return NULL;                                                                           \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                            \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return vec;                                                                                  \
  }                                                                                              \
                                                                                                 \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity, void (*elem_free)(void*), \
                  int (*elem_init)(void*, void*))                                                \
  {                                                                                              \
    vec->size     = size;                                                                        \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))               \
                        ? capacity                                                               \
                        : vec->size + CVEC_##TYPE##_SZ;                                          \
                                                                                                 \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      vec->size = vec->capacity = 0;                                                             \
      return 0;                                                                                  \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num, void (*elem_free)(void*),   \
                       int (*elem_init)(void*, void*))                                           \
  {                                                                                              \
    cvec_sz i;                                                                                   \
                                                                                                 \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                      \
    vec->size     = num;                                                                         \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      vec->size = vec->capacity = 0;                                                             \
      return 0;                                                                                  \
    }                                                                                            \
                                                                                                 \
    if (elem_init) {                                                                             \
      for (i = 0; i < num; ++i) {                                                                \
        if (!elem_init(&vec->a[i], &vals[i])) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                            \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_copyc_##TYPE(void* dest, void* src)                                                   \
  {                                                                                              \
    cvector_##TYPE* vec1 = (cvector_##TYPE*)dest;                                                \
    cvector_##TYPE* vec2 = (cvector_##TYPE*)src;                                                 \
                                                                                                 \
    vec1->a = NULL;                                                                              \
    vec1->size = 0;                                                                              \
    vec1->capacity = 0;                                                                          \
                                                                                                 \
    return cvec_copy_##TYPE(vec1, vec2);                                                         \
  }                                                                                              \
                                                                                                 \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src)                                \
  {                                                                                              \
    int i;                                                                                       \
    TYPE* tmp = NULL;                                                                            \
    if (!(tmp = (TYPE*)CVEC_REALLOC(dest->a, src->capacity*sizeof(TYPE)))) {                     \
      CVEC_ASSERT(tmp != NULL);                                                                  \
      return 0;                                                                                  \
    }                                                                                            \
    dest->a = tmp;                                                                               \
                                                                                                 \
    if (src->elem_init) {                                                                        \
      for (i=0; i<src->size; ++i) {                                                              \
        if (!src->elem_init(&dest->a[i], &src->a[i])) {                                          \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(TYPE));                                     \
    }                                                                                            \
                                                                                                 \
    dest->size = src->size;                                                                      \
    dest->capacity = src->capacity;                                                              \
    dest->elem_free = src->elem_free;                                                            \
    dest->elem_init = src->elem_init;                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE* a)                                             \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    if (vec->elem_init) {                                                                        \
      if (!vec->elem_init(&vec->a[vec->size], a)) {                                              \
        CVEC_ASSERT(0);                                                                          \
        return 0;                                                                                \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[vec->size], a, sizeof(TYPE));                                         \
    }                                                                                            \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_pushm_##TYPE(cvector_##TYPE* vec, TYPE* a)                                            \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[vec->size], a, sizeof(TYPE));                                           \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  void cvec_pop_##TYPE(cvector_##TYPE* vec, TYPE* ret)                                           \
  {                                                                                              \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[--vec->size], sizeof(TYPE));                                     \
    } else {                                                                                     \
      vec->size--;                                                                               \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_free) {                                                                        \
      vec->elem_free(&vec->a[vec->size]);                                                        \
    }                                                                                            \
  }                                                                                              \
                                                                                                 \
  void cvec_popm_##TYPE(cvector_##TYPE* vec, TYPE* ret)                                          \
  {                                                                                              \
    vec->size--;                                                                                 \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[vec->size], sizeof(TYPE));                                       \
    }                                                                                            \
  }                                                                                              \
                                                                                                 \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec) { return &vec->a[vec->size - 1]; }                 \
                                                                                                 \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num)                                       \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity < vec->size + num) {                                                       \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                           \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
                                                                                                 \
    vec->size += num;                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a)                                \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
                                                                                                 \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));                    \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      if (!vec->elem_init(&vec->a[i], a)) {                                                      \
        CVEC_ASSERT(0);                                                                          \
        return 0;                                                                                \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                 \
    }                                                                                            \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insertm_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a)                               \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
                                                                                                 \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));                    \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                   \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)             \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz, j;                                                                           \
    if (vec->capacity < vec->size + num) {                                                       \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                           \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i + num], &vec->a[i], (vec->size - i) * sizeof(TYPE));                  \
    if (vec->elem_init) {                                                                        \
      for (j = 0; j < num; ++j) {                                                                \
        if (!vec->elem_init(&vec->a[j + i], &a[j])) {                                            \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[i], a, num * sizeof(TYPE));                                           \
    }                                                                                            \
    vec->size += num;                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insert_arraym_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)            \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity < vec->size + num) {                                                       \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                           \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i + num], &vec->a[i], (vec->size - i) * sizeof(TYPE));                  \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i], a, num * sizeof(TYPE));                                             \
    vec->size += num;                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret)                    \
  {                                                                                              \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[i], sizeof(TYPE));                                               \
    } else if (vec->elem_free) {                                                                 \
      vec->elem_free(&vec->a[i]);                                                                \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      if (!vec->elem_init(&vec->a[i], a)) {                                                      \
        CVEC_ASSERT(0);                                                                          \
        return 0;                                                                                \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                 \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  void cvec_replacem_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret)                  \
  {                                                                                              \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[i], sizeof(TYPE));                                               \
    }                                                                                            \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                   \
  }                                                                                              \
                                                                                                 \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end)                        \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    cvec_sz d = end - start + 1;                                                                 \
    if (vec->elem_free) {                                                                        \
      for (i = start; i <= end; i++) {                                                           \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[start], &vec->a[end + 1], (vec->size - 1 - end) * sizeof(TYPE));        \
    vec->size -= d;                                                                              \
  }                                                                                              \
                                                                                                 \
  void cvec_remove_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end)                       \
  {                                                                                              \
    cvec_sz d = end - start + 1;                                                                 \
    CVEC_MEMMOVE(&vec->a[start], &vec->a[end + 1], (vec->size - 1 - end) * sizeof(TYPE));        \
    vec->size -= d;                                                                              \
  }                                                                                              \
                                                                                                 \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                     \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    if (vec->capacity < size) {                                                                  \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * (size + CVEC_##TYPE##_SZ)))) {      \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = size + CVEC_##TYPE##_SZ;                                                   \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                     \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    TYPE* tmp;                                                                                   \
    if (size < vec->size) {                                                                      \
      if (vec->elem_free) {                                                                      \
        for (i = vec->size - 1; i >= size; i--) {                                                \
          vec->elem_free(&vec->a[i]);                                                            \
        }                                                                                        \
      }                                                                                          \
      vec->size = size;                                                                          \
    }                                                                                            \
                                                                                                 \
    vec->capacity = size;                                                                        \
                                                                                                 \
    if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * size))) {                             \
      CVEC_ASSERT(tmp != NULL);                                                                  \
      return 0;                                                                                  \
    }                                                                                            \
    vec->a = tmp;                                                                                \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE* val)                                     \
  {                                                                                              \
    cvec_sz i;                                                                                   \
                                                                                                 \
    if (vec->elem_free) {                                                                        \
      for (i = 0; i < vec->size; i++) {                                                          \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      for (i = 0; i < vec->size; i++) {                                                          \
        if (!vec->elem_init(&vec->a[i], val)) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      for (i = 0; i < vec->size; i++) {                                                          \
        CVEC_MEMMOVE(&vec->a[i], val, sizeof(TYPE));                                             \
      }                                                                                          \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE* val)                                    \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    if (vec->elem_free) {                                                                        \
      for (i = 0; i < vec->size; i++) {                                                          \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
      vec->size = vec->capacity;                                                                 \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      for (i = 0; i < vec->capacity; i++) {                                                      \
        if (!vec->elem_init(&vec->a[i], val)) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      for (i = 0; i < vec->capacity; i++) {                                                      \
        CVEC_MEMMOVE(&vec->a[i], val, sizeof(TYPE));                                             \
      }                                                                                          \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  void cvec_clear_##TYPE(cvector_##TYPE* vec)                                                    \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    if (vec->elem_free) {                                                                        \
      for (i = 0; i < vec->size; ++i) {                                                          \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
    vec->size = 0;                                                                               \
  }                                                                                              \
                                                                                                 \
  void cvec_free_##TYPE##_heap(void* vec)                                                        \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                                  \
    if (!tmp) return;                                                                            \
    if (tmp->elem_free) {                                                                        \
      for (i = 0; i < tmp->size; i++) {                                                          \
        tmp->elem_free(&tmp->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
    CVEC_FREE(tmp->a);                                                                           \
    CVEC_FREE(tmp);                                                                              \
  }                                                                                              \
                                                                                                 \
  void cvec_free_##TYPE(void* vec)                                                               \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                                  \
    if (tmp->elem_free) {                                                                        \
      for (i = 0; i < tmp->size; i++) {                                                          \
        tmp->elem_free(&tmp->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
                                                                                                 \
    CVEC_FREE(tmp->a);                                                                           \
    tmp->a        = NULL;                                                                        \
    tmp->size     = 0;                                                                           \
    tmp->capacity = 0;                                                                           \
  }



/* header ends */
#endif


#define RESIZE(x) ((x+1)*2)



#include <time.h>

#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

// TODO struct packing?  save a few bytes?
typedef struct file
{
	char* path;   // could be url;

	// time_t is a long int ...
	time_t modified;
	int size;     // in bytes (hard to believe it'd be bigger than ~2.1 GB)

	//  caching for list mode
	char mod_str[MOD_STR_BUF];
	char size_str[SIZE_STR_BUF];
	char* name;  // pointing at filename in path
} file;

CVEC_NEW_DECLS2(file)

void free_file(void* f);

// comparison functions
int filename_cmp_lt(const void* a, const void* b);
int filename_cmp_gt(const void* a, const void* b);

int filepath_cmp_lt(const void* a, const void* b);
int filepath_cmp_gt(const void* a, const void* b);

int filesize_cmp_lt(const void* a, const void* b);
int filesize_cmp_gt(const void* a, const void* b);

int filemodified_cmp_lt(const void* a, const void* b);
int filemodified_cmp_gt(const void* a, const void* b);

#endif


#ifndef FILE_TYPE_STR
#define FILE_TYPE_STR "Match Exts"
#endif

#define TRUE 1
#define FALSE 0

#ifndef FB_LOG
#define FB_LOG(A, ...) printf(A, __VA_ARGS__)
#endif

#define STRBUF_SZ 1024
#define MAX_PATH_LEN STRBUF_SZ
#define PATH_SEPARATOR '/'

typedef int (*recents_func)(cvector_str* recents, void * userdata);
typedef int (*cmp_func)(const void* a, const void* b);
enum { FB_NAME_UP, FB_NAME_DOWN, FB_SIZE_UP, FB_SIZE_DOWN, FB_MODIFIED_UP, FB_MODIFIED_DOWN };
enum { FB_NAME, FB_SIZE, FB_MODIFIED };

// TODO name? file_explorer? selector?
typedef struct file_browser
{
	char dir[MAX_PATH_LEN];   // cur location
	char file[MAX_PATH_LEN];  // return "value" ie file selected 

	// special bookmarked locations
	char home[MAX_PATH_LEN];
	char desktop[MAX_PATH_LEN];

	// searching
	char text_buf[STRBUF_SZ];
	int text_len;

	recents_func get_recents;
	void* userdata;

	// bools
	int is_recents;
	int is_search_results;
	int is_text_path; // could change to flag if I add a third option
	int list_setscroll;
	int show_hidden;
	int select_dir;

	// does not own memory
	const char** exts;
	int num_exts;
	int ignore_exts; // if true, show all files, not just matching exts

	// list of files in cur directory
	cvector_file files;

	cvector_i search_results;
	int selection;

	// Not used internally, only if the user wants to control the
	// list view see terminal_filebrowser.c
#ifdef FILE_LIST_SZ
	int begin;
	int end;
#endif

	int sorted_state;
	cmp_func c_func;

} file_browser;

int init_file_browser(file_browser* browser, const char** exts, int num_exts, const char* start_dir, recents_func r_func, void* userdata);
void free_file_browser(file_browser* fb);
void switch_dir(file_browser* fb, const char* dir);
void handle_recents(file_browser* fb);

void fb_search_filenames(file_browser* fb);

// TODO think about this
void fb_sort_toggle(file_browser* fb, int sort_type);
#define fb_sort_name(fb) fb_sort_toggle((fb), FB_NAME)
#define fb_sort_size(fb) fb_sort_toggle((fb), FB_SIZE)
#define fb_sort_modified(fb) fb_sort_toggle((fb), FB_MODIFIED)

const char* get_homedir(void);
int fb_scandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts, int show_hidden, int select_dir);
char* mydirname(const char* path, char* dirpath);
char* mybasename(const char* path, char* base);
void normalize_path(char* path);
int bytes2str(int bytes, char* buf, int len);


#endif

#ifdef __cplusplus
}
#endif

// end FILE_BROWSER_H
#endif


#ifdef FILE_BROWSER_IMPLEMENTATION


//#define CVECTOR_IMPLEMENTATION
//#define CVEC_ONLY_INT
//#define CVEC_ONLY_STR
//#define CVEC_SIZE_T i64
//#define PRIcv_sz PRIiMAX
//
//#ifdef CVECTOR_IMPLEMENTATION

#ifndef CVEC_NO_INT

cvec_sz CVEC_I_START_SZ = 50;

#define CVEC_I_ALLOCATOR(x) ((x+1) * 2)

/**
 * Creates a new cvector_i on the heap.
 * Vector size set to (size > 0) ? size : 0;
 * Capacity to (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_I_START_SZ
 * in other words capacity has to be at least 1 and >= to vec->size of course.
 */
cvector_i* cvec_i_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_i* vec;
	if (!(vec = (cvector_i*)CVEC_MALLOC(sizeof(cvector_i)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_I_START_SZ;

	if (!(vec->a = (int*)CVEC_MALLOC(vec->capacity*sizeof(int)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

/** Create (on the heap) and initialize cvector_i with num elements of vals.
 *  Capacity is set to num + CVEC_I_START_SZ.
 */
cvector_i* cvec_init_i_heap(int* vals, cvec_sz num)
{
	cvector_i* vec;
	
	if (!(vec = (cvector_i*)CVEC_MALLOC(sizeof(cvector_i)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_I_START_SZ;
	vec->size = num;
	if (!(vec->a = (int*)CVEC_MALLOC(vec->capacity*sizeof(int)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(int)*num);

	return vec;
}

/** Same as cvec_i_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_i in this case.
 *  This and cvec_init_i should be preferred over the heap versions.
 */
int cvec_i(cvector_i* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_I_START_SZ;

	if (!(vec->a = (int*)CVEC_MALLOC(vec->capacity*sizeof(int)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

/** Same as cvec_init_i_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.
 */
int cvec_init_i(cvector_i* vec, int* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_I_START_SZ;
	vec->size = num;
	if (!(vec->a = (int*)CVEC_MALLOC(vec->capacity*sizeof(int)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(int)*num);

	return 1;
}

/** Makes dest a copy of src.  The parameters
 *  are void so it can be used as the constructor when making
 *  a vector of cvector_i's.  Assumes dest (the structure)
 *  is already allocated (probably on the stack) and that
 *  capacity is 0 (ie the array doesn't need to be freed).
 *
 *  Really just a wrapper around copy, that initializes dest/vec1's
 *  members to NULL/0.  If you pre-initialized dest to 0, you could
 *  just use copy.
 */
int cvec_copyc_i(void* dest, void* src)
{
	cvector_i* vec1 = (cvector_i*)dest;
	cvector_i* vec2 = (cvector_i*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_i(vec1, vec2);
}

/** Makes dest a copy of src.  Assumes dest
 * (the structure) is already allocated (probably on the stack) and
 * is in a valid state (ie array is either NULL or allocated with
 * size and capacity set appropriately).
 *
 * TODO Should I copy capacity, so dest is truly identical or do
 * I only care about the actual contents, and let dest->cap = src->size
 * maybe plus CVEC_I_START_SZ
 */
int cvec_copy_i(cvector_i* dest, cvector_i* src)
{
	int* tmp = NULL;
	if (!(tmp = (int*)CVEC_REALLOC(dest->a, src->capacity*sizeof(int)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;
	
	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(int));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}

/**
 * Append a to end of vector (size increased 1).
 * Capacity is increased by doubling when necessary.
 */
int cvec_push_i(cvector_i* vec, int a)
{
	int* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_I_ALLOCATOR(vec->capacity);
		if (!(tmp = (int*)CVEC_REALLOC(vec->a, sizeof(int)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	
	vec->a[vec->size++] = a;
	return 1;
}

/** Remove and return the last element (size decreased 1).*/
int cvec_pop_i(cvector_i* vec)
{
	return vec->a[--vec->size];
}

/** Return pointer to last element */
int* cvec_back_i(cvector_i* vec)
{
	return &vec->a[vec->size-1];
}

/** Increase the size of the array num items.  Items
 *  are not initialized to anything */
int cvec_extend_i(cvector_i* vec, cvec_sz num)
{
	int* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_I_START_SZ;
		if (!(tmp = (int*)CVEC_REALLOC(vec->a, sizeof(int)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

/**
 * Insert a at index i (0 based).
 * Everything from that index and right is shifted one to the right.
 */
int cvec_insert_i(cvector_i* vec, cvec_sz i, int a)
{
	int* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_I_ALLOCATOR(vec->capacity);
		if (!(tmp = (int*)CVEC_REALLOC(vec->a, sizeof(int)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(int));
	vec->a[i] = a;
	vec->size++;
	return 1;
}

/**
 * Insert the first num elements of array a at index i.
 * Note that it is the user's responsibility to pass in valid
 * arguments.  Also CVEC_MEMMOVE is used so don't try to insert
 * part of the vector array into itself (that would require CVEC_MEMMOVE)
 */
int cvec_insert_array_i(cvector_i* vec, cvec_sz i, int* a, cvec_sz num)
{
	int* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_I_START_SZ;
		if (!(tmp = (int*)CVEC_REALLOC(vec->a, sizeof(int)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(int));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(int));
	vec->size += num;
	return 1;
}

/** Replace value at index i with a, return original value. */
int cvec_replace_i(cvector_i* vec, cvec_sz i, int a)
{
	int tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

/**
 * Erases elements from start to end inclusive.
 * Example cvec_erase_i(myvec, 1, 3) would remove elements at 1, 2, and 3 and the element
 * that was at index 4 would now be at 1 etc.
 */
void cvec_erase_i(cvector_i* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(int));
	vec->size -= d;
}

/** Make sure capacity is at least size(parameter not member). */
int cvec_reserve_i(cvector_i* vec, cvec_sz size)
{
	int* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (int*)CVEC_REALLOC(vec->a, sizeof(int)*(size+CVEC_I_START_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_I_START_SZ;
	}
	return 1;
}

/** Set capacity to size.
 * You will lose data if you shrink the capacity below the current size.
 * If you do, the size will be set to capacity of course.
*/
int cvec_set_cap_i(cvector_i* vec, cvec_sz size)
{
	int* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (int*)CVEC_REALLOC(vec->a, sizeof(int)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

/** Set all size elements to val. */
void cvec_set_val_sz_i(cvector_i* vec, int val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

/** Fills entire allocated array (capacity) with val. */
void cvec_set_val_cap_i(cvector_i* vec, int val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

/** Sets size to 0 (does not clear contents).*/
void cvec_clear_i(cvector_i* vec) { vec->size = 0; }

/** Frees everything so don't use vec after calling this.
 *  Passing NULL is a NO-OP, matching the behavior of free(). */
void cvec_free_i_heap(void* vec)
{
	cvector_i* tmp = (cvector_i*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

/** Frees the internal array and zeros out the members to maintain a
 * consistent state */
void cvec_free_i(void* vec)
{
	cvector_i* tmp = (cvector_i*)vec;
	CVEC_FREE(tmp->a);
	tmp->a = NULL;
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif

#ifndef CVEC_NO_DOUBLE

cvec_sz CVEC_D_START_SZ = 50;

#define CVEC_D_ALLOCATOR(x) ((x+1) * 2)

/**
 * Creates a new cvector_d on the heap.
 * Vector size set to (size > 0) ? size : 0;
 * Capacity to (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_D_START_SZ
 * in other words capacity has to be at least 1 and >= to vec->size of course.
 */
cvector_d* cvec_d_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_d* vec;
	
	if (!(vec = (cvector_d*)CVEC_MALLOC(sizeof(cvector_d)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_D_START_SZ;

	if (!(vec->a = (double*)CVEC_MALLOC(vec->capacity*sizeof(double)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

/** Create (on the heap) and initialize cvector_d with num elements of vals.
 *  Capacity is set to num + CVEC_D_START_SZ.
 */
cvector_d* cvec_init_d_heap(double* vals, cvec_sz num)
{
	cvector_d* vec;
	
	if (!(vec = (cvector_d*)CVEC_MALLOC(sizeof(cvector_d)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_D_START_SZ;
	vec->size = num;
	if (!(vec->a = (double*)CVEC_MALLOC(vec->capacity*sizeof(double)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(double)*num);

	return vec;
}

/** Same as cvec_d_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_d in this case.
 *  This and cvec_init_d should be preferred over the heap versions.
 */
int cvec_d(cvector_d* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_D_START_SZ;

	if (!(vec->a = (double*)CVEC_MALLOC(vec->capacity*sizeof(double)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

/** Same as cvec_init_d_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_d in this case.
 */
int cvec_init_d(cvector_d* vec, double* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_D_START_SZ;
	vec->size = num;
	if (!(vec->a = (double*)CVEC_MALLOC(vec->capacity*sizeof(double)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(double)*num);

	return 1;
}

/** Makes dest a copy of src.  The parameters
 *  are void so it can be used as the constructor when making
 *  a vector of cvector_d's.  Assumes dest (the structure)
 *  is already allocated (probably on the stack) and that
 *  capacity is 0 (ie the array doesn't need to be freed).
 *
 *  Really just a wrapper around copy, that initializes dest/vec1's
 *  members to NULL/0.  If you pre-initialized dest to 0, you could
 *  just use copy.
 */
int cvec_copyc_d(void* dest, void* src)
{
	cvector_d* vec1 = (cvector_d*)dest;
	cvector_d* vec2 = (cvector_d*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_d(vec1, vec2);
}

/** Makes dest a copy of src.  Assumes dest
 * (the structure) is already allocated (probably on the stack) and
 * is in a valid state (ie array is either NULL or allocated with
 * size and capacity set appropriately).
 *
 * TODO Should I copy capacity, so dest is truly identical or do
 * I only care about the actual contents, and let dest->cap = src->size
 * maybe plus CVEC_D_START_SZ
 */
int cvec_copy_d(cvector_d* dest, cvector_d* src)
{
	double* tmp = NULL;
	if (!(tmp = (double*)CVEC_REALLOC(dest->a, src->capacity*sizeof(double)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;
	
	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(double));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


/** Append a to end of vector (size increased 1).
 * Capacity is increased by doubling when necessary.
 */
int cvec_push_d(cvector_d* vec, double a)
{
	double* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_D_ALLOCATOR(vec->capacity);
		if (!(tmp = (double*)CVEC_REALLOC(vec->a, sizeof(double)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	vec->a[vec->size++] = a;
	return 1;
}

/** Remove and return the last element (size decreased 1).*/
double cvec_pop_d(cvector_d* vec)
{
	return vec->a[--vec->size];
}

/** Return pointer to last element */
double* cvec_back_d(cvector_d* vec)
{
	return &vec->a[vec->size-1];
}

/** Increase the size of the array num items.  Items
 *  are not initialized to anything */
int cvec_extend_d(cvector_d* vec, cvec_sz num)
{
	double* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_D_START_SZ;
		if (!(tmp = (double*)CVEC_REALLOC(vec->a, sizeof(double)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

/**
 * Insert a at index i (0 based).
 * Everything from that index and right is shifted one to the right.
 */
int cvec_insert_d(cvector_d* vec, cvec_sz i, double a)
{
	double* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_D_ALLOCATOR(vec->capacity);
		if (!(tmp = (double*)CVEC_REALLOC(vec->a, sizeof(double)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	
	CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(double));
	vec->a[i] = a;
	vec->size++;
	return 1;
}

/**
 * Insert the first num elements of array a at index i.
 * Note that it is the user's responsibility to pass in valid
 * arguments.  Also CVEC_MEMMOVE is used so don't try to insert
 * part of the vector array into itself (that would require CVEC_MEMMOVE)
 */
int cvec_insert_array_d(cvector_d* vec, cvec_sz i, double* a, cvec_sz num)
{
	double* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_D_START_SZ;
		if (!(tmp = (double*)CVEC_REALLOC(vec->a, sizeof(double)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(double));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(double));
	vec->size += num;
	return 1;
}

/** Replace value at index i with a, return original value. */
double cvec_replace_d(cvector_d* vec, cvec_sz i, double a)
{
	double tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

/**
 * Erases elements from start to end inclusive.
 * Example cvec_erase_d(myvec, 1, 3) would remove elements at 1, 2, and 3 and the element
 * that was at index 4 would now be at 1 etc.
 */
void cvec_erase_d(cvector_d* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(double));
	vec->size -= d;
}

/** Make sure capacity is at least size(parameter not member). */
int cvec_reserve_d(cvector_d* vec, cvec_sz size)
{
	double* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (double*)CVEC_REALLOC(vec->a, sizeof(double)*(size+CVEC_D_START_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_D_START_SZ;
	}
	return 1;
}

/** Set capacity to size.
 * You will lose data if you shrink the capacity below the current size.
 * If you do, the size will be set to capacity of course.
*/
int cvec_set_cap_d(cvector_d* vec, cvec_sz size)
{
	double* tmp;
	if (size < vec->size)
		vec->size = size;

	if (!(tmp = (double*)CVEC_REALLOC(vec->a, sizeof(double)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

/** Set all size elements to val. */
void cvec_set_val_sz_d(cvector_d* vec, double val)
{
	cvec_sz i;
	for(i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

/** Fills entire allocated array (capacity) with val. */
void cvec_set_val_cap_d(cvector_d* vec, double val)
{
	cvec_sz i;
	for(i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

/** Sets size to 0 (does not clear contents).*/
void cvec_clear_d(cvector_d* vec) { vec->size = 0; }

/** Frees everything so don't use vec after calling this.
 *  Passing NULL is a NO-OP, matching the behavior of free(). */
void cvec_free_d_heap(void* vec)
{
	cvector_d* tmp = (cvector_d*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

/** Frees the internal array and zeros out the members to maintain a
 * consistent state */
void cvec_free_d(void* vec)
{
	cvector_d* tmp = (cvector_d*)vec;
	CVEC_FREE(tmp->a);
	tmp->a = NULL;
	tmp->size = 0;
	tmp->capacity = 0;
}
#endif

#ifndef CVEC_NO_STR

cvec_sz CVEC_STR_START_SZ = 20;

#define CVEC_STR_ALLOCATOR(x) ((x+1) * 2)

#if CVEC_STRDUP == cvec_strdup
/** Useful utility function since strdup isn't in standard C.*/
char* cvec_strdup(const char* str)
{
	cvec_sz len;
	char* temp;
	if (!str)
		return NULL;

	len = strlen(str);
	temp = (char*)CVEC_MALLOC(len+1);
	if (!temp) {
		CVEC_ASSERT(temp != NULL);
		return NULL;
	}
	temp[len] = 0;
	
	return (char*)CVEC_MEMMOVE(temp, str, len);  /* CVEC_MEMMOVE returns to */
}
#endif

/**
 * Create a new cvector_str on the heap.
 * Vector size set to (size > 0) ? size : 0;
 * Capacity to (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_STR_START_SZ
 * in other words capacity has to be at least 1 and >= to vec->size of course.
 * Note: cvector_str does not copy pointers passed in but duplicates the strings
 * they point to (using CVEC_STRDUP()) so you don't have to worry about freeing
 * or changing the contents of variables that you've pushed or inserted; it
 * won't affect the values in the vector.
 */
cvector_str* cvec_str_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_str* vec;
	if (!(vec = (cvector_str*)CVEC_MALLOC(sizeof(cvector_str)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_STR_START_SZ;

	if (!(vec->a = (char**)CVEC_MALLOC(vec->capacity * sizeof(char*)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}
	/* clearing to 0 here because if the user gave a non-zero initial size, popping/
	 * erasing will crash unless they're NULL.  Really the user should never do that.
	 * They should use cvec_init or otherwise immediately assign to the size elements they
	 * started with.  */
	memset(vec->a, 0, vec->capacity*sizeof(char*));

	return vec;
}

/** Create (on the heap) and initialize cvector_str with num elements of vals.
 */
cvector_str* cvec_init_str_heap(char** vals, cvec_sz num)
{
	cvector_str* vec;
	cvec_sz i;
	
	if (!(vec = (cvector_str*)CVEC_MALLOC(sizeof(cvector_str)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_STR_START_SZ;
	vec->size = num;
	if (!(vec->a = (char**)CVEC_MALLOC(vec->capacity*sizeof(char*)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	for(i=0; i<num; i++) {
		vec->a[i] = CVEC_STRDUP(vals[i]);
	}
	
	return vec;
}

/** Same as cvec_str_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_str in this case
 *  This and cvec_init_str should be preferred over the heap versions.
 */
int cvec_str(cvector_str* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_STR_START_SZ;

	if (!(vec->a = (char**)CVEC_MALLOC(vec->capacity * sizeof(char*)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}
	/* clearing to 0 here because if the user gave a non-zero initial size, popping/
	 * erasing will crash unless they're NULL */
	memset(vec->a, 0, vec->capacity*sizeof(char*));

	return 1;
}

/** Same as cvec_init_str_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_str in this case
 */
int cvec_init_str(cvector_str* vec, char** vals, cvec_sz num)
{
	cvec_sz i;
	
	vec->capacity = num + CVEC_STR_START_SZ;
	vec->size = num;
	if (!(vec->a = (char**)CVEC_MALLOC(vec->capacity*sizeof(char*)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	for(i=0; i<num; i++) {
		vec->a[i] = CVEC_STRDUP(vals[i]);
	}
	
	return 1;
}

/** Makes dest a copy of src.  The parameters
 *  are void so it can be used as the constructor when making
 *  a vector of cvector_str's.  Assumes dest (the structure)
 *  is already allocated (probably on the stack) and that
 *  capacity is 0 (ie the array doesn't need to be freed).
 *
 *  Really just a wrapper around copy, that initializes dest/vec1's
 *  members to NULL/0.  If you pre-initialized dest to 0, you could
 *  just use copy.
 */
int cvec_copyc_str(void* dest, void* src)
{
	cvector_str* vec1 = (cvector_str*)dest;
	cvector_str* vec2 = (cvector_str*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_str(vec1, vec2);
}

/** Makes dest a copy of src.  Assumes dest
 * (the structure) is already allocated (probably on the stack) and
 * is in a valid state (ie array is either NULL or allocated with
 * size and capacity set appropriately).
 *
 * TODO Should I copy capacity, so dest is truly identical or do
 * I only care about the actual contents, and let dest->cap = src->size
 * maybe plus CVEC_STR_START_SZ
 */
int cvec_copy_str(cvector_str* dest, cvector_str* src)
{
	int i;
	char** tmp = NULL;
	if (!(tmp = (char**)CVEC_REALLOC(dest->a, src->capacity*sizeof(char*)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;
	
	for (i=0; i<src->size; ++i) {
		dest->a[i] = CVEC_STRDUP(src->a[i]);
	}
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}

/**
 * Append a to end of vector (size increased 1).
 * Capacity is increased by doubling when necessary.
 */
int cvec_push_str(cvector_str* vec, char* a)
{
	char** tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_STR_ALLOCATOR(vec->capacity);
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	
	vec->a[vec->size++] = CVEC_STRDUP(a);
	return 1;
}

/** same as push but without calling CVEC_STRDUP(a), m suffix is for "move" */
int cvec_pushm_str(cvector_str* vec, char* a)
{
	char** tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_STR_ALLOCATOR(vec->capacity);
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	
	vec->a[vec->size++] = a;
	return 1;
}

/** Remove the last element (size decreased 1).
 *  String is freed.  If ret != NULL strcpy the last element into ret.
 *  It is the user's responsibility to make sure ret can receive it without error
 *  (ie ret has adequate space.) */
void cvec_pop_str(cvector_str* vec, char* ret)
{
	vec->size--;
	if (ret)
		strcpy(ret, vec->a[vec->size]);
	CVEC_FREE(vec->a[vec->size]);
}

/** Return pointer to last element */
char** cvec_back_str(cvector_str* vec)
{
	return &vec->a[vec->size-1];
}

/** Increase the size of the array num items.  Items
 *  are memset to NULL since they will be freed when
    popped or the vector is freed.*/
int cvec_extend_str(cvector_str* vec, cvec_sz num)
{
	char** tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_STR_START_SZ;
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	memset(&vec->a[vec->size], 0, num*sizeof(char*));
	vec->size += num;
	return 1;
}

/**
 * Insert a at index i (0 based).
 * Everything from that index and right is shifted one to the right.
 */
int cvec_insert_str(cvector_str* vec, cvec_sz i, char* a)
{
	char** tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_STR_ALLOCATOR(vec->capacity);
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(char*));
	vec->a[i] = CVEC_STRDUP(a);
	vec->size++;
	return 1;
}

/**
 * Same as insert except no CVEC_STRDUP.
 */
int cvec_insertm_str(cvector_str* vec, cvec_sz i, char* a)
{
	char** tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_STR_ALLOCATOR(vec->capacity);
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(char*));
	vec->a[i] = a;
	vec->size++;
	return 1;
}

/**
 * Insert the first num elements of array a at index i.
 * Note that it is the user's responsibility to pass in valid
 * arguments.
 */
int cvec_insert_array_str(cvector_str* vec, cvec_sz i, char** a, cvec_sz num)
{
	char** tmp;
	cvec_sz tmp_sz, j;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_STR_START_SZ;
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(char*));
	for (j=0; j<num; ++j) {
		vec->a[j+i] = CVEC_STRDUP(a[j]);
	}
	
	vec->size += num;
	return 1;
}

/**
 * Same as insert_array except no CVEC_STRDUP.
 */
int cvec_insert_arraym_str(cvector_str* vec, cvec_sz i, char** a, cvec_sz num)
{
	char** tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_STR_START_SZ;
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(char*));

	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(char*));
	vec->size += num;
	return 1;
}

/**
 * Replace string at i with a. If ret != NULL, strcpy the old str to it.
 * See cvec_pop_str warning
 * */
void cvec_replace_str(cvector_str* vec, cvec_sz i, char* a, char* ret)
{
	if (ret)
		strcpy(ret, vec->a[i]);
	CVEC_FREE(vec->a[i]);
	vec->a[i] = CVEC_STRDUP(a);
}

/**
 * Erases strings from start to end inclusive.
 * Example erases(myvec, 1, 3) would CVEC_FREE and remove strings at 1, 2, and 3 and the string
 * that was at index 4 would now be at 1 etc.
 */
void cvec_erase_str(cvector_str* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz i;
	cvec_sz d = end - start + 1;
	for (i=start; i<=end; i++) {
		CVEC_FREE(vec->a[i]);
	}
	
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(char*));
	vec->size -= d;
}

/** Same as erase except it *does not* call CVEC_FREE. */
void cvec_remove_str(cvector_str* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(char*));
	vec->size -= d;
}

/** Makes sure the vector capacity is >= size (parameter not member). */
int cvec_reserve_str(cvector_str* vec, cvec_sz size)
{
	char** tmp;
	if (vec->capacity < size) {
		if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*(size+CVEC_STR_START_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_STR_START_SZ;
	}
	return 1;
}

/** Set capacity to size.
 * You will lose data if you shrink the capacity below the current size.
 * If you do, the size will be set to capacity of course.
*/
int cvec_set_cap_str(cvector_str* vec, cvec_sz size)
{
	cvec_sz i;
	char** tmp;
	if (size < vec->size) {
		for(i=vec->size-1; i>size-1; i--) {
			CVEC_FREE(vec->a[i]);
		}

		vec->size = size;
	}

	if (!(tmp = (char**)CVEC_REALLOC(vec->a, sizeof(char*)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

/** Sets all size elements to val. */
void cvec_set_val_sz_str(cvector_str* vec, char* val)
{
	cvec_sz i;
	for(i=0; i<vec->size; i++) {
		CVEC_FREE(vec->a[i]);

		/* not worth checking to me see commit msg aa0c5cf */
		vec->a[i] = CVEC_STRDUP(val);
	}
}

/** Fills entire allocated array (capacity) with val.  Size is set
 * to capacity in this case because strings are individually dynamically allocated.
 * This is different from cvector_i, cvector_d and cvector_void (without a CVEC_FREE function) where the size stays the same.
   TODO  Remove this function?  even more unnecessary than for cvector_i and cvector_d and different behavior*/
void cvec_set_val_cap_str(cvector_str* vec, char* val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		if (i<vec->size) {
			CVEC_FREE(vec->a[i]);
		}
		
		vec->a[i] = CVEC_STRDUP(val);
	}
	vec->size = vec->capacity;
}

/** Clears the contents of vector (frees all strings) and sets size to 0. */
void cvec_clear_str(cvector_str* vec)
{
	int i;
	for (i=0; i<vec->size; i++) {
		CVEC_FREE(vec->a[i]);
	}
	
	vec->size = 0;
}

/** Frees contents (individual strings and array) and frees vector so don't use
 *  after calling this. Passing NULL is a NO-OP, matching the behavior of free(). */
void cvec_free_str_heap(void* vec)
{
	cvec_sz i;
	cvector_str* tmp = (cvector_str*)vec;
	if (!tmp) return;
	for (i=0; i<tmp->size; i++) {
		CVEC_FREE(tmp->a[i]);
	}
	
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

/** Frees the internal array and zeros out the members to maintain a
 * consistent state */
void cvec_free_str(void* vec)
{
	cvec_sz i;
	cvector_str* tmp = (cvector_str*)vec;
	for (i=0; i<tmp->size; i++) {
		CVEC_FREE(tmp->a[i]);
	}
	
	CVEC_FREE(tmp->a);
	tmp->a = NULL;
	tmp->size = 0;
	tmp->capacity = 0;
}
#endif

#ifndef CVEC_NO_VOID

cvec_sz CVEC_VOID_START_SZ = 20;

#define CVEC_VOID_ALLOCATOR(x) ((x+1) * 2)

/**
 * Creates a new vector on the heap.
 * Vector size set to (size > 0) ? size : 0;
 * Capacity to (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_VOID_START_SZ
 * in other words capacity has to be at least 1 and >= to vec->size of course.
 * elem_sz is the size of the type you want to store ( ie sizeof(T) where T is your type ).
 * You can pass in an optional function, elem_free, to be called on every element before it is erased
 * from the vector to free any dynamically allocated memory.  Likewise you can pass in elem_init to be
 * as a sort of copy constructor for any insertions if you needed some kind of deep copy/allocations.
 *
 * For example if you passed in sizeof(char*) for elem_sz, and wrappers around the standard free(void*)
 * function for elem_free and CVEC_STRDUP for elem_init you could
 * make cvector_void work *almost* exactly like cvector_str.  The main difference is cvector_str does not
 * check for failure of CVEC_STRDUP while cvector_void does check for failure of elem_init.  The other
 * minor differences are popm and replacem are macros in cvector_str (and the latter returns the result
 * rather than using a double pointer return parameter) and depending on how you defined elem_init
 * and whether you're using the 'move' functions, you have to pass in char**'s instead of char*'s
 * because cvector_void has to use memmove rather than straight assignment.
 *
 * Pass in NULL, to not use the function parameters.
 *
 * The function remove and the 'move' functions (with the m suffix) do not call elem_init or elem_free
 * even if they are set.  This gives you some flexibility and performance when you already have things
 * allocated or want to keep things after removing them from the vector but only some of the time (otherwise
 * you wouldn't have defined elem_free/elem_init in the first place).
 *
 * See the other functions and the tests for more behavioral/usage details.
 */
cvector_void* cvec_void_heap(cvec_sz size, cvec_sz capacity, cvec_sz elem_sz, void(*elem_free)(void*), int(*elem_init)(void*, void*))
{
	cvector_void* vec;
	if (!(vec = (cvector_void*)CVEC_MALLOC(sizeof(cvector_void)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_VOID_START_SZ;

	vec->elem_size = elem_sz;
	
	if (!(vec->a = (cvec_u8*)CVEC_MALLOC(vec->capacity*elem_sz))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}
	/* not clearing to 0 here as in cvector_str because elem_free cannot be calling CVEC_FREE directly
	 * since it takes the address of the element not the element itself */

	vec->elem_free = elem_free;
	vec->elem_init = elem_init;

	return vec;
}

/** Create (on the heap) and initialize vector with num elements of vals.
 *  elem_sz is the size of the type you want to store ( ie sizeof(T) where T is your type ).
 *  See cvec_void_heap() for more information about the elem_free and elem_init parameters.
 */
cvector_void* cvec_init_void_heap(void* vals, cvec_sz num, cvec_sz elem_sz, void(*elem_free)(void*), int(*elem_init)(void*, void*))
{
	cvector_void* vec;
	cvec_sz i;
	
	if (!(vec = (cvector_void*)CVEC_MALLOC(sizeof(cvector_void)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->elem_size = elem_sz;

	vec->capacity = num + CVEC_VOID_START_SZ;
	vec->size = num;
	if (!(vec->a = (cvec_u8*)CVEC_MALLOC(vec->capacity*elem_sz))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	if (elem_init) {
		for (i=0; i<num; ++i) {
			if (!elem_init(&vec->a[i*elem_sz], &((cvec_u8*)vals)[i*elem_sz])) {
				CVEC_ASSERT(0);
				CVEC_FREE(vec->a);
				CVEC_FREE(vec);
				return NULL;
			}
		}
	} else {
		CVEC_MEMMOVE(vec->a, vals, elem_sz*num);
	}
	
	vec->elem_free = elem_free;
	vec->elem_init = elem_init;

	return vec;
}

/** Same as cvec_void_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_void in that case
 */
int cvec_void(cvector_void* vec, cvec_sz size, cvec_sz capacity, cvec_sz elem_sz, void(*elem_free)(void*), int(*elem_init)(void*, void*))
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_VOID_START_SZ;

	vec->elem_size = elem_sz;
	
	if (!(vec->a = (cvec_u8*)CVEC_MALLOC(vec->capacity*elem_sz))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	vec->elem_free = elem_free;
	vec->elem_init = elem_init;

	return 1;
}

/** Same as init_vec_heap() except the vector passed in was declared on the stack so
 *  it isn't allocated in this function.  Use the cvec_free_void in this case
 */
int cvec_init_void(cvector_void* vec, void* vals, cvec_sz num, cvec_sz elem_sz, void(*elem_free)(void*), int(*elem_init)(void*, void*))
{
	cvec_sz i;
	
	vec->elem_size = elem_sz;

	vec->capacity = num + CVEC_VOID_START_SZ;
	vec->size = num;
	if (!(vec->a = (cvec_u8*)CVEC_MALLOC(vec->capacity*elem_sz))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	if (elem_init) {
		for (i=0; i<num; ++i) {
			if (!elem_init(&vec->a[i*elem_sz], &((cvec_u8*)vals)[i*elem_sz])) {
				CVEC_ASSERT(0);
				return 0;
			}
		}
	} else {
		CVEC_MEMMOVE(vec->a, vals, elem_sz*num);
	}

	vec->elem_free = elem_free;
	vec->elem_init = elem_init;

	return 1;
}

/** Makes dest a copy of src.  The parameters
 *  are void so it can be used as the constructor when making
 *  a vector of cvector_void's.  Assumes dest (the structure)
 *  is already allocated (probably on the stack) and that
 *  capacity is 0 (ie the array doesn't need to be freed).
 *
 *  Really just a wrapper around copy, that initializes dest/vec1's
 *  members to NULL/0.  If you pre-initialized dest to 0, you could
 *  just use copy.
 */
int cvec_copyc_void(void* dest, void* src)
{
	cvector_void* vec1 = (cvector_void*)dest;
	cvector_void* vec2 = (cvector_void*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_void(vec1, vec2);
}

/** Makes dest a copy of src.  Assumes dest
 * (the structure) is already allocated (probably on the stack) and
 * is in a valid state (ie array is either NULL or allocated with
 * size and capacity set appropriately).
 *
 * TODO Should I copy capacity, so dest is truly identical or do
 * I only care about the actual contents, and let dest->cap = src->size
 * maybe plus CVEC_VOID_START_SZ
 */
int cvec_copy_void(cvector_void* dest, cvector_void* src)
{
	int i;
	cvec_u8* tmp = NULL;
	if (!(tmp = (cvec_u8*)CVEC_REALLOC(dest->a, src->capacity*src->elem_size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	if (src->elem_init) {
		for (i=0; i<src->size; ++i) {
			if (!src->elem_init(&dest->a[i*src->elem_size], &src->a[i*src->elem_size])) {
				CVEC_ASSERT(0);
				return 0;
			}
		}
	} else {
		/* could use memcpy here since we know we just allocated dest->a */
		CVEC_MEMMOVE(dest->a, src->a, src->size*src->elem_size);
	}

	dest->size = src->size;
	dest->capacity = src->capacity;
	dest->elem_size = src->elem_size;
	dest->elem_free = src->elem_free;
	dest->elem_init = src->elem_init;
	return 1;
}

/** Append a to end of vector (size increased 1).
 * Capacity is increased by doubling when necessary.
 *
 * TODO For all of cvector_void, now that elem_init returns int, is it worth
 * the extra code and overhead of checking it and asserting/returning 0?
 */
int cvec_push_void(cvector_void* vec, void* a)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_VOID_ALLOCATOR(vec->capacity);
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	if (vec->elem_init) {
		if (!vec->elem_init(&vec->a[vec->size*vec->elem_size], a)) {
			CVEC_ASSERT(0);
			return 0;
		}
	} else {
		CVEC_MEMMOVE(&vec->a[vec->size*vec->elem_size], a, vec->elem_size);
	}
	
	vec->size++;
	return 1;
}

/** Same as push except no elem_init even if it's set */
int cvec_pushm_void(cvector_void* vec, void* a)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_VOID_ALLOCATOR(vec->capacity);
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	CVEC_MEMMOVE(&vec->a[vec->size*vec->elem_size], a, vec->elem_size);
	
	vec->size++;
	return 1;
}


/** Remove the last element (size decreased 1).
 * Copy the element into ret if ret is not NULL.  This function assumes
 * that ret is large accept the element and just CVEC_MEMMOVE's it in.
 * Similar to pop_backs it is users responsibility.
 */
void cvec_pop_void(cvector_void* vec, void* ret)
{
	vec->size--;
	if (ret) {
		CVEC_MEMMOVE(ret, &vec->a[vec->size*vec->elem_size], vec->elem_size);
	}
	if (vec->elem_free) {
		vec->elem_free(&vec->a[vec->size*vec->elem_size]);
	}
}

/** Same as pop except no elem_free even if it's set. */
void cvec_popm_void(cvector_void* vec, void* ret)
{
	vec->size--;
	if (ret) {
		CVEC_MEMMOVE(ret, &vec->a[vec->size*vec->elem_size], vec->elem_size);
	}
}

/** Return pointer to last element */
void* cvec_back_void(cvector_void* vec)
{
	return &vec->a[(vec->size-1)*vec->elem_size];
}

/** Increase the size of the array num items.  Items
 *  are not initialized to anything! */
int cvec_extend_void(cvector_void* vec, cvec_sz num)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_VOID_START_SZ;
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

/** Return a void pointer to the ith element.
  * Another way to get elements from vector that is used in vector_tests.c
  * is a macro like this one
  * #define GET_ELEMENT(VEC,I,TYPE) ((TYPE*)&VEC.a[(I)*VEC.elem_size])
*/
void* cvec_get_void(cvector_void* vec, cvec_sz i)
{
	return &vec->a[i*vec->elem_size];
}

/**
 * Insert a at index i (0 based).
 * Everything from that index and right is shifted one to the right.
 */
int cvec_insert_void(cvector_void* vec, cvec_sz i, void* a)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_VOID_ALLOCATOR(vec->capacity);
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	CVEC_MEMMOVE(&vec->a[(i+1)*vec->elem_size], &vec->a[i*vec->elem_size], (vec->size-i)*vec->elem_size);

	if (vec->elem_init) {
		if (!vec->elem_init(&vec->a[i*vec->elem_size], a)) {
			CVEC_ASSERT(0);
			return 0;
		}
	} else {
		CVEC_MEMMOVE(&vec->a[i*vec->elem_size], a, vec->elem_size);
	}

	vec->size++;
	return 1;
}

/** Same as insert but no elem_init even if defined. */
int cvec_insertm_void(cvector_void* vec, cvec_sz i, void* a)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity == vec->size) {
		tmp_sz = CVEC_VOID_ALLOCATOR(vec->capacity);
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}
	CVEC_MEMMOVE(&vec->a[(i+1)*vec->elem_size], &vec->a[i*vec->elem_size], (vec->size-i)*vec->elem_size);

	CVEC_MEMMOVE(&vec->a[i*vec->elem_size], a, vec->elem_size);

	vec->size++;
	return 1;
}

/**
 * Insert the first num elements of array a at index i.
 * Note that it is the user's responsibility to pass in val_id
 * arguments.  Also CVEC_MEMMOVE is used (when there is no elem_init function)
 * so don't try to insert part of the vector array into itself
 * (that would require CVEC_MEMMOVE)
 */
int cvec_insert_array_void(cvector_void* vec, cvec_sz i, void* a, cvec_sz num)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz, j;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_VOID_START_SZ;
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[(i+num)*vec->elem_size], &vec->a[i*vec->elem_size], (vec->size-i)*vec->elem_size);
	if (vec->elem_init) {
		for (j=0; j<num; ++j) {
			if (!vec->elem_init(&vec->a[(j+i)*vec->elem_size], &((cvec_u8*)a)[j*vec->elem_size])) {
				CVEC_ASSERT(0);
				return 0;
			}
		}
	} else {
		CVEC_MEMMOVE(&vec->a[i*vec->elem_size], a, num*vec->elem_size);
	}
	vec->size += num;
	return 1;
}

/** Same as insert_array but no elem_init even if defined. */
int cvec_insert_arraym_void(cvector_void* vec, cvec_sz i, void* a, cvec_sz num)
{
	cvec_u8* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_VOID_START_SZ;
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[(i+num)*vec->elem_size], &vec->a[i*vec->elem_size], (vec->size-i)*vec->elem_size);

	CVEC_MEMMOVE(&vec->a[i*vec->elem_size], a, num*vec->elem_size);
	vec->size += num;
	return 1;
}

/**
 * Replace value at i with a, return old value in ret if non-NULL.
 */
int cvec_replace_void(cvector_void* vec, cvec_sz i, void* a, void* ret)
{
	if (ret) {
		CVEC_MEMMOVE(ret, &vec->a[i*vec->elem_size], vec->elem_size);
	} else if (vec->elem_free) {
		vec->elem_free(&vec->a[i*vec->elem_size]);
	}

	if (vec->elem_init) {
		if (!vec->elem_init(&vec->a[i*vec->elem_size], a)) {
			CVEC_ASSERT(0);
			return 0;
		}
	} else {
		CVEC_MEMMOVE(&vec->a[i*vec->elem_size], a, vec->elem_size);
	}
	return 1;
}

/**
 * Same as replace but no elem_free or elem_init even if they're defined.
 * Because it doesn't call elem_init, there's no chance of failure so there's
 * no return value.
 */
void cvec_replacem_void(cvector_void* vec, cvec_sz i, void* a, void* ret)
{
	if (ret) {
		CVEC_MEMMOVE(ret, &vec->a[i*vec->elem_size], vec->elem_size);
	}

	CVEC_MEMMOVE(&vec->a[i*vec->elem_size], a, vec->elem_size);
}

/**
 * Erases elements from start to end inclusive.
 * Example cvec_erase_void(myvec, 1, 3) would call elem_free (if an elem_free function was provided)
 * and remove elements at 1, 2, and 3 and the element that was at index 4 would now be at 1 etc.
 */
void cvec_erase_void(cvector_void* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz i;
	cvec_sz d = end - start + 1;
	if (vec->elem_free) {
		for (i=start; i<=end; i++) {
			vec->elem_free(&vec->a[i*vec->elem_size]);
		}
	}
	CVEC_MEMMOVE(&vec->a[start*vec->elem_size], &vec->a[(end+1)*vec->elem_size], (vec->size-1-end)*vec->elem_size);
	vec->size -= d;
}

/** Same as erase except it *does not* call elem_free */
void cvec_remove_void(cvector_void* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start*vec->elem_size], &vec->a[(end+1)*vec->elem_size], (vec->size-1-end)*vec->elem_size);
	vec->size -= d;
}

/** Makes sure capacity >= size (the parameter not the member). */
int cvec_reserve_void(cvector_void* vec, cvec_sz size)
{
	cvec_u8* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*(size+CVEC_VOID_START_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_VOID_START_SZ;
	}
	return 1;
}

/** Set capacity to size.
 * You will lose data if you shrink the capacity below the current size.
 * If you do, the size will be set to capacity of course.
*/
int cvec_set_cap_void(cvector_void* vec, cvec_sz size)
{
	cvec_sz i;
	cvec_u8* tmp;
	if (size < vec->size) {
		if (vec->elem_free) {
			for (i=vec->size-1; i>=size; i--) {
				vec->elem_free(&vec->a[i*vec->elem_size]);
			}
		}
		vec->size = size;
	}

	vec->capacity = size;

	if (!(tmp = (cvec_u8*)CVEC_REALLOC(vec->a, vec->elem_size*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec-> a = tmp;
	return 1;
}

/** Set all size elements to val. */
int cvec_set_val_sz_void(cvector_void* vec, void* val)
{
	cvec_sz i;

	if (vec->elem_free) {
		for(i=0; i<vec->size; i++) {
			vec->elem_free(&vec->a[i*vec->elem_size]);
		}
	}
	
	if (vec->elem_init) {
		for (i=0; i<vec->size; i++) {
			if (!vec->elem_init(&vec->a[i*vec->elem_size], val)) {
				CVEC_ASSERT(0);
				return 0;
			}
		}
	} else {
		for (i=0; i<vec->size; i++) {
			CVEC_MEMMOVE(&vec->a[i*vec->elem_size], val, vec->elem_size);
		}
	}
	return 1;
}

/** Fills entire allocated array (capacity) with val.  If you set an elem_free function
 * then size is set to capacity like cvector_str for the same reason, ie I need to know
 * that the elem_free function needs to be called on those elements.
 * TODO Remove this function?  Same reason as set_val_cap_str.
 */
int cvec_set_val_cap_void(cvector_void* vec, void* val)
{
	cvec_sz i;
	if (vec->elem_free) {
		for (i=0; i<vec->size; i++) {
			vec->elem_free(&vec->a[i*vec->elem_size]);
		}
		vec->size = vec->capacity;
	}

	if (vec->elem_init) {
		for (i=0; i<vec->capacity; i++) {
			if (!vec->elem_init(&vec->a[i*vec->elem_size], val)) {
				CVEC_ASSERT(0);
				return 0;
			}
		}
	} else {
		for (i=0; i<vec->capacity; i++) {
			CVEC_MEMMOVE(&vec->a[i*vec->elem_size], val, vec->elem_size);
		}
	}
	return 1;
}

/** Sets size to 0 (does not change contents unless elem_free is set
 *  then it will elem_free all size elements as in cvector_str). */
void cvec_clear_void(cvector_void* vec)
{
	cvec_sz i;
	if (vec->elem_free) {
		for (i=0; i<vec->size; ++i) {
			vec->elem_free(&vec->a[i*vec->elem_size]);
		}
	}
	vec->size = 0;
}

/** Frees everything so don't use vec after calling this. If you set an elem_free function
 * it will be called on all size elements of course. Passing NULL is a NO-OP, matching the behavior
 * of free(). */
void cvec_free_void_heap(void* vec)
{
	cvec_sz i;
	cvector_void* tmp = (cvector_void*)vec;
	if (!tmp) return;
	if (tmp->elem_free) {
		for (i=0; i<tmp->size; i++) {
			tmp->elem_free(&tmp->a[i*tmp->elem_size]);
		}
	}
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

/** Frees the internal array and sets it, size, and capacity to NULL/0 to
 * maintain a consistent state */
void cvec_free_void(void* vec)
{
	cvec_sz i;
	cvector_void* tmp = (cvector_void*)vec;
	if (tmp->elem_free) {
		for (i=0; i<tmp->size; i++) {
			tmp->elem_free(&tmp->a[i*tmp->elem_size]);
		}
	}

	CVEC_FREE(tmp->a);
	tmp->a = NULL;
	tmp->size = 0;
	tmp->capacity = 0;
}

/*! \mainpage CVector notes
 *

\section Intro
This is a relatively simple ANSI compliant C vector library with specific structures and
functions for int's, double's and string's and support for all other types
using either a generic structure where the type is passed in as void* and stored in a cvec_u8 array
(to avoid dereferencing void* warnings and frequent casting) or generated type-specific
vectors using a macro or template system (see below).

The generic vector is very flexible and allows you to provide free and init functions
if you like that it will call at appropriate times similar to the way C++ containers
will call destructors/constructors.

Other modifiable parameters are at the top of the respective cvector.c's
<pre>
cvec_sz CVEC_I_START_SZ = 50;
cvec_sz CVEC_D_START_SZ = 50;
cvec_sz CVEC_STR_START_SZ = 20;
cvec_sz CVEC_VOID_START_SZ = 20;

#define CVEC_I_ALLOCATOR(x) ((x+1) * 2)
#define CVEC_D_ALLOCATOR(x) ((x+1) * 2)
#define CVEC_STR_ALLOCATOR(x) ((x+1) * 2)
#define CVEC_VOID_ALLOCATOR(x) ((x+1) * 2)
</pre>
The allocator macros are used in all functions that increase the size by 1.
In others (constructors, insert_array, reserve) CVEC_X_START_SZ is the amount
extra allocated.

Note that the (x+1) portion allows you to use the non-void vectors
without calling any of the init functions first *if* you zero them out.  This
means size, capacity, and a are 0/NULL which is valid because realloc acts like
malloc when given a NULL pointer.  With cvector_void you still have to set
elem_size, and optionally elem_free/elem_init. See the zero_init_x_test()'s
in cvector_tests.c for example of that use.

The `cvec_sz` type defaults to `size_t` but you can define CVEC_SIZE_T to your
preferred type before including the header which in turn is then `typedef`'d
to `cvec_sz`.  It has to be defined before every header inclusion.  Note, if
you use a signed type, passing a negative value is undefined behavior
(ie it'll likely crash immediately).  Of course if you passed a negative while
using the default `size_t` you'd probably crash anyway as it would wrap around
to a problematically large number.

There are also 2 templates, one for basic types and one for types that contain
dynamically allocated memory and you might want a free and/or init function.
In other words the first template is based off cvector_i and the second is based
off of cvector_void, so look at the corresponding documentation for behavior.

There are 2 ways to use/create your own cvector types.  The easiest way is to use
the macros defined in cvector_macro.h which are also included in the all-in-one header
cvector.h.  You can see how to use them in cvector_tests.c:

	#define RESIZE(a) (((a)+1)*2)

	CVEC_NEW_DECLS(short)
	CVEC_NEW_DECLS2(f_struct)

	CVEC_NEW_DEFS(short, RESIZE)
	CVEC_NEW_DEFS2(f_struct, RESIZE)

The RESIZE macro has to be defined before using the macros for now, serving the
same purpose as the regular allocator macros above.  Obviously the DECL macros
declare type and prototypes while the DEFS define them.  Using the macros for
user made types is much easier than the files because you can call the macro
right in the header where you define the type instead of having to include the
type in the generated file.  Basically 1 step rather than 2-3 and no extra files
needed.

The other way, and the only way in previous versions of CVector, is to generate
your own files from the template files which are located in cvector_template.h
and cvector_template2.h.

To generate your own cvector files for a type just run:

	python3 generate_code.py yourtype

which will generate the results for both templates so just delete the one
you don't want.

cvector_short.h and cvector_f_struct.h are examples of the generated files.

\section des_notes Design Notes
With the exception of CVEC_STRDUP calls in cvector_str, memory allocations are checked and asserted.
I decided that the likelihood of individual string allocations failing is low enough that it wasn't
worth the slowdown or extra code.  However, the equivalent calls to elem_init in vector_void and
generated vector_TYPEs are checked/asserted (since they're more likely to be large enough to possibly
fail).  If not in debug mode (ie NDEBUG is defined) 0 is returned on allocation failure.

For functions that take a ret parameter (ie pop and replace functions for str, void, and type 2
template/macro generated vectors), NULL is a valid option if you don't care to get the value.
I didn't want to force users to make a temporary variable just to catch something they weren't
going to use.

No other error checking is performed.  If you pass bad parameters (ie NULL for the vector
pointer, or end < start for the range functions), bad things will happen.
This is consistent with my belief that it is the caller's responsibility to pass valid arguments
and library code shouldn't be larger/uglier/slower for everyone just to pretty print errors.  This
is also consistent with the C standard library where, for the most part, passing invalid parameters
results in undefined behavior (see section 4.1.6 in C89, 7.1.4 in C99 and C11).

The back functions simply return the address of size - 1.  This is fine even if your size is zero
for the use of <= back_i(myvec) since the beginning of the array will already be > back.  If I were
to return NULL in the case of size 0, you'd just exchange a possible size check before the call for
a possible NULL check after the call.  I choose this way because it doesn't add an if check
to the function so it's smaller/faster, I think the <= use case is more likely, and it's easier
and more normal to know when your vector is empty than to remember to check for NULL after the fact.

The insert functions (insert_i and insert_array_i for example) do allow you to insert at the end.
The CVEC_MEMMOVE inside the functions will simply move 0 bytes if you pass the current size as the index.
C99 and C11 guarrantee this behavior in the standard (and thus C++ does as well).  Though I wrote
this library to be compliant with C89, which does not guarrantee this behavior, I think
it's safe to assume they'd use the same implementation since it doesn't contradict C89 and it
just makes sense.

\section Building
I use premake generated makefiles which are
included in the build subdirectory.  However, if you modified premake4.lua
the command to regenerate them is `premake4 gmake`.  cd into the build
directory and run `make` or `make config=release`. I have not tried it on
windows though it should work (well I'm not sure about CUnit ...).

There is no output of any kind, no errors or warnings.

It has been relatively well tested using CUnit tests which all pass.
I've also run it under valgrind and there are no memory leaks.

<pre>
$ valgrind --leak-check=full -v ./cvector
==116175==
==116175== HEAP SUMMARY:
==116175==     in use at exit: 0 bytes in 0 blocks
==116175==   total heap usage: 10,612 allocs, 10,612 frees, 1,151,748 bytes allocated
==116175==
==116175== All heap blocks were freed -- no leaks are possible
==116175==
==116175== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
$ grep FAIL CUnitAutomated-Results.xml
<FAILED> 0 </FAILED> 
<FAILED> 0 </FAILED> 
<FAILED> 0 </FAILED> 
</pre>

You can probably get Cunit from your package manager but
if you want to get the most up to date version of CUnit go here:

http://cunit.sourceforge.net/index.html
http://sourceforge.net/projects/cunit/

I'm using version 2.1-3.

\section Usage
To actually use the library just copy the appropriate c/h file pair(s) to your project
or just use cvector.h.  To get a good idea of how to use the library and see it in
action and how it should behave, look at cvector_tests.c

\section LICENSE
CVector is licensed under the MIT License.

Copyright (c) 2011-2025 Robert Winkler

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
 *
 *
 */


#endif


//#endif




// NOTE(rswinkle): string sorting taken from
// https://github.com/nothings/stb-imv/blob/master/imv.c
//
// derived from michael herf's code: http://www.stereopsis.com/strcmp4humans.html
//
// Also see GNU strverscmp for similar functionality

// sorts like this:
//     foo.jpg
//     foo1.jpg
//     foo2.jpg
//     foo10.jpg
//     foo_1.jpg
//     food.jpg

// maybe I should just live with "long" and prosper...


// use upper, not lower, to get better sorting versus '_'
// no, use lower not upper, to get sorting that matches explorer
extern inline char tupper(char b)
{
	if (b >= 'A' && b <= 'Z') return b - 'A' + 'a';
	//if (b >= 'a' && b <= 'z') return b - 'a' + 'A';
	return b;
}

extern inline char isnum(char b)
{
	if (b >= '0' && b <= '9') return 1;
	return 0;
}

extern inline i64 parsenum(char **a_p)
{
	char *a = *a_p;
	i64 result = *a - '0';
	++a;

	while (isnum(*a)) {
		// signed integer overflow undefined behavior on very large numbers
		// do I care about that here?
		result *= 10;
		result += *a - '0';
		++a;
	}

	*a_p = a-1;
	return result;
}

int StringCompare(char *a, char *b)
{
   char *orig_a = a, *orig_b = b;

	if (a == b) return 0;

	if (a == NULL) return -1;
	if (b == NULL) return 1;

	while (*a && *b) {

		i64 a0, b0; // will contain either a number or a letter

		if (isnum(*a) && isnum(*b)) {
			a0 = parsenum(&a);
			b0 = parsenum(&b);
		} else {
		// if they are mixed number and character, use ASCII comparison
		// order between them (number before character), not herf's
		// approach (numbers after everything else). this produces the order:
		//     foo.jpg
		//     foo1.jpg
		//     food.jpg
		//     foo_.jpg
		// which I think looks better than having foo_ before food (but
		// I could be wrong, given how a blank space sorts)

			a0 = tupper(*a);
			b0 = tupper(*b);
		}

		if (a0 < b0) return -1;
		if (a0 > b0) return 1;

		++a;
		++b;
	}

	if (*a) return 1;
	if (*b) return -1;

	{
		// if strings differ only by leading 0s, use case-insensitive ASCII sort
		// (note, we should work this out more efficiently by noticing which one changes length first)
		int z = strcasecmp(orig_a, orig_b);
		if (z) return z;
		// if identical case-insensitive, return ASCII sort
		return strcmp(orig_a, orig_b);
	}
}

int StringCompareSort(const void *p, const void *q)
{
   return StringCompare(*(char **) p, *(char **) q);
}



CVEC_NEW_DEFS2(file, RESIZE)

void free_file(void* f)
{
	free(((file*)f)->path);
}

// comparison functions
int filename_cmp_lt(const void* a, const void* b)
{
	return StringCompare(((file*)a)->name, ((file*)b)->name);
}

int filename_cmp_gt(const void* a, const void* b)
{
	return StringCompare(((file*)b)->name, ((file*)a)->name);
}

int filepath_cmp_lt(const void* a, const void* b)
{
	return StringCompare(((file*)a)->path, ((file*)b)->path);
}

int filepath_cmp_gt(const void* a, const void* b)
{
	return StringCompare(((file*)b)->path, ((file*)a)->path);
}

int filesize_cmp_lt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->size < f2->size)
		return -1;
	if (f1->size > f2->size)
		return 1;

	return 0;
}

int filesize_cmp_gt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->size > f2->size)
		return -1;
	if (f1->size < f2->size)
		return 1;

	return 0;
}

int filemodified_cmp_lt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->modified < f2->modified)
		return -1;
	if (f1->modified > f2->modified)
		return 1;

	return 0;
}

int filemodified_cmp_gt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->modified > f2->modified)
		return -1;
	if (f1->modified < f2->modified)
		return 1;

	return 0;
}


// pulls in file and cvector


#include <stdio.h>
#include <ctype.h>

// for realpath/_fullpath
#include <stdlib.h>

//POSIX (mostly) works with MinGW64
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// for getuid
#include <unistd.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define myrealpath(A, B) _fullpath(B, A, 0)
#else
// for getpwuid
#include <pwd.h>

#define myrealpath(A, B) realpath(A, B)
#endif


const char* get_homedir(void)
{
	const char* home = getenv("HOME");
#ifdef _WIN32
	if (!home) home = getenv("USERPROFILE");
#else
	if (!home) home = getpwuid(getuid())->pw_dir;
#endif
	return home;
}

// TODO pass extensions?
int init_file_browser(file_browser* browser, const char** exts, int num_exts, const char* start_dir, recents_func r_func, void* userdata)
{
	memset(browser, 0, sizeof(file_browser));
	
	const char* home = get_homedir();

	size_t l = 0;
	strncpy(browser->home, home, MAX_PATH_LEN);
#ifdef _WIN32
	normalize_path(browser->home);
#endif
	browser->home[MAX_PATH_LEN - 1] = 0;

	home = browser->home;
	const char* sd = home;
	if (start_dir) {
		l = strlen(start_dir);
		struct stat file_stat;
		if (stat(start_dir, &file_stat)) {
			perror("Could not stat start_dir, will use home directory");
		} else if (l >= MAX_PATH_LEN) {
			fprintf(stderr, "start_dir path too long, will use home directory\n");
		} else {
			sd = start_dir;
		}
	}
	snprintf(browser->dir, MAX_PATH_LEN, "%s", sd);
	// cut off trailing '/'
	if (l > 1 && sd[l-1] == '/') {
		browser->dir[l-1] = 0;
	}

	// TODO snprintf instead of strncpy everywhere?
	strcpy(browser->desktop, browser->home);
	l = strlen(browser->desktop);
	strcpy(browser->desktop + l, "/Desktop");

	browser->files.elem_free = free_file;

#ifdef FILE_LIST_SZ
	browser->end = FILE_LIST_SZ;
#endif

	browser->exts = exts;
	browser->num_exts = num_exts;

	fb_scandir(&browser->files, browser->dir, exts, num_exts, 0, 0);

	qsort(browser->files.a, browser->files.size, sizeof(file), filename_cmp_lt);
	browser->sorted_state = FB_NAME_UP;
	browser->c_func = filename_cmp_lt;

	browser->get_recents = r_func;
	browser->userdata = userdata;

	return 1;
}

void reset_file_browser(file_browser* fb, char* start_dir)
{
	assert(fb->home[0]);
	assert(fb->dir[0]);
	assert(fb->desktop[0]);
	assert(fb->files.elem_free == free_file);

	// clear vectors and prior selection
	fb->is_search_results = FALSE;
	fb->select_dir = FALSE;
	fb->file[0] = 0;
	fb->text_len = 0;
	fb->text_buf[0] = 0;

	// TODO do we want to keep the old value?  I feel like not
	fb->ignore_exts = FALSE;

	// set start dir
	size_t l = 0;
	const char* sd = fb->dir;
	if (start_dir) {
		struct stat file_stat;
		if (stat(start_dir, &file_stat)) {
			perror("Could not stat start_dir, will use last directory");
		} else if (l >= MAX_PATH_LEN) {
			fprintf(stderr, "start_dir path too long, will use last directory\n");
		} else {
			sd = start_dir;
			cvec_clear_file(&fb->files);
			cvec_clear_i(&fb->search_results);
			snprintf(fb->dir, MAX_PATH_LEN, "%s", sd);
			l = strlen(start_dir);
		}
	}
	// cut off trailing '/'
	if (l > 1 && sd[l-1] == '/') {
		fb->dir[l-1] = 0;
	}

	// scan and sort
	fb_scandir(&fb->files, fb->dir, fb->exts, fb->num_exts, fb->show_hidden, fb->select_dir);

	qsort(fb->files.a, fb->files.size, sizeof(file), filename_cmp_lt);
	fb->sorted_state = FB_NAME_UP;
	fb->c_func = filename_cmp_lt;
}

void free_file_browser(file_browser* fb)
{
	cvec_free_file(&fb->files);
	cvec_free_i(&fb->search_results);
	memset(fb, 0, sizeof(file_browser));
}

void handle_recents(file_browser* fb)
{
	file f;
	struct stat file_stat;
	struct tm* tmp_tm;
	char* sep;
	char* ext = NULL;

	cvector_str recents = {0};
	int n = fb->get_recents(&recents, fb->userdata);

	fb->is_recents = TRUE;
	fb->dir[0] = 0;
	cvec_clear_file(&fb->files);
	fb->selection = 0;

	const char** exts = fb->exts;
	const int num_exts = fb->num_exts;

	char* p;
	int i, j;
	for (i=0; i<n; i++) {
		p = recents.a[i];

		if (!fb->ignore_exts && num_exts) {
			if ((ext = strrchr(p, '.'))) {
				for (j=0; j<num_exts; ++j) {
					if (!strcasecmp(ext, exts[j]))
						break;
				}
				if (j == num_exts) {
					//free(p);
					continue;
				}
			}
		}
		if (stat(p, &file_stat)) {
			perror("stat");
			//free(p);
		} else {
			f.size = S_ISREG(file_stat.st_mode) ? file_stat.st_size : -1;
			f.path = p;
			f.modified = file_stat.st_mtime;

			bytes2str(f.size, f.size_str, SIZE_STR_BUF);
			tmp_tm = localtime(&f.modified);
			strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T


			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;

			cvec_pushm_file(&fb->files, &f);

			// NULL out pointer in recents string since we moved ownership of
			// the path string to fb->files and don't want a heap use after free
			recents.a[i] = 0;
		}
	}

	qsort(fb->files.a, fb->files.size, sizeof(file), fb->c_func);
	fb->list_setscroll = TRUE;

	FB_LOG("Found %"PRIcv_sz" recent files\n", fb->files.size);

	cvec_free_str(&recents);
}

// toggles search state on sort_type, if it's not sorted that way ascending
// it does that, otherwise in does descending
void fb_sort_toggle(file_browser* fb, int sort_type)
{
	// convert FB_NAME/FB_DOWN to FB_NAME_UP/FB_SIZE_UP etc.
	sort_type *= 2;

	cmp_func cmps[6] = {
		filename_cmp_lt,
		filename_cmp_gt,
		filesize_cmp_lt,
		filesize_cmp_gt,
		filemodified_cmp_lt,
		filemodified_cmp_gt
	};

	// convert to DOWN variant if already in UP variant
	sort_type += fb->sorted_state == sort_type;

	qsort(fb->files.a, fb->files.size, sizeof(file), cmps[sort_type]);
	fb->sorted_state = sort_type;
	fb->c_func = cmps[sort_type];

	if (fb->is_search_results) {
		fb_search_filenames(fb);
	}
}

void fb_search_filenames(file_browser* fb)
{
	// fast enough to do here?  I do it in events?
	char* text = fb->text_buf;
	text[fb->text_len] = 0;
	
	FB_LOG("Final text = \"%s\"\n", text);

	// strcasestr is causing problems on windows
	// so just convert to lower before using strstr
	char lowertext[STRBUF_SZ] = { 0 };
	char lowername[STRBUF_SZ] = { 0 };

	// start at 1 to cut off '/'
	for (int i=0; text[i]; ++i) {
		lowertext[i] = tolower(text[i]);
	}

	cvector_file* files = &fb->files;

	// it'd be kind of cool to add results of multiple searches together if we leave this out
	// of course there might be duplicates.  Or we could make it search within the existing
	// search results, so consecutive searches are && together...
	fb->search_results.size = 0;
	
	int j;
	for (int i=0; i<files->size; ++i) {

		for (j=0; files->a[i].name[j]; ++j) {
			lowername[j] = tolower(files->a[i].name[j]);
		}
		lowername[j] = 0;

		// searching name since I'm showing names not paths in the list
		if (strstr(lowername, lowertext)) {
			FB_LOG("Adding %s\n", files->a[i].path);
			cvec_push_i(&fb->search_results, i);
		}
	}
	FB_LOG("found %d matches\n", (int)fb->search_results.size);
}

// Enough arguments now that I'm thinking of just passing file_browser* and accessing them as members
int fb_scandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts, int show_hidden, int select_dir)
{
	assert(!num_exts || exts);

	char fullpath[STRBUF_SZ] = { 0 };
	struct stat file_stat;
	struct dirent* entry;
	int ret, i=0;
	DIR* dir;
	struct tm* tmp_tm;

	cvec_clear_file(files);

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		return 0;
	}

	char* tmp;
	char* sep;
	char* ext = NULL;
	file f;
	
	// This is to turn windows drives like C:/ into C: so the fullpath below doesn't become C://subdir
	// Can't remove the / before caling opendir or it won't work
	int l = strlen(dirpath);
	const char* fmt_strs[] = { "%s/%s", "%s%s" };
	int has_ts = dirpath[l-1] == '/';

	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? always ignore "." and ".." and all . files unless show_hidden
		if (entry->d_name[0] == '.' && ((!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2])) || !show_hidden)) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, fmt_strs[has_ts], dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			// path too long
			assert(ret >= STRBUF_SZ);
			return 0;
		}
		if (stat(fullpath, &file_stat)) {
			FB_LOG("%s\n", fullpath);
			perror("stat");
			continue;
		}

		if (!S_ISREG(file_stat.st_mode) && !S_ISDIR(file_stat.st_mode)) {
			continue;
		}

		if (S_ISREG(file_stat.st_mode)) {
			if (select_dir) {
				continue;
			}
			f.size = file_stat.st_size;

			ext = strrchr(entry->d_name, '.');

			// NOTE Purposely leaving files with no extension in
			if (ext && num_exts) {
				for (i=0; i<num_exts; ++i) {
					if (!strcasecmp(ext, exts[i]))
						break;
				}
				if (i == num_exts)
					continue;
			}
		} else {
			f.size = -1;
		}

		tmp = myrealpath(fullpath, NULL);
		f.path = realloc(tmp, strlen(tmp)+1);
#ifdef _WIN32
		normalize_path(f.path);
#endif

		f.modified = file_stat.st_mtime;

		// f.size set above separately for files vs directories
		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(files, &f);
	}

	FB_LOG("Found %"PRIcv_sz" files in %s\n", files->size, dirpath);

	closedir(dir);
	return 1;
}

// works same as SUSv2 libgen.h dirname except that
// dirpath is user provided output buffer, assumed large
// enough, return value is dirpath
char* mydirname(const char* path, char* dirpath)
{
	if (!path || !path[0]) {
		dirpath[0] = '.';
		dirpath[1] = 0;
		return dirpath;
	}

	// TODO doesn't correctly handle "/" "/hello" or anything that ends in a '/' like
	// "/some/random/dir/"
	char* last_slash = strrchr(path, PATH_SEPARATOR);
	if (last_slash) {
		strncpy(dirpath, path, last_slash-path);
		dirpath[last_slash-path] = 0;
	} else {
		dirpath[0] = '.';
		dirpath[1] = 0;
	}

	return dirpath;
}

// same as SUSv2 basename in libgen.h except base is output
// buffer
char* mybasename(const char* path, char* base)
{
	if (!path || !path[0]) {
		base[0] = '.';
		base[1] = 0;
		return base;
	}

	int end = strlen(path) - 1;

	if (path[end] == PATH_SEPARATOR)
		end--;

	int start = end;
	while (path[start] != PATH_SEPARATOR && start != 0)
		start--;
	if (path[start] == PATH_SEPARATOR)
		start++;

	memcpy(base, &path[start], end-start+1);
	base[end-start+1] = 0;

	return base;
}

//stupid windows
void normalize_path(char* path)
{
	if (path) {
		for (int i=0; path[i]; ++i) {
			if (path[i] == '\\') {
				path[i] = '/';
			}
		}
	}
}

int bytes2str(int bytes, char* buf, int len)
{
	// empty string for negative numbers
	if (bytes < 0) {
		buf[0] = 0;
		return 1;
	}

	// MiB KiB? 2^10, 2^20?
	// char* iec_sizes[3] = { "bytes", "KiB", "MiB" };
	char* si_sizes[3] = { "bytes", "KB", "MB" }; // GB?  no way

	char** sizes = si_sizes;
	int i = 0;
	double sz = bytes;
	if (sz >= 1000000) {
		sz /= 1000000;
		i = 2;
	} else if (sz >= 1000) {
		sz /= 1000;
		i = 1;
	} else {
		i = 0;
	}

	int ret = snprintf(buf, len, ((i) ? "%.1f %s" : "%.0f %s") , sz, sizes[i]);
	if (ret >= len)
		return 0;

	return 1;
}


void switch_dir(file_browser* fb, const char* dir)
{
	if (dir) {
		if (!strncmp(fb->dir, dir, MAX_PATH_LEN)) {
			FB_LOG("No need to switch to %s\n", dir);
			return;
		}
		strncpy(fb->dir, dir, MAX_PATH_LEN);
	}

	fb->is_recents = FALSE;
	fb->is_search_results = FALSE;
	fb->text_buf[0] = 0;
	fb->text_len = 0;

	FB_LOG("switching to '%s'\n", fb->dir);
#ifndef _WIN32
	fb_scandir(&fb->files, fb->dir, fb->exts, (fb->ignore_exts) ? 0 : fb->num_exts, fb->show_hidden, fb->select_dir);
#else
	if (fb->dir[1]) {
		fb_scandir(&fb->files, fb->dir, fb->exts, (fb->ignore_exts) ? 0 : fb->num_exts, fb->show_hidden, fb->select_dir);
	} else {
		// have to handle "root" special on windows since it doesn't have a unified filesystem
		// like *nix
		char buf[STRBUF_SZ];
		cvec_clear_file(&fb->files);
		int sz = GetLogicalDriveStrings(sizeof(buf), buf);
		file f = {0};
		f.size = -1;
		if (sz > 0) {
			char* p = buf, *p2;
			while (*p && (p2 = strchr(p, 0))) {
				p[2] = '/'; // change \ to / so "C:/" instead of "C:\"

				f.path = strdup(p);
				f.name = f.path;
				cvec_push_file(&fb->files, &f);

				p = p2+1;
			}
		} else {
			DWORD err = GetLastError();
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buf, sizeof(buf), 0);
			FB_LOG("Error getting drive names: %s\n", buf);
		}
	}
#endif
	qsort(fb->files.a, fb->files.size, sizeof(file), fb->c_func);
	fb->list_setscroll = TRUE;
	fb->selection = 0;

#ifdef FILE_LIST_SZ
	fb->begin = 0;
#endif
}
#undef FILE_BROWSER_IMPLEMENTATION
#endif
