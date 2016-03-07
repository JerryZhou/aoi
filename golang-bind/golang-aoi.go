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
	m := &AoiMap{M: C.imapmake(p, s, 5), Units: make(map[int64]*AoiUnit)}
	runtime.SetFinalizer(m, (*AoiMap).Free)
	return m
}

// New Unit
func NewAoiUnit(uniqueId C.iid, x, y C.ireal) *AoiUnit {
	u := &AoiUnit{U: C.imakeunit(uniqueId, x, y)}
	runtime.SetFinalizer(u, (*AoiUnit).Free)
	return u
}

// 释放对象
func (m *AoiUnit) Free() {
	if m.U == nil {
		return
	}
	C.ifreeunit(m.U)
	m.U = nil
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

func aoi_init() {
	n := C.igetcurtick()
	fmt.Println("C.igetcurtick", n)

	pos := C.struct_ipos{x: 0, y: 0}
	size := C.struct_isize{w: 512, h: 512}
	xmap = NewAoiMap(&pos, &size, 5)

	u := NewAoiUnit(0, C.ireal(1.0), C.ireal(1.2))
	xmap.AddUnit(u)
	xmap.Print(0xffff)

	u.U.pos.x = 200
	u.U.pos.y = 100
	xmap.UpdateUnit(u)
	xmap.Print(0xffff)

	xmap = nil
}

func aoi_draw(gc *draw2dgl.GraphicContext) {
	/*
		r, _ := strconv.ParseInt(os.Args[1], 10, 0)
		g, _ := strconv.ParseInt(os.Args[2], 10, 0)
		b, _ := strconv.ParseInt(os.Args[3], 10, 0)
		a, _ := strconv.ParseInt(os.Args[4], 10, 0)
	*/
	r := 255
	g := 0
	b := 0
	a := 255

	gc.BeginPath()
	//draw2dkit.RoundedRectangle(gc, 200, 200, 600, 600, 100, 100)
	draw2dkit.Rectangle(gc, 200, 200, 600, 600)
	gc.SetFillColor(color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)})
	gc.Fill()
}
