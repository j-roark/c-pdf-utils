#ifndef FILLPDF_H
#define FILLPDF_H

#include <cairo/cairo-pdf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//#include <gtk/gtk.h>
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
    json_object *fields;
    struct autobuffer *buf;
    PopplerDocument *template;
};

int pdf_open(const char *in_filename, const char *fields,
             struct fillpdf *out, struct autobuffer *buf);

int fill_template_text(struct fillpdf *fillablepdf);

void pdf_new_buffer(struct autobuffer *buf);

void pdf_free_buffer(struct autobuffer *buf);

cairo_status_t pdf_write(void *b, unsigned char const *data, unsigned int len);

int pdf_render(struct fillpdf *in);

void pdf_close(struct fillpdf *data);

#endif
