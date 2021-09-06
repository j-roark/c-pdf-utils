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

#ifndef FILLPDF_H
#define FILLPDF_H

#include <cairo/cairo-pdf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <poppler-glib/poppler.h>
#include <json-c/json.h>

struct autobuffer {
    unsigned char *data;
    size_t size;
    size_t available;
    size_t _next_allocation_size;
};

struct fillpdf {
    char *in_filename;
    char *out_filename;
    char *fields_raw;
    json_object *fields;
    struct autobuffer *buf;
    PopplerDocument *template;
};

typedef enum {
    TemplateFilenameEmpty,
    TemplateFilenameNotURI,
    TemplateInvalid,
    TemplateEmpty,
    TemplateNoFields,
    TemplateNotExist,
    NoFields
}pdf_err;

const char * pdf_err_str(int err_enum);

int pdf_open(const char *in_filename, struct fillpdf *out,
             struct autobuffer *buf);

int pdf_set_fields(struct fillpdf * fillablepdf, const char *fields);

int pdf_fill_template_fields(struct fillpdf *fillablepdf);

void pdf_new_buffer(struct autobuffer *buf);

void pdf_free_buffer(struct autobuffer *buf);

void pdf_clear(struct fillpdf *fillablepdf); // TODO

unsigned char * pdf_get_buffer(struct autobuffer *buf);

cairo_status_t pdf_write(void *b, unsigned char const *data, unsigned int len); // PRIVATE

int pdf_render(struct fillpdf *in);

void pdf_close(struct fillpdf *data);

void pdf_unescape_json_str(char* obj);

#endif
