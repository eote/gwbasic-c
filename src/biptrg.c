/* BIPTRG - Array/Pointer Handling */
/* Based on BIPTRG.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>

/* Array descriptor structure */

void biptrg_init(void) {
    /* No initialization needed */
}


/* ===== v141 MEDIUM-1: implicit array declaration =====
 * GW-BASIC ASM: an array reference to an un-DIMed name auto-declares it
 * with subscript 0..10 (11 elements) on first use (FNDFRD/ARRAY machinery).
 * Type inferred from the name's type suffix ($ # ! %), default single. */
static var_type_t biptrg_infer_type_from_name(const char *name) {
    size_t n = strlen(name);
    if (n == 0) return VT_SINGLE;
    char last = name[n - 1];
    if (last == '$') return VT_STRING;
    if (last == '#') return VT_DOUBLE;
    if (last == '%') return VT_INTEGER;
    return VT_SINGLE;   /* ! or no suffix */
}

/* v162 HIGH-2: ndims-aware forward declaration */
struct variable *biptrg_ensure_array_nd(const char *name, int ndims);

/* Ensure an array variable exists; if the name is unknown, auto-DIM it as
 * A(10) (indices 0..10). Returns the variable or NULL.
 * v161 HIGH-2 (v162): ndims-aware variant — GW-BASIC auto-DIMs an
 * undeclared array with 10 per DIMENSION (A(1,2) implies 2-D 0..10). */
struct variable *biptrg_ensure_array(const char *name) {
    /* 1-D convenience: NO rank check (multi-dim arrays indexed via
     * the _multi interfaces are legitimate; only the nd-aware call
     * sites validate rank) */
    struct variable *v = gwdata_find_variable(name);
    if (v) return v;
    var_type_t type = biptrg_infer_type_from_name(name);
    if (biptrg_dim_array(name, 10, type) != SUCCESS) return NULL;
    return gwdata_find_variable(name);
}

struct variable *biptrg_ensure_array_nd(const char *name, int ndims) {
    struct variable *v = gwdata_find_variable(name);
    if (v) {
        /* v163 HIGH-2: rank mismatch — an existing 1-D array indexed
         * with 2+ subscripts is a Subscript out of range (ASM FCERRG) */
        if (v->is_array) {
            array_desc_t *d = (array_desc_t *)v->value.s.ptr;
            if (d && d->magic == ARRAY_MAGIC && d->dimensions != ndims) {
                extern uint16_t g_curlin;
                gwdata_set_error(ERRBS, g_curlin, 0);
            }
        }
        return v;
    }
    var_type_t type = biptrg_infer_type_from_name(name);
    if (ndims <= 1) {
        if (biptrg_dim_array(name, 10, type) != SUCCESS) return NULL;
    } else {
        int dims[8];
        for (int i = 0; i < ndims && i < 8; i++) dims[i] = 10;
        if (biptrg_dim_array_multi(name, dims, ndims, type) != SUCCESS) return NULL;
    }
    return gwdata_find_variable(name);
}

/* DIM statement - create array */
int biptrg_dim_array(const char *name, int size, var_type_t type) {
    if (!name || size <= 0) return ERROR;
    
    struct variable *v = gwdata_alloc_variable(name, type);
    if (!v) return ERROR;
    
    /* Create array descriptor */
    array_desc_t *desc = (array_desc_t *)gwram_alloc(sizeof(array_desc_t));
    if (!desc) return ERROR;
    
    desc->magic = ARRAY_MAGIC;
    desc->type = type;
    desc->size = size + 1;  /* GW-BASIC: DIM A(n) creates indices 0..n (n+1 elements) */
    desc->dimensions = 1;
    desc->dim_sizes = (int *)gwram_alloc(sizeof(int));
    if (!desc->dim_sizes) {
        gwram_free(desc);
        return ERROR;
    }
    desc->dim_sizes[0] = size + 1;  /* Store actual allocated size */
    
    /* Allocate array data */
    size_t element_size = (type == VT_STRING) ? sizeof(char *) : 
                          (type == VT_INTEGER) ? sizeof(int32_t) :
                          (type == VT_DOUBLE) ? sizeof(double) : sizeof(float);
    
    desc->data = gwram_calloc(size + 1, element_size);
    if (!desc->data) {
        gwram_free(desc->dim_sizes);
        gwram_free(desc);
        return ERROR;
    }
    
    /* v164 HIGH-1: string array elements default to the EMPTY string
     * (GW-BASIC semantics), not NULL */
    if (type == VT_STRING) {
        char **sdata = (char **)desc->data;
        for (int i = 0; i <= size; i++) {
            sdata[i] = gwram_alloc(1);
            if (sdata[i]) sdata[i][0] = '\0';
        }
    }
    
    /* Store descriptor in variable's string pointer (reuse for array) */
    v->value.s.ptr = (char *)desc;
    v->type = type;  /* Keep original type for identification */
    v->is_array = true;  /* Mark as array to distinguish from scalar */
    
    return SUCCESS;
}

/* DIM statement - create multi-dimensional array */
int biptrg_dim_array_multi(const char *name, const int *dim_sizes, int dimensions, var_type_t type) {
    if (!name || !dim_sizes || dimensions <= 0 || dimensions > 8) return ERROR;
    
    struct variable *v = gwdata_alloc_variable(name, type);
    if (!v) return ERROR;
    
    /* Calculate total size - GW-BASIC: DIM A(m,n) creates indices 0..m, 0..n (m+1)*(n+1) elements */
    int total_size = 1;
    for (int i = 0; i < dimensions; i++) {
        if (dim_sizes[i] < 0) {
            gwram_free(v);
            return ERROR;
        }
        total_size *= (dim_sizes[i] + 1);
    }
    
    /* Create array descriptor */
    array_desc_t *desc = (array_desc_t *)gwram_alloc(sizeof(array_desc_t));
    if (!desc) return ERROR;
    
    desc->magic = ARRAY_MAGIC;
    desc->type = type;
    desc->size = total_size;
    desc->dimensions = dimensions;
    desc->dim_sizes = (int *)gwram_alloc(dimensions * sizeof(int));
    if (!desc->dim_sizes) {
        gwram_free(desc);
        return ERROR;
    }
    for (int i = 0; i < dimensions; i++) {
        desc->dim_sizes[i] = dim_sizes[i] + 1;  /* Store actual allocated size per dimension */
    }
    
    /* Allocate array data */
    size_t element_size = (type == VT_STRING) ? sizeof(char *) : 
                          (type == VT_INTEGER) ? sizeof(int32_t) :
                          (type == VT_DOUBLE) ? sizeof(double) : sizeof(float);
    
    desc->data = gwram_calloc(total_size, element_size);
    if (!desc->data) {
        gwram_free(desc->dim_sizes);
        gwram_free(desc);
        return ERROR;
    }
    
    /* v164 HIGH-1: string array elements default to EMPTY strings */
    if (type == VT_STRING) {
        char **sdata = (char **)desc->data;
        for (int i = 0; i < total_size; i++) {
            sdata[i] = gwram_alloc(1);
            if (sdata[i]) sdata[i][0] = '\0';
        }
    }
    
    /* Store descriptor in variable's string pointer (reuse for array) */
    v->value.s.ptr = (char *)desc;
    v->type = type;
    v->is_array = true;
    
    return SUCCESS;
}

/* Helper: compute flat index from multi-dimensional indices */
static int compute_flat_index(const array_desc_t *desc, const int *indices) {
    int flat_index = 0;
    int stride = 1;
    
    /* Row-major order: last dimension varies fastest */
    for (int i = desc->dimensions - 1; i >= 0; i--) {
        int idx = indices[i];
        if (idx < 0) idx = 0;
        if (idx >= desc->dim_sizes[i]) idx = desc->dim_sizes[i] - 1;
        flat_index += idx * stride;
        stride *= desc->dim_sizes[i];
    }
    
    return flat_index;
}

/* Set numeric array element (multi-dimensional) */
int biptrg_set_array_element_multi(const char *name, const int *indices, float val) {
    if (!name || !indices) return ERROR;
    
    DEBUG_PRINT("biptrg_set_array_element_multi: name='%s', indices=[%d, %d], val=%f", name, indices[0], indices[1], val);
    
    struct variable *v = biptrg_ensure_array(name);
    if (!v) return ERROR;
    
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) return ERROR;
    
    if (desc->magic != ARRAY_MAGIC) return ERROR;
    if (desc->type == VT_STRING) return ERROR;
    
    /* v141 MEDIUM-1: subscript out of range → ERRBS (ASM FCERRG) */
    for (int i = 0; i < desc->dimensions; i++) {
        if (indices[i] < 0 || indices[i] >= desc->dim_sizes[i]) {
            extern uint16_t g_curlin;
            gwdata_set_error(ERRBS, g_curlin, 0);
            return ERROR;
        }
    }
    
    int flat_index = compute_flat_index(desc, indices);
    DEBUG_PRINT("biptrg_set_array_element_multi: flat_index=%d, dim_sizes=[%d, %d], val=%f", flat_index, desc->dim_sizes[0], desc->dim_sizes[1], val);
    
    if (desc->type == VT_INTEGER) {
        int32_t *data = (int32_t *)desc->data;
        data[flat_index] = (int32_t)val;
    } else if (desc->type == VT_SINGLE) {
        float *data = (float *)desc->data;
        data[flat_index] = val;
    } else if (desc->type == VT_DOUBLE) {
        double *data = (double *)desc->data;
        data[flat_index] = (double)val;
    } else {
        return ERROR;
    }
    
    return SUCCESS;
}

/* Get numeric array element (multi-dimensional) */
float biptrg_get_array_element_multi(const char *name, const int *indices) {
    if (!name || !indices) return 0.0f;
    
    DEBUG_PRINT("biptrg_get_array_element_multi: name='%s', indices=[%d, %d]", name, indices[0], indices[1]);
    
    struct variable *v = biptrg_ensure_array(name);
    if (!v) return 0.0f;
    
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) return 0.0f;
    
    if (desc->magic != ARRAY_MAGIC) return 0.0f;
    if (desc->type == VT_STRING) return 0.0f;
    if (!desc->data) return 0.0f;
    
    /* v141 MEDIUM-1: subscript out of range → ERRBS (ASM FCERRG) */
    for (int i = 0; i < desc->dimensions; i++) {
        if (indices[i] < 0 || indices[i] >= desc->dim_sizes[i]) {
            extern uint16_t g_curlin;
            gwdata_set_error(ERRBS, g_curlin, 0);
            return 0.0f;
        }
    }
    
    int flat_index = compute_flat_index(desc, indices);
    DEBUG_PRINT("biptrg_get_array_element_multi: flat_index=%d, dim_sizes=[%d, %d]", flat_index, desc->dim_sizes[0], desc->dim_sizes[1]);
    
    if (desc->type == VT_INTEGER) {
        int32_t *data = (int32_t *)desc->data;
        float result = (float)data[flat_index];
        DEBUG_PRINT("biptrg_get_array_element_multi: returning %f (int)", result);
        return result;
    } else if (desc->type == VT_SINGLE) {
        float *data = (float *)desc->data;
        float result = data[flat_index];
        DEBUG_PRINT("biptrg_get_array_element_multi: returning %f (float)", result);
        return result;
    } else if (desc->type == VT_DOUBLE) {
        double *data = (double *)desc->data;
        float result = (float)data[flat_index];
        DEBUG_PRINT("biptrg_get_array_element_multi: returning %f (double)", result);
        return result;
    }
    
    return 0.0f;
}

/* Set numeric array element */
int biptrg_set_array_element(const char *name, int index, float val) {
    if (!name) return ERROR;
    
    DEBUG_PRINT("biptrg_set_array_element: name='%s', index=%d, val=%f", name, index, val);
    
    struct variable *v = biptrg_ensure_array(name);
    if (!v) {
        DEBUG_PRINT("biptrg_set_array_element: variable not found");
        return ERROR;
    }
    
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) {
        DEBUG_PRINT("biptrg_set_array_element: no descriptor");
        return ERROR;
    }
    
    /* Validate magic number and type */
    if (desc->magic != ARRAY_MAGIC) {
        DEBUG_PRINT("biptrg_set_array_element: magic mismatch");
        return ERROR;
    }
    if (desc->type == VT_STRING) {
        DEBUG_PRINT("biptrg_set_array_element: string array");
        return ERROR;  /* String arrays cannot be written as numeric */
    }
    
    DEBUG_PRINT("biptrg_set_array_element: desc->size=%d, desc->dimensions=%d", desc->size, desc->dimensions);
    
    /* v141 MEDIUM-1: subscript out of range → ERRBS (ASM FCERRG) */
    if (index < 0 || index >= desc->size) {
        extern uint16_t g_curlin;
        gwdata_set_error(ERRBS, g_curlin, 0);
        return ERROR;
    }
    
    DEBUG_PRINT("biptrg_set_array_element: final index=%d", index);
    
    if (desc->type == VT_INTEGER) {
        int32_t *data = (int32_t *)desc->data;
        data[index] = (int32_t)val;
    } else if (desc->type == VT_SINGLE) {
        float *data = (float *)desc->data;
        data[index] = val;
    } else if (desc->type == VT_DOUBLE) {
        double *data = (double *)desc->data;
        data[index] = (double)val;
    } else {
        return ERROR;
    }
    
    return SUCCESS;
}

/* Get numeric array element */
float biptrg_get_array_element(const char *name, int index) {
    if (!name) return 0.0f;
    
    DEBUG_PRINT("biptrg_get_array_element: name='%s', index=%d", name, index);
    
    struct variable *v = biptrg_ensure_array(name);
    if (!v) {
        DEBUG_PRINT("biptrg_get_array_element: variable not found");
        return 0.0f;
    }
    
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) {
        DEBUG_PRINT("biptrg_get_array_element: no descriptor");
        return 0.0f;
    }
    
    /* Validate magic number and type */
    if (desc->magic != ARRAY_MAGIC) {
        DEBUG_PRINT("biptrg_get_array_element: magic mismatch");
        return 0.0f;
    }
    if (desc->type == VT_STRING) {
        DEBUG_PRINT("biptrg_get_array_element: string array");
        return 0.0f;  /* String arrays cannot be read as numeric */
    }
    if (!desc->data) {
        DEBUG_PRINT("biptrg_get_array_element: no data");
        return 0.0f;  /* Defensive check */
    }
    
    DEBUG_PRINT("biptrg_get_array_element: desc->size=%d, desc->dimensions=%d", desc->size, desc->dimensions);
    
    /* v141 MEDIUM-1: subscript out of range → ERRBS (ASM FCERRG) */
    if (index < 0 || index >= desc->size) {
        extern uint16_t g_curlin;
        gwdata_set_error(ERRBS, g_curlin, 0);
        return 0.0f;
    }
    
    DEBUG_PRINT("biptrg_get_array_element: final index=%d", index);
    
    if (desc->type == VT_INTEGER) {
        int32_t *data = (int32_t *)desc->data;
        return (float)data[index];
    } else if (desc->type == VT_SINGLE) {
        float *data = (float *)desc->data;
        return data[index];
    } else if (desc->type == VT_DOUBLE) {
        double *data = (double *)desc->data;
        return (float)data[index];
    }
    
    return 0.0f;
}

/* Set string array element */
int biptrg_set_array_string(const char *name, int index, const char *str) {
    if (!name || !str) return ERROR;
    
    struct variable *v = biptrg_ensure_array(name);
    if (!v) return ERROR;
    
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) return ERROR;
    
    if (desc->type != VT_STRING) return ERROR;
    
    /* v141 MEDIUM-1: subscript out of range → ERRBS (ASM FCERRG) */
    if (index < 0 || index >= desc->size) {
        extern uint16_t g_curlin;
        gwdata_set_error(ERRBS, g_curlin, 0);
        return ERROR;
    }
    
    char **data = (char **)desc->data;
    
    /* Free existing string if any */
    if (data[index]) {
        gwram_free(data[index]);
    }
    
    /* Allocate and copy new string */
    data[index] = (char *)gwram_alloc(strlen(str) + 1);
    if (!data[index]) return ERROR;
    
    strcpy(data[index], str);
    return SUCCESS;
}

/* v161 HIGH-2: multi-dimensional string array element access.
 * Flattens indices via compute_flat_index (same layout as numeric
 * multi arrays). */
int biptrg_set_array_string_multi(const char *name, const int *indices, const char *str) {
    if (!name || !str || !indices) return ERROR;
    struct variable *v = biptrg_ensure_array(name);
    if (!v) return ERROR;
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc || desc->type != VT_STRING) return ERROR;
    for (int i = 0; i < desc->dimensions; i++) {
        if (indices[i] < 0 || indices[i] >= desc->dim_sizes[i]) {
            extern uint16_t g_curlin;
            gwdata_set_error(ERRBS, g_curlin, 0);
            return ERROR;
        }
    }
    int flat = compute_flat_index(desc, indices);
    char **data = (char **)desc->data;
    if (data[flat]) gwram_free(data[flat]);
    data[flat] = (char *)gwram_alloc(strlen(str) + 1);
    if (!data[flat]) return ERROR;
    strcpy(data[flat], str);
    return SUCCESS;
}

char *biptrg_get_array_string_multi(const char *name, const int *indices) {
    if (!name || !indices) return NULL;
    struct variable *v = biptrg_ensure_array(name);
    if (!v) return NULL;
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc || desc->type != VT_STRING) return NULL;
    for (int i = 0; i < desc->dimensions; i++) {
        if (indices[i] < 0 || indices[i] >= desc->dim_sizes[i]) {
            extern uint16_t g_curlin;
            gwdata_set_error(ERRBS, g_curlin, 0);
            return NULL;
        }
    }
    int flat = compute_flat_index(desc, indices);
    char **data = (char **)desc->data;
    return data[flat];
}

/* Get string array element */
char *biptrg_get_array_string(const char *name, int index) {
    if (!name) return NULL;
    
    struct variable *v = biptrg_ensure_array(name);
    if (!v) return NULL;
    
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) return NULL;
    
    if (desc->type != VT_STRING) return NULL;
    
    /* v141 MEDIUM-1: subscript out of range → ERRBS (ASM FCERRG) */
    if (index < 0 || index >= desc->size) {
        extern uint16_t g_curlin;
        gwdata_set_error(ERRBS, g_curlin, 0);
        return NULL;
    }
    
    char **data = (char **)desc->data;
    return data[index];
}

void biptrg_free_array(struct variable *v) {
    if (!v) return;
    array_desc_t *desc = (array_desc_t *)v->value.s.ptr;
    if (!desc) return;
    
    /* Validate that this is actually an array descriptor using magic number */
    if (desc->magic != ARRAY_MAGIC) {
        return;  // Not a valid array descriptor
    }
    
    if (desc->data) gwram_free(desc->data);
    if (desc->dim_sizes) gwram_free(desc->dim_sizes);
    gwram_free(desc);
    v->value.s.ptr = NULL;
    v->value.s.len = 0;
}