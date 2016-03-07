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
	//"os"
	"runtime"
	//"strconv"
	//"unsafe"
)

// wrap for iunit
type AoiUnit struct {
	U *C.struct_iunit
}

// wrap imap
type AoiMap struct {
	M *C.struct_imap

	// 所有对象
	Units map[int64]*AoiUnit
}

// New Map
func NewAoiMap(p *C.struct_ipos, s *C.struct_isize, divide int) *AoiMap {
	m := &AoiMap{M: C.imapmake(p, s, C.int(divide)), Units: make(map[int64]*AoiUnit)}
	runtime.SetFinalizer(m, (*AoiMap).Free)
	return m
}

// New Unit
func NewAoiUnit(uniqueId C.iid, x, y C.ireal) *AoiUnit {
	u := &AoiUnit{U: C.imakeunit(uniqueId, x, y)}
	u.U.radius = 5

	runtime.SetFinalizer(u, (*AoiUnit).Free)
	return u
}

// 绘制
func (u *AoiUnit) Draw(gc *draw2dgl.GraphicContext) {
	gc.BeginPath()
	draw2dkit.Circle(gc, float64(u.U.pos.x), float64(u.U.pos.x), float64(u.U.radius))
	gc.SetFillColor(color.RGBA{uint8(0), uint8(128), uint8(0), uint8(255)})
	gc.Fill()

	fmt.Println("u-pos", u.U.pos)
	fmt.Println("u-radius", u.U.radius)
}

// 释放对象
func (u *AoiUnit) Free() {
	if u.U == nil {
		return
	}
	C.ifreeunit(u.U)
	u.U = nil
	fmt.Println("AoiUnit finalizer")
}

// 打印信息
func (m *AoiMap) Print(require int) {
	C._aoi_print(m.M, C.int(require))
}

// 加入对象
func (m *AoiMap) AddUnit(u *AoiUnit) bool {
	id := int64(u.U.id)
	if _, ok := m.Units[id]; ok {
		return false
	}
	C.imapaddunit(m.M, u.U)
	m.Units[id] = u
	return true
}

// 移除对象
func (m *AoiMap) RemoveUnit(u *AoiUnit) bool {
	id := int64(u.U.id)
	return m.RemoveUnitById(id)
}

// 移除对象
func (m *AoiMap) RemoveUnitById(id int64) bool {
	if u, ok := m.Units[id]; ok {
		C.imapremoveunit(m.M, u.U)
		delete(m.Units, id)
		return true
	}
	return false
}

// 更新单元位置
func (m *AoiMap) UpdateUnit(u *AoiUnit) {
	C.imapupdateunit(m.M, u.U)
}

// Draw vertically spaced lines
func DrawLine(gc *draw2dgl.GraphicContext, x0, y0, x1, y1 float64) {
	// Draw a line
	gc.MoveTo(x0, y0)
	gc.LineTo(x1, y1)
	gc.Stroke()
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

// 绘制
func (m *AoiMap) Draw(gc *draw2dgl.GraphicContext) {
	// 先绘制地图本身
	DrawGrid(gc, float64(m.M.pos.x), float64(m.M.pos.y),
		float64(m.M.pos.x)+float64(m.M.size.w),
		float64(m.M.pos.y)+float64(m.M.size.h),
		float64(m.M.nodesizes[m.M.divide].w),
		float64(m.M.nodesizes[m.M.divide].h),
	)

	DrawLine(gc, float64(m.M.pos.x), float64(m.M.pos.y),
		float64(m.M.pos.x)+float64(m.M.size.w),
		float64(m.M.pos.y)+float64(m.M.size.h))

	// 绘制在地图上的所有单元
	for _, u := range m.Units {
		u.Draw(gc)
	}
	fmt.Println("m-pos", m.M.pos)
	fmt.Println("m-size", m.M.size)
}

// 释放地图
func (m *AoiMap) Free() {
	if m.M == nil {
		return
	}
	C.imapfree(m.M)
	m.M = nil
	m.Units = nil
	fmt.Println("AoiMap finalizer")
}

var xmap *AoiMap

func aoi_init(width, height float64) {

	factor := 1.0 * 3 / 4
	sizeh := height * factor
	sizew := width * factor
	offsetx := (width - sizew) / 2
	offsety := (height - sizeh) / 2

	pos := C.struct_ipos{x: C.ireal(offsetx), y: C.ireal(offsety)}
	size := C.struct_isize{w: C.ireal(sizew), h: C.ireal(sizeh)}
	fmt.Println("values:", width, height, pos, size)
	xmap = NewAoiMap(&pos, &size, 3)

	u := NewAoiUnit(0, C.ireal(offsetx), C.ireal(offsety))
	xmap.AddUnit(u)
	xmap.Print(0xffff)

	u.U.pos.x = u.U.pos.x + 100
	u.U.pos.y = u.U.pos.y + 100
	xmap.UpdateUnit(u)
	xmap.Print(0xffff)
}

func aoi_draw(gc *draw2dgl.GraphicContext) {
	/*
		r, _ := strconv.ParseInt(os.Args[1], 10, 0)
		g, _ := strconv.ParseInt(os.Args[2], 10, 0)
		b, _ := strconv.ParseInt(os.Args[3], 10, 0)
		a, _ := strconv.ParseInt(os.Args[4], 10, 0)
	*/
	xmap.Draw(gc)
}
