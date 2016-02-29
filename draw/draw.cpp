#include "draw.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

// Include headers for OpenGL (gl.h), OpenGL Utility Library (glu.h)
//
// XXX In Mac OS X these headers are located in a different directory.
// XXX Need to revisit conditionalization on operating system.
#if __APPLE__ && __MACH__
#include <OpenGL/gl.h>   // for Mac OS X
#include <OpenGL/glu.h>   // for Mac OS X
#ifndef HAVE_NO_GLUT
#include <GLUT/glut.h>   // for Mac OS X
#endif
#else
#ifdef _MSC_VER
#include <windows.h>
#endif

#include <GL/gl.h>     // for Linux and Windows
#include <GL/glu.h>     // for Linux and Windows
#ifndef HAVE_NO_GLUT
#include <GL/glut.h>   // for Mac OS X
#endif
#endif

// ----------------------------------------------------------------------------
// XXX This is a bit ad hoc.  Need to revisit conditionalization on operating
// XXX system.  As of 5-5-03, this module knows about Win32 (code thanks to
// XXX Leaf Garland and Bruce Mitchener) and Linux/Unix (Craig's original
// XXX version).  It tests for Xbox and Win32 and assumes Linux/Unix 
// XXX otherwise.


#if defined (_XBOX)
    #include <xtl.h>
#elif defined (_WIN32)
    #include <windows.h>
#else
    #include <sys/time.h> 
#endif

// ----------------------------------------------------------------------------
// keeps track of both "real time" and "simulation time"


Draw::Clock Draw::App::clock;


// ----------------------------------------------------------------------------
// camera automatically tracks selected vehicle


Draw::Camera Draw::App::camera;


// ----------------------------------------------------------------------------
// currently selected plug-in (user can choose or cycle through them)


Draw::PlugIn* Draw::App::selectedPlugIn = NULL;


// ----------------------------------------------------------------------------
// currently selected vehicle.  Generally the one the camera follows and
// for which additional information may be displayed.  Clicking the mouse
// near a vehicle causes it to become the Selected Vehicle.


Draw::AbstractVehicle* Draw::App::selectedVehicle = NULL;


// ----------------------------------------------------------------------------
// phase: identifies current phase of the per-frame update cycle


int Draw::App::phase = Draw::App::overheadPhase;


// ----------------------------------------------------------------------------
// graphical annotation: master on/off switch


bool Draw::enableAnnotation = true;


// ----------------------------------------------------------------------------
// XXX apparently MS VC6 cannot handle initialized static const members,
// XXX so they have to be initialized not-inline.


const int Draw::App::overheadPhase = 0;
const int Draw::App::updatePhase = 1;
const int Draw::App::drawPhase = 2;


// ----------------------------------------------------------------------------
// initialize App application

namespace {

    void printPlugIn (Draw::PlugIn& pi) {std::cout << " " << pi << std::endl;} // XXX

} // anonymous namespace

void 
Draw::App::initialize (void)
{
    // select the default PlugIn
    selectDefaultPlugIn ();

    {
        // XXX this block is for debugging purposes,
        // XXX should it be replaced with something permanent?

        std::cout << std::endl << "Known plugins:" << std::endl;   // xxx?
        PlugIn::applyToAll (printPlugIn);                          // xxx?
        std::cout << std::endl;                                    // xxx?

        // identify default PlugIn
        if (!selectedPlugIn) errorExit ("no default PlugIn");
        std::cout << std::endl << "Default plugin:" << std::endl;  // xxx?
        std::cout << " " << *selectedPlugIn << std::endl;          // xxx?
        std::cout << std::endl;                                    // xxx?
    }

    // initialize the default PlugIn
    openSelectedPlugIn ();
}


// ----------------------------------------------------------------------------
// main update function: step simulation forward and redraw scene


void 
Draw::App::updateSimulationAndRedraw (void)
{
    // update global simulation clock
    clock.update ();

    //  start the phase timer (XXX to accurately measure "overhead" time this
    //  should be in displayFunc, or somehow account for time outside this
    //  routine)
    initPhaseTimers ();

    // run selected PlugIn (with simulation's current time and step size)
    updateSelectedPlugIn (clock.getTotalSimulationTime (),
                          clock.getElapsedSimulationTime ());

    // redraw selected PlugIn (based on real time)
    redrawSelectedPlugIn (clock.getTotalRealTime (),
                          clock.getElapsedRealTime ());
}


// ----------------------------------------------------------------------------
// exit App with a given text message or error code


void 
Draw::App::errorExit (const char* message)
{
    printMessage (message);
#ifdef _MSC_VER
    MessageBox(0, message, "App Unfortunate Event", MB_ICONERROR);
#endif
    exit (-1);
}


void 
Draw::App::exit (int exitCode)
{
    ::exit (exitCode);
}


// ----------------------------------------------------------------------------
// select the default PlugIn


void 
Draw::App::selectDefaultPlugIn (void)
{
    PlugIn::sortBySelectionOrder ();
    selectedPlugIn = PlugIn::findDefault ();
}


// ----------------------------------------------------------------------------
// select the "next" plug-in, cycling through "plug-in selection order"


void 
Draw::App::selectNextPlugIn (void)
{
    closeSelectedPlugIn ();
    selectedPlugIn = selectedPlugIn->next ();
    openSelectedPlugIn ();
}


// ----------------------------------------------------------------------------
// handle function keys an a per-plug-in basis


void 
Draw::App::functionKeyForPlugIn (int keyNumber)
{
    selectedPlugIn->handleFunctionKeys (keyNumber);
}


// ----------------------------------------------------------------------------
// return name of currently selected plug-in


const char* 
Draw::App::nameOfSelectedPlugIn (void)
{
    return (selectedPlugIn ? selectedPlugIn->name() : "no PlugIn");
}


// ----------------------------------------------------------------------------
// open the currently selected plug-in


void 
Draw::App::openSelectedPlugIn (void)
{
    camera.reset ();
    selectedVehicle = NULL;
    selectedPlugIn->open ();
}


// ----------------------------------------------------------------------------
// do a simulation update for the currently selected plug-in


void 
Draw::App::updateSelectedPlugIn (const float currentTime,
                                                const float elapsedTime)
{
    // switch to Update phase
    pushPhase (updatePhase);

    // service queued reset request, if any
    doDelayedResetPlugInXXX ();

    // if no vehicle is selected, and some exist, select the first one
    if (selectedVehicle == NULL)
    {
        const AVGroup& vehicles = allVehiclesOfSelectedPlugIn();
        if (vehicles.size() > 0) selectedVehicle = vehicles.front();
    }

    // invoke selected PlugIn's Update method
    selectedPlugIn->update (currentTime, elapsedTime);

    // return to previous phase
    popPhase ();
}


// ----------------------------------------------------------------------------
// redraw graphics for the currently selected plug-in


void 
Draw::App::redrawSelectedPlugIn (const float currentTime,
                                                const float elapsedTime)
{
    // switch to Draw phase
    pushPhase (drawPhase);

    // invoke selected PlugIn's Draw method
    selectedPlugIn->redraw (currentTime, elapsedTime);

    // draw any annotation queued up during selected PlugIn's Update method
    drawAllDeferredLines ();
    drawAllDeferredCirclesOrDisks ();

    // return to previous phase
    popPhase ();
}


// ----------------------------------------------------------------------------
// close the currently selected plug-in


void 
Draw::App::closeSelectedPlugIn (void)
{
    selectedPlugIn->close ();
    selectedVehicle = NULL;
}


// ----------------------------------------------------------------------------
// reset the currently selected plug-in


void 
Draw::App::resetSelectedPlugIn (void)
{
    selectedPlugIn->reset ();
}


namespace {

    // ----------------------------------------------------------------------------
    // XXX this is used by CaptureTheFlag
    // XXX it was moved here from main.cpp on 12-4-02
    // XXX I'm not sure if this is a useful feature or a bogus hack
    // XXX needs to be reconsidered.


    bool gDelayedResetPlugInXXX = false;

} // anonymous namespace
    
    
void 
Draw::App::queueDelayedResetPlugInXXX (void)
{
    gDelayedResetPlugInXXX = true;
}


void 
Draw::App::doDelayedResetPlugInXXX (void)
{
    if (gDelayedResetPlugInXXX)
    {
        resetSelectedPlugIn ();
        gDelayedResetPlugInXXX = false;
    }
}


// ----------------------------------------------------------------------------
// return a group (an STL vector of AbstractVehicle pointers) of all
// vehicles(/agents/characters) defined by the currently selected PlugIn


const Draw::AVGroup& 
Draw::App::allVehiclesOfSelectedPlugIn (void)
{
    return selectedPlugIn->allVehicles ();
}


// ----------------------------------------------------------------------------
// select the "next" vehicle: the one listed after the currently selected one
// in allVehiclesOfSelectedPlugIn


void 
Draw::App::selectNextVehicle (void)
{
    if (selectedVehicle != NULL)
    {
        // get a container of all vehicles
        const AVGroup& all = allVehiclesOfSelectedPlugIn ();
        const AVIterator first = all.begin();
        const AVIterator last = all.end();

        // find selected vehicle in container
        const AVIterator s = std::find (first, last, selectedVehicle);

        // normally select the next vehicle in container
        selectedVehicle = *(s+1);

        // if we are at the end of the container, select the first vehicle
        if (s == last-1) selectedVehicle = *first;

        // if the search failed, use NULL
        if (s == last) selectedVehicle = NULL;
    }
}


// ----------------------------------------------------------------------------
// select vehicle nearest the given screen position (e.g.: of the mouse)


void 
Draw::App::selectVehicleNearestScreenPosition (int x, int y)
{
    selectedVehicle = findVehicleNearestScreenPosition (x, y);
}


// ----------------------------------------------------------------------------
// Find the AbstractVehicle whose screen position is nearest the current the
// mouse position.  Returns NULL if mouse is outside this window or if
// there are no AbstractVehicle.


Draw::AbstractVehicle* 
Draw::App::vehicleNearestToMouse (void)
{
    return (mouseInWindow ? 
            findVehicleNearestScreenPosition (mouseX, mouseY) :
            NULL);
}


// ----------------------------------------------------------------------------
// Find the AbstractVehicle whose screen position is nearest the given window
// coordinates, typically the mouse position.  Returns NULL if there are no
// AbstractVehicles.
//
// This works by constructing a line in 3d space between the camera location
// and the "mouse point".  Then it measures the distance from that line to the
// centers of each AbstractVehicle.  It returns the AbstractVehicle whose
// distance is smallest.
//
// xxx Issues: Should the distanceFromLine test happen in "perspective space"
// xxx or in "screen space"?  Also: I think this would be happy to select a
// xxx vehicle BEHIND the camera location.


Draw::AbstractVehicle* 
Draw::App::findVehicleNearestScreenPosition (int x, int y)
{
    // find the direction from the camera position to the given pixel
    const Vec3 direction = directionFromCameraToScreenPosition (x, y, glutGet (GLUT_WINDOW_HEIGHT));

    // iterate over all vehicles to find the one whose center is nearest the
    // "eye-mouse" selection line
    float minDistance = FLT_MAX;       // smallest distance found so far
    AbstractVehicle* nearest = NULL;   // vehicle whose distance is smallest
    const AVGroup& vehicles = allVehiclesOfSelectedPlugIn();
    for (AVIterator i = vehicles.begin(); i != vehicles.end(); i++)
    {
        // distance from this vehicle's center to the selection line:
        const float d = distanceFromLine ((**i).position(),
                                          camera.position(),
                                          direction);

        // if this vehicle-to-line distance is the smallest so far,
        // store it and this vehicle in the selection registers.
        if (d < minDistance)
        {
            minDistance = d;
            nearest = *i;
        }
    }

    return nearest;
}


// ----------------------------------------------------------------------------
// for storing most recent mouse state


int Draw::App::mouseX = 0;
int Draw::App::mouseY = 0;
bool Draw::App::mouseInWindow = false;


// ----------------------------------------------------------------------------
// set a certain initial camera state used by several plug-ins


void 
Draw::App::init3dCamera (AbstractVehicle& selected)
{
    init3dCamera (selected, cameraTargetDistance, camera2dElevation);
}

void 
Draw::App::init3dCamera (AbstractVehicle& selected,
                                  float distance,
                                  float elevation)
{
    position3dCamera (selected, distance, elevation);
    camera.fixedDistDistance = distance;
    camera.fixedDistVOffset = elevation;
    camera.mode = Camera::cmFixedDistanceOffset;
}


void 
Draw::App::init2dCamera (AbstractVehicle& selected)
{
    init2dCamera (selected, cameraTargetDistance, camera2dElevation);
}

void 
Draw::App::init2dCamera (AbstractVehicle& selected,
                                  float distance,
                                  float elevation)
{
    position2dCamera (selected, distance, elevation);
    camera.fixedDistDistance = distance;
    camera.fixedDistVOffset = elevation;
    camera.mode = Camera::cmFixedDistanceOffset;
}


void 
Draw::App::position3dCamera (AbstractVehicle& selected)
{
    position3dCamera (selected, cameraTargetDistance, camera2dElevation);
}

void 
Draw::App::position3dCamera (AbstractVehicle& selected,
                                            float distance,
                                            float /*elevation*/)
{
    selectedVehicle = &selected;
    if (&selected)
    {
        const Vec3 behind = selected.forward() * -distance;
        camera.setPosition (selected.position() + behind);
        camera.target = selected.position();
    }
}


void 
Draw::App::position2dCamera (AbstractVehicle& selected)
{
    position2dCamera (selected, cameraTargetDistance, camera2dElevation);
}

void 
Draw::App::position2dCamera (AbstractVehicle& selected,
                                            float distance,
                                            float elevation)
{
    // position the camera as if in 3d:
    position3dCamera (selected, distance, elevation);

    // then adjust for 3d:
    Vec3 position3d = camera.position();
    position3d.y += elevation;
    camera.setPosition (position3d);
}


// ----------------------------------------------------------------------------
// camera updating utility used by several plug-ins


void 
Draw::App::updateCamera (const float currentTime,
                                        const float elapsedTime,
                                        const AbstractVehicle& selected)
{
    camera.vehicleToTrack = &selected;
    camera.update (currentTime, elapsedTime, clock.getPausedState ());
}


// ----------------------------------------------------------------------------
// some camera-related default constants


const float Draw::App::camera2dElevation = 8;
const float Draw::App::cameraTargetDistance = 13;
const Draw::Vec3 Draw::App::cameraTargetOffset (0, Draw::App::camera2dElevation, 
                                                                    0);


// ----------------------------------------------------------------------------
// ground plane grid-drawing utility used by several plug-ins


void 
Draw::App::gridUtility (const Vec3& gridTarget)
{
    // round off target to the nearest multiple of 2 (because the
    // checkboard grid with a pitch of 1 tiles with a period of 2)
    // then lower the grid a bit to put it under 2d annotation lines
    const Vec3 gridCenter ((round (gridTarget.x * 0.5f) * 2),
                           (round (gridTarget.y * 0.5f) * 2) - .05f,
                           (round (gridTarget.z * 0.5f) * 2));

    // colors for checkboard
    const Color gray1(0.27f);
    const Color gray2(0.30f);

    // draw 50x50 checkerboard grid with 50 squares along each side
    drawXZCheckerboardGrid (50, 50, gridCenter, gray1, gray2);

    // alternate style
    // drawXZLineGrid (50, 50, gridCenter, gBlack);
}


// ----------------------------------------------------------------------------
// draws a gray disk on the XZ plane under a given vehicle


void 
Draw::App::highlightVehicleUtility (const AbstractVehicle& vehicle)
{
    if (&vehicle != NULL)
        drawXZDisk (vehicle.radius(), vehicle.position(), gGray60, 20);
}


// ----------------------------------------------------------------------------
// draws a gray circle on the XZ plane under a given vehicle


void 
Draw::App::circleHighlightVehicleUtility (const AbstractVehicle& vehicle)
{
    if (&vehicle != NULL) drawXZCircle (vehicle.radius () * 1.1f,
                                        vehicle.position(),
                                        gGray60,
                                        20);
}


// ----------------------------------------------------------------------------
// draw a box around a vehicle aligned with its local space
// xxx not used as of 11-20-02


void 
Draw::App::drawBoxHighlightOnVehicle (const AbstractVehicle& v,
                                               const Color& color)
{
    if (&v)
    {
        const float diameter = v.radius() * 2;
        const Vec3 size (diameter, diameter, diameter);
        drawBoxOutline (v, size, color);
    }
}


// ----------------------------------------------------------------------------
// draws a colored circle (perpendicular to view axis) around the center
// of a given vehicle.  The circle's radius is the vehicle's radius times
// radiusMultiplier.


void 
Draw::App::drawCircleHighlightOnVehicle (const AbstractVehicle& v,
                                                  const float radiusMultiplier,
                                                  const Color& color)
{
    if (&v)
    {
        const Vec3& cPosition = camera.position();
        draw3dCircle  (v.radius() * radiusMultiplier,  // adjusted radius
                       v.position(),                   // center
                       v.position() - cPosition,       // view axis
                       color,                          // drawing color
                       20);                            // circle segments
    }
}


// ----------------------------------------------------------------------------


void 
Draw::App::printMessage (const char* message)
{
    std::cout << "App: " <<  message << std::endl << std::flush;
}


void 
Draw::App::printMessage (const std::ostringstream& message)
{
    printMessage (message.str().c_str());
}


void 
Draw::App::printWarning (const char* message)
{
    std::cout << "App: Warning: " <<  message << std::endl << std::flush;
}


void 
Draw::App::printWarning (const std::ostringstream& message)
{
    printWarning (message.str().c_str());
}


// ------------------------------------------------------------------------
// print list of known commands
//
// XXX this list should be assembled automatically,
// XXX perhaps from a list of "command" objects created at initialization


void 
Draw::App::keyboardMiniHelp (void)
{
    printMessage ("");
    printMessage ("defined single key commands:");
    printMessage ("  r      restart current PlugIn.");
    printMessage ("  s      select next vehicle.");
    printMessage ("  c      select next camera mode.");
    printMessage ("  f      select next preset frame rate");
    printMessage ("  Tab    select next PlugIn.");
    printMessage ("  a      toggle annotation on/off.");
    printMessage ("  Space  toggle between Run and Pause.");
    printMessage ("  ->     step forward one frame.");
    printMessage ("  Esc    exit.");
    printMessage ("");

    // allow PlugIn to print mini help for the function keys it handles
    selectedPlugIn->printMiniHelpForFunctionKeys ();
}


// ----------------------------------------------------------------------------
// manage App phase transitions (xxx and maintain phase timers)


int Draw::App::phaseStackIndex = 0;
const int Draw::App::phaseStackSize = 5;
int Draw::App::phaseStack [Draw::App::phaseStackSize];

namespace Draw {
bool updatePhaseActive = false;
bool drawPhaseActive = false;
}

void 
Draw::App::pushPhase (const int newPhase)
{
    updatePhaseActive = newPhase == Draw::App::updatePhase;
    drawPhaseActive = newPhase == Draw::App::drawPhase;

    // update timer for current (old) phase: add in time since last switch
    updatePhaseTimers ();

    // save old phase
    phaseStack[phaseStackIndex++] = phase;

    // set new phase
    phase = newPhase;

    // check for stack overflow
    if (phaseStackIndex >= phaseStackSize) errorExit ("phaseStack overflow");
}


void 
Draw::App::popPhase (void)
{
    // update timer for current (old) phase: add in time since last switch
    updatePhaseTimers ();

    // restore old phase
    phase = phaseStack[--phaseStackIndex];
    updatePhaseActive = phase == Draw::App::updatePhase;
    drawPhaseActive = phase == Draw::App::drawPhase;
}


// ----------------------------------------------------------------------------


float Draw::App::phaseTimerBase = 0;
float Draw::App::phaseTimers [drawPhase+1];


void 
Draw::App::initPhaseTimers (void)
{
    phaseTimers[drawPhase] = 0;
    phaseTimers[updatePhase] = 0;
    phaseTimers[overheadPhase] = 0;
    phaseTimerBase = clock.getTotalRealTime ();
}


void 
Draw::App::updatePhaseTimers (void)
{
    const float currentRealTime = clock.realTimeSinceFirstClockUpdate();
    phaseTimers[phase] += currentRealTime - phaseTimerBase;
    phaseTimerBase = currentRealTime;
}


// ----------------------------------------------------------------------------


namespace {

    std::string const appVersionName("App 0.8.2");

    // The number of our GLUT window
    int windowID;

    bool gMouseAdjustingCameraAngle = false;
    bool gMouseAdjustingCameraRadius = false;
    int gMouseAdjustingCameraLastX;
    int gMouseAdjustingCameraLastY;




    // ----------------------------------------------------------------------------
    // initialize GL mode settings


    void 
    initGL (void)
    {
        // background = dark gray
        // @todo bknafla Changed the background color to make some screenshots.
        glClearColor (0.3f, 0.3f, 0.3f, 0);
        // glClearColor( 1.0f, 1.0f, 1.0f, 0.0f );

        // enable depth buffer clears
        glClearDepth (1.0f);

        // select smooth shading
        glShadeModel (GL_SMOOTH);

        // enable  and select depth test
        glDepthFunc (GL_LESS);
        glEnable (GL_DEPTH_TEST);

        // turn on backface culling
        glEnable (GL_CULL_FACE);
        glCullFace (GL_BACK);

        // enable blending and set typical "blend into frame buffer" mode
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // reset projection matrix
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
    }





    // ----------------------------------------------------------------------------
    // handler for window resizing


    void 
    reshapeFunc (int width, int height)
    {
        // set viewport to full window
        glViewport(0, 0, width, height);

        // set perspective transformation
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        const GLfloat w = width;
        const GLfloat h = height;
        const GLfloat aspectRatio = (height == 0) ? 1 : w/h;
        const GLfloat fieldOfViewY = 45;
        const GLfloat hither = 1;  // put this on Camera so PlugIns can frob it
        const GLfloat yon = 400;   // put this on Camera so PlugIns can frob it
        gluPerspective (fieldOfViewY, aspectRatio, hither, yon);

        // leave in modelview mode
        glMatrixMode(GL_MODELVIEW);
    }


    // ----------------------------------------------------------------------------
    // This is called (by GLUT) each time a mouse button pressed or released.


    void 
    mouseButtonFunc (int button, int state, int x, int y)
    {
        // if the mouse button has just been released
        if (state == GLUT_UP)
        {
            // end any ongoing mouse-adjusting-camera session
            gMouseAdjustingCameraAngle = false;
            gMouseAdjustingCameraRadius = false;
        }

        // if the mouse button has just been pushed down
        if (state == GLUT_DOWN)
        {
            // names for relevant values of "button" and "state"
            const int  mods       = glutGetModifiers ();
            const bool modNone    = (mods == 0);
            const bool modCtrl    = (mods == GLUT_ACTIVE_CTRL);
            const bool modAlt     = (mods == GLUT_ACTIVE_ALT);
            const bool modCtrlAlt = (mods == (GLUT_ACTIVE_CTRL | GLUT_ACTIVE_ALT));
            const bool mouseL     = (button == GLUT_LEFT_BUTTON);
            const bool mouseM     = (button == GLUT_MIDDLE_BUTTON);
            const bool mouseR     = (button == GLUT_RIGHT_BUTTON);

    #if __APPLE__ && __MACH__
            const bool macosx = true;
    #else
            const bool macosx = false;
    #endif

            // mouse-left (with no modifiers): select vehicle
            if (modNone && mouseL)
            {
                Draw::App::selectVehicleNearestScreenPosition (x, y);
            }

            // control-mouse-left: begin adjusting camera angle (on Mac OS X
            // control-mouse maps to mouse-right for "context menu", this makes
            // App's control-mouse work work the same on OS X as on Linux
            // and Windows, but it precludes using a mouseR context menu)
            if ((modCtrl && mouseL) ||
               (modNone && mouseR && macosx))
            {
                gMouseAdjustingCameraLastX = x;
                gMouseAdjustingCameraLastY = y;
                gMouseAdjustingCameraAngle = true;
            }

            // control-mouse-middle: begin adjusting camera radius
            // (same for: control-alt-mouse-left and control-alt-mouse-middle,
            // and on Mac OS X it is alt-mouse-right)
            if ((modCtrl    && mouseM) ||
                (modCtrlAlt && mouseL) ||
                (modCtrlAlt && mouseM) ||
                (modAlt     && mouseR && macosx))
            {
                gMouseAdjustingCameraLastX = x;
                gMouseAdjustingCameraLastY = y;
                gMouseAdjustingCameraRadius = true;
            }
        }
    }


    // ----------------------------------------------------------------------------
    // called when mouse moves and any buttons are down


    void 
    mouseMotionFunc (int x, int y)
    {
        // are we currently in the process of mouse-adjusting the camera?
        if (gMouseAdjustingCameraAngle || gMouseAdjustingCameraRadius)
        {
            // speed factors to map from mouse movement in pixels to 3d motion
            const float dSpeed = 0.005f;
            const float rSpeed = 0.01f;

            // XY distance (in pixels) that mouse moved since last update
            const float dx = x - gMouseAdjustingCameraLastX;
            const float dy = y - gMouseAdjustingCameraLastY;
            gMouseAdjustingCameraLastX = x;
            gMouseAdjustingCameraLastY = y;

            Draw::Vec3 cameraAdjustment;

            // set XY values according to mouse motion on screen space
            if (gMouseAdjustingCameraAngle)
            {
                cameraAdjustment.x = dx * -dSpeed;
                cameraAdjustment.y = dy * +dSpeed;
            }

            // set Z value according vertical to mouse motion
            if (gMouseAdjustingCameraRadius)
            {
                cameraAdjustment.z = dy * rSpeed;
            }

            // pass adjustment vector to camera's mouse adjustment routine
            Draw::App::camera.mouseAdjustOffset (cameraAdjustment);
        }
    }


    // ----------------------------------------------------------------------------
    // called when mouse moves and no buttons are down


    void 
    mousePassiveMotionFunc (int x, int y)
    {
        Draw::App::mouseX = x;
        Draw::App::mouseY = y;
    }


    // ----------------------------------------------------------------------------
    // called when mouse enters or exits the window


    void 
    mouseEnterExitWindowFunc (int state)
    {
        if (state == GLUT_ENTERED) Draw::App::mouseInWindow = true;
        if (state == GLUT_LEFT)    Draw::App::mouseInWindow = false;
    }


    // ----------------------------------------------------------------------------
    // draw PlugI name in upper lefthand corner of screen


    void 
    drawDisplayPlugInName (void)
    {
        const float h = glutGet (GLUT_WINDOW_HEIGHT);
        const Draw::Vec3 screenLocation (10, h-20, 0);
        draw2dTextAt2dLocation (*Draw::App::nameOfSelectedPlugIn (),
                                screenLocation,
                                Draw::gWhite, Draw::drawGetWindowWidth(), Draw::drawGetWindowHeight());
    }


    // ----------------------------------------------------------------------------
    // draw camera mode name in lower lefthand corner of screen


    void 
    drawDisplayCameraModeName (void)
    {
        std::ostringstream message;
        message << "Camera: " << Draw::App::camera.modeName () << std::ends;
        const Draw::Vec3 screenLocation (10, 10, 0);
        Draw::draw2dTextAt2dLocation (message, screenLocation, Draw::gWhite, Draw::drawGetWindowWidth(), Draw::drawGetWindowHeight());
    }


    // ----------------------------------------------------------------------------
    // helper for drawDisplayFPS



    void 
    writePhaseTimerReportToStream (float phaseTimer,
                                              std::ostringstream& stream)
    {
        // write the timer value in seconds in floating point
        stream << std::setprecision (5) << std::setiosflags (std::ios::fixed);
        stream << phaseTimer;

        // restate value in another form
        stream << std::setprecision (0) << std::setiosflags (std::ios::fixed);
        stream << " (";

        // different notation for variable and fixed frame rate
        if (Draw::App::clock.getVariableFrameRateMode())
        {
            // express as FPS (inverse of phase time)
            stream << 1 / phaseTimer;
            stream << " fps)\n";
        }
        else
        {
            // quantify time as a percentage of frame time
            const int fps = Draw::App::clock.getFixedFrameRate ();
            stream << ((100 * phaseTimer) / (1.0f / fps));
            stream << "% of 1/";
            stream << fps;
            stream << "sec)\n";
        }
    }


    // ----------------------------------------------------------------------------
    // draw text showing (smoothed, rounded) "frames per second" rate
    // (and later a bunch of related stuff was dumped here, a reorg would be nice)
    //
    // XXX note: drawDisplayFPS has morphed considerably and should be called
    // something like displayClockStatus, and that it should be part of
    // App instead of Draw  (cwr 11-23-04)

    float gSmoothedTimerDraw = 0;
    float gSmoothedTimerUpdate = 0;
    float gSmoothedTimerOverhead = 0;

    void
    drawDisplayFPS (void)
    {
        // skip several frames to allow frame rate to settle
        static int skipCount = 10;
        if (skipCount > 0)
        {
            skipCount--;
        }
        else
        {
            // keep track of font metrics and start of next line
            const int lh = 16; // xxx line height
            const int cw = 9; // xxx character width
            Draw::Vec3 screenLocation (10, 10, 0);

            // target and recent average frame rates
            const int targetFPS = Draw::App::clock.getFixedFrameRate ();
            const float smoothedFPS = Draw::App::clock.getSmoothedFPS ();

            // describe clock mode and frame rate statistics
            screenLocation.y += lh;
            std::ostringstream clockStr;
            clockStr << "Clock: ";
            if (Draw::App::clock.getAnimationMode ())
            {
                clockStr << "animation mode (";
                clockStr << targetFPS << " fps,";
                clockStr << " display "<< Draw::round(smoothedFPS) << " fps, ";
                const float ratio = smoothedFPS / targetFPS;
                clockStr << (int) (100 * ratio) << "% of nominal speed)";
            }
            else
            {
                clockStr << "real-time mode, ";
                if (Draw::App::clock.getVariableFrameRateMode ())
                {
                    clockStr << "variable frame rate (";
                    clockStr << Draw::round(smoothedFPS) << " fps)";
                }
                else
                {
                    clockStr << "fixed frame rate (target: " << targetFPS;
                    clockStr << " actual: " << Draw::round(smoothedFPS) << ", ";

                    Draw::Vec3 sp;
                    sp = screenLocation;
                    sp.x += cw * (int) clockStr.tellp ();

                    // create usage description character string
                    std::ostringstream xxxStr;
                    xxxStr << std::setprecision (0)
                           << std::setiosflags (std::ios::fixed)
                           << "usage: " << Draw::App::clock.getSmoothedUsage ()
                           << "%"
                           << std::ends;

                    const int usageLength = ((int) xxxStr.tellp ()) - 1;
                    for (int i = 0; i < usageLength; i++) clockStr << " ";
                    clockStr << ")";

                    // display message in lower left corner of window
                    // (draw in red if the instantaneous usage is 100% or more)
                    const float usage = Draw::App::clock.getUsage ();
                    const Draw::Color color = (usage >= 100) ? Draw::gRed : Draw::gWhite;
                    draw2dTextAt2dLocation (xxxStr, sp, color, Draw::drawGetWindowWidth(), Draw::drawGetWindowHeight());
                }
            }
            if (Draw::App::clock.getPausedState ())
                clockStr << " [paused]";
            clockStr << std::ends;
            draw2dTextAt2dLocation (clockStr, screenLocation, Draw::gWhite, Draw::drawGetWindowWidth(), Draw::drawGetWindowHeight());

            // get smoothed phase timer information
            const float ptd = Draw::App::phaseTimerDraw();
            const float ptu = Draw::App::phaseTimerUpdate();
            const float pto = Draw::App::phaseTimerOverhead();
            const float smoothRate = Draw::App::clock.getSmoothingRate ();
            Draw::blendIntoAccumulator (smoothRate, ptd, gSmoothedTimerDraw);
            Draw::blendIntoAccumulator (smoothRate, ptu, gSmoothedTimerUpdate);
            Draw::blendIntoAccumulator (smoothRate, pto, gSmoothedTimerOverhead);

            // display phase timer information
            screenLocation.y += lh * 4;
            std::ostringstream timerStr;
            timerStr << "update: ";
            writePhaseTimerReportToStream (gSmoothedTimerUpdate, timerStr);
            timerStr << "draw:   ";
            writePhaseTimerReportToStream (gSmoothedTimerDraw, timerStr);
            timerStr << "other:  ";
            writePhaseTimerReportToStream (gSmoothedTimerOverhead, timerStr);
            timerStr << std::ends;
            draw2dTextAt2dLocation (timerStr, screenLocation, Draw::gGreen, Draw::drawGetWindowWidth(), Draw::drawGetWindowHeight());
        }
    }


    // ------------------------------------------------------------------------
    // cycle through frame rate presets  (XXX move this to App)


    void 
    selectNextPresetFrameRate (void)
    {
        // note that the cases are listed in reverse order, and that 
        // the default is case 0 which causes the index to wrap around
        static int frameRatePresetIndex = 0;
        switch (++frameRatePresetIndex)
        {
        case 3: 
            // animation mode at 60 fps
            Draw::App::clock.setFixedFrameRate (60);
            Draw::App::clock.setAnimationMode (true);
            Draw::App::clock.setVariableFrameRateMode (false);
            break;
        case 2: 
            // real-time fixed frame rate mode at 60 fps
            Draw::App::clock.setFixedFrameRate (60);
            Draw::App::clock.setAnimationMode (false);
            Draw::App::clock.setVariableFrameRateMode (false);
            break;
        case 1: 
            // real-time fixed frame rate mode at 24 fps
            Draw::App::clock.setFixedFrameRate (24);
            Draw::App::clock.setAnimationMode (false);
            Draw::App::clock.setVariableFrameRateMode (false);
            break;
        case 0:
        default:
            // real-time variable frame rate mode ("as fast as possible")
            frameRatePresetIndex = 0;
            Draw::App::clock.setFixedFrameRate (0);
            Draw::App::clock.setAnimationMode (false);
            Draw::App::clock.setVariableFrameRateMode (true);
            break;
        }
    }


    // ------------------------------------------------------------------------
    // This function is called (by GLUT) each time a key is pressed.
    //
    // XXX the bulk of this should be moved to App
    //
    // parameter names commented out to prevent compiler warning from "-W"


    void 
    keyboardFunc (unsigned char key, int /*x*/, int /*y*/) 
    {
        std::ostringstream message;

        // ascii codes
        const int tab = 9;
        const int space = 32;
        const int esc = 27; // escape key

        switch (key)
        {
        // reset selected PlugIn
        case 'r':
            Draw::App::resetSelectedPlugIn ();
            message << "reset PlugIn "
                    << '"' << Draw::App::nameOfSelectedPlugIn () << '"'
                    << std::ends;
            Draw::App::printMessage (message);
            break;

        // cycle selection to next vehicle
        case 's':
            Draw::App::printMessage ("select next vehicle/agent");
            Draw::App::selectNextVehicle ();
            break;

        // camera mode cycle
        case 'c':
            Draw::App::camera.selectNextMode ();
            message << "select camera mode "
                    << '"' << Draw::App::camera.modeName () << '"' << std::ends;
            Draw::App::printMessage (message);
            break;

        // select next PlugIn
        case tab:
            Draw::App::selectNextPlugIn ();
            message << "select next PlugIn: "
                    << '"' << Draw::App::nameOfSelectedPlugIn () << '"'
                    << std::ends;
            Draw::App::printMessage (message);
            break;

        // toggle annotation state
        case 'a':
            Draw::App::printMessage (Draw::toggleAnnotationState () ?
                                                    "annotation ON" : "annotation OFF");
            break;

        // toggle run/pause state
        case space:
            Draw::App::printMessage (Draw::App::clock.togglePausedState () ?
                                                    "pause" : "run");
            break;

        // cycle through frame rate (clock mode) presets
        case 'f':
            selectNextPresetFrameRate ();
            message << "set clock to ";
            if (Draw::App::clock.getAnimationMode ())
                message << "animation mode, fixed frame rate ("
                        << Draw::App::clock.getFixedFrameRate () << " fps)";
            else
            {
                message << "real-time mode, ";
                if (Draw::App::clock.getVariableFrameRateMode ())
                    message << "variable frame rate";
                else
                    message << "fixed frame rate ("
                            << Draw::App::clock.getFixedFrameRate () << " fps)";
            }
            message << std::ends;
            Draw::App::printMessage (message);
            break;

        // print minimal help for single key commands
        case '?':
            Draw::App::keyboardMiniHelp ();
            break;

        // exit application with normal status 
        case esc:
            glutDestroyWindow (windowID);
            Draw::App::printMessage ("exit.");
            Draw::App::exit (0);

        default:
            message << "unrecognized single key command: " << key;
            message << " (" << (int)key << ")";//xxx perhaps only for debugging?
            message << std::ends;
            Draw::App::printMessage ("");
            Draw::App::printMessage (message);
            Draw::App::keyboardMiniHelp ();
        }
    }


    // ------------------------------------------------------------------------
    // handles "special" keys,
    // function keys are handled by the PlugIn
    //
    // parameter names commented out to prevent compiler warning from "-W"

    void 
    specialFunc (int key, int /*x*/, int /*y*/)
    {
        std::ostringstream message;

        switch (key)
        {
        case GLUT_KEY_F1:  Draw::App::functionKeyForPlugIn (1);  break;
        case GLUT_KEY_F2:  Draw::App::functionKeyForPlugIn (2);  break;
        case GLUT_KEY_F3:  Draw::App::functionKeyForPlugIn (3);  break;
        case GLUT_KEY_F4:  Draw::App::functionKeyForPlugIn (4);  break;
        case GLUT_KEY_F5:  Draw::App::functionKeyForPlugIn (5);  break;
        case GLUT_KEY_F6:  Draw::App::functionKeyForPlugIn (6);  break;
        case GLUT_KEY_F7:  Draw::App::functionKeyForPlugIn (7);  break;
        case GLUT_KEY_F8:  Draw::App::functionKeyForPlugIn (8);  break;
        case GLUT_KEY_F9:  Draw::App::functionKeyForPlugIn (9);  break;
        case GLUT_KEY_F10: Draw::App::functionKeyForPlugIn (10); break;
        case GLUT_KEY_F11: Draw::App::functionKeyForPlugIn (11); break;
        case GLUT_KEY_F12: Draw::App::functionKeyForPlugIn (12); break;

        case GLUT_KEY_RIGHT:
            Draw::App::clock.setPausedState (true);
            message << "single step forward (frame time: "
                    << Draw::App::clock.advanceSimulationTimeOneFrame ()
                    << ")"
                    << std::endl;
            Draw::App::printMessage (message);
            break;
        }
    }


    // ------------------------------------------------------------------------
    // Main drawing function for App application,
    // drives simulation as a side effect


    void 
    displayFunc (void)
    {
        // clear color and depth buffers
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // run simulation and draw associated graphics
        Draw::App::updateSimulationAndRedraw ();

        // draw text showing (smoothed, rounded) "frames per second" rate
        drawDisplayFPS ();

        // draw the name of the selected PlugIn
        drawDisplayPlugInName ();

        // draw the name of the camera's current mode
        drawDisplayCameraModeName ();

        // draw crosshairs to indicate aimpoint (xxx for debugging only?)
        // drawReticle ();

        // check for errors in drawing module, if so report and exit
        Draw::checkForDrawError ("App::updateSimulationAndRedraw");

        // double buffering, swap back and front buffers
        glFlush ();
        glutSwapBuffers();
    }


} // annonymous namespace



// ----------------------------------------------------------------------------
// do all initialization related to graphics


void 
Draw::initializeGraphics (int argc, char **argv)
{
    // initialize GLUT state based on command line arguments
    glutInit (&argc, argv);  

    // display modes: RGB+Z and double buffered
    GLint mode = GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE;
    glutInitDisplayMode (mode);

    // create and initialize our window with GLUT tools
    // (center window on screen with size equal to "ws" times screen size)
    const int sw = glutGet (GLUT_SCREEN_WIDTH);
    const int sh = glutGet (GLUT_SCREEN_HEIGHT);
    const float ws = 0.8f; // window_size / screen_size
    const int ww = (int) (sw * ws);
    const int wh = (int) (sh * ws);
    glutInitWindowPosition ((int) (sw * (1-ws)/2), (int) (sh * (1-ws)/2));
    glutInitWindowSize (ww, wh);
    windowID = glutCreateWindow (appVersionName.c_str());
    reshapeFunc (ww, wh);
    initGL ();

    // register our display function, make it the idle handler too
    glutDisplayFunc (&displayFunc);  
    glutIdleFunc (&displayFunc);

    // register handler for window reshaping
    glutReshapeFunc (&reshapeFunc);

    // register handler for keyboard events
    glutKeyboardFunc (&keyboardFunc);
    glutSpecialFunc (&specialFunc);

    // register handler for mouse button events
    glutMouseFunc (&mouseButtonFunc);

    // register handler to track mouse motion when any button down
    glutMotionFunc (mouseMotionFunc);

    // register handler to track mouse motion when no buttons down
    glutPassiveMotionFunc (mousePassiveMotionFunc);

    // register handler for when mouse enters or exists the window
    glutEntryFunc (mouseEnterExitWindowFunc);
}


// ----------------------------------------------------------------------------
// run graphics event loop


void 
Draw::runGraphics (void)
{
    glutMainLoop ();  
}



// ----------------------------------------------------------------------------
// accessors for GLUT's window dimensions


float 
Draw::drawGetWindowHeight (void) 
{
    return glutGet (GLUT_WINDOW_HEIGHT);
}


float 
Draw::drawGetWindowWidth  (void) 
{
    return glutGet (GLUT_WINDOW_WIDTH);
}

Draw::Clock::Clock (void)
{
    // default is "real time, variable frame rate" and not paused
    setFixedFrameRate (0);
    setPausedState (false);
    setAnimationMode (false);
    setVariableFrameRateMode (true);

    // real "wall clock" time since launch
    totalRealTime = 0;

    // time simulation has run
    totalSimulationTime = 0;

    // time spent paused
    totalPausedTime = 0;

    // sum of (non-realtime driven) advances to simulation time
    totalAdvanceTime = 0;

    // interval since last simulation time 
    elapsedSimulationTime = 0;

    // interval since last clock update time 
    elapsedRealTime = 0;

    // interval since last clock update,
    // exclusive of time spent waiting for frame boundary when targetFPS>0
    elapsedNonWaitRealTime = 0;

    // "manually" advance clock by this amount on next update
    newAdvanceTime = 0;

    // "Calendar time" when this clock was first updated
#ifdef _WIN32
    basePerformanceCounter = 0;  // from QueryPerformanceCounter on Windows
#else
    baseRealTimeSec = 0;         // from gettimeofday on Linux and Mac OS X
    baseRealTimeUsec = 0;
#endif

    // clock keeps track of "smoothed" running average of recent frame rates.
    // When a fixed frame rate is used, a running average of "CPU load" is
    // kept (aka "non-wait time", the percentage of each frame time (time
    // step) that the CPU is busy).
    smoothedFPS = 0;
    smoothedUsage = 0;
}


// ----------------------------------------------------------------------------
// update this clock, called once per simulation step ("frame") to:
//
//     track passage of real time
//     manage passage of simulation time (modified by Paused state)
//     measure time elapsed between time updates ("frame rate")
//     optionally: "wait" for next realtime frame boundary


void 
Draw::Clock::update (void)
{
    // keep track of average frame rate and average usage percentage
    updateSmoothedRegisters ();

    // wait for next frame time (when targetFPS>0)
    // XXX should this be at the end of the update function?
    frameRateSync ();

    // save previous real time to measure elapsed time
    const float previousRealTime = totalRealTime;

    // real "wall clock" time since this application was launched
    totalRealTime = realTimeSinceFirstClockUpdate ();

    // time since last clock update
    elapsedRealTime = totalRealTime - previousRealTime;

    // accumulate paused time
    if (paused) totalPausedTime += elapsedRealTime;

    // save previous simulation time to measure elapsed time
    const float previousSimulationTime = totalSimulationTime;

    // update total simulation time
    if (getAnimationMode ())
    {
        // for "animation mode" use fixed frame time, ignore real time
        const float frameDuration = 1.0f / getFixedFrameRate ();
        totalSimulationTime += paused ? newAdvanceTime : frameDuration;
        if (!paused) newAdvanceTime += frameDuration - elapsedRealTime;
    }
    else
    {
        // new simulation time is total run time minus time spent paused
        totalSimulationTime = (totalRealTime
                               + totalAdvanceTime
                               - totalPausedTime);
    }


    // update total "manual advance" time
    totalAdvanceTime += newAdvanceTime;

    // how much time has elapsed since the last simulation step?
    elapsedSimulationTime = (paused ?
                             newAdvanceTime :
                             (totalSimulationTime - previousSimulationTime));

    // reset advance amount
    newAdvanceTime = 0;
}


// ----------------------------------------------------------------------------
// "wait" until next frame time (actually spin around this tight loop)
//
//
// (xxx there are probably a smarter ways to do this (using events or
// thread waits (eg usleep)) but they are likely to be unportable. xxx)


void 
Draw::Clock::frameRateSync (void)
{
    // when in real time fixed frame rate mode
    // (not animation mode and not variable frame rate mode)
    if ((! getAnimationMode ()) && (! getVariableFrameRateMode ()))
    {
        // find next (real time) frame start time
        const float targetStepSize = 1.0f / getFixedFrameRate ();
        const float now = realTimeSinceFirstClockUpdate ();
        const int lastFrameCount = (int) (now / targetStepSize);
        const float nextFrameTime = (lastFrameCount + 1) * targetStepSize;

        // record usage ("busy time", "non-wait time") for App app
        elapsedNonWaitRealTime = now - totalRealTime;

        // wait until next frame time
        do {} while (realTimeSinceFirstClockUpdate () < nextFrameTime); 
    }
}


// ----------------------------------------------------------------------------
// force simulation time ahead, ignoring passage of real time.
// Used for App's "single step forward" and animation mode


float 
Draw::Clock::advanceSimulationTimeOneFrame (void)
{
    // decide on what frame time is (use fixed rate, average for variable rate)
    const float fps = (getVariableFrameRateMode () ?
                       getSmoothedFPS () :
                       getFixedFrameRate ());
    const float frameTime = 1 / fps;

    // bump advance time
    advanceSimulationTime (frameTime);

    // return the time value used (for App)
    return frameTime; 
}


void 
Draw::Clock::advanceSimulationTime (const float seconds)
{
    if (seconds < 0) {
        /// @todo - throw? how to handle error conditions? Not by crashing an app!
        std::cerr << "negative arg to advanceSimulationTime - results will not be valid";
    }
    else
        newAdvanceTime += seconds;
}


namespace {

// ----------------------------------------------------------------------------
// Returns the number of seconds of real time (represented as a float) since
// the clock was first updated.
//
// XXX Need to revisit conditionalization on operating system.



    float 
    clockErrorExit (void)
    {
        /// @todo - throw? how to handle error conditions? Not by crashing an app!
        std::cerr << "Problem reading system clock - results will not be valid";
        return 0.0f;
    }

} // anonymous namespace

float 
Draw::Clock::realTimeSinceFirstClockUpdate (void)
#ifdef _WIN32
{
    // get time from Windows
    LONGLONG counter, frequency;
    bool clockOK = (QueryPerformanceCounter ((LARGE_INTEGER *)&counter)  &&
                    QueryPerformanceFrequency ((LARGE_INTEGER *)&frequency));
    if (!clockOK) return clockErrorExit ();

    // ensure the base counter is recorded once after launch
    if (basePerformanceCounter == 0) basePerformanceCounter = counter;

    // real "wall clock" time since launch
    const LONGLONG counterDifference = counter - basePerformanceCounter;
    return ((float) counterDifference) / ((float)frequency);
}
#else
{
    // get time from Linux (Unix, Mac OS X, ...)
    timeval t;
    if (gettimeofday (&t, 0) != 0) return clockErrorExit ();

    // ensure the base time is recorded once after launch
    if (baseRealTimeSec == 0)
    {
        baseRealTimeSec = t.tv_sec;
        baseRealTimeUsec = t.tv_usec;
    }

    // real "wall clock" time since launch
    return (( t.tv_sec  - baseRealTimeSec) +
            ((t.tv_usec - baseRealTimeUsec) / 1000000.0f));
}
#endif

Draw::Color::Color()
: r_(1.0f), g_(1.0f), b_(1.0f), a_ (1.0f)
{
    
}


Draw::Color::Color( float greyValue )
: r_( greyValue ), g_( greyValue ), b_( greyValue ), a_ (1.0f)
{
    
}


Draw::Color::Color( float rValue, float gValue, float bValue, float aValue )
: r_( rValue ), g_( gValue ), b_( bValue ), a_( aValue )
{
    
}


Draw::Color::Color( Vec3 const& vector )
: r_( vector.x ), g_( vector.y ), b_( vector.z ), a_ (1.0f)
{
    
}



float
Draw::Color::r() const
{
    return r_;
}


float
Draw::Color::g() const
{
    return g_;
}


float
Draw::Color::b() const
{
    return b_;
}


float
Draw::Color::a() const
{
    return a_;
}



void
Draw::Color::setR( float value )
{
    r_ = value;
}


void
Draw::Color::setG( float value )
{
    g_ = value;
}


void
Draw::Color::setB( float value )
{
    b_ = value;
}

void
Draw::Color::setA( float value )
{
    a_ = value;
}

void
Draw::Color::set( float rValue, float gValue, float bValue, float aValue )
{
    r_ = rValue;
    g_ = gValue;
    b_ = bValue;
    a_ = aValue;
}


Draw::Vec3
Draw::Color::convertToVec3() const
{
    return Vec3( r_, g_, b_ );
}


Draw::Color&
Draw::Color::operator+=( Color const& other )
{
    r_ += other.r_;
    g_ += other.g_;
    b_ += other.b_;
    return *this;
}


Draw::Color&
Draw::Color::operator-=( Color const& other )
{
    r_ -= other.r_;
    g_ -= other.g_;
    b_ -= other.b_;
    return *this;
}


Draw::Color&
Draw::Color::operator*=( float factor )
{
    r_ *= factor;
    g_ *= factor;
    b_ *= factor;
    return *this;
}

Draw::Color&
Draw::Color::operator/=( float factor )
{
    assert( 0.0f != factor && "Division by zero." );
    return operator*=( 1.0f / factor );
}


Draw::Color
Draw::grayColor( float value )
{
    return Color( value );
}

Draw::Color
Draw::operator+( Color const& lhs, Color const& rhs )
{
    Color result( lhs );
    return result += rhs;
}


Draw::Color
Draw::operator-( Color const& lhs, Color const& rhs )
{
    Color result( lhs );
    return result -= rhs;
}


Draw::Color
Draw::operator*( Color const& lhs, float rhs )
{
    Color result( lhs );
    return result *= rhs;
}


Draw::Color
Draw::operator*( float lhs, Color const& rhs )
{
    return operator*( rhs, lhs );
}


Draw::Color
Draw::operator/( Color const& lhs, float rhs )
{
    Color result( lhs );
    return result /= rhs;
}

Draw::Color const Draw::gBlack(0.0f, 0.0f, 0.0f);
Draw::Color const Draw::gWhite(1.0f, 1.0f, 1.0f);

Draw::Color const Draw::gRed(1.0f, 0.0f, 0.0f);
Draw::Color const Draw::gGreen(0.0f, 1.0f, 0.0f);
Draw::Color const Draw::gBlue(0.0f, 0.0f, 1.0f);
Draw::Color const Draw::gYellow(1.0f, 1.0f, 0.0f);
Draw::Color const Draw::gCyan(0.0f, 1.0f, 1.0f);
Draw::Color const Draw::gMagenta(1.0f, 0.0f, 1.0f);
Draw::Color const Draw::gOrange(1.0f, 0.5f, 0.0f);

Draw::Color const Draw::gDarkRed(0.5f, 0.0f, 0.0f);
Draw::Color const Draw::gDarkGreen(0.0f, 0.5f, 0.0f);
Draw::Color const Draw::gDarkBlue(0.0f, 0.0f, 0.5f);
Draw::Color const Draw::gDarkYellow(0.5f, 0.5f, 0.0f);
Draw::Color const Draw::gDarkCyan(0.0f, 0.5f, 0.5f);
Draw::Color const Draw::gDarkMagenta(0.5f, 0.0f, 0.5f);
Draw::Color const Draw::gDarkOrange(0.5f, 0.25f, 0.0f);

Draw::Color const Draw::gGray10(0.1f);
Draw::Color const Draw::gGray20(0.2f);
Draw::Color const Draw::gGray30(0.3f);
Draw::Color const Draw::gGray40(0.4f);
Draw::Color const Draw::gGray50(0.5f);
Draw::Color const Draw::gGray60(0.6f);
Draw::Color const Draw::gGray70(0.7f);
Draw::Color const Draw::gGray80(0.8f);
Draw::Color const Draw::gGray90(0.9f);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// names for frequently used vector constants


const Draw::Vec3 Draw::Vec3::zero    (0, 0, 0);
const Draw::Vec3 Draw::Vec3::up      (0, 1, 0);
const Draw::Vec3 Draw::Vec3::forward (0, 0, 1);

// XXX  This should be unified with LocalSpace::rightHanded, but I don't want
// XXX  Vec3 to be based on LocalSpace which is based on Vec3.  Perhaps there
// XXX  should be a tiny chirality.h header to define a const?  That could
// XXX  then be included by both Vec3.h and LocalSpace.h

const Draw::Vec3 Draw::Vec3::side    (-1, 0, 0);

// ----------------------------------------------------------------------------
// Returns a position randomly distributed inside a sphere of unit radius
// centered at the origin.  Orientation will be random and length will range
// between 0 and 1


Draw::Vec3 
Draw::RandomVectorInUnitRadiusSphere (void)
{
    Vec3 v;

    do
    {
        v.set ((frandom01()*2) - 1,
               (frandom01()*2) - 1,
               (frandom01()*2) - 1);
    }
    while (v.length() >= 1);

    return v;
}


// ----------------------------------------------------------------------------
// Returns a position randomly distributed on a disk of unit radius
// on the XZ (Y=0) plane, centered at the origin.  Orientation will be
// random and length will range between 0 and 1


Draw::Vec3 
Draw::randomVectorOnUnitRadiusXZDisk (void)
{
    Vec3 v;

    do
    {
        v.set ((frandom01()*2) - 1,
               0,
               (frandom01()*2) - 1);
    }
    while (v.length() >= 1);

    return v;
}


// ----------------------------------------------------------------------------
// Does a "ceiling" or "floor" operation on the angle by which a given vector
// deviates from a given reference basis vector.  Consider a cone with "basis"
// as its axis and slope of "cosineOfConeAngle".  The first argument controls
// whether the "source" vector is forced to remain inside or outside of this
// cone.  Called by vecLimitMaxDeviationAngle and vecLimitMinDeviationAngle.


Draw::Vec3 
Draw::vecLimitDeviationAngleUtility (const bool insideOrOutside,
                                          const Vec3& source,
                                          const float cosineOfConeAngle,
                                          const Vec3& basis)
{
    // immediately return zero length input vectors
    float sourceLength = source.length();
    if (sourceLength == 0) return source;

    // measure the angular diviation of "source" from "basis"
    const Vec3 direction = source / sourceLength;
    float cosineOfSourceAngle = direction.dot (basis);

    // Simply return "source" if it already meets the angle criteria.
    // (note: we hope this top "if" gets compiled out since the flag
    // is a constant when the function is inlined into its caller)
    if (insideOrOutside)
    {
    // source vector is already inside the cone, just return it
    if (cosineOfSourceAngle >= cosineOfConeAngle) return source;
    }
    else
    {
    // source vector is already outside the cone, just return it
    if (cosineOfSourceAngle <= cosineOfConeAngle) return source;
    }

    // find the portion of "source" that is perpendicular to "basis"
    const Vec3 perp = source.perpendicularComponent (basis);

    // normalize that perpendicular
    const Vec3 unitPerp = perp.normalize ();

    // construct a new vector whose length equals the source vector,
    // and lies on the intersection of a plane (formed the source and
    // basis vectors) and a cone (whose axis is "basis" and whose
    // angle corresponds to cosineOfConeAngle)
    float perpDist = sqrtXXX (1 - (cosineOfConeAngle * cosineOfConeAngle));
    const Vec3 c0 = basis * cosineOfConeAngle;
    const Vec3 c1 = unitPerp * perpDist;
    return (c0 + c1) * sourceLength;
}


// ----------------------------------------------------------------------------
// given a vector, return a vector perpendicular to it.  arbitrarily selects
// one of the infinitely many perpendicular vectors.  a zero vector maps to
// itself, otherwise length is irrelevant (empirically, output length seems to
// remain within 20% of input length).


Draw::Vec3 
Draw::findPerpendicularIn3d (const Vec3& direction)
{
    // to be filled in:
    Vec3 quasiPerp;  // a direction which is "almost perpendicular"
    Vec3 result;     // the computed perpendicular to be returned

    // three mutually perpendicular basis vectors
    const Vec3 i (1, 0, 0);
    const Vec3 j (0, 1, 0);
    const Vec3 k (0, 0, 1);

    // measure the projection of "direction" onto each of the axes
    const float id = i.dot (direction);
    const float jd = j.dot (direction);
    const float kd = k.dot (direction);

    // set quasiPerp to the basis which is least parallel to "direction"
    if ((id <= jd) && (id <= kd))
    {
        quasiPerp = i;               // projection onto i was the smallest
    }
    else
    {
        if ((jd <= id) && (jd <= kd))
            quasiPerp = j;           // projection onto j was the smallest
        else
            quasiPerp = k;           // projection onto k was the smallest
    }

    // return the cross product (direction x quasiPerp)
    // which is guaranteed to be perpendicular to both of them
    result.cross (direction, quasiPerp);
    return result;
}

// ----------------------------------------------------------------------------
// Obstacle
// compute steering for a vehicle to avoid this obstacle, if needed


Draw::Vec3
Draw::Obstacle::steerToAvoid (const AbstractVehicle& vehicle,
                                   const float minTimeToCollision) const
{
    // find nearest intersection with this obstacle along vehicle's path
    PathIntersection pi;
    findIntersectionWithVehiclePath (vehicle, pi);
    
    // return steering for vehicle to avoid intersection, or zero if non found
    return pi.steerToAvoidIfNeeded (vehicle, minTimeToCollision);
}


// ----------------------------------------------------------------------------
// Obstacle
// static method to apply steerToAvoid to nearest obstacle in an ObstacleGroup


Draw::Vec3
Draw::Obstacle::
steerToAvoidObstacles (const AbstractVehicle& vehicle,
                       const float minTimeToCollision,
                       const ObstacleGroup& obstacles)
{
    PathIntersection nearest, next;
    
    // test all obstacles in group for an intersection with the vehicle's
    // future path, select the one whose point of intersection is nearest
    firstPathIntersectionWithObstacleGroup (vehicle, obstacles, nearest, next);
    
    // if nearby intersection found, steer away from it, otherwise no steering
    return nearest.steerToAvoidIfNeeded (vehicle, minTimeToCollision);
}


// ----------------------------------------------------------------------------
// Obstacle
// static method to find first vehicle path intersection in an ObstacleGroup
//
// returns its results in the PathIntersection argument "nearest",
// "next" is used to store internal state.


void
Draw::Obstacle::
firstPathIntersectionWithObstacleGroup (const AbstractVehicle& vehicle,
                                        const ObstacleGroup& obstacles,
                                        PathIntersection& nearest,
                                        PathIntersection& next)
{
    // test all obstacles in group for an intersection with the vehicle's
    // future path, select the one whose point of intersection is nearest
    next.intersect = false;
    nearest.intersect = false;
    for (ObstacleIterator o = obstacles.begin(); o != obstacles.end(); o++)
    {
        // find nearest point (if any) where vehicle path intersects obstacle
        // o, storing the results in PathIntersection object "next"
        (**o).findIntersectionWithVehiclePath (vehicle, next);
        
        // if this is the first intersection found, or it is the nearest found
        // so far, store it in PathIntersection object "nearest"
        const bool firstFound = !nearest.intersect;
        const bool nearestFound = (next.intersect &&
                                   (next.distance < nearest.distance));
        if (firstFound || nearestFound) nearest = next;
    }
}


// ----------------------------------------------------------------------------
// PathIntersection
// determine steering once path intersections have been found


Draw::Vec3
Draw::Obstacle::PathIntersection::
steerToAvoidIfNeeded (const AbstractVehicle& vehicle,
                      const float minTimeToCollision) const
{
    // if nearby intersection found, steer away from it, otherwise no steering
    const float minDistanceToCollision = minTimeToCollision * vehicle.speed();
    if (intersect && (distance < minDistanceToCollision))
    {
        // compute avoidance steering force: take the component of
        // steerHint which is lateral (perpendicular to vehicle's
        // forward direction), set its length to vehicle's maxForce
        Vec3 lateral = steerHint.perpendicularComponent (vehicle.forward ());
        return lateral.normalize () * vehicle.maxForce ();
    }
    else
    {
        return Vec3::zero;
    }
}


// ----------------------------------------------------------------------------
// SphereObstacle
// find first intersection of a vehicle's path with this obstacle


void
Draw::
SphereObstacle::
findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                 PathIntersection& pi) const
{
    // This routine is based on the Paul Bourke's derivation in:
    //   Intersection of a Line and a Sphere (or circle)
    //   http://www.swin.edu.au/astronomy/pbourke/geometry/sphereline/
    // But the computation is done in the vehicle's local space, so
    // the line in question is the Z (Forward) axis of the space which
    // simplifies some of the calculations.
    
    float b, c, d, p, q, s;
    Vec3 lc;
    
    // initialize pathIntersection object to "no intersection found"
    pi.intersect = false;
    
    // find sphere's "local center" (lc) in the vehicle's coordinate space
    lc = vehicle.localizePosition (center);
    pi.vehicleOutside = lc.length () > radius;
    
    // if obstacle is seen from inside, but vehicle is outside, must avoid
    // (noticed once a vehicle got outside it ignored the obstacle 2008-5-20)
    if (pi.vehicleOutside && (seenFrom () == inside))
    {
        pi.intersect = true;
        pi.distance = 0.0f;
        pi.steerHint = (center - vehicle.position()).normalize();
        return;
    }
    
    // compute line-sphere intersection parameters
    const float r = radius + vehicle.radius();
    b = -2 * lc.z;
    c = square (lc.x) + square (lc.y) + square (lc.z) - square (r);
    d = (b * b) - (4 * c);
    
    // when the path does not intersect the sphere
    if (d < 0) return;
    
    // otherwise, the path intersects the sphere in two points with
    // parametric coordinates of "p" and "q".  (If "d" is zero the two
    // points are coincident, the path is tangent)
    s = sqrtXXX (d);
    p = (-b + s) / 2;
    q = (-b - s) / 2;
    
    // both intersections are behind us, so no potential collisions
    if ((p < 0) && (q < 0)) return;
    
    // at least one intersection is in front, so intersects our forward
    // path
    pi.intersect = true;
    pi.obstacle = this;
    pi.distance =
    ((p > 0) && (q > 0)) ?
    // both intersections are in front of us, find nearest one
    ((p < q) ? p : q) :
    // otherwise one is ahead and one is behind: we are INSIDE obstacle
    (seenFrom () == outside ?
     // inside a solid obstacle, so distance to obstacle is zero
     0.0f :
     // hollow obstacle (or "both"), pick point that is in front
     ((p > 0) ? p : q));
    pi.surfacePoint =
    vehicle.position() + (vehicle.forward() * pi.distance);
    pi.surfaceNormal = (pi.surfacePoint-center).normalize();
    switch (seenFrom ())
    {
        case outside:
            pi.steerHint = pi.surfaceNormal;
            break;
        case inside:
            pi.steerHint = -pi.surfaceNormal;
            break;
        case both:
            pi.steerHint = pi.surfaceNormal * (pi.vehicleOutside ? 1.0f : -1.0f);
            break;
    }
}


// ----------------------------------------------------------------------------
// BoxObstacle
// find first intersection of a vehicle's path with this obstacle


void
Draw::
BoxObstacle::
findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                 PathIntersection& pi) const
{
    // abbreviations
    const float w = width; // dimensions
    const float h = height;
    const float d = depth;
    const Vec3 s = side (); // local space
    const Vec3 u = up ();
    const Vec3 f = forward ();
    const Vec3 p = position ();
    const Vec3 hw = s * (0.5f * width); // offsets for face centers
    const Vec3 hh = u * (0.5f * height);
    const Vec3 hd = f * (0.5f * depth);
    const seenFromState sf = seenFrom ();
    
    // the box's six rectangular faces
    RectangleObstacle r1 (w, h,  s,  u,  f, p + hd, sf); // front
    RectangleObstacle r2 (w, h, -s,  u, -f, p - hd, sf); // back
    RectangleObstacle r3 (d, h, -f,  u,  s, p + hw, sf); // side
    RectangleObstacle r4 (d, h,  f,  u, -s, p - hw, sf); // other side
    RectangleObstacle r5 (w, d,  s, -f,  u, p + hh, sf); // top
    RectangleObstacle r6 (w, d, -s, -f, -u, p - hh, sf); // bottom
    
    // group the six RectangleObstacle faces together
    ObstacleGroup faces;
    faces.push_back (&r1);
    faces.push_back (&r2);
    faces.push_back (&r3);
    faces.push_back (&r4);
    faces.push_back (&r5);
    faces.push_back (&r6);
    
    // find first intersection of vehicle path with group of six faces
    PathIntersection next;
    firstPathIntersectionWithObstacleGroup (vehicle, faces, pi, next);
    
    // when intersection found, adjust PathIntersection for the box case
    if (pi.intersect)
    {
        pi.obstacle = this;
        pi.steerHint = ((pi.surfacePoint - position ()).normalize () *
                        (pi.vehicleOutside ? 1.0f : -1.0f));
    }
}


// ----------------------------------------------------------------------------
// PlaneObstacle
// find first intersection of a vehicle's path with this obstacle


void
Draw::
PlaneObstacle::
findIntersectionWithVehiclePath (const AbstractVehicle& vehicle,
                                 PathIntersection& pi) const
{
    // initialize pathIntersection object to "no intersection found"
    pi.intersect = false;
    
    const Vec3 lp =  localizePosition (vehicle.position ());
    const Vec3 ld = localizeDirection (vehicle.forward ());
    
    // no obstacle intersection if path is parallel to XY (side/up) plane
    if (ld.dot (Vec3::forward) == 0.0f) return;
    
    // no obstacle intersection if vehicle is heading away from the XY plane
    if ((lp.z > 0.0f) && (ld.z > 0.0f)) return;
    if ((lp.z < 0.0f) && (ld.z < 0.0f)) return;
    
    // no obstacle intersection if obstacle "not seen" from vehicle's side
    if ((seenFrom () == outside) && (lp.z < 0.0f)) return;
    if ((seenFrom () == inside)  && (lp.z > 0.0f)) return;
    
    // find intersection of path with rectangle's plane (XY plane)
    const float ix = lp.x - (ld.x * lp.z / ld.z);
    const float iy = lp.y - (ld.y * lp.z / ld.z);
    const Vec3 planeIntersection (ix, iy, 0.0f);
    
    // no obstacle intersection if plane intersection is outside 2d shape
    if (!xyPointInsideShape (planeIntersection, vehicle.radius ())) return;
    
    // otherwise, the vehicle path DOES intersect this rectangle
    const Vec3 localXYradial = planeIntersection.normalize ();
    const Vec3 radial = globalizeDirection (localXYradial);
    const float sideSign = (lp.z > 0.0f) ? +1.0f : -1.0f;
    const Vec3 opposingNormal = forward () * sideSign;
    pi.intersect = true;
    pi.obstacle = this;
    pi.distance = (lp - planeIntersection).length ();
    pi.steerHint = opposingNormal + radial; // should have "toward edge" term?
    pi.surfacePoint = globalizePosition (planeIntersection);
    pi.surfaceNormal = opposingNormal;
    pi.vehicleOutside = lp.z > 0.0f;
}


// ----------------------------------------------------------------------------
// RectangleObstacle
// determines if a given point on XY plane is inside obstacle shape


bool
Draw::
RectangleObstacle::
xyPointInsideShape (const Vec3& point, float radius) const
{
    const float w = radius + (width * 0.5f);
    const float h = radius + (height * 0.5f);
    return !((point.x >  w) || (point.x < -w) || (point.y >  h) || (point.y < -h));
}


// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// serial numbers  (XXX should this be part of a "App vehicle mixin"?)


int Draw::SimpleVehicle::serialNumberCounter = 0;


// ----------------------------------------------------------------------------
// constructor


Draw::SimpleVehicle::SimpleVehicle (void)
{
    // set inital state
    reset ();

    // maintain unique serial numbers
    serialNumber = serialNumberCounter++;
}


// ----------------------------------------------------------------------------
// destructor


Draw::SimpleVehicle::~SimpleVehicle (void)
{
}


// ----------------------------------------------------------------------------
// adjust the steering force passed to applySteeringForce.
//
// allows a specific vehicle class to redefine this adjustment.
// default is to disallow backward-facing steering at low speed.
//
// xxx should the default be this ad-hocery, or no adjustment?
// xxx experimental 8-20-02
//
// parameter names commented out to prevent compiler warning from "-W"


Draw::Vec3 
Draw::SimpleVehicle::adjustRawSteeringForce (const Vec3& force,
                                                  const float /* deltaTime */)
{
    const float maxAdjustedSpeed = 0.2f * maxSpeed ();

    if ((speed () > maxAdjustedSpeed) || (force == Vec3::zero))
    {
        return force;
    }
    else
    {
        const float range = speed() / maxAdjustedSpeed;
        // const float cosine = interpolate (pow (range, 6), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 10), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 20), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 100), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 50), 1.0f, -1.0f);
        const float cosine = interpolate (pow (range, 20), 1.0f, -1.0f);
        return limitMaxDeviationAngle (force, cosine, forward());
    }
}


// ----------------------------------------------------------------------------
// xxx experimental 9-6-02
//
// apply a given braking force (for a given dt) to our momentum.
//
// (this is intended as a companion to applySteeringForce, but I'm not sure how
// well integrated it is.  It was motivated by the fact that "braking" (as in
// "capture the flag" endgame) by using "forward * speed * -rate" as a steering
// force was causing problems in adjustRawSteeringForce.  In fact it made it
// get NAN, but even if it had worked it would have defeated the braking.
//
// maybe the guts of applySteeringForce should be split off into a subroutine
// used by both applySteeringForce and applyBrakingForce?


void 
Draw::SimpleVehicle::applyBrakingForce (const float rate, const float deltaTime)
{
    const float rawBraking = speed () * rate;
    const float clipBraking = ((rawBraking < maxForce ()) ?
                               rawBraking :
                               maxForce ());

    setSpeed (speed () - (clipBraking * deltaTime));
}


// ----------------------------------------------------------------------------
// apply a given steering force to our momentum,
// adjusting our orientation to maintain velocity-alignment.


void 
Draw::SimpleVehicle::applySteeringForce (const Vec3& force,
                                              const float elapsedTime)
{

    const Vec3 adjustedForce = adjustRawSteeringForce (force, elapsedTime);

    // enforce limit on magnitude of steering force
    const Vec3 clippedForce = adjustedForce.truncateLength (maxForce ());

    // compute acceleration and velocity
    Vec3 newAcceleration = (clippedForce / mass());
    Vec3 newVelocity = velocity();

    // damp out abrupt changes and oscillations in steering acceleration
    // (rate is proportional to time step, then clipped into useful range)
    if (elapsedTime > 0)
    {
        const float smoothRate = clip (9 * elapsedTime, 0.15f, 0.4f);
        blendIntoAccumulator (smoothRate,
                              newAcceleration,
                              _smoothedAcceleration);
    }

    // Euler integrate (per frame) acceleration into velocity
    newVelocity += _smoothedAcceleration * elapsedTime;

    // enforce speed limit
    newVelocity = newVelocity.truncateLength (maxSpeed ());

    // update Speed
    setSpeed (newVelocity.length());

    // Euler integrate (per frame) velocity into position
    setPosition (position() + (newVelocity * elapsedTime));

    // regenerate local space (by default: align vehicle's forward axis with
    // new velocity, but this behavior may be overridden by derived classes.)
    regenerateLocalSpace (newVelocity, elapsedTime);

    // maintain path curvature information
    measurePathCurvature (elapsedTime);

    // running average of recent positions
    blendIntoAccumulator (elapsedTime * 0.06f, // QQQ
                          position (),
                          _smoothedPosition);
}


// ----------------------------------------------------------------------------
// the default version: keep FORWARD parallel to velocity, change UP as
// little as possible.
//
// parameter names commented out to prevent compiler warning from "-W"


void 
Draw::SimpleVehicle::regenerateLocalSpace (const Vec3& newVelocity,
                                                const float /* elapsedTime */)
{
    // adjust orthonormal basis vectors to be aligned with new velocity
    if (speed() > 0) regenerateOrthonormalBasisUF (newVelocity / speed());
}


// ----------------------------------------------------------------------------
// alternate version: keep FORWARD parallel to velocity, adjust UP according
// to a no-basis-in-reality "banking" behavior, something like what birds and
// airplanes do

// XXX experimental cwr 6-5-03


void 
Draw::SimpleVehicle::regenerateLocalSpaceForBanking (const Vec3& newVelocity,
                                                          const float elapsedTime)
{
    // the length of this global-upward-pointing vector controls the vehicle's
    // tendency to right itself as it is rolled over from turning acceleration
    const Vec3 globalUp (0, 0.2f, 0);

    // acceleration points toward the center of local path curvature, the
    // length determines how much the vehicle will roll while turning
    const Vec3 accelUp = _smoothedAcceleration * 0.05f;

    // combined banking, sum of UP due to turning and global UP
    const Vec3 bankUp = accelUp + globalUp;

    // blend bankUp into vehicle's UP basis vector
    const float smoothRate = elapsedTime * 3;
    Vec3 tempUp = up();
    blendIntoAccumulator (smoothRate, bankUp, tempUp);
    setUp (tempUp.normalize());

//  annotationLine (position(), position() + (globalUp * 4), gWhite);  // XXX
//  annotationLine (position(), position() + (bankUp   * 4), gOrange); // XXX
//  annotationLine (position(), position() + (accelUp  * 4), gRed);    // XXX
//  annotationLine (position(), position() + (up ()    * 1), gYellow); // XXX

    // adjust orthonormal basis vectors to be aligned with new velocity
    if (speed() > 0) regenerateOrthonormalBasisUF (newVelocity / speed());
}


// ----------------------------------------------------------------------------
// measure path curvature (1/turning-radius), maintain smoothed version


void 
Draw::SimpleVehicle::measurePathCurvature (const float elapsedTime)
{
    if (elapsedTime > 0)
    {
        const Vec3 dP = _lastPosition - position ();
        const Vec3 dF = (_lastForward - forward ()) / dP.length ();
        const Vec3 lateral = dF.perpendicularComponent (forward ());
        const float sign = (lateral.dot (side ()) < 0) ? 1.0f : -1.0f;
        _curvature = lateral.length() * sign;
        blendIntoAccumulator (elapsedTime * 4.0f,
                              _curvature,
                              _smoothedCurvature);
        _lastForward = forward ();
        _lastPosition = position ();
    }
}


// ----------------------------------------------------------------------------
// draw lines from vehicle's position showing its velocity and acceleration


void 
Draw::SimpleVehicle::annotationVelocityAcceleration (float maxLengthA, 
                                                          float maxLengthV)
{
    const float desat = 0.4f;
    const float aScale = maxLengthA / maxForce ();
    const float vScale = maxLengthV / maxSpeed ();
    const Vec3& p = position();
    const Color aColor (desat, desat, 1); // bluish
    const Color vColor (    1, desat, 1); // pinkish

    annotationLine (p, p + (velocity ()           * vScale), vColor);
    annotationLine (p, p + (_smoothedAcceleration * aScale), aColor);
}


// ----------------------------------------------------------------------------
// predict position of this vehicle at some time in the future
// (assumes velocity remains constant, hence path is a straight line)
//
// XXX Want to encapsulate this since eventually I want to investigate
// XXX non-linear predictors.  Maybe predictFutureLocalSpace ?
//
// XXX move to a vehicle utility mixin?


Draw::Vec3 
Draw::SimpleVehicle::predictFuturePosition (const float predictionTime) const
{
    return position() + (velocity() * predictionTime);
}


// ---------------------------------------------------------------------------


Draw::Camera::Camera (void)
{
    reset ();
}


// ----------------------------------------------------------------------------
// reset all camera state to default values


void 
Draw::Camera::reset (void)
{
    // reset camera's position and orientation
    resetLocalSpace ();

    // "look at" point, center of view
    target = Vec3::zero;

    // vehicle being tracked
    vehicleToTrack = NULL;

    // aim at predicted position of vehicleToTrack, this far into thefuture
    aimLeadTime = 1;

    // make first update abrupt
    smoothNextMove = false;

    // relative rate at which camera transitions proceed
    smoothMoveSpeed = 1.5f;

    // select camera aiming mode
    mode = cmFixed;

    // "constant distance from vehicle" camera mode parameters
    fixedDistDistance = 1;
    fixedDistVOffset = 0;

    // "look straight down at vehicle" camera mode parameters
    lookdownDistance = 30;

    // "static" camera mode parameters
    fixedPosition.set (75, 75, 75);
    fixedTarget = Vec3::zero;
    fixedUp = Vec3::up;

    // "fixed local offset" camera mode parameters
    fixedLocalOffset.set (5, 5, -5);

    // "offset POV" camera mode parameters
    povOffset.set (0, 1, -3);
}


// ----------------------------------------------------------------------------
// called once per frame to update camera state according to currently
// selected mode and per-mode parameters.  Works in position/target/up
// ("look at") space.
//
// parameter names commented out to prevent compiler warning from "-W"


void 
Draw::Camera::update (const float /*currentTime*/,
                        const float elapsedTime,
                        const bool simulationPaused)
{
    // vehicle being tracked (just a reference with a more concise name)
    const AbstractVehicle& v = *vehicleToTrack;
    const bool noVehicle = vehicleToTrack == NULL;
    
    // new position/target/up, set in switch below, defaults to current
    Vec3 newPosition = position();
    Vec3 newTarget = target;
    Vec3 newUp = up();


    // prediction time to compensate for lag caused by smoothing moves
    const float antiLagTime = simulationPaused ? 0 : 1 / smoothMoveSpeed;

    // aim at a predicted future position of the target vehicle
    const float predictionTime = aimLeadTime + antiLagTime;

    // set new position/target/up according to camera aim mode
    switch (mode)
    {
    case cmFixed:
        newPosition = fixedPosition;
        newTarget = fixedTarget;
        newUp = fixedUp;
        break;

    case cmFixedDistanceOffset:
        if (noVehicle) break;
        newUp = Vec3::up; // xxx maybe this should be v.up ?
        newTarget = v.predictFuturePosition (predictionTime);
        newPosition = constDistHelper (elapsedTime);
        break;

    case cmStraightDown:
        if (noVehicle) break;
        newUp = v.forward();
        newTarget = v.predictFuturePosition (predictionTime);
        newPosition = newTarget;
        newPosition.y += lookdownDistance;
        break;

    case cmFixedLocalOffset:
        if (noVehicle) break;
        newUp = v.up();
        newTarget = v.predictFuturePosition (predictionTime);
        newPosition = v.globalizePosition (fixedLocalOffset);
        break;

    case cmOffsetPOV:
        {
            if (noVehicle) break;
            newUp = v.up();
            const Vec3 futurePosition = v.predictFuturePosition (antiLagTime);
            const Vec3 globalOffset = v.globalizeDirection (povOffset);
            newPosition = futurePosition + globalOffset;
            // XXX hack to improve smoothing between modes (no effect on aim)
            const float L = 10;
            newTarget = newPosition + (v.forward() * L);
            break;
        }
    default:
        break;
    }

    // blend from current position/target/up towards new values
    smoothCameraMove (newPosition, newTarget, newUp, elapsedTime);

    // set camera in draw module
    drawCameraLookAt (position(), target, up());
}


// ----------------------------------------------------------------------------
// Smoothly move camera: blend (at a rate controlled by smoothMoveSpeed)
// from current camera state toward newly determined camera state.
//
// The flag smoothNextMove can be set (with doNotSmoothNextMove()) to
// make next update (say at simulation initialization time).


void 
Draw::Camera::smoothCameraMove (const Vec3& newPosition,
                                     const Vec3& newTarget,
                                     const Vec3& newUp,
                                     const float elapsedTime)
{
    if (smoothNextMove)
    {
        const float smoothRate = elapsedTime * smoothMoveSpeed;

        Vec3 tempPosition = position();
        Vec3 tempUp = up();
        blendIntoAccumulator (smoothRate, newPosition, tempPosition);
        blendIntoAccumulator (smoothRate, newTarget,   target);
        blendIntoAccumulator (smoothRate, newUp,       tempUp);
        setPosition (tempPosition);
        setUp (tempUp);

        // xxx not sure if these are needed, seems like a good idea
        // xxx (also if either up or oldUP are zero, use the other?)
        // xxx (even better: force up to be perp to target-position axis))
        if (up() == Vec3::zero)
            setUp (Vec3::up);
        else
            setUp (up().normalize ());
    }
    else
    {
        smoothNextMove = true;

        setPosition (newPosition);
        target   = newTarget;
        setUp (newUp);
    }
}


// ----------------------------------------------------------------------------
// computes a new camera position which follows "target" at distant of
// "dragTargetDistance"
//
// parameter names commented out to prevent compiler warning from "-W"


Draw::Vec3 
Draw::Camera::constDistHelper (const float /*elapsedTime*/)
{
    // is the "global up"/"vertical" offset constraint enabled?  (it forces
    // the camera's global-up (Y) cordinate to be a above/below the target
    // vehicle by a given offset.)
    const bool constrainUp = (fixedDistVOffset != 0);

    // vector offset from target to current camera position
    const Vec3 adjustedPosition (position().x,
                                 (constrainUp) ? target.y : position().y,
                                 position().z);
    const Vec3 offset = adjustedPosition - target;

    // current distance between them
    const float distance = offset.length();

    // move camera only when geometry is well-defined (avoid degenerate case)
    if (distance == 0)
    {
        return position();
    }
    else
    {
        // unit vector along original offset
        const Vec3 unitOffset = offset / distance;

        // new offset of length XXX
        const float xxxDistance = sqrtXXX (square (fixedDistDistance) -
                                           square (fixedDistVOffset));
        const Vec3 newOffset = unitOffset * xxxDistance;

        // return new camera position: adjust distance to target
        return target + newOffset + Vec3 (0, fixedDistVOffset, 0);
    }
}


// ----------------------------------------------------------------------------
// select next camera mode, used by App


void 
Draw::Camera::selectNextMode (void)
{
    mode = successorMode (mode);
    if (mode >= cmEndMode) mode = successorMode (cmStartMode);
}


// ----------------------------------------------------------------------------
// cycles through the various camera modes


Draw::Camera::cameraMode 
Draw::Camera::successorMode (const cameraMode cm) const
{
    return (cameraMode)(((int)cm) + 1);
}


// ----------------------------------------------------------------------------
// string naming current camera mode, used by App


char const* 
Draw::Camera::modeName (void)
{
    switch (mode)
    {
    case  cmFixed:               return "static";                break;
    case  cmFixedDistanceOffset: return "fixed distance offset"; break;
    case  cmFixedLocalOffset:    return "fixed local offset";    break;
    case  cmOffsetPOV:           return "offset POV";            break;
    case  cmStraightDown:        return "straight down";         break;
    default:                     return "?";
    }
}


// ----------------------------------------------------------------------------
// adjust the offest vector of the current camera mode based on a
// "mouse adjustment vector" from App (xxx experiment 10-17-02)


void 
Draw::Camera::mouseAdjustOffset (const Vec3& adjustment)
{
    // vehicle being tracked (just a reference with a more concise name)
    const AbstractVehicle& v = *vehicleToTrack;

    switch (mode)
    {
    case cmFixed:
        {
            const Vec3 offset = fixedPosition - fixedTarget;
            const Vec3 adjusted = mouseAdjustPolar (adjustment, offset);
            fixedPosition = fixedTarget + adjusted;
            break;
        }
    case cmFixedDistanceOffset:
        {
            // XXX this is the oddball case, adjusting "position" instead
            // XXX of mode parameters, hence no smoothing during adjustment
            // XXX Plus the fixedDistVOffset feature complicates things
            const Vec3 offset = position() - target;
            const Vec3 adjusted = mouseAdjustPolar (adjustment, offset);
            // XXX --------------------------------------------------
//             position = target + adjusted;
//             fixedDistDistance = adjusted.length();
//             fixedDistVOffset = position.y - target.y;
            // XXX --------------------------------------------------
//             const float s = smoothMoveSpeed * (1.0f/40f);
//             const Vec3 newPosition = target + adjusted;
//             position = interpolate (s, position, newPosition);
//             fixedDistDistance = interpolate (s, fixedDistDistance, adjusted.length());
//             fixedDistVOffset = interpolate (s, fixedDistVOffset, position.y - target.y);
            // XXX --------------------------------------------------
//          position = target + adjusted;
            setPosition (target + adjusted);
            fixedDistDistance = adjusted.length();
//          fixedDistVOffset = position.y - target.y;
            fixedDistVOffset = position().y - target.y;
            // XXX --------------------------------------------------
            break;
        }
    case cmStraightDown:
        {
            const Vec3 offset (0, 0, lookdownDistance);
            const Vec3 adjusted = mouseAdjustPolar (adjustment, offset);
            lookdownDistance = adjusted.z;
            break;
        }
    case cmFixedLocalOffset:
        {
            const Vec3 offset = v.globalizeDirection (fixedLocalOffset);
            const Vec3 adjusted = mouseAdjustPolar (adjustment, offset);
            fixedLocalOffset = v.localizeDirection (adjusted);
            break;
        }
    case cmOffsetPOV:
        {
            // XXX this might work better as a translation control, it is
            // XXX non-obvious using a polar adjustment when the view
            // XXX center is not at the camera aim target
            const Vec3 offset = v.globalizeDirection (povOffset);
            const Vec3 adjusted = mouseAdjustOrtho (adjustment, offset);
            povOffset = v.localizeDirection (adjusted);
            break;
        }
    default:
        break;
    }
}


// ----------------------------------------------------------------------------


Draw::Vec3 
Draw::Camera::mouseAdjust2 (const bool polar,
                                 const Vec3& adjustment,
                                 const Vec3& offsetToAdjust)
{
    // value to be returned
    Vec3 result = offsetToAdjust;

    // using the camera's side/up axes (essentially: screen space) move the
    // offset vector sideways according to adjustment.x and vertically
    // according to adjustment.y, constrain the offset vector's length to
    // stay the same, hence the offset's "tip" stays on the surface of a
    // sphere.
    const float oldLength = result.length ();
    const float rate = polar ? oldLength : 1;
    result += xxxls().side() * (adjustment.x * rate);
    result += xxxls().up()   * (adjustment.y * rate);
    if (polar)
    {
        const float newLength = result.length ();
        result *= oldLength / newLength;
    }

    // change the length of the offset vector according to adjustment.z
    if (polar)
        result *= (1 + adjustment.z);
    else
        result += xxxls().forward() * adjustment.z;

    return result;
}


// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// PlugIn registry
//
// XXX replace with STL utilities


int Draw::PlugIn::itemsInRegistry = 0;
const int Draw::PlugIn::totalSizeOfRegistry = 1000;
Draw::PlugIn* Draw::PlugIn::registry [totalSizeOfRegistry];


// ----------------------------------------------------------------------------
// constructor


Draw::PlugIn::PlugIn (void)
{
    // save this new instance in the registry
    addToRegistry ();
}


// ----------------------------------------------------------------------------
// destructor


Draw::PlugIn::~PlugIn() {}


// ----------------------------------------------------------------------------
// returns pointer to the next PlugIn in "selection order"


Draw::PlugIn* 
Draw::PlugIn::next (void)
{
    for (int i = 0; i < itemsInRegistry; i++)
    {
        if (this == registry[i])
        {
            const bool atEnd = (i == (itemsInRegistry - 1));
            return registry [atEnd ? 0 : i + 1];
        }
    }
    return NULL;
}


// ----------------------------------------------------------------------------
// search the class registry for a Plugin with the given name
// returns NULL if none is found


Draw::PlugIn* 
Draw::PlugIn::findByName (const char* string)
{
    if (string)
    {
        for (int i = 0; i < itemsInRegistry; i++)
        {
            PlugIn& pi = *registry[i];
            const char* s = pi.name();
            if (s && (std::strcmp (string, s) == 0)) return &pi;
        }
    }
    return NULL;
}


// ----------------------------------------------------------------------------
// apply a given function to all PlugIns in the registry


void 
Draw::PlugIn::applyToAll (plugInCallBackFunction f)
{
    for (int i = 0; i < itemsInRegistry; i++)
    {
        f (*registry[i]);
    }
}


// ----------------------------------------------------------------------------
// sort PlugIn registry by "selection order"
//
// XXX replace with STL utilities


void 
Draw::PlugIn::sortBySelectionOrder (void)
{
    // I know, I know, just what the world needs:
    // another inline shell sort implementation...

    // starting at each of the first n-1 elements of the array
    for (int i = 0; i < itemsInRegistry-1; i++)
    {
        // scan over subsequent pairs, swapping if larger value is first
        for (int j = i+1; j < itemsInRegistry; j++)
        {
            const float iKey = registry[i]->selectionOrderSortKey ();
            const float jKey = registry[j]->selectionOrderSortKey ();

            if (iKey > jKey)
            {
                PlugIn* temporary = registry[i];
                registry[i] = registry[j];
                registry[j] = temporary;
            }
        }
    }
}


// ----------------------------------------------------------------------------
// returns pointer to default PlugIn (currently, first in registry)


Draw::PlugIn* 
Draw::PlugIn::findDefault (void)
{
    // return NULL if no PlugIns exist
    if (itemsInRegistry == 0) return NULL;

    // otherwise, return the first PlugIn that requests initial selection
    for (int i = 0; i < itemsInRegistry; i++)
    {
        if (registry[i]->requestInitialSelection ()) return registry[i];
    }

    // otherwise, return the "first" PlugIn (in "selection order")
    return registry[0];
}


// ----------------------------------------------------------------------------
// save this instance in the class's registry of instances
// (for use by contractors)


void 
Draw::PlugIn::addToRegistry (void)
{
    // save this instance in the registry
    registry[itemsInRegistry++] = this;
}


// ----------------------------------------------------------------------------
namespace {
    // ------------------------------------------------------------------------
    // emit an OpenGL vertex based on a Vec3
    
    inline void iglVertexVec3 (const Draw::Vec3& v)
    {
        glVertex3f (v.x, v.y, v.z);
    }
    
    // ------------------------------------------------------------------------
    // OpenGL-specific routine for error check, report, and exit
    
    void
    checkForGLError (const char* locationDescription)
    {
        // normally (when no error) just return
        const int lastGlError = glGetError();
        if (lastGlError == GL_NO_ERROR) return;
        
        // otherwise print vaguely descriptive error message, then exit
        std::cerr << std::endl << "App: OpenGL error ";
        switch (lastGlError)
        {
            case GL_INVALID_ENUM:      std::cerr << "GL_INVALID_ENUM";      break;
            case GL_INVALID_VALUE:     std::cerr << "GL_INVALID_VALUE";     break;
            case GL_INVALID_OPERATION: std::cerr << "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:    std::cerr << "GL_STACK_OVERFLOW";    break;
            case GL_STACK_UNDERFLOW:   std::cerr << "GL_STACK_UNDERFLOW";   break;
            case GL_OUT_OF_MEMORY:     std::cerr << "GL_OUT_OF_MEMORY";     break;
#ifndef _WIN32
            case GL_TABLE_TOO_LARGE:   std::cerr << "GL_TABLE_TOO_LARGE";   break;
#endif
        }
        if (locationDescription) std::cerr << " in " << locationDescription;
        std::cerr << std::endl << std::endl << std::flush;
        /// @todo - a library should never bail, this is an application function
        // throw an exception? Call a registered exit hook?
        //Draw::App::exit (1);
    }
    
    // ----------------------------------------------------------------------------
    // draw 3d "graphical annotation" lines, used for debugging
    
    inline void iDrawLine (const Draw::Vec3& startPoint,
                           const Draw::Vec3& endPoint,
                           const Draw::Color& color)
    {
        Draw::warnIfInUpdatePhase ("iDrawLine");
        glColor3f (color.r(), color.g(), color.b());
        glBegin (GL_LINES);
        glVertexVec3 (startPoint);
        glVertexVec3 (endPoint);
        glEnd ();
    }
    
    // ----------------------------------------------------------------------------
    // Draw a single OpenGL triangle given three Vec3 vertices.
    
    inline void iDrawTriangle (const Draw::Vec3& a,
                               const Draw::Vec3& b,
                               const Draw::Vec3& c,
                               const Draw::Color& color)
    {
        Draw::warnIfInUpdatePhase ("iDrawTriangle");
        glColor3f (color.r(), color.g(), color.b());
        glBegin (GL_TRIANGLES);
        {
            Draw::glVertexVec3 (a);
            Draw::glVertexVec3 (b);
            Draw::glVertexVec3 (c);
        }
        glEnd ();
    }
    
    
    // ------------------------------------------------------------------------
    // Draw a single OpenGL quadrangle given four Vec3 vertices, and color.
    
    inline void iDrawQuadrangle (const Draw::Vec3& a,
                                 const Draw::Vec3& b,
                                 const Draw::Vec3& c,
                                 const Draw::Vec3& d,
                                 const Draw::Color& color)
    {
        Draw::warnIfInUpdatePhase ("iDrawQuadrangle");
        glColor3f (color.r(), color.g(), color.b());
        glBegin (GL_QUADS);
        {
            Draw::glVertexVec3 (a);
            Draw::glVertexVec3 (b);
            Draw::glVertexVec3 (c);
            Draw::glVertexVec3 (d);
        }
        glEnd ();
    }
    
    // ------------------------------------------------------------------------
    // Between matched sets of these two calls, assert that all polygons
    // will be drawn "double sided", that is, without back-face culling
    
    inline void beginDoubleSidedDrawing (void)
    {
        glPushAttrib (GL_ENABLE_BIT);
        glDisable (GL_CULL_FACE);
    }
    
    
    inline void endDoubleSidedDrawing (void)
    {
        glPopAttrib ();
    }
    
    inline GLint begin2dDrawing (float w, float h)
    {
        // store OpenGL matrix mode
        GLint originalMatrixMode;
        glGetIntegerv (GL_MATRIX_MODE, &originalMatrixMode);
        
        // clear projection transform
        glMatrixMode (GL_PROJECTION);
        glPushMatrix ();
        glLoadIdentity ();
        
        // set up orthogonal projection onto window's screen space
        glOrtho (0.0f, w, 0.0f, h, -1.0f, 1.0f);
        
        // clear model transform
        glMatrixMode (GL_MODELVIEW);
        glPushMatrix ();
        glLoadIdentity ();
        
        // return original matrix mode for saving (stacking)
        return originalMatrixMode;
    }
    
    
    inline void end2dDrawing (GLint originalMatrixMode)
    {
        // restore previous model/projection transformation state
        glPopMatrix ();
        glMatrixMode (GL_PROJECTION);
        glPopMatrix ();
        
        // restore OpenGL matrix mode
        glMatrixMode (originalMatrixMode);
    }
    
}   // end anonymous namespace

void
Draw::glVertexVec3 (const Vec3& v)
{
    iglVertexVec3 (v);
}




// ----------------------------------------------------------------------------
// warn when draw functions are called during App's update phase


void
Draw::warnIfInUpdatePhase2 (const char* name)
{
    std::ostringstream message;
    message << "use annotation (during simulation update, do not call ";
    message << name;
    message << ")";
    message << std::ends;
    //{{{ [Jerry]
    //std::cerr << message;       // send message to cerr, let host app worry about where to redirect it
    //}}}
}




void
Draw::drawLine (const Vec3& startPoint,
                     const Vec3& endPoint,
                     const Color& color)
{
    iDrawLine (startPoint, endPoint, color);
}


// ----------------------------------------------------------------------------
// draw a line with alpha blending

// see also glAlphaFunc
// glBlendFunc (GL_SRC_ALPHA)
// glEnable (GL_BLEND)


void
Draw::drawLineAlpha (const Vec3& startPoint,
                          const Vec3& endPoint,
                          const Color& color,
                          const float alpha)
{
    warnIfInUpdatePhase ("drawLineAlpha");
    glColor4f (color.r(), color.g(), color.b(), alpha);
    glBegin (GL_LINES);
    Draw::glVertexVec3 (startPoint);
    Draw::glVertexVec3 (endPoint);
    glEnd ();
}





void
Draw::drawTriangle (const Vec3& a,
                         const Vec3& b,
                         const Vec3& c,
                         const Color& color)
{
    iDrawTriangle (a, b, c, color);
}






void
Draw::drawQuadrangle (const Vec3& a,
                           const Vec3& b,
                           const Vec3& c,
                           const Vec3& d,
                           const Color& color)
{
    iDrawQuadrangle (a, b, c, d, color);
}


// ------------------------------------------------------------------------
// draws a "wide line segment": a rectangle of the given width and color
// whose mid-line connects two given endpoints


void
Draw::drawXZWideLine (const Vec3& startPoint,
                           const Vec3& endPoint,
                           const Color& color,
                           float width)
{
    warnIfInUpdatePhase ("drawXZWideLine");
    
    const Vec3 offset = endPoint - startPoint;
    const Vec3 along = offset.normalize();
    const Vec3 perp = gGlobalSpace.localRotateForwardToSide (along);
    const Vec3 radius = perp * width / 2;
    
    const Vec3 a = startPoint + radius;
    const Vec3 b = endPoint + radius;
    const Vec3 c = endPoint - radius;
    const Vec3 d = startPoint - radius;
    
    iDrawQuadrangle (a, b, c, d, color);
}






// ------------------------------------------------------------------------
// General purpose circle/disk drawing routine.  Draws circles or disks (as
// specified by "filled" argument) and handles both special case 2d circles
// on the XZ plane or arbitrary circles in 3d space (as specified by "in3d"
// argument)


void
Draw::drawCircleOrDisk (const float radius,
                             const Vec3& axis,
                             const Vec3& center,
                             const Color& color,
                             const int segments,
                             const bool filled,
                             const bool in3d)
{
    LocalSpace ls;
    if (in3d)
    {
        // define a local space with "axis" as the Y/up direction
        // (XXX should this be a method on  LocalSpace?)
        const Vec3 unitAxis = axis.normalize ();
        const Vec3 unitPerp = findPerpendicularIn3d (axis).normalize ();
        ls.setUp (unitAxis);
        ls.setForward (unitPerp);
        ls.setPosition (center);
        ls.setUnitSideFromForwardAndUp ();
    }
    
    // make disks visible (not culled) from both sides
    if (filled) beginDoubleSidedDrawing ();
    
    // point to be rotated about the (local) Y axis, angular step size
    Vec3 pointOnCircle (radius, 0, 0);
    const float step = (2 * OPENSTEER_M_PI) / segments;
    
    // set drawing color
    glColor3f (color.r(), color.g(), color.b());
    
    // begin drawing a triangle fan (for disk) or line loop (for circle)
    glBegin (filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    
    // for the filled case, first emit the center point
    if (filled) iglVertexVec3 (in3d ? ls.position() : center);
    
    // rotate p around the circle in "segments" steps
    float sin=0, cos=0;
    const int vertexCount = filled ? segments+1 : segments;
    for (int i = 0; i < vertexCount; i++)
    {
        // emit next point on circle, either in 3d (globalized out
        // of the local space), or in 2d (offset from the center)
        iglVertexVec3 (in3d ?
                       ls.globalizePosition (pointOnCircle) :
                       (Vec3) (pointOnCircle + center));
        
        // rotate point one more step around circle
        pointOnCircle = pointOnCircle.rotateAboutGlobalY (step, sin, cos);
    }
    
    // close drawing operation
    glEnd ();
    if (filled) endDoubleSidedDrawing ();
}


// ------------------------------------------------------------------------


void
Draw::draw3dCircleOrDisk (const float radius,
                               const Vec3& center,
                               const Vec3& axis,
                               const Color& color,
                               const int segments,
                               const bool filled)
{
    // draw a circle-or-disk in the given local space
    drawCircleOrDisk (radius, axis, center, color, segments, filled, true);
}


// ------------------------------------------------------------------------
// drawing utility used by both drawXZCircle and drawXZDisk


void
Draw::drawXZCircleOrDisk (const float radius,
                               const Vec3& center,
                               const Color& color,
                               const int segments,
                               const bool filled)
{
    // draw a circle-or-disk on the XZ plane
    drawCircleOrDisk (radius, Vec3::zero, center, color, segments, filled, false);
}


// ------------------------------------------------------------------------
// draw a circular arc on the XZ plane, from a start point, around a center,
// for a given arc length, in a given number of segments and color.  The
// sign of arcLength determines the direction in which the arc is drawn.
//
// XXX maybe this should alow allow "in3d" cricles about an given axis
// XXX maybe there should be a "filled" version of this
// XXX maybe this should be merged in with drawCircleOrDisk


void
Draw::drawXZArc (const Vec3& start,
                      const Vec3& center,
                      const float arcLength,
                      const int segments,
                      const Color& color)
{
    warnIfInUpdatePhase ("drawXZArc");
    
    // "spoke" is initially the vector from center to start,
    // it is then rotated around its tail
    Vec3 spoke = start - center;
    
    // determine the angular step per segment
    const float radius = spoke.length ();
    const float twoPi = 2 * OPENSTEER_M_PI;
    const float circumference = twoPi * radius;
    const float arcAngle = twoPi * arcLength / circumference;
    const float step = arcAngle / segments;
    
    // set drawing color
    glColor3f (color.r(), color.g(), color.b());
    
    // begin drawing a series of connected line segments
    glBegin (GL_LINE_STRIP);
    
    // draw each segment along arc
    float sin=0, cos=0;
    for (int i = 0; i < segments; i++)
    {
        // emit next point on arc
        iglVertexVec3 (spoke + center);
        
        // rotate point to next step around circle
        spoke = spoke.rotateAboutGlobalY (step, sin, cos);
    }
    
    // close drawing operation
    glEnd ();
}


// ------------------------------------------------------------------------
// a simple 2d vehicle on the XZ plane


void
Draw::drawBasic2dCircularVehicle (const AbstractVehicle& vehicle,
                                       const Color& color)
{
    // "aspect ratio" of body (as seen from above)
    const float x = 0.5f;
    const float y = sqrtXXX (1 - (x * x));
    
    // radius and position of vehicle
    const float r = vehicle.radius();
    const Vec3& p = vehicle.position();
    
    // shape of triangular body
    const Vec3 u = r * 0.05f * Vec3 (0, 1, 0); // slightly up
    const Vec3 f = r * vehicle.forward();
    const Vec3 s = r * vehicle.side() * x;
    const Vec3 b = r * vehicle.forward() * -y;
    
    // draw double-sided triangle (that is: no (back) face culling)
    beginDoubleSidedDrawing ();
    iDrawTriangle (p + f + u,
                   p + b - s + u,
                   p + b + s + u,
                   color);
    endDoubleSidedDrawing ();
    
    // draw the circular collision boundary
    drawXZCircle (r, p + u, gWhite, 20);
}


// ------------------------------------------------------------------------
// a simple 3d vehicle


void
Draw::drawBasic3dSphericalVehicle (const AbstractVehicle& vehicle,
                                        const Color& color)
{
    // "aspect ratio" of body (as seen from above)
    const float x = 0.5f;
    const float y = sqrtXXX (1 - (x * x));
    
    // radius and position of vehicle
    const float r = vehicle.radius();
    const Vec3& p = vehicle.position();
    
    // body shape parameters
    const Vec3 f = r * vehicle.forward();
    const Vec3 s = r * vehicle.side() * x;
    const Vec3 u = r * vehicle.up() * x * 0.5f;
    const Vec3 b = r * vehicle.forward() * -y;
    
    // vertex positions
    const Vec3 nose   = p + f;
    const Vec3 side1  = p + b - s;
    const Vec3 side2  = p + b + s;
    const Vec3 top    = p + b + u;
    const Vec3 bottom = p + b - u;
    
    // colors
    const float j = +0.05f;
    const float k = -0.05f;
    const Color color1 = color + Color(j, j, k);
    const Color color2 = color + Color(j, k, j);
    const Color color3 = color + Color(k, j, j);
    const Color color4 = color + Color(k, j, k);
    const Color color5 = color + Color(k, k, j);
    
    // draw body
    iDrawTriangle (nose,  side1,  top,    color1);  // top, side 1
    iDrawTriangle (nose,  top,    side2,  color2);  // top, side 2
    iDrawTriangle (nose,  bottom, side1,  color3);  // bottom, side 1
    iDrawTriangle (nose,  side2,  bottom, color4);  // bottom, side 2
    iDrawTriangle (side1, side2,  top,    color5);  // top back
    iDrawTriangle (side2, side1,  bottom, color5);  // bottom back
}


// drawBasic3dSphericalVehicle with a supplied draw routine
// provided so non-OpenGL based apps can draw a boid

void
Draw::drawBasic3dSphericalVehicle (drawTriangleRoutine draw, const AbstractVehicle& vehicle,
                                        const Color& color)
{
    // "aspect ratio" of body (as seen from above)
    const float x = 0.5f;
    const float y = sqrtXXX (1 - (x * x));
    
    // radius and position of vehicle
    const float r = vehicle.radius();
    const Vec3& p = vehicle.position();
    
    // body shape parameters
    const Vec3 f = r * vehicle.forward();
    const Vec3 s = r * vehicle.side() * x;
    const Vec3 u = r * vehicle.up() * x * 0.5f;
    const Vec3 b = r * vehicle.forward() * -y;
    
    // vertex positions
    const Vec3 nose   = p + f;
    const Vec3 side1  = p + b - s;
    const Vec3 side2  = p + b + s;
    const Vec3 top    = p + b + u;
    const Vec3 bottom = p + b - u;
    
    // colors
    const float j = +0.05f;
    const float k = -0.05f;
    const Color color1 = color + Color (j, j, k);
    const Color color2 = color + Color (j, k, j);
    const Color color3 = color + Color (k, j, j);
    const Color color4 = color + Color (k, j, k);
    const Color color5 = color + Color (k, k, j);
    
    // draw body
    draw (nose,  side1,  top,    color1);  // top, side 1
    draw (nose,  top,    side2,  color2);  // top, side 2
    draw (nose,  bottom, side1,  color3);  // bottom, side 1
    draw (nose,  side2,  bottom, color4);  // bottom, side 2
    draw (side1, side2,  top,    color5);  // top back
    draw (side2, side1,  bottom, color5);  // bottom back
}





// ------------------------------------------------------------------------
// draw a (filled-in, polygon-based) square checkerboard grid on the XZ
// (horizontal) plane.
//
// ("size" is the length of a side of the overall grid, "subsquares" is the
// number of subsquares along each edge (for example a standard checkboard
// has eight), "center" is the 3d position of the center of the grid,
// color1 and color2 are used for alternating subsquares.)


void
Draw::drawXZCheckerboardGrid (const float size,
                                   const int subsquares,
                                   const Vec3& center,
                                   const Color& color1,
                                   const Color& color2)
{
    const float half = size/2;
    const float spacing = size / subsquares;
    
    beginDoubleSidedDrawing ();
    {
        bool flag1 = false;
        float p = -half;
        Vec3 corner;
        for (int i = 0; i < subsquares; i++)
        {
            bool flag2 = flag1;
            float q = -half;
            for (int j = 0; j < subsquares; j++)
            {
                corner.set (p, 0, q);
                corner += center;
                iDrawQuadrangle (corner,
                                 corner + Vec3 (spacing, 0,       0),
                                 corner + Vec3 (spacing, 0, spacing),
                                 corner + Vec3 (0,       0, spacing),
                                 flag2 ? color1 : color2);
                flag2 = !flag2;
                q += spacing;
            }
            flag1 = !flag1;
            p += spacing;
        }
    }
    endDoubleSidedDrawing ();
}


// ------------------------------------------------------------------------
// draw a square grid of lines on the XZ (horizontal) plane.
//
// ("size" is the length of a side of the overall grid, "subsquares" is the
// number of subsquares along each edge (for example a standard checkboard
// has eight), "center" is the 3d position of the center of the grid, lines
// are drawn in the specified "color".)


void
Draw::drawXZLineGrid (const float size,
                           const int subsquares,
                           const Vec3& center,
                           const Color& color)
{
    warnIfInUpdatePhase ("drawXZLineGrid");
    
    const float half = size/2;
    const float spacing = size / subsquares;
    
    // set grid drawing color
    glColor3f (color.r(), color.g(), color.b());
    
    // draw a square XZ grid with the given size and line count
    glBegin (GL_LINES);
    float q = -half;
    for (int i = 0; i < (subsquares + 1); i++)
    {
        const Vec3 x1 (q, 0, +half); // along X parallel to Z
        const Vec3 x2 (q, 0, -half);
        const Vec3 z1 (+half, 0, q); // along Z parallel to X
        const Vec3 z2 (-half, 0, q);
        
        iglVertexVec3 (x1 + center);
        iglVertexVec3 (x2 + center);
        iglVertexVec3 (z1 + center);
        iglVertexVec3 (z2 + center);
        
        q += spacing;
    }
    glEnd ();
}


// ------------------------------------------------------------------------
// draw the three axes of a LocalSpace: three lines parallel to the
// basis vectors of the space, centered at its origin, of lengths
// given by the coordinates of "size".


void
Draw::drawAxes  (const AbstractLocalSpace& ls,
                      const Vec3& size,
                      const Color& color)
{
    const Vec3 x (size.x / 2, 0, 0);
    const Vec3 y (0, size.y / 2, 0);
    const Vec3 z (0, 0, size.z / 2);
    
    iDrawLine (ls.globalizePosition (x), ls.globalizePosition (x * -1), color);
    iDrawLine (ls.globalizePosition (y), ls.globalizePosition (y * -1), color);
    iDrawLine (ls.globalizePosition (z), ls.globalizePosition (z * -1), color);
}


// ------------------------------------------------------------------------
// draw the edges of a box with a given position, orientation, size
// and color.  The box edges are aligned with the axes of the given
// LocalSpace, and it is centered at the origin of that LocalSpace.
// "size" is the main diagonal of the box.
//
// use gGlobalSpace to draw a box aligned with global space


void
Draw::drawBoxOutline  (const AbstractLocalSpace& localSpace,
                            const Vec3& size,
                            const Color& color)
{
    const Vec3 s = size / 2.0f;  // half of main diagonal
    
    const Vec3 a (+s.x, +s.y, +s.z);
    const Vec3 b (+s.x, -s.y, +s.z);
    const Vec3 c (-s.x, -s.y, +s.z);
    const Vec3 d (-s.x, +s.y, +s.z);
    
    const Vec3 e (+s.x, +s.y, -s.z);
    const Vec3 f (+s.x, -s.y, -s.z);
    const Vec3 g (-s.x, -s.y, -s.z);
    const Vec3 h (-s.x, +s.y, -s.z);
    
    const Vec3 A = localSpace.globalizePosition (a);
    const Vec3 B = localSpace.globalizePosition (b);
    const Vec3 C = localSpace.globalizePosition (c);
    const Vec3 D = localSpace.globalizePosition (d);
    
    const Vec3 E = localSpace.globalizePosition (e);
    const Vec3 F = localSpace.globalizePosition (f);
    const Vec3 G = localSpace.globalizePosition (g);
    const Vec3 H = localSpace.globalizePosition (h);
    
    iDrawLine (A, B, color);
    iDrawLine (B, C, color);
    iDrawLine (C, D, color);
    iDrawLine (D, A, color);
    
    iDrawLine (A, E, color);
    iDrawLine (B, F, color);
    iDrawLine (C, G, color);
    iDrawLine (D, H, color);
    
    iDrawLine (E, F, color);
    iDrawLine (F, G, color);
    iDrawLine (G, H, color);
    iDrawLine (H, E, color);
}


namespace {
    
    // ------------------------------------------------------------------------
    // this comes up often enough to warrant its own warning function
    
    inline void drawCameraLookAtCheck (const Draw::Vec3& cameraPosition,
                                       const Draw::Vec3& pointToLookAt,
                                       const Draw::Vec3& up)
    {
        const Draw::Vec3 view = pointToLookAt - cameraPosition;
        const Draw::Vec3 perp = view.perpendicularComponent (up);
        if (perp == Draw::Vec3::zero)
            std::cerr << "Draw - LookAt: degenerate camera";
    }
    
} // anonymous namespace

// ------------------------------------------------------------------------
// Define scene's camera (viewing transformation) in terms of the camera's
// position, the point to look at (an "aim point" in the scene which will
// end up at the center of the camera's view), and an "up" vector defining
// the camera's "roll" around the "view axis" between cameraPosition and
// pointToLookAt (the image of the up vector will be vertical in the
// camera's view).


void
Draw::drawCameraLookAt (const Vec3& cameraPosition,
                             const Vec3& pointToLookAt,
                             const Vec3& up)
{
    // check for valid "look at" parameters
    drawCameraLookAtCheck (cameraPosition, pointToLookAt, up);
    
    // use LookAt from OpenGL Utilities
    glLoadIdentity ();
    gluLookAt (cameraPosition.x, cameraPosition.y, cameraPosition.z,
               pointToLookAt.x,  pointToLookAt.y,  pointToLookAt.z,
               up.x,             up.y,             up.z);
}



void
Draw::draw2dLine (const Vec3& startPoint,
                       const Vec3& endPoint,
                       const Color& color,
                       float w, float h)
{
    const GLint originalMatrixMode = begin2dDrawing (w, h);
    
    iDrawLine (startPoint, endPoint, color);
    
    end2dDrawing (originalMatrixMode);
}

// ------------------------------------------------------------------------
// draw a reticle at the center of the window.  Currently it is small
// crosshair with a gap at the center, drawn in white with black borders


void
Draw::drawReticle (float w, float h)
{
    const int a = 10;
    const int b = 30;
    
    draw2dLine (Vec3 (w+a, h,   0), Vec3 (w+b, h,   0), gWhite, w, h);
    draw2dLine (Vec3 (w,   h+a, 0), Vec3 (w,   h+b, 0), gWhite, w, h);
    draw2dLine (Vec3 (w-a, h,   0), Vec3 (w-b, h,   0), gWhite, w, h);
    draw2dLine (Vec3 (w,   h-a, 0), Vec3 (w,   h-b, 0), gWhite, w, h);
    
    glLineWidth (3);
    draw2dLine (Vec3 (w+a, h,   0), Vec3 (w+b, h,   0), gBlack, w, h);
    draw2dLine (Vec3 (w,   h+a, 0), Vec3 (w,   h+b, 0), gBlack, w, h);
    draw2dLine (Vec3 (w-a, h,   0), Vec3 (w-b, h,   0), gBlack, w, h);
    draw2dLine (Vec3 (w,   h-a, 0), Vec3 (w,   h-b, 0), gBlack, w, h);
    glLineWidth (1);
}


// ------------------------------------------------------------------------


// code (from main.cpp) used to draw "forward ruler" on vehicle

//     // xxx --------------------------------------------------
//     {
//         const Vec3 p = gSelectedVehicle->position;
//         const Vec3 f = gSelectedVehicle->forward;
//         const Vec3 s = gSelectedVehicle->side * 0.25f;
//         for (float i = 0; i <= 5; i++)
//         {
//             drawLine (p + (f * +i) + s, p + (f * +i) - s, gGray60);
//             drawLine (p + (f * -i) + s, p + (f * -i) - s, gGray60);
//         }
//     }
//     // xxx --------------------------------------------------


// ------------------------------------------------------------------------
// check for errors during redraw, report any and then exit


void
Draw::checkForDrawError (const char * locationDescription)
{
    checkForGLError (locationDescription);
}


// ----------------------------------------------------------------------------
// return a normalized direction vector pointing from the camera towards a
// given point on the screen: the ray that would be traced for that pixel


Draw::Vec3
Draw::directionFromCameraToScreenPosition (int x, int y, int h)
{
    // Get window height, viewport, modelview and projection matrices
    GLint vp[4];
    GLdouble mMat[16], pMat[16];
    glGetIntegerv (GL_VIEWPORT, vp);
    glGetDoublev (GL_MODELVIEW_MATRIX, mMat);
    glGetDoublev (GL_PROJECTION_MATRIX, pMat);
    GLdouble un0x, un0y, un0z, un1x, un1y, un1z;
    
    // Unproject mouse position at near and far clipping planes
    gluUnProject (x, h-y, 0, mMat, pMat, vp, &un0x, &un0y, &un0z);
    gluUnProject (x, h-y, 1, mMat, pMat, vp, &un1x, &un1y, &un1z);
    
    // "direction" is the normalized difference between these far and near
    // unprojected points.  Its parallel to the "eye-mouse" selection line.
    const Vec3 diffNearFar (un1x-un0x, un1y-un0y, un1z-un0z);
    const Vec3 direction = diffNearFar.normalize ();
    return direction;
}


namespace {
    
    // ----------------------------------------------------------------------------
    // deferred draw line
    //
    // For use during simulation phase.
    // Stores description of lines to be drawn later.
    
    
    class DeferredLine
    {
    public:
        
        static void addToBuffer (const Draw::Vec3& s,
                                 const Draw::Vec3& e,
                                 const Draw::Color& c)
        {
            DeferredLine dl;
            dl.startPoint = s;
            dl.endPoint = e;
            dl.color = c;
            
            lines.push_back (dl);
        }
        
        static void drawAll (void)
        {
            // draw all deferred lines
            for (DeferredLines::iterator i = lines.begin();
                 i < lines.end();
                 i++)
            {
                DeferredLine& dl = *i;
                iDrawLine (dl.startPoint, dl.endPoint, dl.color);
            }
            
            // clear list of deferred lines
            lines.clear ();
        }
        
        typedef std::vector<DeferredLine> DeferredLines;
        
    private:
        
        Draw::Vec3 startPoint;
        Draw::Vec3 endPoint;
        Draw::Color color;
        
        static DeferredLines lines;
    };
    
    
    DeferredLine::DeferredLines DeferredLine::lines;
    
    
} // anonymous namespace


void
Draw::deferredDrawLine (const Vec3& startPoint,
                             const Vec3& endPoint,
                             const Color& color)
{
    DeferredLine::addToBuffer (startPoint, endPoint, color);
}


void
Draw::drawAllDeferredLines (void)
{
    DeferredLine::drawAll ();
}


namespace {
    
    // ----------------------------------------------------------------------------
    // deferred draw circle
    // XXX for now, just a modified copy of DeferredLine
    //
    // For use during simulation phase.
    // Stores description of circles to be drawn later.
    
    
    class DeferredCircle
    {
    public:
        
        static void addToBuffer (const float radius,
                                 const Draw::Vec3& axis,
                                 const Draw::Vec3& center,
                                 const Draw::Color& color,
                                 const int segments,
                                 const bool filled,
                                 const bool in3d)
        {
            DeferredCircle dc;
            dc.radius   = radius;
            dc.axis     = axis;
            dc.center   = center;
            dc.color    = color;
            dc.segments = segments;
            dc.filled   = filled;
            dc.in3d     = in3d;
            circles.push_back (dc);
        }
        
        static void drawAll (void)
        {
            // draw all deferred circles
            for (DeferredCircles::iterator i = circles.begin();
                 i < circles.end();
                 i++)
            {
                DeferredCircle& dc = *i;
                drawCircleOrDisk (dc.radius, dc.axis, dc.center, dc.color,
                                  dc.segments, dc.filled, dc.in3d);
            }
            
            // clear list of deferred circles
            circles.clear ();
        }
        
        typedef std::vector<DeferredCircle> DeferredCircles;
        
    private:
        
        float radius;
        Draw::Vec3 axis;
        Draw::Vec3 center;
        Draw::Color color;
        int segments;
        bool filled;
        bool in3d;
        
        static DeferredCircles circles;
    };
    
    
    DeferredCircle::DeferredCircles DeferredCircle::circles;
    
    
} // anonymous namesopace


void
Draw::deferredDrawCircleOrDisk (const float radius,
                                     const Vec3& axis,
                                     const Vec3& center,
                                     const Color& color,
                                     const int segments,
                                     const bool filled,
                                     const bool in3d)
{
    DeferredCircle::addToBuffer (radius, axis, center, color,
                                 segments, filled, in3d);
}


void
Draw::drawAllDeferredCirclesOrDisks (void)
{
    DeferredCircle::drawAll ();
}


// ------------------------------------------------------------------------
// Functions for drawing text (in GLUT's 9x15 bitmap font) in a given
// color, starting at a location on the screen which can be specified
// in screen space (draw2dTextAt2dLocation) or as the screen space
// projection of a location in 3d model space (draw2dTextAt3dLocation)
//
// based on code originally from:
//   Introduction to OpenGL - L23a - Displaying Text
//   http://www.york.ac.uk/services/cserv/sw/graphics/OPENGL/L23a.html

// xxx  Note: I *think* "const char* const s" means that both the pointer s
// xxx  AND the char string it points to are declared read only.  I should
// xxx  check that this is really the case.  I added it based on something
// xxx  from Telespace (Pedestrian constructor) xxx

// xxx  and for THAT matter, why not just use reference ("&") args instead?





// ----------------------------------------------------------------------------
// draw string s right-justified in the upper righthand corner


//     // XXX display the total number of AbstractVehicles created
//     {
//         std::ostringstream s;
//         s << "vehicles: " << xxx::SerialNumberCounter << std::ends;

//         // draw string s right-justified in the upper righthand corner
//         const int h = glutGet (GLUT_WINDOW_HEIGHT);
//         const int w = glutGet (GLUT_WINDOW_WIDTH);
//         const int fontWidth = 9; // for GLUT_BITMAP_9_BY_15
//         const int fontHeight = 15; // for GLUT_BITMAP_9_BY_15
//         const int x = w - (fontWidth * s.pcount());
//         const int y = h - (fontHeight + 5);
//         const Vec3 screenLocation (x, y, 0);
//         draw2dTextAt2dLocation (s, screenLocation, gWhite);
//     }



// // void draw2dTextAt3dLocation (const char* s,
// void draw2dTextAt3dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     // set text color and raster position
//     glColor3f (color.r(), color.g(), color.b());
//     glRasterPos3f (location.x, location.y, location.z);

//     // loop over each character in string (until null terminator)
//     int lines = 0;
//     for (const char* p = s; *p; p++)
//     {
//         if (*p == '\n')
//         {
//             // handle "new line" character, reset raster position
//             lines++;
//             const int fontHeight = 15; // for GLUT_BITMAP_9_BY_15
//             const int vOffset = lines * (fontHeight + 1);
//             glRasterPos3f (location.x, location.y-vOffset, location.z);

//         }
//         else
//         {
//             // otherwise draw character bitmap
//             glutBitmapCharacter (GLUT_BITMAP_9_BY_15, *p);
//         }
//     }
// }


// // void draw2dTextAt2dLocation (char* s,
// void draw2dTextAt2dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     // store OpenGL matrix mode
//     int savedMatrixMode;
//     glGetIntegerv (GL_MATRIX_MODE, &savedMatrixMode);

//     // clear projection transform
//     glMatrixMode (GL_PROJECTION);
//     glPushMatrix ();
//     glLoadIdentity ();

//     // set up orthogonal projection onto window's screen space
//     const float w = glutGet (GLUT_WINDOW_WIDTH);
//     const float h = glutGet (GLUT_WINDOW_HEIGHT);
//     glOrtho (0.0f, w, 0.0f, h, -1.0f, 1.0f);

//     // clear model transform
//     glMatrixMode (GL_MODELVIEW);
//     glPushMatrix ();
//     glLoadIdentity ();

//     // draw text at specified location (which is now interpreted as
//     // relative to screen space) and color
//     draw2dTextAt3dLocation (s, location, color);

//     // restore previous model/projection transformation state
//     glPopMatrix ();
//     glMatrixMode (GL_PROJECTION);
//     glPopMatrix ();

//     // restore OpenGL matrix mode
//     glMatrixMode (savedMatrixMode);
// }




// // for now these cannot be nested (would need to have a stack of saved
// // xxx  matrix modes instead of just a global).



// int gxxxsavedMatrixMode;


// inline void begin2dDrawing (void)
// {
//     // store OpenGL matrix mode
// //     int savedMatrixMode;
//     glGetIntegerv (GL_MATRIX_MODE, &gxxxsavedMatrixMode);

//     // clear projection transform
//     glMatrixMode (GL_PROJECTION);
//     glPushMatrix ();
//     glLoadIdentity ();

//     // set up orthogonal projection onto window's screen space
//     const float w = glutGet (GLUT_WINDOW_WIDTH);
//     const float h = glutGet (GLUT_WINDOW_HEIGHT);
//     glOrtho (0.0f, w, 0.0f, h, -1.0f, 1.0f);

//     // clear model transform
//     glMatrixMode (GL_MODELVIEW);
//     glPushMatrix ();
//     glLoadIdentity ();
// }


// inline void end2dDrawing (void)
// {
//     // restore previous model/projection transformation state
//     glPopMatrix ();
//     glMatrixMode (GL_PROJECTION);
//     glPopMatrix ();

//     // restore OpenGL matrix mode
//     glMatrixMode (gxxxsavedMatrixMode);
// }



// void draw2dTextAt3dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     // set text color and raster position
//     glColor3f (color.r(), color.g(), color.b());
//     glRasterPos3f (location.x, location.y, location.z);

//     // loop over each character in string (until null terminator)
//     int lines = 0;
//     for (const char* p = s; *p; p++)
//     {
//         if (*p == '\n')

//             // handle "new line" character, reset raster position
//             lines++;
//             const int fontHeight = 15; // for GLUT_BITMAP_9_BY_15
//             const int vOffset = lines * (fontHeight + 1);
//             glRasterPos3f (location.x, location.y-vOffset, location.z);

//         }
//         else
//         {
//             // otherwise draw character bitmap
//             glutBitmapCharacter (GLUT_BITMAP_9_BY_15, *p);
//         }
//     }
// }


// void draw2dTextAt2dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
// //     // store OpenGL matrix mode
// //     int savedMatrixMode;
// //     glGetIntegerv (GL_MATRIX_MODE, &savedMatrixMode);

// //     // clear projection transform
// //     glMatrixMode (GL_PROJECTION);
// //     glPushMatrix ();
// //     glLoadIdentity ();

// //     // set up orthogonal projection onto window's screen space
// //     const float w = glutGet (GLUT_WINDOW_WIDTH);
// //     const float h = glutGet (GLUT_WINDOW_HEIGHT);
// //     glOrtho (0.0f, w, 0.0f, h, -1.0f, 1.0f);

// //     // clear model transform
// //     glMatrixMode (GL_MODELVIEW);
// //     glPushMatrix ();
// //     glLoadIdentity ();

//     begin2dDrawing ();

//     // draw text at specified location (which is now interpreted as
//     // relative to screen space) and color
//     draw2dTextAt3dLocation (s, location, color);

// //     // restore previous model/projection transformation state
// //     glPopMatrix ();
// //     glMatrixMode (GL_PROJECTION);
// //     glPopMatrix ();

// //     // restore OpenGL matrix mode
// //     glMatrixMode (savedMatrixMode);

//     end2dDrawing ();

// }


// // for now these cannot be nested (would need to have a stack of saved
// // xxx  matrix modes instead of just a global).



// int gxxxsavedMatrixMode;


// inline void begin2dDrawing (void)
// {
//     // store OpenGL matrix mode
// //     int savedMatrixMode;
//     glGetIntegerv (GL_MATRIX_MODE, &gxxxsavedMatrixMode);

//     // clear projection transform
//     glMatrixMode (GL_PROJECTION);
//     glPushMatrix ();
//     glLoadIdentity ();

//     // set up orthogonal projection onto window's screen space
//     const float w = glutGet (GLUT_WINDOW_WIDTH);
//     const float h = glutGet (GLUT_WINDOW_HEIGHT);
//     glOrtho (0.0f, w, 0.0f, h, -1.0f, 1.0f);

//     // clear model transform
//     glMatrixMode (GL_MODELVIEW);
//     glPushMatrix ();
//     glLoadIdentity ();
// }


// inline void end2dDrawing (void)
// {
//     // restore previous model/projection transformation state
//     glPopMatrix ();
//     glMatrixMode (GL_PROJECTION);
//     glPopMatrix ();

//     // restore OpenGL matrix mode
//     glMatrixMode (gxxxsavedMatrixMode);
// }



// void draw2dTextAt3dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     // set text color and raster position
//     glColor3f (color.r(), color.g(), color.b());
//     glRasterPos3f (location.x, location.y, location.z);

//     // switch into 2d screen space in case we need to handle a new-line
//     begin2dDrawing ();
//     GLint rasterPosition[4];
//     glGetIntegerv(GL_CURRENT_RASTER_POSITION, rasterPosition);
//     glRasterPos2i (rasterPosition[0], rasterPosition[1]);

//     // loop over each character in string (until null terminator)
//     int lines = 0;
//     for (const char* p = s; *p; p++)
//     {
//         if (*p == '\n')
//         {
//             // handle new-line character, reset raster position
//             lines++;
//             const int fontHeight = 15; // for GLUT_BITMAP_9_BY_15
//             const int vOffset = lines * (fontHeight + 1);
//             glRasterPos2i (rasterPosition[0], rasterPosition[1] - vOffset);
//         }
//         else
//         {
//             // otherwise draw character bitmap
//             glutBitmapCharacter (GLUT_BITMAP_9_BY_15, *p);
//         }
//     }

//     // xxx
//     end2dDrawing ();
// }


// void draw2dTextAt2dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     begin2dDrawing ();

//     // draw text at specified location (which is now interpreted as
//     // relative to screen space) and color
//     draw2dTextAt3dLocation (s, location, color);

//     end2dDrawing ();
// }


// // for now these cannot be nested (would need to have a stack of saved
// // xxx  matrix modes instead of just a global).
// int gxxxsavedMatrixMode;


// void draw2dTextAt3dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     // set text color and raster position
//     glColor3f (color.r(), color.g(), color.b());
//     glRasterPos3f (location.x, location.y, location.z);

//     // switch into 2d screen space in case we need to handle a new-line
//     GLint rasterPosition[4];
//     glGetIntegerv (GL_CURRENT_RASTER_POSITION, rasterPosition);
//     const GLint originalMatrixMode = begin2dDrawing ();

//     //xxx uncommenting this causes the "2d" version to print the wrong thing
//     //xxx with it out the first line of a multi-line "3d" string jiggles
//     //glRasterPos2i (rasterPosition[0], rasterPosition[1]);

//     // loop over each character in string (until null terminator)
//     int lines = 0;
//     for (const char* p = s; *p; p++)
//     {
//         if (*p == '\n')
//         {
//             // handle new-line character, reset raster position
//             lines++;
//             const int fontHeight = 15; // for GLUT_BITMAP_9_BY_15
//             const int vOffset = lines * (fontHeight + 1);
//             glRasterPos2i (rasterPosition[0], rasterPosition[1] - vOffset);
//         }
//         else
//         {
//             // otherwise draw character bitmap
//             glutBitmapCharacter (GLUT_BITMAP_9_BY_15, *p);
//         }
//     }

//     // switch back out of 2d screen space
//     end2dDrawing (originalMatrixMode);
// }


// void draw2dTextAt2dLocation (const char* const s,
//                              const Vec3 location,
//                              const Vec3 color)
// {
//     const GLint originalMatrixMode = begin2dDrawing ();

//     // draw text at specified location (which is now interpreted as
//     // relative to screen space) and color
//     draw2dTextAt3dLocation (s, location, color);

//     end2dDrawing (originalMatrixMode);
// }


void
Draw::draw2dTextAt3dLocation (const char& text,
                                   const Vec3& location,
                                   const Color& color, float w, float h)
{
    // XXX NOTE: "it would be nice if" this had a 2d screenspace offset for
    // the origin of the text relative to the screen space projection of
    // the 3d point.
    
    // set text color and raster position
    glColor3f (color.r(), color.g(), color.b());
    glRasterPos3f (location.x, location.y, location.z);
    
    // switch into 2d screen space in case we need to handle a new-line
    GLint rasterPosition[4];
    glGetIntegerv (GL_CURRENT_RASTER_POSITION, rasterPosition);
    const GLint originalMatrixMode = begin2dDrawing (w, h);
    
    //xxx uncommenting this causes the "2d" version to print the wrong thing
    //xxx with it out the first line of a multi-line "3d" string jiggles
    //glRasterPos2i (rasterPosition[0], rasterPosition[1]);
    
    // loop over each character in string (until null terminator)
    int lines = 0;
    for (const char* p = &text; *p; p++)
    {
        if (*p == '\n')
        {
            // handle new-line character, reset raster position
            lines++;
            const int fontHeight = 15; // for GLUT_BITMAP_9_BY_15
            const int vOffset = lines * (fontHeight + 1);
            glRasterPos2i (rasterPosition[0], rasterPosition[1] - vOffset);
        }
        else
        {
            // otherwise draw character bitmap
#ifndef HAVE_NO_GLUT
            glutBitmapCharacter (GLUT_BITMAP_9_BY_15, *p);
#else
            // no character drawing with GLUT presently
#endif
        }
    }
    
    // switch back out of 2d screen space
    end2dDrawing (originalMatrixMode);
}

void
Draw::draw2dTextAt3dLocation (const std::ostringstream& text,
                                   const Vec3& location,
                                   const Color& color, float w, float h)
{
    draw2dTextAt3dLocation (*text.str().c_str(), location, color, w, h);
}


void
Draw::draw2dTextAt2dLocation (const char& text,
                                   const Vec3 location,
                                   const Color& color, float w, float h)
{
    const GLint originalMatrixMode = begin2dDrawing (w, h);
    
    // draw text at specified location (which is now interpreted as
    // relative to screen space) and color
    draw2dTextAt3dLocation (text, location, color, w, h);
    
    end2dDrawing (originalMatrixMode);
}


void
Draw::draw2dTextAt2dLocation (const std::ostringstream& text,
                                   const Vec3 location,
                                   const Color& color, float w, float h)
{
    draw2dTextAt2dLocation (*text.str().c_str(), location, color, w, h);
}





// ----------------------------------------------------------------------------


namespace Draw {
    
    // This class provides methods to draw spheres.  The shape is represented
    // as a "geodesic" mesh of triangles generated by subviding an icosahedron
    // until an edge length criteria is met.  Supports wireframe and unshaded
    // triangle drawing styles.  Provides front/back/both culling of faces.
    //
    // see drawSphere below
    //
    class DrawSphereHelper
    {
    public:
        Vec3 center;
        float radius;
        float maxEdgeLength;
        bool filled;
        Color color;
        bool drawFrontFacing;
        bool drawBackFacing;
        Vec3 viewpoint;
        
        // default constructor (at origin, radius=1, white, wireframe, nocull)
        DrawSphereHelper ()
        : center (Vec3::zero),
        radius (1.0f),
        maxEdgeLength (1.0f),
        filled (false),
        color (gWhite),
        drawFrontFacing (true),
        drawBackFacing (true),
        viewpoint (Vec3::zero)
        {}
        
        
        // "kitchen sink" constructor (allows specifying everything)
        DrawSphereHelper (const Vec3 _center,
                          const float _radius,
                          const float _maxEdgeLength,
                          const bool _filled,
                          const Color& _color,
                          const bool _drawFrontFacing,
                          const bool _drawBackFacing,
                          const Vec3& _viewpoint)
        : center (_center),
        radius (_radius),
        maxEdgeLength (_maxEdgeLength),
        filled (_filled),
        color (_color),
        drawFrontFacing (_drawFrontFacing),
        drawBackFacing (_drawBackFacing),
        viewpoint (_viewpoint)
        {}
        
        
        // draw as an icosahedral geodesic sphere
        void draw (void) const
        {
            // Geometry based on Paul Bourke's excellent article:
            //   Platonic Solids (Regular polytopes in 3D)
            //   http://astronomy.swin.edu.au/~pbourke/polyhedra/platonic/
            const float sqrt5 = sqrtXXX (5.0f);
            const float phi = (1.0f + sqrt5) * 0.5f; // "golden ratio"
            // ratio of edge length to radius
            const float ratio = sqrtXXX (10.0f + (2.0f * sqrt5)) / (4.0f * phi);
            const float a = (radius / ratio) * 0.5;
            const float b = (radius / ratio) / (2.0f * phi);
            
            // define the icosahedron's 12 vertices:
            const Vec3 v1  = center + Vec3 ( 0,  b, -a);
            const Vec3 v2  = center + Vec3 ( b,  a,  0);
            const Vec3 v3  = center + Vec3 (-b,  a,  0);
            const Vec3 v4  = center + Vec3 ( 0,  b,  a);
            const Vec3 v5  = center + Vec3 ( 0, -b,  a);
            const Vec3 v6  = center + Vec3 (-a,  0,  b);
            const Vec3 v7  = center + Vec3 ( 0, -b, -a);
            const Vec3 v8  = center + Vec3 ( a,  0, -b);
            const Vec3 v9  = center + Vec3 ( a,  0,  b);
            const Vec3 v10 = center + Vec3 (-a,  0, -b);
            const Vec3 v11 = center + Vec3 ( b, -a,  0);
            const Vec3 v12 = center + Vec3 (-b, -a,  0);
            
            // draw the icosahedron's 20 triangular faces:
            drawMeshedTriangleOnSphere (v1, v2, v3);
            drawMeshedTriangleOnSphere (v4, v3, v2);
            drawMeshedTriangleOnSphere (v4, v5, v6);
            drawMeshedTriangleOnSphere (v4, v9, v5);
            drawMeshedTriangleOnSphere (v1, v7, v8);
            drawMeshedTriangleOnSphere (v1, v10, v7);
            drawMeshedTriangleOnSphere (v5, v11, v12);
            drawMeshedTriangleOnSphere (v7, v12, v11);
            drawMeshedTriangleOnSphere (v3, v6, v10);
            drawMeshedTriangleOnSphere (v12, v10, v6);
            drawMeshedTriangleOnSphere (v2, v8, v9);
            drawMeshedTriangleOnSphere (v11, v9, v8);
            drawMeshedTriangleOnSphere (v4, v6, v3);
            drawMeshedTriangleOnSphere (v4, v2, v9);
            drawMeshedTriangleOnSphere (v1, v3, v10);
            drawMeshedTriangleOnSphere (v1, v8, v2);
            drawMeshedTriangleOnSphere (v7, v10, v12);
            drawMeshedTriangleOnSphere (v7, v11, v8);
            drawMeshedTriangleOnSphere (v5, v12, v6);
            drawMeshedTriangleOnSphere (v5, v9, v11);
        }
        
        
        // given two points, take midpoint and project onto this sphere
        inline Vec3 midpointOnSphere (const Vec3& a, const Vec3& b) const
        {
            const Vec3 midpoint = (a + b) * 0.5f;
            const Vec3 unitRadial = (midpoint - center).normalize ();
            return center + (unitRadial * radius);
        }
        
        
        // given three points on the surface of this sphere, if the triangle
        // is "small enough" draw it, otherwise subdivide it into four smaller
        // triangles and recursively draw each of them.
        void drawMeshedTriangleOnSphere (const Vec3& a,
                                         const Vec3& b,
                                         const Vec3& c) const
        {
            // if all edges are short enough
            if ((((a - b).length ()) < maxEdgeLength) &&
                (((b - c).length ()) < maxEdgeLength) &&
                (((c - a).length ()) < maxEdgeLength))
            {
                // draw triangle
                drawTriangleOnSphere (a, b, c);
            }
            else // otherwise subdivide and recurse
            {
                // find edge midpoints
                const Vec3 ab = midpointOnSphere (a, b);
                const Vec3 bc = midpointOnSphere (b, c);
                const Vec3 ca = midpointOnSphere (c, a);
                
                // recurse on four sub-triangles
                drawMeshedTriangleOnSphere ( a, ab, ca);
                drawMeshedTriangleOnSphere (ab,  b, bc);
                drawMeshedTriangleOnSphere (ca, bc,  c);
                drawMeshedTriangleOnSphere (ab, bc, ca);
            }
        }
        
        
        // draw one mesh element for drawMeshedTriangleOnSphere
        void drawTriangleOnSphere (const Vec3& a,
                                   const Vec3& b,
                                   const Vec3& c) const
        {
            // draw triangle, subject to the camera orientation criteria
            // (according to drawBackFacing and drawFrontFacing)
            const Vec3 triCenter = (a + b + c) / 3.0f;
            const Vec3 triNormal = triCenter - center; // not unit length
            const Vec3 view = triCenter - viewpoint;
            const float dot = view.dot (triNormal); // project normal on view
            const bool seen = ((dot>0.0f) ? drawBackFacing : drawFrontFacing);
            if (seen)
            {
                if (filled)
                {
                    // draw filled triangle
                    if (drawFrontFacing)
                        drawTriangle (c, b, a, color);
                    else
                        drawTriangle (a, b, c, color);
                }
                else
                {
                    // draw triangle edges (use trick to avoid drawing each
                    // edge twice (for each adjacent triangle) unless we are
                    // culling and this tri is near the sphere's silhouette)
                    const float unitDot = view.dot (triNormal.normalize ());
                    const float t = 0.05f; // near threshold
                    const bool nearSilhouette = (unitDot<t) || (unitDot>-t);
                    if (nearSilhouette && !(drawBackFacing&&drawFrontFacing))
                    {
                        drawLine (a, b, color);
                        drawLine (b, c, color);
                        drawLine (c, a, color);
                    }
                    else
                    {
                        drawMeshedTriangleLine (a, b, color);
                        drawMeshedTriangleLine (b, c, color);
                        drawMeshedTriangleLine (c, a, color);
                    }
                }
            }
        }
        
        
        // Draws line from A to B but not from B to A: assumes each edge
        // will be drawn in both directions, picks just one direction for
        // drawing according to an arbitary but reproducable criterion.
        void drawMeshedTriangleLine (const Vec3& a,
                                     const Vec3& b,
                                     const Color& color) const
        {
            if (a.x != b.x)
            {
                if (a.x > b.x) drawLine (a, b, color);
            }
            else
            {
                if (a.y != b.y)
                {
                    if (a.y > b.y) drawLine (a, b, color);
                }
                else
                {
                    if (a.z > b.z) drawLine (a, b, color);
                }
            }
        }
        
    };
    
    
    // draw a sphere (wireframe or opaque, with front/back/both culling)
    void drawSphere (const Vec3 center,
                     const float radius,
                     const float maxEdgeLength,
                     const bool filled,
                     const Color& color,
                     const bool drawFrontFacing,
                     const bool drawBackFacing,
                     const Vec3& viewpoint)
    {
        const DrawSphereHelper s (center, radius, maxEdgeLength, filled, color,
                                  drawFrontFacing, drawBackFacing, viewpoint);
        s.draw ();
    }
    
    
    // draw a SphereObstacle
    void drawSphereObstacle (const SphereObstacle& so,
                             const float maxEdgeLength,
                             const bool filled,
                             const Color& color,
                             const Vec3& viewpoint)
    {
        bool front, back;
        switch (so.seenFrom ())
        {
            default:
            case Obstacle::both:    front = true;  back = true;  break;
            case Obstacle::inside:  front = false; back = true;  break;
            case Obstacle::outside: front = true;  back = false; break;
        }
        drawSphere (so.center, so.radius, maxEdgeLength,
                    filled, color, front, back, viewpoint);
    }
    
    
} // namespace Draw


// ----------------------------------------------------------------------------



