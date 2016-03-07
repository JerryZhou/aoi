package main

/*
#cgo CFLAGS: -I ../
#cgo LDFLAGS: -L ../ -laoi
#include "../aoi.h"
*/
import "C"

import (
	"fmt"
	"github.com/llgcode/draw2d/draw2dgl"
	"github.com/llgcode/draw2d/draw2dkit"
	"image/color"
	"os"
	"strconv"
	//"unsafe"
)

type ipos C.struct_ipos
type isize C.struct_isize
type imap C.struct_imap

func aoi_init() {
	n := C.igetcurtick()
	fmt.Println("C.igetcurtick", n)

	pos := C.struct_ipos{x: 0, y: 0}
	size := C.struct_isize{w: 0, h: 0}
	xmap := C.imapmake(&pos, &size, 5)
	C._aoi_print(xmap, 0xffff)
	C.imapfree(xmap)
}

func aoi_draw(gc *draw2dgl.GraphicContext) {
	r, _ := strconv.ParseInt(os.Args[1], 10, 0)
	g, _ := strconv.ParseInt(os.Args[2], 10, 0)
	b, _ := strconv.ParseInt(os.Args[3], 10, 0)
	a, _ := strconv.ParseInt(os.Args[4], 10, 0)

	gc.BeginPath()
	//draw2dkit.RoundedRectangle(gc, 200, 200, 600, 600, 100, 100)
	draw2dkit.Rectangle(gc, 200, 200, 600, 600)
	gc.SetFillColor(color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)})
	gc.Fill()
}
