package main

import (
	//"fmt"
	"github.com/llgcode/draw2d/draw2dgl"
	"github.com/llgcode/draw2d/draw2dkit"
	"image/color"
	//"math/rand"
	//"os"
	//"runtime"
	//"strconv"
	//"unsafe"
)

/*
#cgo CFLAGS: -I ../
#include "../aoi.h"
*/
import "C"

var (
	ColorUnit         = color.RGBA{0, 128, 0, 255}
	ColorUnitAim      = color.RGBA{126, 12, 77, 233}
	ColorRect         = color.RGBA{uint8(0), uint8(0), uint8(128), uint8(255)}
	ColorSearchCircle = color.RGBA{55, 105, 12, 128}
	ColorGreen        = color.RGBA{0, 211, 0, 255}
)

// Draw vertically spaced lines
func DrawLine(gc *draw2dgl.GraphicContext, x0, y0, x1, y1 float64) {
	// Draw a line
	gc.MoveTo(x0, y0)
	gc.LineTo(x1, y1)
	gc.Stroke()
}

func DrawMoveToC(gc *draw2dgl.GraphicContext, pos *C.struct_ipos) {
	gc.MoveTo(float64(pos.x), float64(pos.y))
}

func DrawLineToC(gc *draw2dgl.GraphicContext, pos *C.struct_ipos) {
	gc.LineTo(float64(pos.x), float64(pos.y))
}

func DrawLineEndC(gc *draw2dgl.GraphicContext) {
	gc.Stroke()
}

func DrawLineC(gc *draw2dgl.GraphicContext, pos *C.struct_ipos, size *C.struct_isize) {
	DrawLine(gc, float64(pos.x), float64(pos.y),
		float64(pos.x)+float64(size.w),
		float64(pos.y)+float64(size.h))
}

// Draw a grid
func DrawGrid(gc *draw2dgl.GraphicContext, x0, y0, x1, y1, dividew, divideh float64) {
	for w := x0; w <= x1; w = w + dividew {
		DrawLine(gc, w, y0, w, y1)
	}
	for h := y0; h <= y1; h = h + divideh {
		DrawLine(gc, x0, h, x1, h)
	}
}

func DrawGridC(gc *draw2dgl.GraphicContext,
	pos *C.struct_ipos,
	size *C.struct_isize,
	divide *C.struct_isize) {
	DrawGrid(gc, float64(pos.x), float64(pos.y),
		float64(pos.x)+float64(size.w),
		float64(pos.y)+float64(size.h),
		float64(divide.w),
		float64(divide.h),
	)
}

// Draw a rect
func DrawRect(gc *draw2dgl.GraphicContext, x0, y0, x1, y1 float64) {
	gc.BeginPath()
	draw2dkit.Rectangle(gc, x0, y0, x1, y1)
	gc.SetFillColor(ColorRect)
	gc.Fill()
}

func DrawRectC(gc *draw2dgl.GraphicContext, pos *C.struct_ipos, size *C.struct_isize) {
	DrawRect(gc, float64(pos.x),
		float64(pos.y),
		float64(pos.x+size.w),
		float64(pos.y+size.h))
}

func DrawCircle(gc *draw2dgl.GraphicContext, c color.RGBA, x, y, radius float64) {
	gc.BeginPath()
	draw2dkit.Circle(gc, x, y, radius)
	gc.SetFillColor(c)
	gc.Fill()
}

func DrawCircleC(gc *draw2dgl.GraphicContext, c color.RGBA, pos *C.struct_ipos, radius C.ireal) {
	DrawCircle(gc, c, float64(pos.x), float64(pos.y), float64(radius))
}
