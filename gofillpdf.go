package cfillpdf

/*
#cgo pkg-config: cairo poppler-glib
#cgo LDFLAGS: -L ./bin -ljson-c -L ./bin -lcfillpdf -Wl,-rpath=./bin

#include "./include/fillpdf.h"
*/
import "C"
import (
	"fmt"
	"unsafe"
)

// remember to set this GODEBUG=cgocheck=0
// this is because cgo has some strict rules about reusing pointers to go objects
// but if we practice memory lifetime rules- fillpdf.c implements fixes for any type of volatility
// this reduces overhead

struct FillablePDf {
	template string
	_pdf *C.struct_fillpdf
	_buf *C.struct_autobuffer
}

func NewFillablePdf(template string) FillablePDf {
	res = FillablePDf{_pdf: &C.struct_fillpdf{}, _buf: &C.struct_autobuffer{}}
	C.pdf_new_buffer(res._buf)
}

func (p *FillablePDF) Fill(fields strting) {
	_templ :=  C.CString(p.template)
	_fields := C.CString(fields)

	_ = C.pdf_open(_templ, _fields, p._pdf, p._buf)

	_ = C.fill_template_text(p._pdf)
	_ = C.pdf_render(p._pdf)

	C.free(unsafe.Pointer(_templ))
	C.free(unsafe.Pointer(_fields))
}

func (p *FillablePDF) Data() []uint8 {
	return C.GoBytes(
		unsafe.Pointer(p._buf.data),
		C.int(p._buf.size)
	)
}

func (p *FillablePDF) Close() {
	C.pdf_close(p._pdf)
}
