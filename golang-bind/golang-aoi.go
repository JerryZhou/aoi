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
	"math/rand"
	//"os"
	"runtime"
	//"strconv"
	"unsafe"
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
}

func (s *AoiSearchResult) Clean() {
	C.isearchresultclean(s.S)
}

func (s *AoiSearchResult) Len() int {
	return int(C.ireflistlen(s.S.units))
}

func (s *AoiSearchResult) Units() []*AoiUnit {
	units := []*AoiUnit{}
	for first := C.ireflistfirst(s.S.units); first != nil; first = first.next {
		C.irefretain(first.value)
		u := &AoiUnit{U: (*C.struct_iunit)(unsafe.Pointer(first.value))}
		runtime.SetFinalizer(u, (*AoiUnit).Free)

		units = append(units, u)
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
	u.U.radius = 10
	u.Color = color.RGBA{0, 128, 0, 255}

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
func (m *AoiMap) Search(result *AoiSearchResult, x, y, xrange float64) int {
	pos := C.struct_ipos{C.ireal(x), C.ireal(y)}
	C.imapsearchfrompos(m.M, &pos, result.S, C.ireal(xrange))
	return result.Len()
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
	gc.SetFillColor(color.RGBA{uint8(0), uint8(0), uint8(128), uint8(255)})
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

var xmap *AoiMap
var search *AoiSearchResult

func aoi_init(width, height float64) {

	factor := 1.0 * 3 / 4
	sizeh := height * factor
	sizew := width * factor
	offsetx := (width - sizew) / 2
	offsety := (height - sizeh) / 2

	pos := C.struct_ipos{x: C.ireal(offsetx), y: C.ireal(offsety)}
	size := C.struct_isize{w: C.ireal(sizew), h: C.ireal(sizeh)}
	fmt.Println("values:", width, height, pos, size)
	xmap = NewAoiMap(&pos, &size, 5)

	for i := 0; i < 100; i++ {
		u := NewAoiUnit(C.iid(i),
			C.ireal(offsetx+float64(rand.Int31n(int32(sizew)))),
			C.ireal(offsety+float64(rand.Int31n(int32(sizeh)))),
		)
		xmap.AddUnit(u)
	}

	search = NewAoiSearchResult()
	xmap.Search(search, 800, 800, 300)
	units := search.Units()
	for _, v := range units {
		v.Color = color.RGBA{128, 128, 128, 255}
		fmt.Println(v)
	}

	// xmap.UpdateUnit(u)
	// xmap.Print(0xffff)
}

func aoi_draw(gc *draw2dgl.GraphicContext) {
	/*
		r, _ := strconv.ParseInt(os.Args[1], 10, 0)
		g, _ := strconv.ParseInt(os.Args[2], 10, 0)
		b, _ := strconv.ParseInt(os.Args[3], 10, 0)
		a, _ := strconv.ParseInt(os.Args[4], 10, 0)
	*/
	xmap.Draw(gc)
	DrawCircle(gc, color.RGBA{55, 105, 12, 128}, 800, 800, 300)
	units := search.Units()
	for _, v := range units {
		v.Color = color.RGBA{126, 12, 77, 233}
		v.Draw(gc)
	}
}
