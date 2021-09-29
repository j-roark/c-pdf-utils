#include "../../src/c_pdf_utils.c"
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

char *IN_FILENAME = "../shared/example_form.pdf";
char *FIELDS = "{"
        "\"Name\":"
        "\"Cthulhu\","
        "\"Address\":"
        "\"R'lyeh\","
        "\"Text5\":"
        "\"Drive any human that gazes upon my form to insanity\","
        "\"Text6\":"
        "\"Hang out with my cultist buds.\""
"}";

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    char *in_filename_path = realpath(IN_FILENAME, NULL);
    if (in_filename_path == NULL) {
        printf("invalid filename\n");
        return 1;
    }
    printf("filling from form template at: %s\n", in_filename_path);

    struct fillpdf *_p, pdf;
    struct autobuffer *_b, buf;

    int err;

    pdf_new_buffer(&buf);
    err = pdf_open(in_filename_path, &pdf, &buf);
    if (err != 0) {
        printf("error: %s\n", pdf_err_str(err));
        return 1;
    }

    err = pdf_set_fields(&pdf, FIELDS);
    if (err != 0) {
        printf("error: %s\n", pdf_err_str(err));
        return 1;
    }

    err = pdf_fill_template_fields(&pdf);
    if (err != 0) {
        printf("error: %s\n", pdf_err_str(err));
        return 1;
    }

    err = pdf_render_page(&pdf, 0);
    if (err != 0) {
        printf("error: %s\n", pdf_err_str(err));
        return 1;
    }

    printf("done!\n");
    printf(
        "output data size: %u bytes\n",
        (&buf)->size
    );

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf(
        "done in %lu us\n",
        (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000
    );

    FILE *_f = fopen("./test_output.pdf","w");
    if (_f == NULL) {
        printf("cannot open destination file\n");
        return 1;
    }

    fwrite(pdf_get_buffer(&buf), (&buf)->size, 1, _f);
    fclose(_f);
    pdf_close(&pdf);
    return 0;
}
