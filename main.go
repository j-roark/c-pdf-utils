package main

/*
#cgo pkg-config: cairo poppler-glib
#cgo CFLAGS: -I/usr/include/cairo/ -I /usr/include/pango-1.0/ -I /usr/include/atk-1.0/ -I /usr/include/gdk-pixbuf-2.0/ -I /usr/lib/x86_64-linux-gnu/glib-2.0/include -I /usr/local/include/ -I /usr/include/poppler/glib/
#cgo LDFLAGS: -L /usr/local/lib/ -ljson-c

#include "./include/fillpdf.h"
#include "./src/fillpdf.c"
*/
import "C"
import (
	"fmt"
	"unsafe"
	"os"
)

// remember to set this GODEBUG=cgocheck=0
// this is because cgo has some strict rules about reusing pointers to go objects
// but if we practice memory lifetime rules- fillpdf.c implements fixes for any type of volatility
// this reduces overhead

func main() {
	// must be an absolute path :(
	template := C.CString("/sample_pdf.pdf")

	_fields := "{"+
		"\"Name\":"+
		"\"John Roark\"}"+
	"}"

	fields := C.CString(_fields)
	pdf := &C.struct_fillpdf{}
	buf := &C.struct_autobuffer{}

	C.pdf_new_buffer(buf)
	_ = C.pdf_open(template, fields, pdf, buf)

	_ = C.fill_template_text(pdf)
	_ = C.pdf_render(pdf)

	C.free(unsafe.Pointer(fields))
	C.free(unsafe.Pointer(template))

	data := C.GoBytes(unsafe.Pointer(buf.data), C.int(buf.size)) // []uint8 basically

	f, err := os.Create("output.pdf")
	if err != nil {
		fmt.Println(err)
		return
	}
	f.Write(data)

	C.pdf_close(pdf)
}
