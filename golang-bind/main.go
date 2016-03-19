package main

import (
	"github.com/go-gl/gl/v2.1/gl"
	"github.com/go-gl/glfw/v3.1/glfw"
	"github.com/llgcode/draw2d"
	"github.com/llgcode/draw2d/draw2dgl"
	"log"
	//"image"
	//"reflect"
	"runtime"
)

func init() {
	// This is needed to arrange that main() runs on main thread.
	// See documentation for functions that are only allowed to be called from the main thread.
	runtime.LockOSThread()
}

var (
	// global rotation
	rotate         int
	width, height  int
	scalex, scaley float64
	redraw         = true
	font           draw2d.FontData
	chooseplugin   = 1
)

type Plugin interface {
	Init(width, height float64)
	Draw(gc *draw2dgl.GraphicContext)
	MousePress(x, y float64)
	MouseMove(x, y float64)
}

func G_mouse_translate(x, y float64) (float64, float64) {
	return scalex * x, float64(height) - scaley*y
}

//var plugin = &AOI{}
var plugin = &Navi{}

func reshape(window *glfw.Window, w, h int) {
	gl.ClearColor(1, 1, 1, 1)
	/* Establish viewing area to cover entire window. */
	gl.Viewport(0, 0, int32(w), int32(h))
	/* PROJECTION Matrix mode. */
	gl.MatrixMode(gl.PROJECTION)
	/* Reset project matrix. */
	gl.LoadIdentity()
	/* Map abstract coords directly to window coords. */
	gl.Ortho(0, float64(w), 0, float64(h), -1, 1)
	/* Invert Y axis so increasing Y goes down. */
	gl.Scalef(1, 1, 1)
	/* Shift origin up to upper-left corner. */
	//gl.Translatef(0, float32(h), 0)
	gl.Enable(gl.BLEND)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
	gl.Disable(gl.DEPTH_TEST)
	width, height = w, h
	redraw = true
}

// Ask to refresh
func invalidate() {
	redraw = true
}

func display() {
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

	gl.LineWidth(2)
	gc := draw2dgl.NewGraphicContext(width, height)
	gc.SetFontData(draw2d.FontData{
		Name:   "luxi",
		Family: draw2d.FontFamilyMono,
		Style:  draw2d.FontStyleBold | draw2d.FontStyleItalic})

	plugin.Draw(gc)

	gl.Flush() /* Single buffered, so needs a flush. */
}

func onChar(w *glfw.Window, char rune) {
	log.Println(char)
}

func onKey(w *glfw.Window, key glfw.Key, scancode int, action glfw.Action, mods glfw.ModifierKey) {
	switch {
	case key == glfw.KeyEscape && action == glfw.Press,
		key == glfw.KeyQ && action == glfw.Press:
		w.SetShouldClose(true)
	}
}

func onMouseBtn(
	w *glfw.Window,
	button glfw.MouseButton,
	action glfw.Action,
	mod glfw.ModifierKey) {
	x, y := w.GetCursorPos()
	if action == glfw.Release {
		plugin.MousePress(G_mouse_translate(x, y))
	}
}

func onMouseMove(w *glfw.Window, xpos float64, ypos float64) {
	action := w.GetMouseButton(glfw.MouseButtonLeft)
	if action == glfw.Press {
		plugin.MouseMove(G_mouse_translate(xpos, ypos))
	}
}

func main() {
	err := glfw.Init()
	if err != nil {
		panic(err)
	}
	defer glfw.Terminate()
	width, height = 800, 800
	window, err := glfw.CreateWindow(width, height, "AOI-Golang", nil, nil)
	if err != nil {
		panic(err)
	}
	swidth, sheight := window.GetFramebufferSize()
	scalex = float64(swidth) / float64(width)
	scaley = float64(sheight) / float64(height)
	width = swidth
	height = sheight

	window.MakeContextCurrent()
	window.SetSizeCallback(reshape)
	window.SetKeyCallback(onKey)
	window.SetCharCallback(onChar)
	window.SetMouseButtonCallback(onMouseBtn)
	window.SetCursorPosCallback(onMouseMove)

	glfw.SwapInterval(1)

	err = gl.Init()
	if err != nil {
		panic(err)
	}

	plugin.Init(float64(width), float64(height))

	reshape(window, width, height)
	for !window.ShouldClose() {
		if redraw {
			display()
			window.SwapBuffers()
			redraw = false
		}
		glfw.PollEvents()
		//		time.Sleep(2 * time.Second)
	}
}
