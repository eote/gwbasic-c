/* BIPTRG - Array/Pointer Handling */
/* Based on BIPTRG.ASM */

#include "gwbasic.h"
#include <stdlib.h>
#include <string.h>

/* Array descriptor structure */
typedef struct {
    var_type_t type;
    int size;
    int dimensions;
    int *dim_sizes;  /* For multi-dimensional arrays */
    void *data;      /* Array data */
} array_desc_t;

void biptrg_init(void) {
    /* No initialization needed */
}

/* DIM statement - create array */
int biptrg_dim_array(const char *name, int size, var_type_t type) {
    if (!name || size <= 0) return ERROR;
    
    struct variable *v = gwdata_alloc_variable(name, type);
    if (!v) return ERROR;
    
    /* Create array descriptor */
    array_desc_t *desc = (array_desc_t *)gwram_alloc(sizeof(array_desc_t));
    if (!desc) return ERROR;
    
    desc->type = type;
    desc->size = size;
    desc->dimensions = 1;
    desc->dim_sizes = (int *)gwram_alloc(sizeof(int));
    if (!desc->dim_sizes) {
        gwram_free(desc);
        return ERROR;
    }
    desc->dim_sizes[0] = size;
    
    /* Allocate array data */
    size_t element_size = (type == VT_STRING) ? sizeof(char *) : 
                          (type == VT_INTEGER) ? sizeof(int32_t) :
                          (type == VT_DOUBLE) ? sizeof(double) : sizeof(float);
    
    desc->data = gwram_calloc(size, element_size);
    if (!desc->data) {
        gwram_free(desc->dim_sizes);
        gwram_free(desc);
        return ERROR;
    }
    
    /* Store descriptor in variable's string pointer (reuse for array) */
    v->value.s = (char *)desc;
    v->type = type;  /* Keep original type for identification */
    
    return SUCCESS;
}

/* Set numeric array element */
int biptrg_set_array_element(const char *name, int index, float val) {
    if (!name) return ERROR;
    
    struct variable *v = gwdata_find_variable(name);
    if (!v) return ERROR;
    
    array_desc_t *desc = (array_desc_t *)v->value.s;
    if (!desc) return ERROR;
    
    if (index < 0 || index >= desc->size) return ERROR;
    
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
    
    struct variable *v = gwdata_find_variable(name);
    if (!v) return 0.0f;
    
    array_desc_t *desc = (array_desc_t *)v->value.s;
    if (!desc) return 0.0f;
    
    if (index < 0 || index >= desc->size) return 0.0f;
    
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
    
    struct variable *v = gwdata_find_variable(name);
    if (!v) return ERROR;
    
    array_desc_t *desc = (array_desc_t *)v->value.s;
    if (!desc) return ERROR;
    
    if (desc->type != VT_STRING) return ERROR;
    if (index < 0 || index >= desc->size) return ERROR;
    
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

/* Get string array element */
char *biptrg_get_array_string(const char *name, int index) {
    if (!name) return NULL;
    
    struct variable *v = gwdata_find_variable(name);
    if (!v) return NULL;
    
    array_desc_t *desc = (array_desc_t *)v->value.s;
    if (!desc) return NULL;
    
    if (desc->type != VT_STRING) return NULL;
    if (index < 0 || index >= desc->size) return NULL;
    
    char **data = (char **)desc->data;
    return data[index];
}