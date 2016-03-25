package main

import (
	"fmt"
	"github.com/llgcode/draw2d/draw2dgl"
	"github.com/llgcode/draw2d/draw2dkit"
	"image/color"
	"math/rand"
	//"os"
	"runtime"
	//"strconv"
	"unsafe"
)

/*
#cgo CFLAGS: -I ../
#cgo LDFLAGS: -L ../ -laoi
#include "../aoi.h"
#include "../navi.h"
*/
import "C"

var (
	UnitRadius           = 10
	SearchRadius float64 = 200.0
)

// wrap for iunit
type AoiUnit struct {
	U     *C.struct_iunit
	Color color.RGBA
}

// wrap imap
type AoiMap struct {
	M *C.struct_imap

	// 所有对象
	Units map[int64]*AoiUnit
}

// wrap search result
type AoiSearchResult struct {
	S *C.struct_isearchresult

	X      float64
	Y      float64
	Radius float64

	M *AoiMap
}

// New Search Result
func NewAoiSearchResult() *AoiSearchResult {
	s := &AoiSearchResult{S: C.isearchresultmake()}
	runtime.SetFinalizer(s, (*AoiSearchResult).Free)
	return s
}

func (s *AoiSearchResult) Free() {
	if s.S == nil {
		return
	}
	C.isearchresultfree(s.S)
	s.S = nil
	s.M = nil
}

func (s *AoiSearchResult) Clean() {
	C.isearchresultclean(s.S)
}

func (s *AoiSearchResult) Len() int {
	return int(C.ireflistlen(s.S.units))
}

func (s *AoiSearchResult) MarkUnits(c color.RGBA) {
	units := s.Units()
	for _, v := range units {
		v.Color = c
	}
}

func (s *AoiSearchResult) Draw(gc *draw2dgl.GraphicContext) {
	DrawCircle(gc, ColorSearchCircle, s.X, s.Y, s.Radius)
	units := s.Units()
	for _, v := range units {
		v.Draw(gc)
	}
}

func (s *AoiSearchResult) Units() []*AoiUnit {
	units := []*AoiUnit{}
	for first := C.ireflistfirst(s.S.units); first != nil; first = first.next {
		u := (*C.struct_iunit)(unsafe.Pointer(first.value))
		units = append(units, s.M.Units[(int64)(u.id)])
	}
	return units
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
	u.U.radius = C.ireal(int(rand.Int31n(int32(UnitRadius/2))) + UnitRadius/2)
	u.Color = ColorUnit

	runtime.SetFinalizer(u, (*AoiUnit).Free)
	return u
}

func (u *AoiUnit) String() string {
	return fmt.Sprintf("{uid: %v pos: {%v, %v} radius: %v}",
		u.U.id, u.U.pos.x, u.U.pos.y, u.U.radius)
}

// 绘制
func (u *AoiUnit) Draw(gc *draw2dgl.GraphicContext) {
	gc.BeginPath()
	draw2dkit.Circle(gc, float64(u.U.pos.x), float64(u.U.pos.y), float64(u.U.radius))
	gc.SetFillColor(u.Color)
	gc.Fill()
}

// 释放对象
func (u *AoiUnit) Free() {
	if u.U == nil {
		return
	}
	C.ifreeunit(u.U)
	u.U = nil
	// fmt.Println("AoiUnit finalizer")
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

// 搜索
func (m *AoiMap) Search(result *AoiSearchResult, x, y, radius float64) int {
	result.MarkUnits(ColorUnit)

	pos := C.struct_ipos{C.ireal(x), C.ireal(y)}
	C.imapsearchfrompos(m.M, &pos, result.S, C.ireal(radius))
	result.M = m
	result.X = x
	result.Y = y
	result.Radius = radius
	result.MarkUnits(ColorUnitAim)

	return result.Len()
}

// 更新单元位置
func (m *AoiMap) UpdateUnit(u *AoiUnit) {
	C.imapupdateunit(m.M, u.U)
}

// 绘制
func (m *AoiMap) Draw(gc *draw2dgl.GraphicContext) {
	// 先绘制地图本身
	DrawGridC(gc, &m.M.pos, &m.M.size, &m.M.nodesizes[m.M.divide])

	// 搞一条线，清晰一点
	DrawLineC(gc, &m.M.pos, &m.M.size)

	// 叶子节点
	leaf := m.M.leaf
	for ; leaf != nil; leaf = leaf.next {
		DrawRectC(gc, &leaf.code.pos, &m.M.nodesizes[leaf.level])
	}

	// 绘制在地图上的所有单元
	for _, u := range m.Units {
		u.Draw(gc)
	}
}

// 释放地图
func (m *AoiMap) Free() {
	if m.M == nil {
		return
	}
	C.imapfree(m.M)
	m.M = nil
	m.Units = nil
	// fmt.Println("AoiMap finalizer")
}

//************************************************************
//************************************************************

type AOI struct {
	xmap   *AoiMap
	search *AoiSearchResult
}

func (a *AOI) Init(width, height float64) {
	factor := 1.0 * 3 / 4
	sizeh := height * factor
	sizew := width * factor
	offsetx := (width - sizew) / 2
	offsety := (height - sizeh) / 2

	pos := C.struct_ipos{x: C.ireal(offsetx), y: C.ireal(offsety)}
	size := C.struct_isize{w: C.ireal(sizew), h: C.ireal(sizeh)}
	fmt.Println("values:", width, height, pos, size)
	a.xmap = NewAoiMap(&pos, &size, 5)

	for i := 0; i < 100; i++ {
		u := NewAoiUnit(C.iid(i),
			C.ireal(offsetx+float64(rand.Int31n(int32(sizew)))),
			C.ireal(offsety+float64(rand.Int31n(int32(sizeh)))),
		)
		a.xmap.AddUnit(u)
	}

	a.search = NewAoiSearchResult()
	a.xmap.Search(a.search, 800, 800, SearchRadius)

}
func (a *AOI) Draw(gc *draw2dgl.GraphicContext) {
	a.xmap.Draw(gc)
	a.search.Draw(gc)

}
func (a *AOI) MousePress(x, y float64) {
	a.xmap.Search(a.search, x, y, SearchRadius)
	redraw = true
}
func (a *AOI) MouseMove(x, y float64) {
	a.xmap.Search(a.search, x, y, SearchRadius)
	redraw = true
}
