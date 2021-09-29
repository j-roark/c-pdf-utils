/*
cfillpdf
Copyright (C) 2021 John B Roark

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "../include/c_pdf_utils.h"

const char * pdf_err_str(int err_enum) {
    pdf_err err = (pdf_err) err_enum;
    switch (err) {
    case TemplateFilenameEmpty:
        return "input template filename is empty";

    case TemplateFilenameNotURI:
        return "filename is not a valid format (cannot convert to URI)";

    case TemplateInvalid:
        return "template cannot be opened";

    case TemplateEmpty:
        return "template has no pages";

    case TemplateNoFields:
        return "template has no fields";

    case TemplateNotExist:
        return "template file does not exist";

    case TemplatePageNotExist:
        return "page does not exist for this template";

    case NoFields:
        return "json string for field filling is empty";

    default:
        return "unkown error";
    }
}

int pdf_open(
    const char * in_filename,
    struct fillpdf * out,
    struct autobuffer * buf
) {
    // input strings from the FFI might be volatile, its best we copy them to the heap
    unsigned len = strlen(in_filename);
    if (len == 0) {
        return TemplateFilenameEmpty;
    }

    out -> in_filename = (char * ) malloc(len + 1);
    strcpy(out -> in_filename, in_filename);

    // open the template PDF

    // does the template pdf file exist?
    if (!access(out -> in_filename, F_OK) == 0) {
        return TemplateNotExist;
    }

    GError * err = NULL; // <- must be initialized as NULL for some reason

    gchar * uri = g_filename_to_uri(in_filename, NULL, & err);
    if (uri == NULL) {
        g_free(uri);
        return TemplateFilenameNotURI;
    }

    if (err != NULL) {
        g_clear_error( & err);
        return TemplateInvalid;
    }

    out -> template = poppler_document_new_from_file(uri, NULL, & err);
    g_free(uri);
    if (out -> template == NULL) {
        return TemplateInvalid;
    }

    if (err != NULL) {
        g_clear_error( & err);
        return TemplateInvalid;
    }

    out -> buf = buf;

    return 0;
}

int pdf_set_fields(struct fillpdf * fillablepdf, const char *fields) {
    // input strings from the FFI might be volatile, its best we copy them to the heap
    unsigned len = strlen(fields);
    if (len == 0) {
        return NoFields;
    }

    fillablepdf -> fields_raw = (char *) malloc(len + 1);
    strcpy(fillablepdf -> fields_raw, fields);

    fillablepdf -> fields = json_tokener_parse(fillablepdf -> fields_raw);

    return 0;
}

int pdf_fill_template_fields(struct fillpdf * fillablepdf) {
    int n = pdf_get_pages(fillablepdf);
    if (n == 0) {
        return TemplateEmpty;
    }

    GList * pdf_fields;
    struct json_object * obj;
    int n_maps = 0;

    for (int i = 0; i < n; i++) {
        PopplerPage * page = poppler_document_get_page(fillablepdf -> template, i);
        pdf_fields = poppler_page_get_form_field_mapping(page);
        PopplerFormFieldMapping * fieldmap;
        n_maps = g_list_length(pdf_fields);

        for (int idx = 0; idx < n_maps; idx++) {
            fieldmap = g_list_nth_data(pdf_fields, idx);
            if (fieldmap == NULL) {
                return TemplateNoFields;
            }
            gchar * field_name = poppler_form_field_get_name(fieldmap -> field);

            if (!json_object_object_get_ex(fillablepdf -> fields, field_name, & obj)) {
                continue; // no json object exists for this field
            }

            char * txt = (char * ) json_object_to_json_string(obj);
            pdf_unescape_json_str(txt);
            poppler_form_field_text_set_text(
                fieldmap -> field,
                txt
            );

            obj = NULL;
            g_free(field_name);
        }

        poppler_page_free_form_field_mapping(pdf_fields);
    }

    return 0;
}

void pdf_new_buffer(struct autobuffer * buf) {
    buf -> data = NULL;
    buf -> size = 0;
    buf -> available = 0;
    buf -> _next_allocation_size = 5000;
}

void pdf_free_buffer(struct autobuffer * buf) {
    free(buf -> data);
    buf = NULL;
}

void pdf_clear(struct fillpdf * fillablepdf) {} // TODO

unsigned char * pdf_get_buffer(struct autobuffer * buf) {
    return buf -> data;
}

cairo_status_t pdf_write(void * b, unsigned char
    const * data, unsigned int len) {
    struct autobuffer * buf = b;
    size_t realloc_size = buf -> size + len;

    if (buf -> available < realloc_size) {
        while (buf -> available <= realloc_size) {
            buf -> data = realloc(
                buf -> data,
                buf -> available + buf -> _next_allocation_size
            );
            buf -> available += buf -> _next_allocation_size;
            buf -> _next_allocation_size *= 2;
        }
    }

    memcpy( & (buf -> data[buf -> size]), data, len);
    buf -> size += len;

    return CAIRO_STATUS_SUCCESS;
}

int pdf_render_page(struct fillpdf * in, int p) {
    PopplerPage * page;
    GError * err = NULL; // <- must be initialized as NULL for some reason
    cairo_status_t status;
    double width, height;

    page = poppler_document_get_page( in -> template, p);
    if (page == NULL) {
        return TemplatePageNotExist;
    }

    poppler_page_get_size(page, & width, & height);

    cairo_surface_t * pdf_surf = cairo_pdf_surface_create_for_stream(
        pdf_write, in -> buf,
        width,
        height
    );

    cairo_t * context = cairo_create(pdf_surf);
    poppler_page_render_for_printing(page, context);
    cairo_surface_show_page(pdf_surf);
    g_object_unref(page);

    status = cairo_status(context);
    if (status && status != CAIRO_STATUS_SUCCESS) {
        return status;
    }

    cairo_destroy(context);
    cairo_surface_finish(pdf_surf);

    status = cairo_surface_status(pdf_surf);
    if (status && status != CAIRO_STATUS_SUCCESS) {
        return status;
    }

    cairo_surface_destroy(pdf_surf);

    return 0;
}

int pdf_get_pages(struct fillpdf * in) {
    return poppler_document_get_n_pages(in -> template);
}

void pdf_close(struct fillpdf * data) {
    free(data -> in_filename);
    free(data -> fields);
    free(data -> fields_raw);
    g_object_unref(data -> template);
    pdf_free_buffer(data -> buf);
    data = NULL;
}

void pdf_unescape_json_str(char * obj) {
    char * tmp;
    int i;
    int last_bckslsh = 0;
    unsigned len = strlen(obj) + 1;

    // remove first "
    if (obj[0] == 34) {
        strcpy(&obj[0], &obj[1]);
        len--;
    }

    // remove second ""
    if (obj[len - 2] == 34) {
        strcpy(&obj[len - 2], &obj[len - 1]);
    }

    for (i = 0; i < len; i++) {
        switch (obj[i]) {
        case 0:
            return;
        case 47 | '/':
            // remove escaped forward slashes: "\/"
            // json-c is escaping these for some reason :(
            if (last_bckslsh == i - 1) {
                strcpy(&obj[last_bckslsh], &obj[i]);
            }
        case 92 | '\\':
            last_bckslsh = i;
        }
    }
}
