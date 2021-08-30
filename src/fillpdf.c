// Copyright 2021 John B Roark. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "../include/fillpdf.h"

int pdf_open(
        const char *in_filename,
        const char *fields,
        struct fillpdf *out,
        struct autobuffer *buf
)   {
    // input strings from the FFI might be volatile, its best we copy them to the heap
    unsigned len = strlen(in_filename);
    if (len == 0) { return 1; }

    out->in_filename = (char *) malloc(len + 1);
    strcpy(out->in_filename, in_filename);

    // open the template PDF
    GError *err = NULL; // <- must be initialized as NULL for some reason

    gchar *uri = g_filename_to_uri(in_filename, NULL, &err);
    if (uri == NULL) {
        g_free(uri);
        return 1;
    }

    out->template = poppler_document_new_from_file(uri, NULL, &err);
    g_free(uri);
    if (out->template == NULL) {
        return 1;
    }

    // initialize the json object parser
    if (strlen(fields) == 0) {
        return 1;
    }

    out->fields = json_tokener_parse(fields);
    out->buf    = buf;

    return 0;
}

int fill_template_text(struct fillpdf *fillablepdf) {
    int n = poppler_document_get_n_pages(fillablepdf->template);
    if (n == 0) { return 1; }

    GList *pdf_fields;
    struct json_object *obj;
    int n_maps = 0;

    for (int i = 0; i < n; i += 1) {
        PopplerPage *page = poppler_document_get_page(fillablepdf->template, i);
        pdf_fields = poppler_page_get_form_field_mapping(page);

        PopplerFormFieldMapping *fieldmap;
        n_maps = g_list_length(pdf_fields);

        for (int idx = 0; i < n_maps; i += 1) {
            fieldmap = g_list_nth_data(pdf_fields, i);
            if (fieldmap == NULL) {
                return 1;
            }
            gchar *field_name = poppler_form_field_get_name(fieldmap->field);

            if (!json_object_object_get_ex(fillablepdf->fields, field_name, &obj)) {
                continue; // no json object exists for this field
            }

            poppler_form_field_text_set_text(
                fieldmap->field,
                json_object_to_json_string(obj)
            );

            obj = NULL;
            g_free(field_name);
        }

        poppler_page_free_form_field_mapping(pdf_fields);
    }

    return 0;
}

void pdf_new_buffer(struct autobuffer *buf) {
    buf->data=NULL;
    buf->size=0;
    buf->available=0;
    buf->_next_allocation_size = 5000;
}

void pdf_free_buffer(struct autobuffer *buf) {
    free(buf->data);
    buf = NULL;
}

cairo_status_t pdf_write(void *b, unsigned char const *data, unsigned int len) {
    struct autobuffer* buf = b;
    size_t realloc_size = buf->size+len;

    if(buf->available < realloc_size) {
        while(buf->available <= realloc_size) {
            buf->data = realloc(
                buf->data,
                buf->available + buf->_next_allocation_size
            );
            buf->available += buf->_next_allocation_size;
            buf->_next_allocation_size *= 2;
        }
    }

    memcpy(&(buf->data[buf->size]), data, len);
    buf->size += len;

    return CAIRO_STATUS_SUCCESS;
}

int pdf_render(struct fillpdf *in) {
    PopplerPage *page;
    GError *err = NULL; // <- must be initialized as NULL for some reason
    cairo_status_t status;
    double width, height;

    page = poppler_document_get_page(in->template, 0);
    if (page == NULL) {
        return 1;
    }

    poppler_page_get_size(page, &width, &height);

    cairo_surface_t *pdf_surf = cairo_pdf_surface_create_for_stream(
        pdf_write,
        in->buf,
        width,
        height
    );

    cairo_t *context = cairo_create(pdf_surf);
    poppler_page_render_for_printing(page, context);
    cairo_surface_show_page(pdf_surf);
    g_object_unref(page);

    status = cairo_status(context);
    if (status) {
        printf("%s\n", cairo_status_to_string (status));
    }

    cairo_destroy(context);
    cairo_surface_finish(pdf_surf);

    status = cairo_surface_status(pdf_surf);
    if (status) {
        printf("%s\n", cairo_status_to_string (status));
    }

    cairo_surface_destroy(pdf_surf);

    return 0;
}

void pdf_close(struct fillpdf *data) {
    free(data->in_filename);
    free(data->fields);
    g_object_unref(data->template);
    pdf_free_buffer(data->buf);
    data = NULL;
}
